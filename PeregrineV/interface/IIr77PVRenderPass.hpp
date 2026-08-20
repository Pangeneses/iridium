#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <cstring>
#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

typedef struct IIr77PVRenderPass : virtual public IIr77Enlisted {
    IIr77PVRenderPass() = default;

    virtual std::shared_ptr<IIr77Return const> CreateRenderPass() = 0;

    virtual std::shared_ptr<IIr77Return const> GetRenderPass(VkRenderPass* render_pass) = 0;

    virtual ~IIr77PVRenderPass() = default;
}* PIr77PVRenderPass;
}  // namespace NSIr77REDOS
