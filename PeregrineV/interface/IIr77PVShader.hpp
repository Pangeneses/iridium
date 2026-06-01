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

enum class Ir77PVShaderStage : uint32_t {
    Vertex = 0,
    Fragment = 1,
    Compute = 2,
    Geometry = 3,
    TessellationControl = 4,
    TessellationEvaluation = 5,
    Mesh = 6,
    Task = 7,
    RayGeneration = 8,
    RayMiss = 9,
    RayClosestHit = 10,
    RayAnyHit = 11,
    RayIntersection = 12,
};

typedef struct IIr77PVShader : virtual public IIr77Enlisted {
    IIr77PVShader() = default;

    virtual std::shared_ptr<IIr77Return const> SetStage(Ir77PVShaderStage const& stage) = 0;

    virtual std::shared_ptr<IIr77Return const> GetStage(Ir77PVShaderStage& stage) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetBytecode(std::vector<uint32_t> const& spirv) = 0;

    virtual std::shared_ptr<IIr77Return const> GetBytecode(std::vector<uint32_t>& spirv) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetEntryPoint(std::string const& entry) = 0;

    virtual std::shared_ptr<IIr77Return const> GetEntryPoint(std::string& entry) const = 0;

    virtual std::shared_ptr<IIr77Return const> Build() = 0;

    virtual ~IIr77PVShader() = default;
}* PIr77PVShader;
}  // namespace NSIr77REDOS

/*
Internal — VkShaderModule. Build fires vkCreateShaderModule from the SPIR-V bytecode. The module handle is then consumed by IIr77PVPipeline at pipeline build
time via VkPipelineShaderStageCreateInfo. SetBytecode takes the raw SPIR-V uint32_t vector — your RAM filesystem will feed this directly once that layer exists,
replacing the std::vector source with a buffer handle. Keep that in mind as a future seam.
*/