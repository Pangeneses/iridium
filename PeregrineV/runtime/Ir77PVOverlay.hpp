// runtime/GPU/Ir77PVOverlay.hpp
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
#include "../interface/IIr77PVSwapchain.hpp"
#include "../interface/IIr77PVLayout.hpp"
#include "../interface/IIr77PVOverlay.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77PVOverlay : public Ir77Enlisted, public IIr77PVOverlay, public std::enable_shared_from_this<Ir77PVOverlay> {
   public:
    Ir77PVOverlay() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    ~Ir77PVOverlay() {
        if (!m_device) return;

        VkDevice device;
        m_device->GetDevice(&device);

        vkDeviceWaitIdle(device);

        DestroyResources(device);

        // extent-independent -- destroyed only here, never on resize
        if (m_sampler != VK_NULL_HANDLE) vkDestroySampler(device, m_sampler, nullptr);

        vkDestroyFence(device, m_upload_fence, nullptr);
        vkDestroyCommandPool(device, m_upload_cmd_pool, nullptr);
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVOverlay>(uid);

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

        else if (iid == &GUIDIIr77PVOverlay)
            obj = std::shared_ptr<IIr77PVOverlay>(shared_from_this(), static_cast<IIr77PVOverlay*>(this));

        else if (iid == &GUIDIr77PVOverlay)
            obj = std::shared_ptr<Ir77PVOverlay>(shared_from_this(), static_cast<Ir77PVOverlay*>(this));

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

    std::shared_ptr<IIr77Return const> SetSwapchain(std::shared_ptr<IIr77PVSwapchain> swapchain) {
        m_swapchain = swapchain;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetLayout(std::shared_ptr<IIr77PVLayout> layout_cef) {
        m_layout_cef = layout_cef;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetResizing(bool const& resizing) {
        m_resizing = resizing;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CreateResources() {
        if (!m_device) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: device not set.");
        if (!m_swapchain) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: swapchain not set.");
        if (!m_layout_cef) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: CEF layout not set.");
        if (m_allocator == VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: allocator not set.");

        VkDevice device;
        m_device->GetDevice(&device);

        m_swapchain->GetSwapchainExtents(m_swapchain_extent);

        if (!CreateStagingBuffer()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: staging buffer failed.");

        if (!CreateImage()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: image failed.");

        if (!DefineImageView(device)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: image view failed.");

        if (!DefineSampler(device)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: sampler failed.");

        if (!DefineUploadCommandPool(device)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: upload command pool failed.");

        if (!AllocateUploadCommandBuffer(device)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: upload command buffer failed.");

        if (!DefineUploadFence(device)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: upload fence failed.");

        // CEF layout has a single set at index 0
        if (m_layout_cef->AllocateSet(0, &m_descriptor_set)->ID() != GUIDIr77OperationSucceeded) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: descriptor set allocation failed.");
        }

        UpdateDescriptorSet(device);

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Overlay resources created.");
    }

    std::shared_ptr<IIr77Return const> Resize() {
        m_resizing = true;

        VkDevice device;
        m_device->GetDevice(&device);

        vkDeviceWaitIdle(device);

        DestroyResources(device);

        m_swapchain->GetSwapchainExtents(m_swapchain_extent);

        auto const result = RebuildExtentResources(device);

        m_resizing = false;

        return result;
    }

    std::shared_ptr<IIr77Return const> UploadFrame(const void* buffer, int cef_width, int cef_height) {
        if (m_resizing) return Ir77RETURN<Ir77OperationSucceeded>();
        if (buffer == nullptr || cef_width <= 0 || cef_height <= 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: invalid frame.");

        VkDevice device;
        m_device->GetDevice(&device);

        // CEF size drives the texture size here, not the swapchain -- rebuild at the painted size
        if (static_cast<std::uint32_t>(cef_width) != m_swapchain_extent.width || static_cast<std::uint32_t>(cef_height) != m_swapchain_extent.height) {
            vkDeviceWaitIdle(device);

            m_resizing = true;

            DestroyResources(device);

            m_swapchain_extent.width = static_cast<std::uint32_t>(cef_width);
            m_swapchain_extent.height = static_cast<std::uint32_t>(cef_height);

            auto const result = RebuildExtentResources(device);

            m_resizing = false;

            if (result->ID() != GUIDIr77OperationSucceeded) return result;
        }

        VkQueue queue{VK_NULL_HANDLE};
        if (!FindPresentQueue(&queue, nullptr)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: no presentation-capable queue found.");

        vkWaitForFences(device, 1, &m_upload_fence, VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &m_upload_fence);

        std::memcpy(m_staging_mapped, buffer, static_cast<std::size_t>(cef_width) * static_cast<std::size_t>(cef_height) * 4);
        vmaFlushAllocation(m_allocator, m_staging_allocation, 0, VK_WHOLE_SIZE);

        vkResetCommandBuffer(m_upload_cmd_buf, 0);

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        if (vkBeginCommandBuffer(m_upload_cmd_buf, &begin_info) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: vkBeginCommandBuffer failed.");
        }

        VkImageMemoryBarrier to_transfer{};
        to_transfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        to_transfer.oldLayout = m_image_layout;
        to_transfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        to_transfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_transfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_transfer.image = m_image;
        to_transfer.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        to_transfer.srcAccessMask = (m_image_layout == VK_IMAGE_LAYOUT_UNDEFINED) ? 0 : VK_ACCESS_SHADER_READ_BIT;
        to_transfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        VkPipelineStageFlags const src_stage =
            (m_image_layout == VK_IMAGE_LAYOUT_UNDEFINED) ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

        vkCmdPipelineBarrier(m_upload_cmd_buf, src_stage, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &to_transfer);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {m_swapchain_extent.width, m_swapchain_extent.height, 1};

        vkCmdCopyBufferToImage(m_upload_cmd_buf, m_staging_buffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        VkImageMemoryBarrier to_shader_read{};
        to_shader_read.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        to_shader_read.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        to_shader_read.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        to_shader_read.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_shader_read.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_shader_read.image = m_image;
        to_shader_read.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        to_shader_read.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        to_shader_read.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(m_upload_cmd_buf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                             &to_shader_read);

        m_image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        if (vkEndCommandBuffer(m_upload_cmd_buf) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: vkEndCommandBuffer failed.");
        }

        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &m_upload_cmd_buf;

        if (vkQueueSubmit(queue, 1, &submit_info, m_upload_fence) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: vkQueueSubmit failed.");
        }

        m_ever_uploaded = true;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetImageView(VkImageView* image_view) {
        *image_view = m_image_view;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetSampler(VkSampler* sampler) {
        *sampler = m_sampler;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetExtent(VkExtent2D& extent) {
        extent = m_swapchain_extent;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetDescriptorSet(VkDescriptorSet* descriptor_set) {
        *descriptor_set = m_descriptor_set;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> IsResizing(bool& resizing) const {
        resizing = m_resizing;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> HasEverUploaded(bool& uploaded) const {
        uploaded = m_ever_uploaded;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    // staging + image + view at the current extent, then repoint the descriptor at the new view
    std::shared_ptr<IIr77Return const> RebuildExtentResources(VkDevice device) {
        if (!CreateStagingBuffer()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: staging buffer resize failed.");

        if (!CreateImage()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: image resize failed.");

        if (!DefineImageView(device)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: image view resize failed.");

        UpdateDescriptorSet(device);

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Overlay resized.");
    }

    bool FindPresentQueue(VkQueue* queue, std::uint32_t* family_index) {
        std::vector<Ir77PVQueueFamily> queue_families{};
        m_device->GetQueueFamilies(queue_families);

        for (std::size_t i = 0; i < queue_families.size(); i++) {
            if (queue_families[i].presentation == VK_TRUE) {
                if (queue) *queue = queue_families[i].queue;
                if (family_index) *family_index = static_cast<std::uint32_t>(i);
                return true;
            }
        }

        return false;
    }

    bool CreateStagingBuffer() {
        VkDeviceSize const size = static_cast<VkDeviceSize>(m_swapchain_extent.width) * m_swapchain_extent.height * 4;

        VkBufferCreateInfo buffer_info{};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = size;
        buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo alloc_info{};
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
        alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo result_info{};
        if (vmaCreateBuffer(m_allocator, &buffer_info, &alloc_info, &m_staging_buffer, &m_staging_allocation, &result_info) != VK_SUCCESS) {
            return false;
        }

        m_staging_mapped = result_info.pMappedData;

        return true;
    }

    bool CreateImage() {
        VkImageCreateInfo image_info{};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.format = VK_FORMAT_B8G8R8A8_UNORM;
        image_info.extent = {m_swapchain_extent.width, m_swapchain_extent.height, 1};
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo alloc_info{};
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        if (vmaCreateImage(m_allocator, &image_info, &alloc_info, &m_image, &m_image_allocation, nullptr) != VK_SUCCESS) {
            return false;
        }

        m_image_layout = VK_IMAGE_LAYOUT_UNDEFINED;

        return true;
    }

    bool DefineImageView(VkDevice device) {
        VkImageViewCreateInfo view_info{};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = m_image;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = VK_FORMAT_B8G8R8A8_UNORM;
        view_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        return vkCreateImageView(device, &view_info, nullptr, &m_image_view) == VK_SUCCESS;
    }

    bool DefineSampler(VkDevice device) {
        if (m_sampler != VK_NULL_HANDLE) return true;  // extent-independent -- only create once

        VkSamplerCreateInfo sampler_info{};
        sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_info.magFilter = VK_FILTER_LINEAR;
        sampler_info.minFilter = VK_FILTER_LINEAR;
        sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.anisotropyEnable = VK_FALSE;
        sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        sampler_info.unnormalizedCoordinates = VK_FALSE;
        sampler_info.compareEnable = VK_FALSE;
        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        return vkCreateSampler(device, &sampler_info, nullptr, &m_sampler) == VK_SUCCESS;
    }

    bool DefineUploadCommandPool(VkDevice device) {
        if (m_upload_cmd_pool != VK_NULL_HANDLE) return true;

        std::uint32_t family_index{0};
        if (!FindPresentQueue(nullptr, &family_index)) return false;

        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex = family_index;

        return vkCreateCommandPool(device, &pool_info, nullptr, &m_upload_cmd_pool) == VK_SUCCESS;
    }

    bool AllocateUploadCommandBuffer(VkDevice device) {
        if (m_upload_cmd_buf != VK_NULL_HANDLE) return true;

        VkCommandBufferAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool = m_upload_cmd_pool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = 1;

        return vkAllocateCommandBuffers(device, &alloc_info, &m_upload_cmd_buf) == VK_SUCCESS;
    }

    bool DefineUploadFence(VkDevice device) {
        if (m_upload_fence != VK_NULL_HANDLE) return true;

        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        return vkCreateFence(device, &fence_info, nullptr, &m_upload_fence) == VK_SUCCESS;
    }

    // extent-dependent resources only -- sampler, pool, command buffer, fence and descriptor set survive a resize
    void DestroyResources(VkDevice device) {
        if (m_image_view != VK_NULL_HANDLE) vkDestroyImageView(device, m_image_view, nullptr);
        if (m_image != VK_NULL_HANDLE) vmaDestroyImage(m_allocator, m_image, m_image_allocation);
        if (m_staging_buffer != VK_NULL_HANDLE) vmaDestroyBuffer(m_allocator, m_staging_buffer, m_staging_allocation);

        m_image_view = VK_NULL_HANDLE;
        m_image = VK_NULL_HANDLE;
        m_image_allocation = VK_NULL_HANDLE;
        m_image_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        m_staging_buffer = VK_NULL_HANDLE;
        m_staging_allocation = VK_NULL_HANDLE;
        m_staging_mapped = nullptr;
    }

    void UpdateDescriptorSet(VkDevice device) {
        VkDescriptorImageInfo image_info{};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = m_image_view;
        image_info.sampler = m_sampler;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_descriptor_set;
        write.dstBinding = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo = &image_info;

        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }

   private:
    bool m_resizing{false};

    bool m_ever_uploaded{false};

    std::shared_ptr<IIr77PVDevice> m_device;

    std::shared_ptr<IIr77PVSwapchain> m_swapchain;

    std::shared_ptr<IIr77PVLayout> m_layout_cef;

    VkExtent2D m_swapchain_extent{};

    VmaAllocator m_allocator{VK_NULL_HANDLE};

    VkBuffer m_staging_buffer{VK_NULL_HANDLE};

    VmaAllocation m_staging_allocation{VK_NULL_HANDLE};

    void* m_staging_mapped{nullptr};

    VkImage m_image{VK_NULL_HANDLE};

    VmaAllocation m_image_allocation{VK_NULL_HANDLE};

    VkImageLayout m_image_layout{VK_IMAGE_LAYOUT_UNDEFINED};

    VkImageView m_image_view{VK_NULL_HANDLE};

    VkSampler m_sampler{VK_NULL_HANDLE};

    VkCommandPool m_upload_cmd_pool{VK_NULL_HANDLE};

    VkCommandBuffer m_upload_cmd_buf{VK_NULL_HANDLE};

    VkFence m_upload_fence{VK_NULL_HANDLE};

    VkDescriptorSet m_descriptor_set{VK_NULL_HANDLE};
};
}  // namespace NSIr77PeregrineV