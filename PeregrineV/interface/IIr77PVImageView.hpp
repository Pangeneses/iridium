#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <cstring>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

struct IIr77PVImage;

typedef struct IIr77PVImageView : virtual public IIr77Enlisted {
    IIr77PVImageView() = default;

    virtual std::shared_ptr<IIr77Return const> SetImage(std::shared_ptr<IIr77PVImage const>& image) = 0;

    virtual std::shared_ptr<IIr77Return const> GetImage(std::shared_ptr<IIr77PVImage const>& image) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetFormat(VkFormat format) = 0;

    virtual std::shared_ptr<IIr77Return const> GetFormat(VkFormat& format) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetViewType(VkImageViewType type) = 0;

    virtual std::shared_ptr<IIr77Return const> GetViewType(VkImageViewType& type) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetAspect(VkImageAspectFlags aspect) = 0;

    virtual std::shared_ptr<IIr77Return const> GetAspect(VkImageAspectFlags& aspect) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetMipRange(uint32_t base, uint32_t count) = 0;

    virtual std::shared_ptr<IIr77Return const> GetMipRange(uint32_t& base, uint32_t& count) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetLayerRange(uint32_t base, uint32_t count) = 0;

    virtual std::shared_ptr<IIr77Return const> GetLayerRange(uint32_t& base, uint32_t& count) const = 0;

    virtual std::shared_ptr<IIr77Return const> Build() = 0;

    virtual ~IIr77PVImageView() = default;
}* PIr77PVImageView;
}  // namespace NSIr77REDOS

/*
Internal — VkImageView, component swizzle (almost always identity). Swizzle could be exposed if you need texture channel remapping but that's a niche case —
leave it internal for now. Build fires vkCreateImageView using the set state.
*/