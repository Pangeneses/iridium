#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

#include <cstring>
#include <memory>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"
#include "../../Ir77RT/dictionary/IDIr77RET.hpp"

#include "../../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../../interface/IIr77PVDevice.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77PVBufferVertex : public Ir77Enlisted, public std::enable_shared_from_this<Ir77PVBufferVertex> {
   public:
    Ir77PVBufferVertex() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    ~Ir77PVBufferVertex() {
        VkDevice device;
        m_device->GetDevice(&device);

        vkDeviceWaitIdle(device);

        if (m_vertex_buffer != VK_NULL_HANDLE) vmaDestroyBuffer(m_allocator, m_vertex_buffer, m_vertex_allocation);
        if (m_index_buffer != VK_NULL_HANDLE) vmaDestroyBuffer(m_allocator, m_index_buffer, m_index_allocation);
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVBufferVertex>(uid);

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

        else if (iid == &GUIDIr77PVBufferVertex)
            obj = std::shared_ptr<Ir77PVBufferVertex>(shared_from_this(), static_cast<Ir77PVBufferVertex*>(this));

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

    std::shared_ptr<IIr77Return const> UploadVertices(const void* vertex_data, VkDeviceSize const& size_bytes) {
        VkDevice device;
        m_device->GetDevice(&device);

        m_size = size_bytes;

        VkBufferCreateInfo staging_info{};
        staging_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        staging_info.size = size_bytes;
        staging_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        staging_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo staging_alloc_info{};
        staging_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
        staging_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer staging_buffer{VK_NULL_HANDLE};
        VmaAllocation staging_allocation{VK_NULL_HANDLE};
        VmaAllocationInfo staging_result{};

        if (vmaCreateBuffer(m_allocator, &staging_info, &staging_alloc_info, &staging_buffer, &staging_allocation, &staging_result) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: vertex staging buffer failed.");
        }

        std::memcpy(staging_result.pMappedData, vertex_data, static_cast<size_t>(size_bytes));

        VkBufferCreateInfo vertex_info{};
        vertex_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vertex_info.size = size_bytes;
        vertex_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        vertex_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo vertex_alloc_info{};
        vertex_alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        if (vmaCreateBuffer(m_allocator, &vertex_info, &vertex_alloc_info, &m_vertex_buffer, &m_vertex_allocation, nullptr) != VK_SUCCESS) {
            vmaDestroyBuffer(m_allocator, staging_buffer, staging_allocation);
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: vertex buffer failed.");
        }

        std::vector<Ir77PVQueueFamily> queue_families;
        m_device->GetQueueFamilies(queue_families);

        VkQueue queue{VK_NULL_HANDLE};
        VkCommandPool cmd_pool{VK_NULL_HANDLE};
        bool found = false;
        for (int i = 0; i < queue_families.size(); i++) {
            if (queue_families[i].presentation == VK_TRUE) {
                queue = queue_families[i].queue;

                VkCommandPoolCreateInfo pool_info{};
                pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                pool_info.queueFamilyIndex = i;

                if (vkCreateCommandPool(device, &pool_info, nullptr, &cmd_pool) != VK_SUCCESS) {
                    vmaDestroyBuffer(m_allocator, staging_buffer, staging_allocation);
                    return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: vertex upload command pool failed.");
                }

                found = true;
                break;
            }
        }

        if (!found) {
            vmaDestroyBuffer(m_allocator, staging_buffer, staging_allocation);
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: no presentation-capable queue found for vertex upload.");
        }

        VkCommandBufferAllocateInfo cmd_alloc_info{};
        cmd_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_alloc_info.commandPool = cmd_pool;
        cmd_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd_alloc_info.commandBufferCount = 1;

        VkCommandBuffer cmd_buf{VK_NULL_HANDLE};
        vkAllocateCommandBuffers(device, &cmd_alloc_info, &cmd_buf);

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd_buf, &begin_info);

        VkBufferCopy copy_region{};
        copy_region.size = size_bytes;
        vkCmdCopyBuffer(cmd_buf, staging_buffer, m_vertex_buffer, 1, &copy_region);

        vkEndCommandBuffer(cmd_buf);

        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &cmd_buf;

        vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);  // one-time load — a CPU stall here is fine

        vkFreeCommandBuffers(device, cmd_pool, 1, &cmd_buf);
        vkDestroyCommandPool(device, cmd_pool, nullptr);
        vmaDestroyBuffer(m_allocator, staging_buffer, staging_allocation);

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: vertex buffer uploaded.");
    }

    std::shared_ptr<IIr77Return const> UploadIndices(const void* index_data, VkDeviceSize size_bytes, std::uint32_t index_count, VkIndexType index_type) {
        VkDevice device;
        m_device->GetDevice(&device);

        m_index_count = index_count;
        m_index_type = index_type;

        VkBufferCreateInfo staging_info{};
        staging_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        staging_info.size = size_bytes;
        staging_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        staging_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo staging_alloc_info{};
        staging_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
        staging_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer staging_buffer{VK_NULL_HANDLE};
        VmaAllocation staging_allocation{VK_NULL_HANDLE};
        VmaAllocationInfo staging_result{};

        if (vmaCreateBuffer(m_allocator, &staging_info, &staging_alloc_info, &staging_buffer, &staging_allocation, &staging_result) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: index staging buffer failed.");
        }

        std::memcpy(staging_result.pMappedData, index_data, static_cast<size_t>(size_bytes));

        VkBufferCreateInfo index_info{};
        index_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        index_info.size = size_bytes;
        index_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        index_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo index_alloc_info{};
        index_alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        if (vmaCreateBuffer(m_allocator, &index_info, &index_alloc_info, &m_index_buffer, &m_index_allocation, nullptr) != VK_SUCCESS) {
            vmaDestroyBuffer(m_allocator, staging_buffer, staging_allocation);
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: index buffer failed.");
        }

        std::vector<Ir77PVQueueFamily> queue_families;
        m_device->GetQueueFamilies(queue_families);

        VkQueue queue{VK_NULL_HANDLE};
        VkCommandPool cmd_pool{VK_NULL_HANDLE};
        bool found = false;
        for (int i = 0; i < queue_families.size(); i++) {
            if (queue_families[i].presentation == VK_TRUE) {
                queue = queue_families[i].queue;

                VkCommandPoolCreateInfo pool_info{};
                pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                pool_info.queueFamilyIndex = i;

                if (vkCreateCommandPool(device, &pool_info, nullptr, &cmd_pool) != VK_SUCCESS) {
                    vmaDestroyBuffer(m_allocator, staging_buffer, staging_allocation);
                    return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: index upload command pool failed.");
                }

                found = true;
                break;
            }
        }

        if (!found) {
            vmaDestroyBuffer(m_allocator, staging_buffer, staging_allocation);
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: no presentation-capable queue found for index upload.");
        }

        VkCommandBufferAllocateInfo cmd_alloc_info{};
        cmd_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_alloc_info.commandPool = cmd_pool;
        cmd_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd_alloc_info.commandBufferCount = 1;

        VkCommandBuffer cmd_buf{VK_NULL_HANDLE};
        vkAllocateCommandBuffers(device, &cmd_alloc_info, &cmd_buf);

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd_buf, &begin_info);

        VkBufferCopy copy_region{};
        copy_region.size = size_bytes;
        vkCmdCopyBuffer(cmd_buf, staging_buffer, m_index_buffer, 1, &copy_region);

        vkEndCommandBuffer(cmd_buf);

        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &cmd_buf;

        vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);

        vkFreeCommandBuffers(device, cmd_pool, 1, &cmd_buf);
        vkDestroyCommandPool(device, cmd_pool, nullptr);
        vmaDestroyBuffer(m_allocator, staging_buffer, staging_allocation);

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: index buffer uploaded.");
    }

    std::shared_ptr<IIr77Return const> GetBuffer(VkBuffer* buffer) {
        *buffer = m_vertex_buffer;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetIndexBuffer(VkBuffer* buffer) {
        *buffer = m_index_buffer;
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetIndexCount(std::uint32_t& count) {
        count = m_index_count;
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetIndexType(VkIndexType& type) {
        type = m_index_type;
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    std::shared_ptr<IIr77PVDevice> m_device;

    VmaAllocator m_allocator{VK_NULL_HANDLE};

    VkBuffer m_vertex_buffer{VK_NULL_HANDLE};

    VmaAllocation m_vertex_allocation{VK_NULL_HANDLE};

    VkDeviceSize m_size{0};

    VkBuffer m_index_buffer{VK_NULL_HANDLE};

    VmaAllocation m_index_allocation{VK_NULL_HANDLE};

    std::uint32_t m_index_count{0};

    VkIndexType m_index_type{VK_INDEX_TYPE_UINT32};
};
}  // namespace NSIr77PeregrineV