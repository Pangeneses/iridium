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

typedef struct IIr77PVRenderPass : virtual public IIr77Enlisted {
    IIr77PVRenderPass() = default;

    virtual std::shared_ptr<IIr77Return const> AddColorAttachment(VkFormat const& format, VkImageLayout const& initial, VkImageLayout const& final_layout,
                                                                  VkAttachmentLoadOp const& load, VkAttachmentStoreOp const& store) = 0;

    virtual std::shared_ptr<IIr77Return const> SetDepthAttachment(VkFormat const& format, VkImageLayout const& initial, VkImageLayout const& final_layout,
                                                                  VkAttachmentLoadOp const& load, VkAttachmentStoreOp const& store) = 0;

    virtual std::shared_ptr<IIr77Return const> AddDependency(VkSubpassDependency const& dependency) = 0;

    virtual std::shared_ptr<IIr77Return const> GetDependencies(std::vector<VkSubpassDependency>& dependencies) const = 0;

    virtual std::shared_ptr<IIr77Return const> Build() = 0;

    virtual ~IIr77PVRenderPass() = default;
}* PIr77PVRenderPass;
}  // namespace NSIr77REDOS

/*
Internal — VkRenderPass, attachment descriptions, subpass descriptions. Single subpass for now — multi-subpass can come later when you need it.
Build fires vkCreateRenderPass. The attachment order set here must match the IIr77PVFramebuffer attachment order — that's the contract between the two.
*/