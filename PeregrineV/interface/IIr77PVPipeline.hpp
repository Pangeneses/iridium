#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <cstring>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

enum class Ir77PVPipelineType : uint32_t {
    Graphics = 0,
    Compute = 1,
    RayTrace = 2,
};

struct IIr77PVShader;
struct IIr77PVDescriptorLayout;
struct IIr77PVRenderPass;

typedef struct IIr77PVPipeline : virtual public IIr77Enlisted {
    IIr77PVPipeline() = default;

    virtual std::shared_ptr<IIr77Return const> SetShader(std::shared_ptr<IIr77PVShader const>& shader) = 0;

    virtual std::shared_ptr<IIr77Return const> GetShader(std::shared_ptr<IIr77PVShader const>& shader) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetLayout(std::shared_ptr<IIr77PVDescriptorLayout const>& layout) = 0;

    virtual std::shared_ptr<IIr77Return const> GetLayout(std::shared_ptr<IIr77PVDescriptorLayout const>& layout) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetRenderPass(std::shared_ptr<IIr77PVRenderPass const>& pass) = 0;

    virtual std::shared_ptr<IIr77Return const> GetRenderPass(std::shared_ptr<IIr77PVRenderPass const>& pass) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetType(Ir77PVPipelineType const& type) = 0;

    virtual std::shared_ptr<IIr77Return const> GetType(Ir77PVPipelineType& type) const = 0;

    virtual std::shared_ptr<IIr77Return const> Build() = 0;

    virtual ~IIr77PVPipeline() = default;
}* PIr77PVPipeline;
}  // namespace NSIr77REDOS