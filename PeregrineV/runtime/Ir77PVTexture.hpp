#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"
#include "../../Ir77RT/dictionary/IDIr77RET.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77PVDevice.hpp"
#include "../interface/IIr77PVTexture.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77PVTexture : public Ir77Enlisted, public IIr77PVTexture, public std::enable_shared_from_this<Ir77PVTexture> {
   public:
    Ir77PVTexture() {
        m_enlisted_uuid.Generate();

        m_enlisted = std::chrono::system_clock::now();
    }

    ~Ir77PVTexture() {
        if (!m_device) return;

        VkDevice device;
        m_device->GetDevice(&device);

        Destroy(device);
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVTexture>(uid);

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

        else if (iid == &GUIDIIr77PVTexture)
            obj = std::shared_ptr<IIr77PVTexture>(shared_from_this(), static_cast<IIr77PVTexture*>(this));

        else if (iid == &GUIDIr77PVTexture)
            obj = std::shared_ptr<Ir77PVTexture>(shared_from_this(), static_cast<Ir77PVTexture*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // configuration -- SetKind first; it resets format and sampler to the kind's defaults
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

    std::shared_ptr<IIr77Return const> SetKind(Ir77PVTextureKind const& kind) {
        m_kind = kind;
        m_sampler_desc = Ir77PVSamplerDesc{};

        switch (m_kind) {
            case Ir77PVTextureKind::Color:
                m_format = VK_FORMAT_R8G8B8A8_SRGB;
                break;

            case Ir77PVTextureKind::Data:
                m_format = VK_FORMAT_R8G8B8A8_UNORM;
                break;

            case Ir77PVTextureKind::Cube:
                m_format = VK_FORMAT_R8G8B8A8_SRGB;
                m_sampler_desc.address = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                break;

            case Ir77PVTextureKind::Depth:
                // outside the shadow map = lit (border depth 1.0); compare on by default for sampler2DShadow / hardware PCF
                m_format = VK_FORMAT_D32_SFLOAT;
                m_sampler_desc.address = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
                m_sampler_desc.compare = true;
                break;

            default:
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTexture: unknown kind.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetFormat(VkFormat const& format) {
        m_format = format;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // anisotropy > 0 is honoured only if the device supports samplerAnisotropy, and is clamped to its limit.
    // The logical device must also have enabled the feature -- that is not checkable from here.
    std::shared_ptr<IIr77Return const> SetSampler(Ir77PVSamplerDesc const& sampler) {
        m_sampler_desc = sampler;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // creation
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> CreateFromPixels(VkCommandPool const& pool, VkQueue const& queue, void const* pixels, std::uint32_t const& width,
                                                         std::uint32_t const& height, bool const& mipmaps) {
        if (m_kind != Ir77PVTextureKind::Color && m_kind != Ir77PVTextureKind::Data)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTexture: CreateFromPixels needs Color or Data kind.");

        if (pixels == nullptr || width == 0 || height == 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTexture: empty pixels.");

        return CreateSampled(pool, queue, {pixels}, width, height, mipmaps);
    }

    std::shared_ptr<IIr77Return const> CreateCube(VkCommandPool const& pool, VkQueue const& queue, std::array<void const*, 6> const& faces,
                                                   std::uint32_t const& size, bool const& mipmaps) {
        if (m_kind != Ir77PVTextureKind::Cube) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTexture: CreateCube needs Cube kind.");

        if (size == 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTexture: cube size is zero.");

        for (auto const* face : faces)
            if (face == nullptr) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTexture: missing cube face.");

        return CreateSampled(pool, queue, std::vector<void const*>(faces.begin(), faces.end()), size, size, mipmaps);
    }

    std::shared_ptr<IIr77Return const> CreateSolid(VkCommandPool const& pool, VkQueue const& queue, std::uint32_t const& rgba) {
        if (m_kind != Ir77PVTextureKind::Color && m_kind != Ir77PVTextureKind::Data)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTexture: CreateSolid needs Color or Data kind.");

        if (BytesPerPixel(m_format) != 4) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTexture: CreateSolid needs a 4-byte format.");

        // 1x1 never needs filtering between texels
        m_sampler_desc.filter = VK_FILTER_NEAREST;

        return CreateSampled(pool, queue, {&rgba}, 1, 1, false);
    }

    // Cleared to 1.0 and left in DEPTH_STENCIL_READ_ONLY_OPTIMAL, so it is valid to sample before any shadow pass has run
    // (reads as "fully lit"). The shadow render pass can use initialLayout UNDEFINED or DEPTH_STENCIL_READ_ONLY_OPTIMAL and
    // must use finalLayout DEPTH_STENCIL_READ_ONLY_OPTIMAL. A 1x1 Depth texture doubles as the shadow-map placeholder.
    std::shared_ptr<IIr77Return const> CreateDepth(VkCommandPool const& pool, VkQueue const& queue, std::uint32_t const& width,
                                                    std::uint32_t const& height) {
        if (m_kind != Ir77PVTextureKind::Depth) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTexture: CreateDepth needs Depth kind.");

        if (width == 0 || height == 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTexture: depth extent is zero.");

        if (!Ready()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTexture: device or allocator not set.");

        VkDevice device;
        m_device->GetDevice(&device);

        Destroy(device);

        m_extent = {width, height};
        m_layers = 1;
        m_mip_levels = 1;

        VkImageUsageFlags const usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        if (!CreateImage(usage, 0)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTexture: depth image failed.");

        VkCommandBuffer cmd{VK_NULL_HANDLE};
        if (!BeginOneTime(device, pool, &cmd)) {
            Destroy(device);
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTexture: depth init command buffer failed.");
        }

        Barrier(cmd, VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

        VkClearDepthStencilValue const clear{1.0f, 0};
        VkImageSubresourceRange const range{VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
        vkCmdClearDepthStencilImage(cmd, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear, 1, &range);

        Barrier(cmd, VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

        if (!EndOneTime(device, pool, queue, cmd)) {
            Destroy(device);
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTexture: depth init submit failed.");
        }

        // linear on a compare sampler = 2x2 hardware PCF; needs the format feature, otherwise fall back to nearest
        if (m_sampler_desc.filter == VK_FILTER_LINEAR && !HasFormatFeatures(VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
            m_sampler_desc.filter = VK_FILTER_NEAREST;

        if (!CreateView(device, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT) || !CreateSamplerObject(device)) {
            Destroy(device);
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTexture: depth view or sampler failed.");
        }

        m_read_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Depth Texture.");
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // queries
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> GetImageInfo(VkDescriptorImageInfo* info) {
        if (m_view == VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTexture: not created.");

        info->imageView = m_view;
        info->sampler = m_sampler;
        info->imageLayout = m_read_layout;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetImage(VkImage* image) {
        *image = m_image;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetView(VkImageView* view) {
        *view = m_view;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetSampler(VkSampler* sampler) {
        *sampler = m_sampler;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetExtent(VkExtent2D* extent) {
        *extent = m_extent;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetFormat(VkFormat* format) {
        *format = m_format;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetMipLevels(std::uint32_t* mip_levels) {
        *mip_levels = m_mip_levels;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetKind(Ir77PVTextureKind* kind) {
        *kind = m_kind;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // internals
    // -------------------------------------------------------------------------------------------------------------------------------------
   private:
    bool Ready() const { return m_device && m_allocator != VK_NULL_HANDLE; }

    static std::uint32_t BytesPerPixel(VkFormat const& format) {
        switch (format) {
            case VK_FORMAT_R8G8B8A8_SRGB:
            case VK_FORMAT_R8G8B8A8_UNORM:
            case VK_FORMAT_B8G8R8A8_SRGB:
            case VK_FORMAT_B8G8R8A8_UNORM:
            case VK_FORMAT_R32_SFLOAT:
                return 4;
            case VK_FORMAT_R8_UNORM:
                return 1;
            case VK_FORMAT_R8G8_UNORM:
                return 2;
            case VK_FORMAT_R16G16B16A16_SFLOAT:
                return 8;
            case VK_FORMAT_R32G32B32A32_SFLOAT:
                return 16;
            default:
                return 0;
        }
    }

    // Shared path for Color / Data / Cube: stage every layer, copy to mip 0, then blit the chain or transition straight to read
    std::shared_ptr<IIr77Return const> CreateSampled(VkCommandPool const& pool, VkQueue const& queue, std::vector<void const*> const& layers,
                                                      std::uint32_t const& width, std::uint32_t const& height, bool const& mipmaps) {
        if (!Ready()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTexture: device or allocator not set.");

        std::uint32_t const bpp = BytesPerPixel(m_format);
        if (bpp == 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTexture: unsupported upload format.");

        VkDevice device;
        m_device->GetDevice(&device);

        Destroy(device);

        bool const can_blit = HasFormatFeatures(VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT | VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT);

        m_extent = {width, height};
        m_layers = static_cast<std::uint32_t>(layers.size());
        m_mip_levels = (mipmaps && can_blit) ? static_cast<std::uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1 : 1;

        bool const cube = m_kind == Ir77PVTextureKind::Cube;

        VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        if (m_mip_levels > 1) usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        if (!CreateImage(usage, cube ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTexture: image failed.");

        // staging -- layers back to back
        VkDeviceSize const layer_bytes = static_cast<VkDeviceSize>(width) * height * bpp;
        VkDeviceSize const total_bytes = layer_bytes * m_layers;

        VkBufferCreateInfo staging_info{};
        staging_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        staging_info.size = total_bytes;
        staging_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        staging_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo staging_alloc{};
        staging_alloc.usage = VMA_MEMORY_USAGE_AUTO;
        staging_alloc.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer staging{VK_NULL_HANDLE};
        VmaAllocation staging_allocation{VK_NULL_HANDLE};
        VmaAllocationInfo staging_result{};

        if (vmaCreateBuffer(m_allocator, &staging_info, &staging_alloc, &staging, &staging_allocation, &staging_result) != VK_SUCCESS) {
            Destroy(device);
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTexture: staging failed.");
        }

        for (std::uint32_t i = 0; i < m_layers; i++)
            std::memcpy(static_cast<std::uint8_t*>(staging_result.pMappedData) + layer_bytes * i, layers[i], static_cast<std::size_t>(layer_bytes));

        vmaFlushAllocation(m_allocator, staging_allocation, 0, VK_WHOLE_SIZE);

        // record
        VkCommandBuffer cmd{VK_NULL_HANDLE};
        if (!BeginOneTime(device, pool, &cmd)) {
            vmaDestroyBuffer(m_allocator, staging, staging_allocation);
            Destroy(device);
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTexture: upload command buffer failed.");
        }

        Barrier(cmd, VK_IMAGE_ASPECT_COLOR_BIT, 0, m_mip_levels, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

        std::vector<VkBufferImageCopy> regions(m_layers);
        for (std::uint32_t i = 0; i < m_layers; i++) {
            regions[i].bufferOffset = layer_bytes * i;
            regions[i].imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, i, 1};
            regions[i].imageExtent = {width, height, 1};
        }

        vkCmdCopyBufferToImage(cmd, staging, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_layers, regions.data());

        if (m_mip_levels > 1) {
            GenerateMips(cmd);
        } else {
            Barrier(cmd, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        }

        bool const submitted = EndOneTime(device, pool, queue, cmd);

        vmaDestroyBuffer(m_allocator, staging, staging_allocation);

        if (!submitted) {
            Destroy(device);
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTexture: upload submit failed.");
        }

        if (!CreateView(device, cube ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT) || !CreateSamplerObject(device)) {
            Destroy(device);
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVTexture: view or sampler failed.");
        }

        m_read_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Texture.");
    }

    // Each level is blitted from the one above, then that source level moves to SHADER_READ; the last level moves at the end
    void GenerateMips(VkCommandBuffer cmd) {
        std::int32_t w = static_cast<std::int32_t>(m_extent.width);
        std::int32_t h = static_cast<std::int32_t>(m_extent.height);

        for (std::uint32_t level = 1; level < m_mip_levels; level++) {
            Barrier(cmd, VK_IMAGE_ASPECT_COLOR_BIT, level - 1, 1, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

            std::int32_t const next_w = std::max(w / 2, 1);
            std::int32_t const next_h = std::max(h / 2, 1);

            VkImageBlit blit{};
            blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, level - 1, 0, m_layers};
            blit.srcOffsets[1] = {w, h, 1};
            blit.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, level, 0, m_layers};
            blit.dstOffsets[1] = {next_w, next_h, 1};

            vkCmdBlitImage(cmd, m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

            Barrier(cmd, VK_IMAGE_ASPECT_COLOR_BIT, level - 1, 1, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

            w = next_w;
            h = next_h;
        }

        Barrier(cmd, VK_IMAGE_ASPECT_COLOR_BIT, m_mip_levels - 1, 1, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }

    // true when every bit in `features` is supported for m_format with optimal tiling
    bool HasFormatFeatures(VkFormatFeatureFlags const& features) {
        VkPhysicalDevice physical{VK_NULL_HANDLE};
        m_device->GetPhysicalDevice(&physical);

        VkFormatProperties props{};
        vkGetPhysicalDeviceFormatProperties(physical, m_format, &props);

        return (props.optimalTilingFeatures & features) == features;
    }

    bool CreateImage(VkImageUsageFlags const& usage, VkImageCreateFlags const& flags) {
        VkImageCreateInfo image_info{};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.flags = flags;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.format = m_format;
        image_info.extent = {m_extent.width, m_extent.height, 1};
        image_info.mipLevels = m_mip_levels;
        image_info.arrayLayers = m_layers;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.usage = usage;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo alloc_info{};
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        return vmaCreateImage(m_allocator, &image_info, &alloc_info, &m_image, &m_allocation, nullptr) == VK_SUCCESS;
    }

    bool CreateView(VkDevice device, VkImageViewType const& type, VkImageAspectFlags const& aspect) {
        VkImageViewCreateInfo view_info{};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = m_image;
        view_info.viewType = type;
        view_info.format = m_format;
        view_info.subresourceRange = {aspect, 0, m_mip_levels, 0, m_layers};

        return vkCreateImageView(device, &view_info, nullptr, &m_view) == VK_SUCCESS;
    }

    bool CreateSamplerObject(VkDevice device) {
        VkPhysicalDevice physical{VK_NULL_HANDLE};
        m_device->GetPhysicalDevice(&physical);

        VkPhysicalDeviceFeatures features{};
        vkGetPhysicalDeviceFeatures(physical, &features);

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physical, &properties);

        bool const anisotropic = m_sampler_desc.anisotropy > 0.0f && features.samplerAnisotropy == VK_TRUE;
        bool const compare = m_sampler_desc.compare && m_kind == Ir77PVTextureKind::Depth;

        VkSamplerCreateInfo sampler_info{};
        sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_info.magFilter = m_sampler_desc.filter;
        sampler_info.minFilter = m_sampler_desc.filter;
        sampler_info.mipmapMode = m_sampler_desc.filter == VK_FILTER_LINEAR ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
        sampler_info.addressModeU = m_sampler_desc.address;
        sampler_info.addressModeV = m_sampler_desc.address;
        sampler_info.addressModeW = m_sampler_desc.address;
        sampler_info.anisotropyEnable = anisotropic ? VK_TRUE : VK_FALSE;
        sampler_info.maxAnisotropy = anisotropic ? std::min(m_sampler_desc.anisotropy, properties.limits.maxSamplerAnisotropy) : 1.0f;
        sampler_info.compareEnable = compare ? VK_TRUE : VK_FALSE;
        sampler_info.compareOp = compare ? VK_COMPARE_OP_LESS_OR_EQUAL : VK_COMPARE_OP_ALWAYS;
        sampler_info.minLod = 0.0f;
        sampler_info.maxLod = static_cast<float>(m_mip_levels);
        sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        sampler_info.unnormalizedCoordinates = VK_FALSE;

        return vkCreateSampler(device, &sampler_info, nullptr, &m_sampler) == VK_SUCCESS;
    }

    void Barrier(VkCommandBuffer cmd, VkImageAspectFlags const& aspect, std::uint32_t const& base_mip, std::uint32_t const& mip_count,
                 VkImageLayout const& from, VkImageLayout const& to, VkAccessFlags const& src_access, VkAccessFlags const& dst_access,
                 VkPipelineStageFlags const& src_stage, VkPipelineStageFlags const& dst_stage) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = from;
        barrier.newLayout = to;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_image;
        barrier.subresourceRange = {aspect, base_mip, mip_count, 0, m_layers};
        barrier.srcAccessMask = src_access;
        barrier.dstAccessMask = dst_access;

        vkCmdPipelineBarrier(cmd, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    static bool BeginOneTime(VkDevice device, VkCommandPool pool, VkCommandBuffer* cmd) {
        VkCommandBufferAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool = pool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(device, &alloc_info, cmd) != VK_SUCCESS) return false;

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        if (vkBeginCommandBuffer(*cmd, &begin_info) != VK_SUCCESS) {
            vkFreeCommandBuffers(device, pool, 1, cmd);
            return false;
        }

        return true;
    }

    static bool EndOneTime(VkDevice device, VkCommandPool pool, VkQueue queue, VkCommandBuffer cmd) {
        bool ok = vkEndCommandBuffer(cmd) == VK_SUCCESS;

        if (ok) {
            VkSubmitInfo submit_info{};
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &cmd;

            ok = vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE) == VK_SUCCESS;
            if (ok) vkQueueWaitIdle(queue);
        }

        vkFreeCommandBuffers(device, pool, 1, &cmd);

        return ok;
    }

    void Destroy(VkDevice device) {
        if (m_sampler != VK_NULL_HANDLE) vkDestroySampler(device, m_sampler, nullptr);
        if (m_view != VK_NULL_HANDLE) vkDestroyImageView(device, m_view, nullptr);
        if (m_image != VK_NULL_HANDLE) vmaDestroyImage(m_allocator, m_image, m_allocation);

        m_sampler = VK_NULL_HANDLE;
        m_view = VK_NULL_HANDLE;
        m_image = VK_NULL_HANDLE;
        m_allocation = VK_NULL_HANDLE;
        m_read_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    }

   private:
    std::shared_ptr<IIr77PVDevice> m_device;

    VmaAllocator m_allocator{VK_NULL_HANDLE};

    Ir77PVTextureKind m_kind{Ir77PVTextureKind::Color};

    VkFormat m_format{VK_FORMAT_R8G8B8A8_SRGB};

    Ir77PVSamplerDesc m_sampler_desc{};

    VkExtent2D m_extent{};

    std::uint32_t m_layers{1};

    std::uint32_t m_mip_levels{1};

    VkImage m_image{VK_NULL_HANDLE};

    VmaAllocation m_allocation{VK_NULL_HANDLE};

    VkImageView m_view{VK_NULL_HANDLE};

    VkSampler m_sampler{VK_NULL_HANDLE};

    VkImageLayout m_read_layout{VK_IMAGE_LAYOUT_UNDEFINED};
};
}  // namespace NSIr77PeregrineV