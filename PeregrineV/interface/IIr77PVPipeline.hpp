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

    virtual std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context) = 0;

    virtual std::shared_ptr<IIr77Return const> CreatePipeline() = 0;

    virtual ~IIr77PVPipeline() = default;
}* PIr77PVPipeline;
}  // namespace NSIr77PeregrineV