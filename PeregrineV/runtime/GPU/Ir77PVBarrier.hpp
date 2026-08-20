#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../../dictionary/IDIIr77PeregrineV.hpp"
#include "../../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../../interface/IIr77PVBarrier.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77PVBarrier : public Ir77Enlisted, public IIr77PVBarrier, public std::enable_shared_from_this<Ir77PVBarrier> {
   public:
    Ir77PVBarrier() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberOfUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVBarrier>(uid);

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

        else if (iid == &GUIDIIr77PVBarrier)
            obj = std::shared_ptr<IIr77PVBarrier>(shared_from_this(), static_cast<IIr77PVBarrier*>(this));

        else if (iid == &GUIDIr77PVBarrier)
            obj = std::shared_ptr<Ir77PVBarrier>(shared_from_this(), static_cast<Ir77PVBarrier*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context) {
        m_context = context;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CreatePipelineBarrier() {
        m_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        m_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        m_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        m_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        m_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        m_barrier.image = dst;
        m_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        m_barrier.subresourceRange.baseMipLevel = 0;
        m_barrier.subresourceRange.levelCount = 1;
        m_barrier.subresourceRange.baseArrayLayer = 0;
        m_barrier.subresourceRange.layerCount = 1;
        m_barrier.srcAccessMask = 0;
        m_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(m_cmd_buf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &m_barrier);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CopyBufferToImage {
        if (m_frame_dirty.load(std::memory_order_acquire)) {
            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;
            region.imageOffset = {0, 0, 0};
            region.imageExtent = {m_swapchain_extent.width, m_swapchain_extent.height, 1};

        }
            vkCmdCopyBufferToImage(m_cmd_buf, m_staging_buf, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

            m_frame_dirty.store(false, std::memory_order_release);

            return Ir77RETURN<Ir77OperationSucceeded>();
        }

        std::shared_ptr<IIr77Return const> TransitionSwapchainImage() {
            m_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            m_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            m_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            m_barrier.dstAccessMask = 0;

            vkCmdPipelineBarrier(m_cmd_buf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &m_barrier);

            return Ir77RETURN<Ir77OperationSucceeded>();
        }
        
       private:
        std::shared_ptr<IIr77Enlisted> m_context;

        VkImageMemoryBarrier m_barrier;

        VkBufferImageCopy m_buffer_copy;

        VkPipelineStageFlags2 m_stage_flags;

        VkAccessFlags2 m_access_flags_2;

        VkImageLayout m_layout;
    };
}  // namespace NSIr77PeregrineV