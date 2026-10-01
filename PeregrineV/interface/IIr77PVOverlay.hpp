#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

#include <memory>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVDevice.hpp"
#include "IIr77PVSwapchain.hpp"
#include "IIr77PVLayout.hpp"

#include "../runtime/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

typedef struct IIr77PVOverlay : virtual public IIr77Enlisted {
    IIr77PVOverlay() = default;

    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    virtual std::shared_ptr<IIr77Return const> SetAllocator(VmaAllocator allocator) = 0;

    virtual std::shared_ptr<IIr77Return const> SetSwapchain(std::shared_ptr<IIr77PVSwapchain> swapchain) = 0;

    virtual std::shared_ptr<IIr77Return const> SetLayout(std::shared_ptr<IIr77PVLayout> layout_cef) = 0;

    virtual std::shared_ptr<IIr77Return const> SetResizing(bool const& resizing) = 0;

    virtual std::shared_ptr<IIr77Return const> CreateResources() = 0;

    virtual std::shared_ptr<IIr77Return const> Resize() = 0;

    virtual std::shared_ptr<IIr77Return const> UploadFrame(const void* buffer, int cef_width, int cef_height) = 0;

    virtual std::shared_ptr<IIr77Return const> GetImageView(VkImageView* image_view) = 0;

    virtual std::shared_ptr<IIr77Return const> GetSampler(VkSampler* sampler) = 0;

    virtual std::shared_ptr<IIr77Return const> GetExtent(VkExtent2D& extent) = 0;

    virtual std::shared_ptr<IIr77Return const> GetDescriptorSet(VkDescriptorSet* descriptor_set) = 0;

    virtual std::shared_ptr<IIr77Return const> IsResizing(bool& resizing) const = 0;

    virtual std::shared_ptr<IIr77Return const> HasEverUploaded(bool& uploaded) const = 0;

    virtual ~IIr77PVOverlay() = default;
}* pIIr77PVOverlay;
}  // namespace NSIr77PeregrineV