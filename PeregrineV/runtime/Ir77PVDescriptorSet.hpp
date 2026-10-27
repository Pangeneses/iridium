#pragma once

#include <vulkan/vulkan.h>

#include <array>
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

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77PVDescriptorSet.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// =============================================================================================================================================
// Ir77PVDescriptorSet -- THE descriptor set. One per device: the bindless table every pipeline reads through Ir77PVLayout.
//
//   create:   SetDevice / SetLayout -> CreateDescriptorSet { DefinePool -> DefineSet -> DefineSlots }
//   register: RegisterTexture / RegisterBuffer / RegisterStorageImage / RegisterAccelerationStructure -> a slot, marked with what it holds
//             (GetSlotKind). Shaders get the number through push constants or other buffers.
//   update:   UpdateTexture (an overlay's new extent) / UpdateAccelerationStructure (a TLAS regrown) -- same slot, same kind, new descriptor
//   release:  Release -- free now, handed out by the next Register. Waiting out the frames in flight is the caller's: at runtime everything
//             goes through Transfer's graveyard (Bury); teardown releases after the device idles.
//   record:   Bind, once per command buffer per bind point
//
// Texture slot 0 is the white placeholder -- registered first by Init, permanent -- so a texture slot nobody set samples white.
// =============================================================================================================================================
class Ir77PVDescriptorSet : public Ir77Enlisted, public IIr77PVDescriptorSet, public std::enable_shared_from_this<Ir77PVDescriptorSet> {
   public:
    Ir77PVDescriptorSet() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    // the owner waits for the device to go idle before releasing this; destroying the pool frees the set
    ~Ir77PVDescriptorSet() {
        if (!m_device || m_pool == VK_NULL_HANDLE) return;

        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        vkDestroyDescriptorPool(device, m_pool, nullptr);
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVDescriptorSet>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PeregrineV>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77PVDescriptorSet)
            obj = std::shared_ptr<IIr77PVDescriptorSet>(shared_from_this(), static_cast<IIr77PVDescriptorSet*>(this));

        else if (iid == GUIDIr77PVDescriptorSet)
            obj = std::shared_ptr<Ir77PVDescriptorSet>(shared_from_this(), static_cast<Ir77PVDescriptorSet*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // wiring -- Ir77PVInit
    // -------------------------------------------------------------------------------------------------------------------------------------
   public:
    std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) {
        m_device = device;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetLayout(std::shared_ptr<IIr77PVLayout> layout) {
        m_layout = layout;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // create
    // -------------------------------------------------------------------------------------------------------------------------------------

    // The pool, the one set, and a slot allocator per binding sized from the layout's capacity
    std::shared_ptr<IIr77Return const> CreateDescriptorSet() {
        if (!m_device) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDescriptorSet: SetDevice not called.");
        if (!m_layout) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDescriptorSet: SetLayout not called.");
        if (m_pool != VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDescriptorSet: descriptor set already created.");

        m_layout->GetCapacity(m_capacity);

        if (m_capacity.textures == 0 || m_capacity.buffers == 0 || m_capacity.storage_images == 0)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDescriptorSet: layout has no capacity -- CreateLayout first.");

        std::shared_ptr<IIr77Return const> result{};

        if (Failed(DefinePool(), result)) return result;
        if (Failed(DefineSet(), result)) return result;

        DefineSlots();

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Bindless Descriptor Set.");
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // register -- a new slot, written now
    // -------------------------------------------------------------------------------------------------------------------------------------

    // layout = the layout the image is in whenever a shader samples it (a depth texture: DEPTH_STENCIL_READ_ONLY_OPTIMAL)
    std::shared_ptr<IIr77Return const> RegisterTexture(VkImageView const& view, VkSampler const& sampler, Ir77PVSlotKind const& kind, std::uint32_t& index,
                                                       VkImageLayout const& layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        std::shared_ptr<IIr77Return const> result{};
        if (Failed(AllocateSlot(BINDLESS_BINDING_TEXTURES, kind, index), result)) return result;

        WriteTexture(index, view, sampler, layout);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> RegisterBuffer(VkBuffer const& buffer, VkDeviceSize const& offset, VkDeviceSize const& range,
                                                      Ir77PVSlotKind const& kind, std::uint32_t& index) {
        std::shared_ptr<IIr77Return const> result{};
        if (Failed(AllocateSlot(BINDLESS_BINDING_BUFFERS, kind, index), result)) return result;

        WriteBuffer(index, buffer, offset, range);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The image must be in VK_IMAGE_LAYOUT_GENERAL whenever a shader uses it
    std::shared_ptr<IIr77Return const> RegisterStorageImage(VkImageView const& view, Ir77PVSlotKind const& kind, std::uint32_t& index) {
        std::shared_ptr<IIr77Return const> result{};
        if (Failed(AllocateSlot(BINDLESS_BINDING_STORAGE_IMAGES, kind, index), result)) return result;

        WriteStorageImage(index, view);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Binding 3 -- a TLAS, kind TopLevel. Ray-query devices only: without them the binding has no room and this fails.
    std::shared_ptr<IIr77Return const> RegisterAccelerationStructure(VkAccelerationStructureKHR const& structure, std::uint32_t& index) {
        std::shared_ptr<IIr77Return const> result{};
        if (Failed(AllocateSlot(BINDLESS_BINDING_ACCELERATION, Ir77PVSlotKind::TopLevel, index), result)) return result;

        WriteAccelerationStructure(index, structure);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // update -- rewrite a registered slot in place. Only while no frame in flight reads it (UPDATE_UNUSED_WHILE_PENDING): the overlay and
    // the TLAS ring update after the device idles. When frames might read it, register a new slot and Bury the old one instead.
    // -------------------------------------------------------------------------------------------------------------------------------------

    std::shared_ptr<IIr77Return const> UpdateTexture(std::uint32_t const& index, VkImageView const& view, VkSampler const& sampler,
                                                     VkImageLayout const& layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        std::shared_ptr<IIr77Return const> result{};
        if (Failed(CheckRegistered(BINDLESS_BINDING_TEXTURES, index), result)) return result;

        WriteTexture(index, view, sampler, layout);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> UpdateAccelerationStructure(std::uint32_t const& index, VkAccelerationStructureKHR const& structure) {
        std::shared_ptr<IIr77Return const> result{};
        if (Failed(CheckRegistered(BINDLESS_BINDING_ACCELERATION, index), result)) return result;

        WriteAccelerationStructure(index, structure);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // release
    // -------------------------------------------------------------------------------------------------------------------------------------

    // binding = BINDLESS_BINDING_TEXTURES / _BUFFERS / _STORAGE_IMAGES / _ACCELERATION. Free now -- the next Register may hand it out, so
    // only a slot no frame in flight can read (Transfer's graveyard, or teardown after the device idles). The placeholder slot is permanent.
    std::shared_ptr<IIr77Return const> Release(std::uint32_t const& binding, std::uint32_t const& index) {
        std::shared_ptr<IIr77Return const> result{};
        if (Failed(CheckRegistered(binding, index), result)) return result;

        if (binding == BINDLESS_BINDING_TEXTURES && index == BINDLESS_PLACEHOLDER_TEXTURE)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDescriptorSet: the placeholder texture slot is permanent.");

        m_slots[binding].kinds[index] = Ir77PVSlotKind::None;
        m_slots[binding].free.push_back(index);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // record / query
    // -------------------------------------------------------------------------------------------------------------------------------------

    // The one set at set 0 for a bind point (graphics / compute / ray tracing). Once per command buffer per bind point -- every pipeline
    // shares the layout, so pipeline switches don't disturb it.
    std::shared_ptr<IIr77Return const> Bind(VkCommandBuffer const& cmd, VkPipelineBindPoint const& bind_point) {
        VkPipelineLayout pipeline_layout{VK_NULL_HANDLE};
        m_layout->GetPipelineLayout(&pipeline_layout);

        vkCmdBindDescriptorSets(cmd, bind_point, pipeline_layout, 0, 1, &m_set, 0, nullptr);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // What a slot holds -- None when it was never registered, has been released, or is out of range
    std::shared_ptr<IIr77Return const> GetSlotKind(std::uint32_t const& binding, std::uint32_t const& index, Ir77PVSlotKind& kind) const {
        kind = Ir77PVSlotKind::None;

        if (binding < BINDLESS_BINDING_COUNT && index < m_slots[binding].kinds.size()) kind = m_slots[binding].kinds[index];

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // CreateDescriptorSet's steps
    // -------------------------------------------------------------------------------------------------------------------------------------
   private:
    std::shared_ptr<IIr77Return const> DefinePool() {
        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        std::vector<VkDescriptorPoolSize> pool_sizes{};
        pool_sizes.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_capacity.textures});
        pool_sizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_capacity.buffers});
        pool_sizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, m_capacity.storage_images});

        if (m_capacity.acceleration_structures > 0) pool_sizes.push_back({VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, m_capacity.acceleration_structures});

        VkDescriptorPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        pool_info.maxSets = 1;
        pool_info.poolSizeCount = static_cast<std::uint32_t>(pool_sizes.size());
        pool_info.pPoolSizes = pool_sizes.data();

        if (vkCreateDescriptorPool(device, &pool_info, nullptr, &m_pool) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDescriptorSet: vkCreateDescriptorPool failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineSet() {
        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        VkDescriptorSetLayout set_layout{VK_NULL_HANDLE};
        m_layout->GetDescriptorSetLayout(&set_layout);

        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = m_pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &set_layout;

        if (vkAllocateDescriptorSets(device, &alloc_info, &m_set) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDescriptorSet: vkAllocateDescriptorSets failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // One allocator per binding, every slot None. Binding 3 has no room without ray queries -- registering into it fails.
    void DefineSlots() {
        m_slots[BINDLESS_BINDING_TEXTURES].capacity = m_capacity.textures;
        m_slots[BINDLESS_BINDING_BUFFERS].capacity = m_capacity.buffers;
        m_slots[BINDLESS_BINDING_STORAGE_IMAGES].capacity = m_capacity.storage_images;
        m_slots[BINDLESS_BINDING_ACCELERATION].capacity = m_capacity.acceleration_structures;

        for (Ir77PVSlotAllocator& slots : m_slots) slots.kinds.assign(slots.capacity, Ir77PVSlotKind::None);
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // slots
    // -------------------------------------------------------------------------------------------------------------------------------------

    // A released slot first, otherwise the next never-used one -- marked with its kind
    std::shared_ptr<IIr77Return const> AllocateSlot(std::uint32_t const& binding, Ir77PVSlotKind const& kind, std::uint32_t& index) {
        if (m_set == VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDescriptorSet: CreateDescriptorSet not called.");

        Ir77PVSlotAllocator& slots = m_slots[binding];

        if (!slots.free.empty()) {
            index = slots.free.back();
            slots.free.pop_back();
        } else {
            if (slots.next >= slots.capacity)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDescriptorSet: bindless binding " + std::to_string(binding) + " is full.");

            index = slots.next++;
        }

        slots.kinds[index] = kind;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The slot holds something -- Update and Release only touch registered slots
    std::shared_ptr<IIr77Return const> CheckRegistered(std::uint32_t const& binding, std::uint32_t const& index) const {
        if (binding >= BINDLESS_BINDING_COUNT) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDescriptorSet: unknown binding.");

        if (index >= m_slots[binding].kinds.size() || m_slots[binding].kinds[index] == Ir77PVSlotKind::None)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDescriptorSet: slot " + std::to_string(index) + " of binding " + std::to_string(binding) +
                                                           " is not registered (never was, or already released).");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // descriptor writes -- the slot is known good by here
    // -------------------------------------------------------------------------------------------------------------------------------------

    void WriteTexture(std::uint32_t const& index, VkImageView const& view, VkSampler const& sampler, VkImageLayout const& layout) {
        VkDescriptorImageInfo image_info{};
        image_info.sampler = sampler;
        image_info.imageView = view;
        image_info.imageLayout = layout;

        WriteDescriptor(BINDLESS_BINDING_TEXTURES, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, index, &image_info, nullptr, nullptr);
    }

    void WriteBuffer(std::uint32_t const& index, VkBuffer const& buffer, VkDeviceSize const& offset, VkDeviceSize const& range) {
        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = buffer;
        buffer_info.offset = offset;
        buffer_info.range = range;

        WriteDescriptor(BINDLESS_BINDING_BUFFERS, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, index, nullptr, &buffer_info, nullptr);
    }

    void WriteStorageImage(std::uint32_t const& index, VkImageView const& view) {
        VkDescriptorImageInfo image_info{};
        image_info.imageView = view;
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        WriteDescriptor(BINDLESS_BINDING_STORAGE_IMAGES, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, index, &image_info, nullptr, nullptr);
    }

    // a TLAS travels in pNext (VkWriteDescriptorSetAccelerationStructureKHR), not in image / buffer info
    void WriteAccelerationStructure(std::uint32_t const& index, VkAccelerationStructureKHR const& structure) {
        VkWriteDescriptorSetAccelerationStructureKHR structure_info{};
        structure_info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        structure_info.accelerationStructureCount = 1;
        structure_info.pAccelerationStructures = &structure;

        WriteDescriptor(BINDLESS_BINDING_ACCELERATION, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, index, nullptr, nullptr, &structure_info);
    }

    void WriteDescriptor(std::uint32_t const& binding, VkDescriptorType const& type, std::uint32_t const& index, VkDescriptorImageInfo const* image_info,
                         VkDescriptorBufferInfo const* buffer_info, void const* next) {
        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = next;
        write.dstSet = m_set;
        write.dstBinding = binding;
        write.dstArrayElement = index;
        write.descriptorCount = 1;
        write.descriptorType = type;
        write.pImageInfo = image_info;
        write.pBufferInfo = buffer_info;

        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }

   private:
    // one per binding
    struct Ir77PVSlotAllocator {
        std::uint32_t capacity{0};
        std::uint32_t next{0};                // first never-used slot
        std::vector<std::uint32_t> free{};    // released slots -- handed out first
        std::vector<Ir77PVSlotKind> kinds{};  // [slot] -- what it holds; None = free
    };

    std::shared_ptr<IIr77PVDevice> m_device{};

    std::shared_ptr<IIr77PVLayout> m_layout{};

    Ir77PVBindlessCapacity m_capacity{};

    VkDescriptorPool m_pool{VK_NULL_HANDLE};  // owns m_set

    VkDescriptorSet m_set{VK_NULL_HANDLE};

    std::array<Ir77PVSlotAllocator, BINDLESS_BINDING_COUNT> m_slots{};  // [binding]
};
}  // namespace NSIr77PeregrineV