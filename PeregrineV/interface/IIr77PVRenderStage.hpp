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

enum class Ir77PVRenderStageType : uint32_t {
    Graphics = 0,  // raster draw calls
    Compute = 1,   // compute dispatch
    RayTrace = 2,  // ray tracing dispatch
    Transfer = 3,  // copy / blit
    UI = 4,        // CEF / vector / SDF composite
};

struct IIr77PVPipeline;
struct IIr77PVRenderTarget;
struct IIr77PVDepthTarget;
struct IIr77PVCommandBuffer;

typedef struct IIr77PVRenderStage : virtual public IIr77Enlisted {
    IIr77PVRenderStage() = default;

    virtual std::shared_ptr<IIr77Return const> SetName(std::string const& name) = 0;

    virtual std::shared_ptr<IIr77Return const> GetName(std::string& name) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetType(Ir77PVRenderStageType const& type) = 0;

    virtual std::shared_ptr<IIr77Return const> GetType(Ir77PVRenderStageType& type) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetEnabled(bool enabled) = 0;

    virtual std::shared_ptr<IIr77Return const> GetEnabled(bool& enabled) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetPipeline(std::shared_ptr<IIr77PVPipeline const>& pipeline) = 0;

    virtual std::shared_ptr<IIr77Return const> GetPipeline(std::shared_ptr<IIr77PVPipeline const>& pipeline) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetRenderTarget(std::shared_ptr<IIr77PVRenderTarget const>& target) = 0;

    virtual std::shared_ptr<IIr77Return const> GetRenderTarget(std::shared_ptr<IIr77PVRenderTarget const>& target) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetDepthTarget(std::shared_ptr<IIr77PVDepthTarget const>& target) = 0;

    virtual std::shared_ptr<IIr77Return const> GetDepthTarget(std::shared_ptr<IIr77PVDepthTarget const>& target) const = 0;

    virtual std::shared_ptr<IIr77Return const> AddInput(std::shared_ptr<IIr77PVRenderTarget const>& target) = 0;

    virtual std::shared_ptr<IIr77Return const> GetInputs(std::vector<std::shared_ptr<IIr77PVRenderTarget const>>& inputs) const = 0;

    virtual std::shared_ptr<IIr77Return const> Execute(std::shared_ptr<IIr77PVCommandBuffer const>& cmd) = 0;

    virtual std::shared_ptr<IIr77Return const> Build() = 0;

    virtual ~IIr77PVRenderStage() = default;
}* PIr77PVRenderStage;
}  // namespace NSIr77REDOS

/*
Execute on the graph walks the stage list, skips disabled stages, calls Execute on each with the frame's command buffer, handles inter-stage barriers for render
target transitions automatically. The loop drives the graph, the graph drives the stages, the stages drive the pipelines. Clean all the way down.
*/