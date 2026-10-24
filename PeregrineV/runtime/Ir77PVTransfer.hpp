#pragma once

#include <vk_mem_alloc.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "../types/Ir77PVTypes.hpp"

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/types/Ir77Types.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"
#include "../../Ir77RT/runtime/Ir77Return.hpp"

#include "../interface/IIr77PVDevice.hpp"
#include "../interface/IIr77PVDescriptorSet.hpp"
#include "../interface/IIr77PVBuffer.hpp"
#include "../interface/IIr77PVTransfer.hpp"

#include "./Ir77PVBuffer.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// =============================================================================================================================================
// Ir77PVTransfer -- getting data onto the device and letting it go again. Shared plumbing for Materials and Geometry, plus storage buffers.
//
//   startup:   Ir77PVInit wires it -- SetDevice / SetAllocator / SetDescriptorSet
//   plumbing:  Upload (staging, wait or not) / CreateStaticBuffer / CreateDynamicBuffer / RegisterBuffer -> ... -> ReleaseSlots / Bury
//   gpu data:  CreateStorageBuffer (compute writes it, shaders read it by slot) -> UploadStorageBuffer -> DestroyStorageBuffer
//   frame:     Tick -- bindless slot aging, finished staging buffers, the graveyard
// One device; it never sees Ir77PVInit.
//
// Every buffer made here stays in m_buffers until Bury'd and aged out, so nothing is freed while a frame in flight can still read it.
// =============================================================================================================================================
class Ir77PVTransfer : public Ir77Enlisted, public IIr77PVTransfer, public std::enable_shared_from_this<Ir77PVTransfer> {
   public:
    Ir77PVTransfer() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    // Ir77PVInit releases this after Geometry and Materials, before its allocator and device -- both still alive here
    ~Ir77PVTransfer() {
        if (!m_device) return;

        VkDevice vk_device{VK_NULL_HANDLE};
        m_device->GetDevice(&vk_device);
        if (vk_device != VK_NULL_HANDLE) vkDeviceWaitIdle(vk_device);

        // everything below frees its own memory -- the device is idle and the allocator still alive here
        m_pending.clear();  // uploads still pending -- finished after the wait
        m_storage_buffers.clear();
        m_graveyard.clear();
        m_buffers.clear();
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVTransfer>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77PeregrineV>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77PVTransfer)
            obj = std::shared_ptr<IIr77PVTransfer>(shared_from_this(), static_cast<IIr77PVTransfer*>(this));

        else if (iid == GUIDIr77PVTransfer)
            obj = std::shared_ptr<Ir77PVTransfer>(shared_from_this(), static_cast<Ir77PVTransfer*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

    // =========================================================================================================================================
    // wiring -- Ir77PVInit sets these as each piece exists
    // =========================================================================================================================================
   public:
    std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) {
        m_device = device;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetAllocator(VmaAllocator allocator) {
        m_allocator = allocator;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetDescriptorSet(std::shared_ptr<IIr77PVDescriptorSet> descriptor_set) {
        m_descriptor_set = descriptor_set;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // per frame -- Ir77PVInit::Tick calls this once per frame, before Geometry's Tick and before Paint records.
    //   0. ages the bindless table's released slots
    //   1. drops the staging buffers of uploads whose fence has signalled
    //   2. ages the graveyard and releases anything no frame in flight can still read
    // =========================================================================================================================================
    std::shared_ptr<IIr77Return const> Tick() {
        if (!m_descriptor_set) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTransfer: bindless table not set.");

        // 0. released slots become reusable after MAX_FRAMES_IN_FLIGHT Ticks
        m_descriptor_set->Tick();

        // 1. finished uploads -- dropping the staging buffer frees its memory, command buffer and fence
        std::erase_if(m_pending, [](std::shared_ptr<IIr77PVBuffer> const& staging) {
            bool finished{true};
            staging->IsUploadFinished(finished);
            return finished;
        });

        // 2. graveyard
        auto& graves = m_graveyard;

        std::size_t keep{0};
        for (std::size_t i = 0; i < graves.size(); i++) {
            if (graves[i].frames_left > 0) {
                graves[i].frames_left--;
                if (keep != i) graves[keep] = std::move(graves[i]);  // self-move would empty the vectors
                keep++;
                continue;
            }

            for (auto const& buffer : graves[i].buffers) std::erase(m_buffers, buffer);
        }
        graves.resize(keep);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // plumbing -- what Materials and Geometry build on
    // =========================================================================================================================================

    // One Staging buffer per batch, on the device's upload pool. wait = true blocks until the copy is done; false returns at once -- the
    // staging buffer stays in m_pending until Tick sees it finished.
    std::shared_ptr<IIr77Return const> Upload(std::vector<Ir77PVUploadEntry> const& entries, bool const& wait = true) {
        if (!m_device) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTransfer: device not set.");

        VkCommandPool pool{VK_NULL_HANDLE};
        VkQueue queue{VK_NULL_HANDLE};

        auto result = m_device->GetUploadPool(pool, queue);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        auto staging = std::static_pointer_cast<IIr77PVBuffer>(std::make_shared<Ir77PVBuffer>());

        staging->SetDevice(m_device);
        staging->SetAllocator(m_allocator);
        staging->SetMode(Ir77PVBufferMode::Staging);

        result = staging->UploadBatch(pool, queue, entries, wait);

        if (result->ID() == GUIDIr77OperationSucceeded && !wait) m_pending.push_back(staging);

        return result;
    }

    // Device-local, one copy -- filled by Upload. Kept in the buffer list the graveyard releases from.
    std::shared_ptr<IIr77Return const> CreateStaticBuffer(VkBufferUsageFlags const& usage, VkDeviceSize const& size, std::shared_ptr<IIr77PVBuffer>& buffer) {
        return DefineBuffer(usage, size, Ir77PVBufferMode::Static, 1, buffer);
    }

    // Host-written every frame -- one copy per frame in flight, persistently mapped. Write with Update(frame, ...). Kept in the buffer list
    // the graveyard releases from.
    std::shared_ptr<IIr77Return const> CreateDynamicBuffer(VkBufferUsageFlags const& usage, VkDeviceSize const& size, std::shared_ptr<IIr77PVBuffer>& buffer) {
        return DefineBuffer(usage, size, Ir77PVBufferMode::Dynamic, MAX_FRAMES_IN_FLIGHT, buffer);
    }

    // Held until MAX_FRAMES_IN_FLIGHT + 1 Ticks have passed -- every frame that could have recorded them has finished by then. Null entries
    // are skipped.
    std::shared_ptr<IIr77Return const> Bury(std::vector<std::shared_ptr<IIr77PVBuffer>> const& buffers) {
        Ir77PVGrave grave{};
        for (auto const& buffer : buffers)
            if (buffer) grave.buffers.push_back(buffer);
        grave.frames_left = MAX_FRAMES_IN_FLIGHT + 1;

        m_graveyard.push_back(std::move(grave));

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // One Static buffer copy -> a bindless buffer slot
    std::shared_ptr<IIr77Return const> RegisterBuffer(std::shared_ptr<IIr77PVBuffer> const& buffer, std::uint32_t& slot) {
        if (!buffer) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTransfer: no buffer.");

        if (!m_descriptor_set) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTransfer: bindless table not set.");

        VkDescriptorBufferInfo info{};

        auto const result = buffer->GetBufferInfo(0, &info);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        return m_descriptor_set->RegisterBuffer(info.buffer, info.offset, info.range, slot);
    }

    // Slots back to the bindless table -- reusable after MAX_FRAMES_IN_FLIGHT Ticks. Unset slots (BINDLESS_NO_SLOT) are skipped.
    std::shared_ptr<IIr77Return const> ReleaseSlots(std::uint32_t const& binding, std::vector<std::uint32_t> const& slots) {
        if (!m_descriptor_set) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTransfer: bindless table not set.");

        for (auto const slot : slots)
            if (slot != BINDLESS_NO_SLOT) m_descriptor_set->Release(binding, slot);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // storage buffers -- GPU data a compute pass writes and shaders read by slot (storage buffers, set 0 binding 1): height fields, particle
    // state, indirect commands. One copy shared by every frame in flight; the Compute pass waits for the previous frame's readers before
    // writing. Also usable as a draw's / dispatch's indirect buffer, or bound as vertex / index data.
    // =========================================================================================================================================

    // size bytes, filled from data (zeros when data is null). Blocking upload. buffer_id for Get / Upload / Destroy; slot for the shaders.
    std::shared_ptr<IIr77Return const> CreateStorageBuffer(VkDeviceSize const& size, void const* data, std::uint32_t& buffer_id, std::uint32_t& slot) {
        if (size == 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTransfer: storage buffer size is 0.");

        std::shared_ptr<IIr77PVBuffer> buffer{};

        if (CreateStaticBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                   VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                               size, buffer)
                ->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTransfer: storage buffer creation failed.");

        std::vector<std::uint8_t> zeros{};
        if (!data) zeros.assign(static_cast<std::size_t>(size), 0);

        VkBuffer dst{VK_NULL_HANDLE};
        buffer->GetBuffer(0, &dst);

        if (Upload({{dst, data ? data : zeros.data(), size}}, true)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTransfer: storage buffer upload failed.");

        auto const result = RegisterBuffer(buffer, slot);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        buffer_id = static_cast<std::uint32_t>(m_storage_buffers.size());
        m_storage_buffers.push_back(buffer);
        m_storage_slots.push_back(slot);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The buffer itself -- for item.indirect / dispatch.indirect
    std::shared_ptr<IIr77Return const> GetStorageBuffer(std::uint32_t const& buffer_id, std::shared_ptr<IIr77PVBuffer>& buffer) {
        if (buffer_id >= m_storage_buffers.size() || !m_storage_buffers[buffer_id])
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTransfer: unknown storage buffer.");

        buffer = m_storage_buffers[buffer_id];

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // CPU rewrite of [offset, offset + size) -- resets, spawn parameters. The copy runs in submission order ahead of later frames.
    std::shared_ptr<IIr77Return const> UploadStorageBuffer(std::uint32_t const& buffer_id, void const* data, VkDeviceSize const& size,
                                                           VkDeviceSize const& offset, bool const& wait) {
        std::shared_ptr<IIr77PVBuffer> buffer{};

        auto const result = GetStorageBuffer(buffer_id, buffer);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        VkBuffer dst{VK_NULL_HANDLE};
        buffer->GetBuffer(0, &dst);

        return Upload({{dst, data, size, offset}}, wait);
    }

    // Releases the slot and frees the buffer once no frame in flight can read it. Remove the draws / dispatches using it first.
    std::shared_ptr<IIr77Return const> DestroyStorageBuffer(std::uint32_t const& buffer_id) {
        std::shared_ptr<IIr77PVBuffer> buffer{};

        auto const result = GetStorageBuffer(buffer_id, buffer);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        ReleaseSlots(BINDLESS_BINDING_BUFFERS, {m_storage_slots[buffer_id]});
        Bury({buffer});

        m_storage_buffers[buffer_id].reset();
        m_storage_slots[buffer_id] = BINDLESS_NO_SLOT;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    std::shared_ptr<IIr77Return const> DefineBuffer(VkBufferUsageFlags const& usage, VkDeviceSize const& size, Ir77PVBufferMode const& mode,
                                                    std::uint32_t const& copies, std::shared_ptr<IIr77PVBuffer>& buffer) {
        buffer.reset();

        if (!m_device || !m_allocator) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTransfer: device or allocator not set.");

        auto created = std::static_pointer_cast<IIr77PVBuffer>(std::make_shared<Ir77PVBuffer>());

        created->SetDevice(m_device);

        created->SetAllocator(m_allocator);

        created->SetUsage(usage);

        created->SetMode(mode);

        auto const result = created->CreateResources(size, copies);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        m_buffers.push_back(created);
        buffer = created;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    // wiring -- Ir77PVInit
    std::shared_ptr<IIr77PVDevice> m_device{};

    VmaAllocator m_allocator{VK_NULL_HANDLE};

    std::shared_ptr<IIr77PVDescriptorSet> m_descriptor_set{};

    // owned
    std::vector<std::shared_ptr<IIr77PVBuffer>> m_buffers{};  // every Static / Dynamic buffer made here -- the graveyard releases from here

    std::vector<std::shared_ptr<IIr77PVBuffer>> m_storage_buffers{};  // [buffer_id] -- empty once destroyed

    std::vector<std::uint32_t> m_storage_slots{};  // [buffer_id] -> bindless buffer slot

    std::vector<std::shared_ptr<IIr77PVBuffer>> m_pending{};  // Staging buffers of uploads submitted with wait = false

    std::vector<Ir77PVGrave> m_graveyard{};  // released, waiting out the frames in flight
};
}  // namespace NSIr77PeregrineV
