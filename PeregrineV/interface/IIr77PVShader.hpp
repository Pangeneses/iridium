#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVDevice.hpp"

#include "../server/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

typedef struct IIr77PVShader : virtual public IIr77Enlisted {
    IIr77PVShader() = default;

    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    virtual std::shared_ptr<IIr77Return const> LoadShader(std::string const& filename, std::uint64_t const& shader_id, Ir77PVShaderStage shader_stage) = 0;

    virtual std::shared_ptr<IIr77Return const> GetPipelineShaderStageInfos(std::vector<VkPipelineShaderStageCreateInfo>& infos,
                                                                           std::vector<std::uint64_t> const& id_list) = 0;

    virtual ~IIr77PVShader() = default;
}* pIIr77PVShader;
}  // namespace NSIr77PeregrineV
