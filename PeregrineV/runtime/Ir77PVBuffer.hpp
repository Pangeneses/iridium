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
        ReleaseUpload();
        Release();
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

    // copies = frames in flight for Dynamic; ignored (forced to 1) for Static and Staging
    std::shared_ptr<IIr77Return const> CreateResources(VkDeviceSize const& size, std::uint32_t const& copies) {
        if (m_allocator == VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVBuffer: allocator not set.");
        if (m_usage == 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVBuffer: usage not set.");
        if (size == 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVBuffer: size is zero.");

        Release();

        m_size = size;

        std::uint32_t const count = (m_mode == Ir77PVBufferMode::Dynamic) ? copies : 1;
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

                case Ir77PVBufferMode::Staging:
                    buffer_info.usage = m_usage | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
                    alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
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

    // Staging mode only. Sizes this buffer to the batch, packs every entry into it and copies each to its dst at dst_offset in one submit.
    // The barrier makes the copies visible to everything submitted later on this queue.
    // wait = true blocks until the copy is done. wait = false returns at once -- keep this buffer alive until IsUploadFinished reports
    // true, then drop it; the destructor frees the memory, the command buffer and the fence.
    std::shared_ptr<IIr77Return const> UploadBatch(VkCommandPool const& pool, VkQueue const& queue, std::vector<Ir77PVUploadEntry> const& entries,
                                                   bool const& wait) {
        if (m_mode != Ir77PVBufferMode::Staging) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVBuffer: UploadBatch requires Staging mode.");
        if (!m_device) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVBuffer: device not set.");
        if (m_upload_cmd != VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVBuffer: an upload is already in flight on this staging buffer.");

        VkDeviceSize total{0};
        for (auto const& e : entries) {
            if (e.size == 0) continue;
            if (e.dst == VK_NULL_HANDLE || e.data == nullptr) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVBuffer: upload entry has no destination or data.");
            total += e.size;
        }
        if (total == 0) return Ir77RETURN<Ir77OperationSucceeded>();

        m_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        auto const created = CreateResources(total, 1);
        if (created->ID() != GUIDIr77OperationSucceeded) return created;

        // pack back to back; one copy region per entry
        std::vector<VkBufferCopy> regions(entries.size());
        VkDeviceSize cursor{0};

        for (std::size_t i = 0; i < entries.size(); i++) {
            if (entries[i].size == 0) continue;

            std::memcpy(static_cast<std::uint8_t*>(m_mapped_ptrs[0]) + cursor, entries[i].data, static_cast<std::size_t>(entries[i].size));

            regions[i].srcOffset = cursor;
            regions[i].dstOffset = entries[i].dst_offset;
            regions[i].size = entries[i].size;

            cursor += entries[i].size;
        }

        vmaFlushAllocation(m_allocator, m_allocations[0], 0, total);  // no-op on coherent memory

        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        m_upload_pool = pool;

        VkCommandBufferAllocateInfo cmd_alloc{};
        cmd_alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd_alloc.commandPool = pool;
        cmd_alloc.commandBufferCount = 1;

        VkResult result = vkAllocateCommandBuffers(device, &cmd_alloc, &m_upload_cmd);

        if (result == VK_SUCCESS) {
            VkCommandBufferBeginInfo begin_info{};
            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            result = vkBeginCommandBuffer(m_upload_cmd, &begin_info);
        }

        if (result == VK_SUCCESS) {
            for (std::size_t i = 0; i < entries.size(); i++) {
                if (entries[i].size == 0) continue;
                vkCmdCopyBuffer(m_upload_cmd, m_buffers[0], entries[i].dst, 1, &regions[i]);
            }

            // visible to whatever reads it next -- vertex / index fetch, indirect, shaders, later copies
            VkMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            vkCmdPipelineBarrier(m_upload_cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);

            result = vkEndCommandBuffer(m_upload_cmd);
        }

        if (result == VK_SUCCESS) {
            VkFenceCreateInfo fence_info{};
            fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            result = vkCreateFence(device, &fence_info, nullptr, &m_upload_fence);
        }

        if (result == VK_SUCCESS) {
            VkSubmitInfo submit_info{};
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &m_upload_cmd;
            result = vkQueueSubmit(queue, 1, &submit_info, m_upload_fence);
        }

        if (result != VK_SUCCESS) {
            ReleaseUpload();
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVBuffer: batch upload submit failed.");
        }

        m_upload_submitted = true;

        if (!wait) return Ir77RETURN<Ir77OperationSucceeded>();

        result = vkWaitForFences(device, 1, &m_upload_fence, VK_TRUE, UINT64_MAX);
        ReleaseUpload();

        if (result != VK_SUCCESS) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVBuffer: batch upload wait failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // true when no upload is in flight on this buffer, or its fence has signalled
    std::shared_ptr<IIr77Return const> IsUploadFinished(bool& finished) {
        finished = true;

        if (!m_upload_submitted || !m_device) return Ir77RETURN<Ir77OperationSucceeded>();

        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        finished = vkGetFenceStatus(device, m_upload_fence) == VK_SUCCESS;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Static and Staging buffers ignore the copy index so callers can always pass the frame index
    std::shared_ptr<IIr77Return const> GetBuffer(std::uint32_t const& copy, VkBuffer* buffer) {
        std::uint32_t const index = (m_mode == Ir77PVBufferMode::Dynamic) ? copy : 0;
        if (index >= m_buffers.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVBuffer: copy index out of range.");

        *buffer = m_buffers[index];

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetBufferInfo(std::uint32_t const& copy, VkDescriptorBufferInfo* info) {
        std::uint32_t const index = (m_mode == Ir77PVBufferMode::Dynamic) ? copy : 0;
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
    // Frees every copy. Waits for the GPU first -- a frame in flight may still be reading the old buffers.
    // Staging skips the device-wide wait: its only reader is its own upload, which ReleaseUpload has already waited for.
    void Release() {
        if (m_buffers.empty()) return;

        if (m_device && m_mode != Ir77PVBufferMode::Staging) {
            VkDevice device;
            m_device->GetDevice(&device);

            vkDeviceWaitIdle(device);
        }

        for (std::size_t i = 0; i < m_buffers.size(); i++) {
            if (m_buffers[i] != VK_NULL_HANDLE) vmaDestroyBuffer(m_allocator, m_buffers[i], m_allocations[i]);
        }

        m_buffers.clear();
        m_allocations.clear();
        m_mapped_ptrs.clear();
        m_size = 0;
    }

    // Frees the upload's command buffer and fence -- after waiting on the fence if it was submitted, so the GPU is never still using them
    void ReleaseUpload() {
        if (!m_device) return;

        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        if (m_upload_submitted) vkWaitForFences(device, 1, &m_upload_fence, VK_TRUE, UINT64_MAX);

        if (m_upload_fence != VK_NULL_HANDLE) vkDestroyFence(device, m_upload_fence, nullptr);
        if (m_upload_cmd != VK_NULL_HANDLE) vkFreeCommandBuffers(device, m_upload_pool, 1, &m_upload_cmd);

        m_upload_fence = VK_NULL_HANDLE;
        m_upload_cmd = VK_NULL_HANDLE;
        m_upload_pool = VK_NULL_HANDLE;
        m_upload_submitted = false;
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

    // Staging uploads -- the pool belongs to the device; this buffer only borrows it for its one command buffer
    VkCommandPool m_upload_pool{VK_NULL_HANDLE};

    VkCommandBuffer m_upload_cmd{VK_NULL_HANDLE};

    VkFence m_upload_fence{VK_NULL_HANDLE};

    bool m_upload_submitted{false};
};
}  // namespace NSIr77PeregrineV