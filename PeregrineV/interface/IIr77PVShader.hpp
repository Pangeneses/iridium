#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVDevice.hpp"

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

typedef struct Ir77PVShaderInfo {
    Ir77PVShaderStage stage;
    VkPipelineShaderStageCreateInfo stage_create_info;
    std::vector<char> byte_code;
    std::uint32_t size;
}* pIr77PVShaderInfo;

typedef struct IIr77PVShader : virtual public IIr77Enlisted {
    IIr77PVShader() = default;

    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    virtual std::shared_ptr<IIr77Return const> ReadShader(std::string const& file) = 0;

    virtual std::shared_ptr<IIr77Return const> AddShaderInfo(Ir77PVShaderInfo const& info, std::uint64_t const& id) = 0;

    virtual std::shared_ptr<IIr77Return const> GetPipelineShaderStageInfos(std::vector<VkPipelineShaderStageCreateInfo>& infos,
                                                                           std::vector<std::uint64_t> const& id_list) = 0;

    virtual ~IIr77PVShader() = default;
}* pIIr77PVShader;
}  // namespace NSIr77PeregrineV
