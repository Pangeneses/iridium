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
//   create:   SetDevice / SetLayout -> CreateDescriptorSet -> DefinePool -> DefineSet
//   register: RegisterTexture / RegisterBuffer / RegisterStorageImage -> slot index for push constants or other buffers
//   change:   UpdateTexture / UpdateBuffer / UpdateStorageImage -- same slot, new view or buffer (overlay resize, buffer regrow)
//   free:     Release -> slot is retired, reused only after MAX_FRAMES_IN_FLIGHT Ticks (frames in flight may still read it)
//   per frame Tick, then per command buffer Bind
//
// Slot 0 of the texture array is reserved for the placeholder texture, so an unset material index (0) samples something valid.
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

    // the owner waits for the device to go idle before releasing the set; the pool frees it
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
    // configuration
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
    std::shared_ptr<IIr77Return const> CreateDescriptorSet() {
        if (!m_device) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDescriptorSet: SetDevice not called.");
        if (!m_layout) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDescriptorSet: SetLayout not called.");
        if (m_pool != VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDescriptorSet: descriptor set already created.");

        m_layout->GetCapacity(m_capacity);

        auto result = DefinePool();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        result = DefineSet();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        m_slots[BINDLESS_BINDING_TEXTURES].capacity = m_capacity.textures;
        m_slots[BINDLESS_BINDING_BUFFERS].capacity = m_capacity.buffers;
        m_slots[BINDLESS_BINDING_STORAGE_IMAGES].capacity = m_capacity.storage_images;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Bindless Descriptor Set.");
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // register -- returns the slot index shaders use
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> RegisterTexture(VkImageView const& view, VkSampler const& sampler, std::uint32_t& index) {
        auto const result = DefineSlot(BINDLESS_BINDING_TEXTURES, index);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        return UpdateTexture(index, view, sampler);
    }

    std::shared_ptr<IIr77Return const> RegisterBuffer(VkBuffer const& buffer, VkDeviceSize const& offset, VkDeviceSize const& range,
                                                      std::uint32_t& index) {
        auto const result = DefineSlot(BINDLESS_BINDING_BUFFERS, index);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        return UpdateBuffer(index, buffer, offset, range);
    }

    std::shared_ptr<IIr77Return const> RegisterStorageImage(VkImageView const& view, std::uint32_t& index) {
        auto const result = DefineSlot(BINDLESS_BINDING_STORAGE_IMAGES, index);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        return UpdateStorageImage(index, view);
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // update -- rewrite a slot in place. Safe while frames are in flight as long as none of them reads this slot (UPDATE_UNUSED_WHILE_PENDING);
    // when they might, register a new slot and Release the old one instead.
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> UpdateTexture(std::uint32_t const& index, VkImageView const& view, VkSampler const& sampler) {
        if (index >= m_capacity.textures) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDescriptorSet: texture slot out of range.");

        VkDescriptorImageInfo image_info{};
        image_info.sampler = sampler;
        image_info.imageView = view;
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        WriteSlot(BINDLESS_BINDING_TEXTURES, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, index, &image_info, nullptr);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> UpdateBuffer(std::uint32_t const& index, VkBuffer const& buffer, VkDeviceSize const& offset,
                                                    VkDeviceSize const& range) {
        if (index >= m_capacity.buffers) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDescriptorSet: buffer slot out of range.");

        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = buffer;
        buffer_info.offset = offset;
        buffer_info.range = range;

        WriteSlot(BINDLESS_BINDING_BUFFERS, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, index, nullptr, &buffer_info);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> UpdateStorageImage(std::uint32_t const& index, VkImageView const& view) {
        if (index >= m_capacity.storage_images) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDescriptorSet: storage image slot out of range.");

        VkDescriptorImageInfo image_info{};
        image_info.imageView = view;
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        WriteSlot(BINDLESS_BINDING_STORAGE_IMAGES, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, index, &image_info, nullptr);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // release / per frame
    // -------------------------------------------------------------------------------------------------------------------------------------
    // binding = BINDLESS_BINDING_TEXTURES / _BUFFERS / _STORAGE_IMAGES. The slot is reused only after MAX_FRAMES_IN_FLIGHT Ticks.
    std::shared_ptr<IIr77Return const> Release(std::uint32_t const& binding, std::uint32_t const& index) {
        if (binding >= BINDLESS_BINDING_COUNT) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDescriptorSet: unknown binding.");
        if (index >= m_slots[binding].next) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDescriptorSet: slot was never registered.");
        if (binding == BINDLESS_BINDING_TEXTURES && index == BINDLESS_PLACEHOLDER_TEXTURE)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDescriptorSet: the placeholder texture slot is permanent.");

        m_retired.push_back(Ir77PVRetiredSlot{binding, index, MAX_FRAMES_IN_FLIGHT});

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Once per frame, before recording: returns retired slots no frame in flight can still read to their free lists
    std::shared_ptr<IIr77Return const> Tick() {
        std::size_t keep{0};

        for (std::size_t i = 0; i < m_retired.size(); i++) {
            if (m_retired[i].frames_left > 0) {
                m_retired[i].frames_left--;
                m_retired[keep++] = m_retired[i];
                continue;
            }

            m_slots[m_retired[i].binding].free.push_back(m_retired[i].index);
        }

        m_retired.resize(keep);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Binds the one set at set 0 for a bind point (graphics / compute / ray tracing). Once per command buffer per bind point --
    // pipeline changes don't disturb it, since every pipeline shares the layout.
    std::shared_ptr<IIr77Return const> Bind(VkCommandBuffer const& cmd, VkPipelineBindPoint const& bind_point) {
        VkPipelineLayout pipeline_layout{VK_NULL_HANDLE};
        m_layout->GetPipelineLayout(&pipeline_layout);

        vkCmdBindDescriptorSets(cmd, bind_point, pipeline_layout, 0, 1, &m_set, 0, nullptr);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // queries
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> GetSet(VkDescriptorSet* set) {
        *set = m_set;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // steps
    // -------------------------------------------------------------------------------------------------------------------------------------
   private:
    std::shared_ptr<IIr77Return const> DefinePool() {
        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        std::array<VkDescriptorPoolSize, BINDLESS_BINDING_COUNT> pool_sizes{};
        pool_sizes[BINDLESS_BINDING_TEXTURES] = {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_capacity.textures};
        pool_sizes[BINDLESS_BINDING_BUFFERS] = {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_capacity.buffers};
        pool_sizes[BINDLESS_BINDING_STORAGE_IMAGES] = {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, m_capacity.storage_images};

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

    // A free (recycled) slot first, otherwise the next unused one
    std::shared_ptr<IIr77Return const> DefineSlot(std::uint32_t const& binding, std::uint32_t& index) {
        if (m_set == VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDescriptorSet: CreateDescriptorSet not called.");

        auto& slots = m_slots[binding];

        if (!slots.free.empty()) {
            index = slots.free.back();
            slots.free.pop_back();
            return Ir77RETURN<Ir77OperationSucceeded>();
        }

        if (slots.next >= slots.capacity)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDescriptorSet: bindless binding " + std::to_string(binding) + " is full.");

        index = slots.next++;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    void WriteSlot(std::uint32_t const binding, VkDescriptorType const type, std::uint32_t const index, VkDescriptorImageInfo const* image_info,
                   VkDescriptorBufferInfo const* buffer_info) {
        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
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
    struct Ir77PVSlotAllocator {
        std::uint32_t capacity{0};
        std::uint32_t next{0};                // first never-used slot
        std::vector<std::uint32_t> free{};    // released slots, past their frames-in-flight wait
    };

    struct Ir77PVRetiredSlot {
        std::uint32_t binding{0};
        std::uint32_t index{0};
        std::uint32_t frames_left{0};
    };

    std::shared_ptr<IIr77PVDevice> m_device{};

    std::shared_ptr<IIr77PVLayout> m_layout{};

    Ir77PVBindlessCapacity m_capacity{};

    VkDescriptorPool m_pool{VK_NULL_HANDLE};

    VkDescriptorSet m_set{VK_NULL_HANDLE};

    std::array<Ir77PVSlotAllocator, BINDLESS_BINDING_COUNT> m_slots{};

    std::vector<Ir77PVRetiredSlot> m_retired{};
};

}  // namespace NSIr77PeregrineV