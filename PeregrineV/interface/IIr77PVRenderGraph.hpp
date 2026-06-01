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

struct IIr77PVRenderStage;
struct IIr77PVFrame;
struct IIr77PVQueue;

typedef struct IIr77PVRenderGraph : virtual public IIr77Enlisted {
    IIr77PVRenderGraph() = default;

    virtual std::shared_ptr<IIr77Return const> AddStage(std::shared_ptr<IIr77PVRenderStage const>& stage) = 0;

    virtual std::shared_ptr<IIr77Return const> GetStages(std::vector<std::shared_ptr<IIr77PVRenderStage const>>& stages) const = 0;

    virtual std::shared_ptr<IIr77Return const> RemoveStage(std::shared_ptr<IIr77GUID const>& uid) = 0;

    virtual std::shared_ptr<IIr77Return const> GetStage(std::shared_ptr<IIr77GUID const>& uid, std::shared_ptr<IIr77PVRenderStage const>& stage) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetFrame(std::shared_ptr<IIr77PVFrame const>& frame) = 0;

    virtual std::shared_ptr<IIr77Return const> GetFrame(std::shared_ptr<IIr77PVFrame const>& frame) const = 0;

    virtual std::shared_ptr<IIr77Return const> Execute(std::shared_ptr<IIr77PVQueue const>& queue) = 0;

    virtual std::shared_ptr<IIr77Return const> Reset() = 0;

    virtual std::shared_ptr<IIr77Return const> Build() = 0;

    virtual ~IIr77PVRenderGraph() = default;
}* PIr77PVRenderGraph;
}  // namespace NSIr77REDOS

/*
Execute on the graph walks the stage list, skips disabled stages, calls Execute on each with the frame's command buffer, handles inter-stage barriers for render
target transitions automatically. The loop drives the graph, the graph drives the stages, the stages drive the pipelines. Clean all the way down.
*/