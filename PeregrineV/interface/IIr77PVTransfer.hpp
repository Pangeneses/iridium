#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <cstdint>
#include <memory>
#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVDevice.hpp"
#include "IIr77PVDescriptorSet.hpp"
#include "IIr77PVBuffer.hpp"

#include "../types/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// Getting data onto the device and letting it go again -- uploads, Static / Dynamic buffers, bindless buffer slots, the graveyard, and
// storage buffers (GPU data a compute pass writes and shaders read by slot). Materials and Geometry build on it.
//   Ir77PVInit wires it: SetDevice / SetAllocator / SetDescriptorSet. Ir77PVInit::Tick calls Tick once per frame.
typedef struct IIr77PVTransfer : virtual public IIr77Enlisted {
    IIr77PVTransfer() = default;

    // ---- wiring -- Ir77PVInit sets these as each piece exists ---------------------------------------------------------------------------

    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    virtual std::shared_ptr<IIr77Return const> SetAllocator(VmaAllocator allocator) = 0;

    virtual std::shared_ptr<IIr77Return const> SetDescriptorSet(std::shared_ptr<IIr77PVDescriptorSet> descriptor_set) = 0;

    // ---- per frame / plumbing --------------------------------------------------------------------------------------------------------------

    // per frame -- Ir77PVInit::Tick calls this once per frame, before Geometry's Tick and before Paint records.
    //   1. drops the staging buffers of uploads whose fence has signalled
    //   2. ages the graveyard and releases every grave no frame in flight can still read
    virtual std::shared_ptr<IIr77Return const> Tick() = 0;

    // One Staging buffer per batch, on the device's upload pool. wait = true blocks until the copy is done; false returns at once -- the
    // staging buffer stays in m_pending until Tick sees it finished.
    virtual std::shared_ptr<IIr77Return const> Upload(std::vector<Ir77PVUploadEntry> const& entries, bool const& wait = true) = 0;

    // Device-local, one copy -- filled by Upload. Kept in the buffer list the graveyard releases from.
    virtual std::shared_ptr<IIr77Return const> CreateStaticBuffer(VkBufferUsageFlags const& usage, VkDeviceSize const& size, std::shared_ptr<IIr77PVBuffer>& buffer) = 0;

    // Host-written every frame -- one copy per frame in flight, persistently mapped. Write with Update(frame, ...). Kept in the buffer list
    // the graveyard releases from.
    virtual std::shared_ptr<IIr77Return const> CreateDynamicBuffer(VkBufferUsageFlags const& usage, VkDeviceSize const& size, std::shared_ptr<IIr77PVBuffer>& buffer) = 0;

    // Everything an owner lets go of in one step -- buffers, textures, bindless slots, released() -- held until MAX_FRAMES_IN_FLIGHT + 1 Ticks
    // have passed, then released together. The only way anything is freed at runtime. Empty entries are skipped.
    virtual std::shared_ptr<IIr77Return const> Bury(Ir77PVGrave const& grave) = 0;

    // One Static buffer copy -> a bindless buffer slot holding kind
    virtual std::shared_ptr<IIr77Return const> RegisterBuffer(std::shared_ptr<IIr77PVBuffer> const& buffer, Ir77PVSlotKind const& kind, std::uint32_t& slot) = 0;

    // ---- storage buffers -- one copy shared by every frame in flight (set 0 binding 1) -------------------------------------------------------

    // size bytes, filled from data (zeros when data is null). Blocking upload. buffer_id for Get / Upload / Destroy; slot for the shaders.
    virtual std::shared_ptr<IIr77Return const> CreateStorageBuffer(VkDeviceSize const& size, void const* data, std::uint32_t& buffer_id, std::uint32_t& slot) = 0;

    // The buffer itself -- for item.indirect / dispatch.indirect
    virtual std::shared_ptr<IIr77Return const> GetStorageBuffer(std::uint32_t const& buffer_id, std::shared_ptr<IIr77PVBuffer>& buffer) = 0;

    // CPU rewrite of [offset, offset + size) -- resets, spawn parameters. The copy runs in submission order ahead of later frames.
    virtual std::shared_ptr<IIr77Return const> UploadStorageBuffer(std::uint32_t const& buffer_id, void const* data, VkDeviceSize const& size, VkDeviceSize const& offset, bool const& wait) = 0;

    // The buffer and its slot into one grave -- both freed once no frame in flight can read them. Remove the draws / dispatches using it first.
    virtual std::shared_ptr<IIr77Return const> DestroyStorageBuffer(std::uint32_t const& buffer_id) = 0;

    virtual ~IIr77PVTransfer() = default;
}* pIIr77PVTransfer;
}  // namespace NSIr77PeregrineV
