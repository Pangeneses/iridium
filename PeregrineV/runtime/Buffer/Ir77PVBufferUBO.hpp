#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

#include <cstring>
#include <memory>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"
#include "../../Ir77RT/dictionary/IDIr77RET.hpp"

#include "../../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../../interface/IIr77PVDevice.hpp"
#include "../../interface/IIr77PVLayout.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77PVBufferUBO : public Ir77Enlisted, public std::enable_shared_from_this<Ir77PVBufferUBO> {
   public:
    Ir77PVBufferUBO() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    ~Ir77PVBufferUBO() {
        VkDevice device;
        m_device->GetDevice(&device);

        vkDeviceWaitIdle(device);

        for (uint32_t i = 0; i < m_buffers.size(); i++) {
            if (m_buffers[i] != VK_NULL_HANDLE) vmaDestroyBuffer(m_allocator, m_buffers[i], m_allocations[i]);
        }
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVBufferUBO>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PeregrineV>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    IIr77GUID* const QueryInterface(IIr77GUID const* iid, std::shared_ptr<void>& obj) {
        if (iid == &GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == &GUIDIr77PVBufferUBO)
            obj = std::shared_ptr<Ir77PVBufferUBO>(shared_from_this(), static_cast<Ir77PVBufferUBO*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) {
        m_device = device;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetAllocator(VmaAllocator allocator) {
        m_allocator = allocator;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetLayoutUBO(std::shared_ptr<IIr77PVLayout> layout_ubo) {
        m_layout_ubo = layout_ubo;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CreateResources(VkDeviceSize ubo_size, uint32_t frames_in_flight) {
        VkDevice device;
        m_device->GetDevice(&device);

        m_ubo_size = ubo_size;

        m_buffers.resize(frames_in_flight);
        m_allocations.resize(frames_in_flight);
        m_mapped_ptrs.resize(frames_in_flight);
        m_descriptor_sets.resize(frames_in_flight);

        for (uint32_t i = 0; i < frames_in_flight; i++) {
            VkBufferCreateInfo buffer_info{};
            buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            buffer_info.size = ubo_size;
            buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo alloc_info{};
            alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
            alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

            VmaAllocationInfo result_info{};
            if (vmaCreateBuffer(m_allocator, &buffer_info, &alloc_info, &m_buffers[i], &m_allocations[i], &result_info) != VK_SUCCESS) {
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: UBO buffer creation failed.");
            }

            m_mapped_ptrs[i] = result_info.pMappedData;

            if (m_layout_ubo->AllocateSet(&m_descriptor_sets[i])->ID() != &GUIDIr77OperationSucceeded) {
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: UBO descriptor set allocation failed.");
            }

            UpdateDescriptorSet(device, i);
        }

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: UBO buffers created.");
    }

    std::shared_ptr<IIr77Return const> UpdateUBO(uint32_t frame_index, const void* data, VkDeviceSize size) {
        std::memcpy(m_mapped_ptrs[frame_index], data, static_cast<size_t>(size));

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetDescriptorSet(uint32_t frame_index, VkDescriptorSet* set) {
        *set = m_descriptor_sets[frame_index];

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    void UpdateDescriptorSet(VkDevice device, uint32_t slot) {
        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = m_buffers[slot];
        buffer_info.offset = 0;
        buffer_info.range = m_ubo_size;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_descriptor_sets[slot];
        write.dstBinding = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &buffer_info;

        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }

   private:
    std::shared_ptr<IIr77PVDevice> m_device;

    VmaAllocator m_allocator{VK_NULL_HANDLE};

    std::shared_ptr<IIr77PVLayout> m_layout_ubo;

    VkDeviceSize m_ubo_size{0};

    std::vector<VkBuffer> m_buffers;

    std::vector<VmaAllocation> m_allocations;

    std::vector<void*> m_mapped_ptrs;

    std::vector<VkDescriptorSet> m_descriptor_sets;
};
}  // namespace NSIr77PeregrineV