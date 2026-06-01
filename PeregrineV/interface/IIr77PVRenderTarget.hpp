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
struct IIr77PVFramebuffer;
struct IIr77PVRenderPass;
struct IIr77PVMemory;

typedef struct IIr77PVRenderTarget : virtual public IIr77Enlisted {
    IIr77PVRenderTarget() = default;

    virtual std::shared_ptr<IIr77Return const> SetFormat(VkFormat const& format) = 0;

    virtual std::shared_ptr<IIr77Return const> GetFormat(VkFormat& format) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetExtent(VkExtent2D const& extent) = 0;

    virtual std::shared_ptr<IIr77Return const> GetExtent(VkExtent2D& extent) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetMemory(std::shared_ptr<IIr77PVMemory const>& memory) = 0;

    virtual std::shared_ptr<IIr77Return const> GetMemory(std::shared_ptr<IIr77PVMemory const>& memory) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetRenderPass(std::shared_ptr<IIr77PVRenderPass const>& pass) = 0;

    virtual std::shared_ptr<IIr77Return const> GetRenderPass(std::shared_ptr<IIr77PVRenderPass const>& pass) const = 0;

    virtual std::shared_ptr<IIr77Return const> GetImage(std::shared_ptr<IIr77PVImage const>& image) const = 0;

    virtual std::shared_ptr<IIr77Return const> GetImageView(std::shared_ptr<IIr77PVImageView const>& view) const = 0;

    virtual std::shared_ptr<IIr77Return const> GetFramebuffer(std::shared_ptr<IIr77PVFramebuffer const>& framebuffer) const = 0;

    virtual std::shared_ptr<IIr77Return const> Build() = 0;

    virtual ~IIr77PVRenderTarget() = default;
}* PIr77PVRenderTarget;
}  // namespace NSIr77REDOS

/*
Internal — owns and builds IIr77PVImage, IIr77PVImageView, IIr77PVFramebuffer as a unit. Caller sets format, extent, memory, render pass — Build creates all
three internally and wires them together. Getters expose them read-only for binding into command buffers or descriptor sets.
*/