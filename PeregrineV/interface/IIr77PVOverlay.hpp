#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <cstdint>
#include <memory>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVDevice.hpp"
#include "IIr77PVSwapchain.hpp"
#include "IIr77PVDescriptorSet.hpp"

#include "../types/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// One window's CEF texture: staging buffer, sampled image, view, sampler, one bindless texture slot, and its own upload command buffer + fence.
// Grow-only: the image is reallocated only when a frame doesn't fit; the texture slot stays the same for the overlay's whole life.
typedef struct IIr77PVOverlay : virtual public IIr77Enlisted {
    IIr77PVOverlay() = default;

    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    virtual std::shared_ptr<IIr77Return const> SetAllocator(VmaAllocator allocator) = 0;

    virtual std::shared_ptr<IIr77Return const> SetSwapchain(std::shared_ptr<IIr77PVSwapchain> swapchain) = 0;

    // The device's bindless table -- the overlay image gets one texture slot in it
    virtual std::shared_ptr<IIr77Return const> SetDescriptorSet(std::shared_ptr<IIr77PVDescriptorSet> descriptor_set) = 0;

    // Set by Paint around a swapchain rebuild -- paints and draws are skipped while true
    virtual std::shared_ptr<IIr77Return const> SetResizing(bool const& resizing) = 0;

    // Image sized to the swapchain, sampler, upload commands, texture slot
    virtual std::shared_ptr<IIr77Return const> CreateResources() = 0;

    // Back to the swapchain's size -- releases headroom the image grew into
    virtual std::shared_ptr<IIr77Return const> Resize() = 0;

    // One CEF paint (BGRA, tightly packed). Skipped while resizing.
    virtual std::shared_ptr<IIr77Return const> UploadFrame(void const* buffer, int cef_width, int cef_height) = 0;

    virtual std::shared_ptr<IIr77Return const> GetImageView(VkImageView* image_view) = 0;

    virtual std::shared_ptr<IIr77Return const> GetSampler(VkSampler* sampler) = 0;

    // The image's size -- can be larger than the swapchain (grow-only headroom)
    virtual std::shared_ptr<IIr77Return const> GetExtent(VkExtent2D& extent) = 0;

    // Bindless texture slot of the overlay image -> pc.user[0] for the CEF shaders
    virtual std::shared_ptr<IIr77Return const> GetTextureSlot(std::uint32_t& slot) = 0;

    virtual std::shared_ptr<IIr77Return const> IsResizing(bool& resizing) const = 0;

    virtual std::shared_ptr<IIr77Return const> HasEverUploaded(bool& uploaded) const = 0;

    virtual ~IIr77PVOverlay() = default;
}* pIIr77PVOverlay;
}  // namespace NSIr77PeregrineV