#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <cstring>
#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

struct IIr77PVRenderPass;
struct IIr77PVImageView;

typedef struct IIr77PVFramebuffer : virtual public IIr77Enlisted {
    IIr77PVFramebuffer() = default;

    virtual std::shared_ptr<IIr77Return const> SetRenderPass(std::shared_ptr<IIr77PVRenderPass const>& pass) = 0;

    virtual std::shared_ptr<IIr77Return const> GetRenderPass(std::shared_ptr<IIr77PVRenderPass const>& pass) const = 0;

    virtual std::shared_ptr<IIr77Return const> AddAttachment(std::shared_ptr<IIr77PVImageView const>& view) = 0;

    virtual std::shared_ptr<IIr77Return const> GetAttachments(std::vector<std::shared_ptr<IIr77PVImageView const>>& views) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetExtent(VkExtent2D const& extent) = 0;

    virtual std::shared_ptr<IIr77Return const> GetExtent(VkExtent2D& extent) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetLayers(uint32_t layers) = 0;

    virtual std::shared_ptr<IIr77Return const> GetLayers(uint32_t& layers) const = 0;

    virtual std::shared_ptr<IIr77Return const> Build() = 0;

    virtual ~IIr77PVFramebuffer() = default;
}* PIr77PVFramebuffer;
}  // namespace NSIr77REDOS

/*
Internal — VkFramebuffer. Build fires vkCreateFramebuffer from the render pass + attachment views + extent.
AddAttachment rather than SetAttachments — you build the list incrementally, order matters since it must match the render pass attachment order. Next —
IIr77PVRenderPass?
*/