// runtime/GPU/Ir77PVOverlay.hpp
#pragma once

#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>

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

// =============================================================================================================================================
// Ir77PVOverlay -- one window's CEF texture: staging buffer, sampled image, view, sampler, descriptor set (CEF layout, set 0, binding 0)
// and its own upload command buffer + fence.
//
//   create:    SetDevice / SetAllocator / SetSwapchain / SetLayout -> CreateResources (image sized to the swapchain)
//   per paint: UploadFrame -- grow-only: the image is reallocated only when a frame doesn't fit (rounded up to 512, clamped to the
//              device limit); smaller frames go to the top-left and the rest of the image is cleared to transparent
//   draw:      GetDescriptorSet; skip while IsResizing or before HasEverUploaded
// =============================================================================================================================================
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

        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);
        if (device == VK_NULL_HANDLE) return;

        vkDeviceWaitIdle(device);

        DestroyExtentResources(device);

        // extent-independent -- destroyed only here, never on resize
        if (m_sampler != VK_NULL_HANDLE) vkDestroySampler(device, m_sampler, nullptr);
        if (m_upload_fence != VK_NULL_HANDLE) vkDestroyFence(device, m_upload_fence, nullptr);
        if (m_upload_pool != VK_NULL_HANDLE) vkDestroyCommandPool(device, m_upload_pool, nullptr);  // frees m_upload_cmd with it
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

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77PVOverlay)
            obj = std::shared_ptr<IIr77PVOverlay>(shared_from_this(), static_cast<IIr77PVOverlay*>(this));

        else if (iid == GUIDIr77PVOverlay)
            obj = std::shared_ptr<Ir77PVOverlay>(shared_from_this(), static_cast<Ir77PVOverlay*>(this));

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

    // set by Paint around a swapchain rebuild -- paints and draws are skipped while true
    std::shared_ptr<IIr77Return const> SetResizing(bool const& resizing) {
        m_resizing = resizing;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // create
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> CreateResources() {
        if (!m_device) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: device not set.");
        if (!m_swapchain) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: swapchain not set.");
        if (!m_layout_cef) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: CEF layout not set.");
        if (m_allocator == VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: allocator not set.");

        m_swapchain->GetSwapchainExtents(m_image_extent);

        // extent-independent first -- the descriptor write in DefineExtentResources needs the sampler and the set
        auto result = DefineSampler();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        result = DefineUploadCommands();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        result = DefineDescriptorSet();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        result = DefineExtentResources();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Overlay.");
    }

    // Back to the swapchain's size -- releases headroom the image grew into
    std::shared_ptr<IIr77Return const> Resize() {
        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        m_resizing = true;

        vkDeviceWaitIdle(device);

        DestroyExtentResources(device);

        m_swapchain->GetSwapchainExtents(m_image_extent);

        auto const result = DefineExtentResources();

        m_resizing = false;

        return result;
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // per paint
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> UploadFrame(void const* buffer, int cef_width, int cef_height) {
        if (m_resizing) return Ir77RETURN<Ir77OperationSucceeded>();
        if (buffer == nullptr || cef_width <= 0 || cef_height <= 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: invalid frame.");

        std::uint32_t const frame_width = static_cast<std::uint32_t>(cef_width);
        std::uint32_t const frame_height = static_cast<std::uint32_t>(cef_height);

        auto result = DefineFit(frame_width, frame_height);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        VkQueue queue{VK_NULL_HANDLE};
        std::uint32_t family{0};

        result = m_device->GetPresentQueue(queue, family);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        // the previous upload must be done before its staging memory and command buffer are reused
        vkWaitForFences(device, 1, &m_upload_fence, VK_TRUE, UINT64_MAX);

        std::memcpy(m_staging_mapped, buffer, static_cast<std::size_t>(frame_width) * frame_height * BYTES_PER_PIXEL);
        vmaFlushAllocation(m_allocator, m_staging_allocation, 0, VK_WHOLE_SIZE);

        result = RecordUpload(frame_width, frame_height);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        vkResetFences(device, 1, &m_upload_fence);

        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &m_upload_cmd;

        if (vkQueueSubmit(queue, 1, &submit_info, m_upload_fence) != VK_SUCCESS) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: vkQueueSubmit failed.");

        m_ever_uploaded = true;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // queries
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> GetImageView(VkImageView* image_view) {
        *image_view = m_image_view;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetSampler(VkSampler* sampler) {
        *sampler = m_sampler;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // the image's size -- can be larger than the swapchain (grow-only headroom)
    std::shared_ptr<IIr77Return const> GetExtent(VkExtent2D& extent) {
        extent = m_image_extent;

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

    // -------------------------------------------------------------------------------------------------------------------------------------
    // define -- extent-independent (once)
    // -------------------------------------------------------------------------------------------------------------------------------------
   private:
    std::shared_ptr<IIr77Return const> DefineSampler() {
        if (m_sampler != VK_NULL_HANDLE) return Ir77RETURN<Ir77OperationSucceeded>();

        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        VkSamplerCreateInfo sampler_info{};
        sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_info.magFilter = VK_FILTER_LINEAR;
        sampler_info.minFilter = VK_FILTER_LINEAR;
        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.anisotropyEnable = VK_FALSE;
        sampler_info.compareEnable = VK_FALSE;
        sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        sampler_info.unnormalizedCoordinates = VK_FALSE;

        if (vkCreateSampler(device, &sampler_info, nullptr, &m_sampler) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: sampler creation failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Own pool with RESET_COMMAND_BUFFER -- the one command buffer is re-recorded every paint (the device's upload pool is transient,
    // for record-once buffers), and a fence created signalled so the first UploadFrame doesn't wait
    std::shared_ptr<IIr77Return const> DefineUploadCommands() {
        if (m_upload_pool != VK_NULL_HANDLE) return Ir77RETURN<Ir77OperationSucceeded>();

        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        VkQueue queue{VK_NULL_HANDLE};
        std::uint32_t family{0};

        auto const result = m_device->GetPresentQueue(queue, family);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex = family;

        if (vkCreateCommandPool(device, &pool_info, nullptr, &m_upload_pool) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: upload command pool creation failed.");

        VkCommandBufferAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool = m_upload_pool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(device, &alloc_info, &m_upload_cmd) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: upload command buffer allocation failed.");

        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateFence(device, &fence_info, nullptr, &m_upload_fence) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: upload fence creation failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // CEF layout has a single set at index 0; it survives every resize -- only its image binding is rewritten
    std::shared_ptr<IIr77Return const> DefineDescriptorSet() {
        if (m_descriptor_set != VK_NULL_HANDLE) return Ir77RETURN<Ir77OperationSucceeded>();

        if (m_layout_cef->AllocateSet(0, &m_descriptor_set)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: descriptor set allocation failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // define -- extent-dependent (rebuilt when the image grows or Resize runs)
    // -------------------------------------------------------------------------------------------------------------------------------------

    // staging + image + view at m_image_extent, then the descriptor repointed at the new view
    std::shared_ptr<IIr77Return const> DefineExtentResources() {
        auto result = DefineStagingBuffer();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        result = DefineImage();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        result = DefineImageView();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        return DefineDescriptorImage();
    }

    // Grow-only: reallocates only when the frame doesn't fit -- rounded up to 512 so a window drag crosses few boundaries, clamped to the
    // device's maximum image size
    std::shared_ptr<IIr77Return const> DefineFit(std::uint32_t const& frame_width, std::uint32_t const& frame_height) {
        if (frame_width <= m_image_extent.width && frame_height <= m_image_extent.height) return Ir77RETURN<Ir77OperationSucceeded>();

        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        VkPhysicalDeviceProperties properties{};
        m_device->GetDeviceProperties(properties);

        std::uint32_t const limit = properties.limits.maxImageDimension2D;
        if (frame_width > limit || frame_height > limit) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: frame larger than the device's maximum image size.");

        auto const headroom = [limit](std::uint32_t const& v) { return std::min((v + 511u) & ~511u, limit); };

        m_resizing = true;

        vkDeviceWaitIdle(device);

        DestroyExtentResources(device);

        m_image_extent.width = std::max(m_image_extent.width, headroom(frame_width));
        m_image_extent.height = std::max(m_image_extent.height, headroom(frame_height));

        auto const result = DefineExtentResources();

        m_resizing = false;

        return result;
    }

    std::shared_ptr<IIr77Return const> DefineStagingBuffer() {
        VkBufferCreateInfo buffer_info{};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = static_cast<VkDeviceSize>(m_image_extent.width) * m_image_extent.height * BYTES_PER_PIXEL;
        buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo alloc_info{};
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
        alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo result_info{};
        if (vmaCreateBuffer(m_allocator, &buffer_info, &alloc_info, &m_staging_buffer, &m_staging_allocation, &result_info) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: staging buffer creation failed.");

        m_staging_mapped = result_info.pMappedData;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineImage() {
        VkImageCreateInfo image_info{};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.format = OVERLAY_FORMAT;
        image_info.extent = {m_image_extent.width, m_image_extent.height, 1};
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo alloc_info{};
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        if (vmaCreateImage(m_allocator, &image_info, &alloc_info, &m_image, &m_image_allocation, nullptr) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: image creation failed.");
        
        m_image_layout = VK_IMAGE_LAYOUT_UNDEFINED;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineImageView() {
        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        VkImageViewCreateInfo view_info{};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = m_image;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = OVERLAY_FORMAT;
        view_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        if (vkCreateImageView(device, &view_info, nullptr, &m_image_view) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: image view creation failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // binding 0 -> the current view; the layout is what the image is in by the time any draw samples it
    std::shared_ptr<IIr77Return const> DefineDescriptorImage() {
        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

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

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // record
    // -------------------------------------------------------------------------------------------------------------------------------------

    // to TRANSFER_DST -> clear the whole image -> copy the frame to the top-left -> to SHADER_READ
    std::shared_ptr<IIr77Return const> RecordUpload(std::uint32_t const& frame_width, std::uint32_t const& frame_height) {
        vkResetCommandBuffer(m_upload_cmd, 0);

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        if (vkBeginCommandBuffer(m_upload_cmd, &begin_info) != VK_SUCCESS) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: vkBeginCommandBuffer failed.");

        bool const first = m_image_layout == VK_IMAGE_LAYOUT_UNDEFINED;

        // the previous frame's draws sampled it in the fragment shader; the first upload has nothing to wait for
        ImageBarrier(m_image_layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, first ? 0 : VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                     first ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

        // the area outside this frame stays transparent when the frame is smaller than the image
        VkClearColorValue const transparent{{0.0f, 0.0f, 0.0f, 0.0f}};
        VkImageSubresourceRange const whole{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCmdClearColorImage(m_upload_cmd, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &transparent, 1, &whole);

        // clear before copy -- both write the same image
        VkMemoryBarrier clear_then_copy{};
        clear_then_copy.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        clear_then_copy.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        clear_then_copy.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(m_upload_cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &clear_then_copy, 0, nullptr, 0, nullptr);

        // staging holds a tightly packed frame_width x frame_height frame
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = frame_width;
        region.bufferImageHeight = frame_height;
        region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {frame_width, frame_height, 1};

        vkCmdCopyBufferToImage(m_upload_cmd, m_staging_buffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        ImageBarrier(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

        m_image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        if (vkEndCommandBuffer(m_upload_cmd) != VK_SUCCESS) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVOverlay: vkEndCommandBuffer failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    void ImageBarrier(VkImageLayout const& from, VkImageLayout const& to, VkAccessFlags const& src_access, VkAccessFlags const& dst_access,
                      VkPipelineStageFlags const& src_stage, VkPipelineStageFlags const& dst_stage) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = from;
        barrier.newLayout = to;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_image;
        barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        barrier.srcAccessMask = src_access;
        barrier.dstAccessMask = dst_access;

        vkCmdPipelineBarrier(m_upload_cmd, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // destroy
    // -------------------------------------------------------------------------------------------------------------------------------------

    // extent-dependent only -- sampler, pool, command buffer, fence and descriptor set survive a resize
    void DestroyExtentResources(VkDevice device) {
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

   private:
    // CEF paints BGRA; UNORM so the shader gets the bytes as-is
    static constexpr VkFormat OVERLAY_FORMAT = VK_FORMAT_B8G8R8A8_UNORM;

    static constexpr std::uint32_t BYTES_PER_PIXEL{4};

    std::shared_ptr<IIr77PVDevice> m_device{};

    std::shared_ptr<IIr77PVSwapchain> m_swapchain{};

    std::shared_ptr<IIr77PVLayout> m_layout_cef{};

    VmaAllocator m_allocator{VK_NULL_HANDLE};

    bool m_resizing{false};

    bool m_ever_uploaded{false};

    VkExtent2D m_image_extent{};  // can exceed the swapchain -- grow-only headroom

    // extent-dependent
    VkBuffer m_staging_buffer{VK_NULL_HANDLE};

    VmaAllocation m_staging_allocation{VK_NULL_HANDLE};

    void* m_staging_mapped{nullptr};

    VkImage m_image{VK_NULL_HANDLE};

    VmaAllocation m_image_allocation{VK_NULL_HANDLE};

    VkImageLayout m_image_layout{VK_IMAGE_LAYOUT_UNDEFINED};

    VkImageView m_image_view{VK_NULL_HANDLE};

    // extent-independent
    VkSampler m_sampler{VK_NULL_HANDLE};

    VkCommandPool m_upload_pool{VK_NULL_HANDLE};

    VkCommandBuffer m_upload_cmd{VK_NULL_HANDLE};

    VkFence m_upload_fence{VK_NULL_HANDLE};

    VkDescriptorSet m_descriptor_set{VK_NULL_HANDLE};
};
}  // namespace NSIr77PeregrineV