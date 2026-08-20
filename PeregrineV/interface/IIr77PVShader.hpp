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

struct Ir77PVShaderInfo {
    VkPipelineShaderStageCreateInfo create_info;
    Ir77PVShaderStage stage;
    std::vector<uint32_t> byte_code;
    std::uint32_t size;
    std::string entry_point;
    VkShaderModule shader_module;
};

typedef struct IIr77PVShader : virtual public IIr77Enlisted {
    IIr77PVShader() = default;

    virtual std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context) = 0;

    virtual std::shared_ptr<IIr77Return const> AddShaderInfo(Ir77PVShaderInfo const& info, std::uint64_t const& id) = 0;

    virtual std::shared_ptr<IIr77Return const> GetShaderInfo(Ir77PVShaderInfo& info, std::uint64_t const& id) const = 0;

    virtual std::shared_ptr<IIr77Return const> GetPipelineShaderStageInfos(std::vector<VkPipelineShaderStageCreateInfo>& infos,
                                                                           std::vector<std::uint64_t> id_list) = 0;

    virtual std::shared_ptr<IIr77Return const> GetShaders(Ir77PVShaderInfo& info, std::shared_ptr<IIr77GUID const>& uid) const = 0;

    virtual ~IIr77PVShader() = default;
}* PIr77PVShader;
}  // namespace NSIr77PeregrineV
