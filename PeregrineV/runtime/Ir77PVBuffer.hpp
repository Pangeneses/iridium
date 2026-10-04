#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77PVDevice.hpp"
#include "../interface/IIr77PVBuffer.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77PVBuffer : public Ir77Enlisted, public IIr77PVBuffer, public std::enable_shared_from_this<Ir77PVBuffer> {
   public:
    Ir77PVBuffer() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    ~Ir77PVBuffer() {
        if (m_device) {
            VkDevice device;
            m_device->GetDevice(&device);

            vkDeviceWaitIdle(device);
        }

        for (std::size_t i = 0; i < m_buffers.size(); i++) {
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
        seat_shared_uuid<&GUIDIr77PVBuffer>(uid);

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

        else if (iid == GUIDIIr77PVBuffer)
            obj = std::shared_ptr<IIr77PVBuffer>(shared_from_this(), static_cast<IIr77PVBuffer*>(this));

        else if (iid == GUIDIr77PVBuffer)
            obj = std::shared_ptr<Ir77PVBuffer>(shared_from_this(), static_cast<Ir77PVBuffer*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
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

    std::shared_ptr<IIr77Return const> SetUsage(VkBufferUsageFlags const& usage) {
        m_usage = usage;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetMode(Ir77PVBufferMode const& mode) {
        m_mode = mode;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // copies = frames in flight for Dynamic; ignored (forced to 1) for Static
    std::shared_ptr<IIr77Return const> CreateResources(VkDeviceSize const& size, std::uint32_t const& copies) {
        if (m_allocator == VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVBuffer: allocator not set.");
        if (m_usage == 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVBuffer: usage not set.");
        if (size == 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVBuffer: size is zero.");

        m_size = size;

        std::uint32_t const count = (m_mode == Ir77PVBufferMode::Static) ? 1 : copies;
        if (count == 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVBuffer: copy count is zero.");

        m_buffers.assign(count, VK_NULL_HANDLE);
        m_allocations.assign(count, VK_NULL_HANDLE);
        m_mapped_ptrs.assign(count, nullptr);

        for (std::uint32_t i = 0; i < count; i++) {
            VkBufferCreateInfo buffer_info{};
            buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            buffer_info.size = m_size;
            buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo alloc_info{};

            switch (m_mode) {
                case Ir77PVBufferMode::Dynamic:
                    buffer_info.usage = m_usage;
                    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
                    alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
                    break;

                case Ir77PVBufferMode::Static:
                    buffer_info.usage = m_usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
                    break;

                default:
                    return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVBuffer: unknown buffer mode.");
            }

            VmaAllocationInfo result_info{};
            if (vmaCreateBuffer(m_allocator, &buffer_info, &alloc_info, &m_buffers[i], &m_allocations[i], &result_info) != VK_SUCCESS) {
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVBuffer: buffer creation failed.");
            }

            m_mapped_ptrs[i] = result_info.pMappedData;
        }

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: buffers created.");
    }

    // Dynamic only: memcpy into the copy for this frame
    std::shared_ptr<IIr77Return const> Update(std::uint32_t const& copy, void const* data, VkDeviceSize const& size, VkDeviceSize const& offset = 0) {
        if (m_mode != Ir77PVBufferMode::Dynamic) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVBuffer: Update requires Dynamic mode.");
        if (copy >= m_buffers.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVBuffer: copy index out of range.");
        if (offset + size > m_size) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVBuffer: write exceeds buffer size.");

        std::memcpy(static_cast<std::uint8_t*>(m_mapped_ptrs[copy]) + offset, data, static_cast<std::size_t>(size));

        // no-op on HOST_COHERENT memory, required otherwise
        vmaFlushAllocation(m_allocator, m_allocations[copy], offset, size);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Static only: staging buffer -> one-time copy -> wait. Call at load time, not per frame.
    std::shared_ptr<IIr77Return const> UploadBatch(VkDevice device, VmaAllocator allocator, VkCommandPool pool, VkQueue queue,
                                                   std::vector<Ir77PVUploadEntry> const& entries) {
        VkDeviceSize total{0};
        for (auto const& e : entries) total += e.size;
        if (total == 0) return Ir77RETURN<Ir77OperationSucceeded>();

        VkBufferCreateInfo staging_info{};
        staging_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        staging_info.size = total;
        staging_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        staging_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo staging_alloc_info{};
        staging_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
        staging_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer staging{VK_NULL_HANDLE};
        VmaAllocation staging_allocation{VK_NULL_HANDLE};
        VmaAllocationInfo staging_result{};

        if (vmaCreateBuffer(allocator, &staging_info, &staging_alloc_info, &staging, &staging_allocation, &staging_result) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(nullptr, "Ir77PVBuffer: batch staging buffer creation failed.");

        std::vector<VkDeviceSize> offsets(entries.size());
        VkDeviceSize cursor{0};
        for (std::size_t i = 0; i < entries.size(); i++) {
            offsets[i] = cursor;
            std::memcpy(static_cast<std::uint8_t*>(staging_result.pMappedData) + cursor, entries[i].data, static_cast<std::size_t>(entries[i].size));
            cursor += entries[i].size;
        }
        vmaFlushAllocation(allocator, staging_allocation, 0, total);

        VkCommandBufferAllocateInfo cmd_alloc{};
        cmd_alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd_alloc.commandPool = pool;
        cmd_alloc.commandBufferCount = 1;

        VkCommandBuffer cmd{VK_NULL_HANDLE};
        if (vkAllocateCommandBuffers(device, &cmd_alloc, &cmd) != VK_SUCCESS) {
            vmaDestroyBuffer(allocator, staging, staging_allocation);
            return Ir77RETURN<Ir77NotConfigured>(nullptr, "Ir77PVBuffer: batch upload command buffer allocation failed.");
        }

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &begin_info);

        for (std::size_t i = 0; i < entries.size(); i++) {
            VkBufferCopy region{};
            region.srcOffset = offsets[i];
            region.dstOffset = 0;
            region.size = entries[i].size;
            vkCmdCopyBuffer(cmd, staging, entries[i].dst, 1, &region);
        }

        vkEndCommandBuffer(cmd);

        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &cmd;

        VkResult const result = vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
        if (result == VK_SUCCESS) vkQueueWaitIdle(queue);

        vkFreeCommandBuffers(device, pool, 1, &cmd);
        vmaDestroyBuffer(allocator, staging, staging_allocation);

        if (result != VK_SUCCESS) return Ir77RETURN<Ir77NotConfigured>(nullptr, "Ir77PVBuffer: batch upload submit failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Static buffers ignore the copy index so callers can always pass the frame index
    std::shared_ptr<IIr77Return const> GetBuffer(std::uint32_t const& copy, VkBuffer* buffer) {
        std::uint32_t const index = (m_mode == Ir77PVBufferMode::Static) ? 0 : copy;
        if (index >= m_buffers.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVBuffer: copy index out of range.");

        *buffer = m_buffers[index];

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetBufferInfo(std::uint32_t const& copy, VkDescriptorBufferInfo* info) {
        std::uint32_t const index = (m_mode == Ir77PVBufferMode::Static) ? 0 : copy;
        if (index >= m_buffers.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVBuffer: copy index out of range.");

        info->buffer = m_buffers[index];
        info->offset = 0;
        info->range = m_size;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetSize(VkDeviceSize* size) {
        *size = m_size;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetCopies(std::uint32_t* copies) {
        *copies = static_cast<std::uint32_t>(m_buffers.size());

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetMode(Ir77PVBufferMode* mode) {
        *mode = m_mode;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    std::shared_ptr<IIr77PVDevice> m_device;

    VmaAllocator m_allocator{VK_NULL_HANDLE};

    VkBufferUsageFlags m_usage{0};

    Ir77PVBufferMode m_mode{Ir77PVBufferMode::Dynamic};

    VkDeviceSize m_size{0};

    std::vector<VkBuffer> m_buffers{};

    std::vector<VmaAllocation> m_allocations{};

    std::vector<void*> m_mapped_ptrs{};
};
}  // namespace NSIr77PeregrineV