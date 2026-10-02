#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <memory>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVInstance.hpp"
#include "IIr77PVDevice.hpp"
#include "IIr77PVSwapchain.hpp"
#include "IIr77PVRenderPass.hpp"
#include "IIr77PVLayout.hpp"
#include "IIr77PVShader.hpp"

#include "../server/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

typedef struct IIr77PVPipelineCPT : virtual public IIr77Enlisted {
    IIr77PVPipelineCPT() = default;

    virtual std::shared_ptr<IIr77Return const> SetInstance(std::shared_ptr<IIr77PVInstance> instance) = 0;

    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    virtual std::shared_ptr<IIr77Return const> SetSwapchain(std::shared_ptr<IIr77PVSwapchain> swapchain) = 0;

    virtual std::shared_ptr<IIr77Return const> SetRenderPass(std::shared_ptr<IIr77PVRenderPass> render_pass) = 0;

    virtual std::shared_ptr<IIr77Return const> SetLayout(std::shared_ptr<IIr77PVLayout> pipeline_layout) = 0;

    virtual std::shared_ptr<IIr77Return const> SetShader(std::shared_ptr<IIr77PVShader> shader_stack) = 0;

    virtual std::shared_ptr<IIr77Return const> SetKind(Ir77PVComputeKind const& kind) = 0;

    virtual std::shared_ptr<IIr77Return const> CreatePipeline() = 0;

    virtual std::shared_ptr<IIr77Return const> GetPipeline(VkPipeline* pipeline) = 0;

    virtual std::shared_ptr<IIr77Return const> GetKind(Ir77PVComputeKind* kind) = 0;

    virtual ~IIr77PVPipelineCPT() = default;
}* pIIr77PVPipelineCPT;
}  // namespace NSIr77PeregrineV