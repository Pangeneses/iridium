#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <array>
#include <cstdint>
#include <memory>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVDevice.hpp"

#include "../server/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

typedef struct IIr77PVTexture : virtual public IIr77Enlisted {
    IIr77PVTexture() = default;

    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    virtual std::shared_ptr<IIr77Return const> SetAllocator(VmaAllocator allocator) = 0;

    // Call first -- resets format and sampler to the kind's defaults (Depth: D32, clamp-to-border white, compare on)
    virtual std::shared_ptr<IIr77Return const> SetKind(Ir77PVTextureKind const& kind) = 0;

    // Overrides the kind's default format (e.g. VK_FORMAT_R16G16B16A16_SFLOAT for an HDR cube)
    virtual std::shared_ptr<IIr77Return const> SetFormat(VkFormat const& format) = 0;

    // anisotropy > 0 is used only if the device supports it (clamped to its limit); the logical device must enable samplerAnisotropy
    virtual std::shared_ptr<IIr77Return const> SetSampler(Ir77PVSamplerDesc const& sampler) = 0;

    // Color / Data: tightly packed pixels, width * height * bytes-per-pixel
    virtual std::shared_ptr<IIr77Return const> CreateFromPixels(VkCommandPool const& pool, VkQueue const& queue, void const* pixels, std::uint32_t const& width,
                                                                std::uint32_t const& height, bool const& mipmaps) = 0;

    // Cube: six faces in +X, -X, +Y, -Y, +Z, -Z order, each size * size * bytes-per-pixel
    virtual std::shared_ptr<IIr77Return const> CreateCube(VkCommandPool const& pool, VkQueue const& queue, std::array<void const*, 6> const& faces,
                                                          std::uint32_t const& size, bool const& mipmaps) = 0;

    // Color / Data: 1x1 of one RGBA8 value (bytes R,G,B,A packed little-endian: 0xAABBGGRR)
    virtual std::shared_ptr<IIr77Return const> CreateSolid(VkCommandPool const& pool, VkQueue const& queue, std::uint32_t const& rgba) = 0;

    // Depth: shadow-map target, cleared to 1.0 and left in DEPTH_STENCIL_READ_ONLY_OPTIMAL so it can be sampled before any
    // shadow pass runs. The shadow render pass must end in DEPTH_STENCIL_READ_ONLY_OPTIMAL. 1x1 doubles as the shadow placeholder.
    virtual std::shared_ptr<IIr77Return const> CreateDepth(VkCommandPool const& pool, VkQueue const& queue, std::uint32_t const& width,
                                                           std::uint32_t const& height) = 0;

    virtual std::shared_ptr<IIr77Return const> GetImageInfo(VkDescriptorImageInfo* info) = 0;

    virtual std::shared_ptr<IIr77Return const> GetImage(VkImage* image) = 0;

    virtual std::shared_ptr<IIr77Return const> GetView(VkImageView* view) = 0;

    virtual std::shared_ptr<IIr77Return const> GetSampler(VkSampler* sampler) = 0;

    virtual std::shared_ptr<IIr77Return const> GetExtent(VkExtent2D* extent) = 0;

    virtual std::shared_ptr<IIr77Return const> GetFormat(VkFormat* format) = 0;

    virtual std::shared_ptr<IIr77Return const> GetMipLevels(std::uint32_t* mip_levels) = 0;

    virtual std::shared_ptr<IIr77Return const> GetKind(Ir77PVTextureKind* kind) = 0;

    virtual ~IIr77PVTexture() = default;
}* pIIr77PVTexture;
}  // namespace NSIr77PeregrineV