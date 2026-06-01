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
struct IIr77PVImageView;
struct IIr77PVMemory;

typedef struct IIr77PVDepthTarget : virtual public IIr77Enlisted {
    IIr77PVDepthTarget() = default;

    virtual std::shared_ptr<IIr77Return const> SetFormat(VkFormat const& format) = 0;

    virtual std::shared_ptr<IIr77Return const> GetFormat(VkFormat& format) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetExtent(VkExtent2D const& extent) = 0;

    virtual std::shared_ptr<IIr77Return const> GetExtent(VkExtent2D& extent) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetMemory(std::shared_ptr<IIr77PVMemory const>& memory) = 0;

    virtual std::shared_ptr<IIr77Return const> GetMemory(std::shared_ptr<IIr77PVMemory const>& memory) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetStencil(bool enabled) = 0;

    virtual std::shared_ptr<IIr77Return const> GetStencil(bool& enabled) const = 0;

    virtual std::shared_ptr<IIr77Return const> GetImage(std::shared_ptr<IIr77PVImage const>& image) const = 0;

    virtual std::shared_ptr<IIr77Return const> GetImageView(std::shared_ptr<IIr77PVImageView const>& view) const = 0;

    virtual std::shared_ptr<IIr77Return const> Build() = 0;

    virtual ~IIr77PVDepthTarget() = default;
}* PIr77PVDepthTarget;
}  // namespace NSIr77REDOS

/*
No framebuffer here — depth target attaches into a render target's framebuffer, it doesn't own one. SetStencil toggles combined depth/stencil vs depth-only,
which drives format selection and aspect flags internally.
*/