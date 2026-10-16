#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVDevice.hpp"

#include "../types/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// The device's shader registry -- every shader module by name. A handle is an index into the registry, stable for its life, so pipeline
// descriptions can hold handles instead of compiled-in IDs. Each shader's stage and interface come from its own SPIR-V, and a shader that
// breaks the shader contract (Ir77PVTypes.hpp) is rejected when it is registered.
typedef struct IIr77PVShader : virtual public IIr77Enlisted {
    IIr77PVShader() = default;

    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    // ---- register ----------------------------------------------------------------------------------------------------------------------------
    // A SPIR-V file under a name. Fails if the name is already registered or the shader breaks the contract.
    virtual std::shared_ptr<IIr77Return const> LoadShader(std::string const& name, std::string const& filename, Ir77PVShaderHandle& handle) = 0;

    // SPIR-V already in memory (runtime compile, project files) under a name. Fails if the name is already registered or the shader breaks
    // the contract.
    virtual std::shared_ptr<IIr77Return const> AddShader(std::string const& name, std::vector<std::uint32_t> const& spirv, Ir77PVShaderHandle& handle) = 0;

    // ---- queries -----------------------------------------------------------------------------------------------------------------------------
    // IR77PV_NO_SHADER and a failure when the name isn't registered
    virtual std::shared_ptr<IIr77Return const> FindShader(std::string const& name, Ir77PVShaderHandle& handle) = 0;

    virtual std::shared_ptr<IIr77Return const> GetShaderInfo(Ir77PVShaderHandle const& handle, Ir77PVShaderInfo& info) = 0;

    // Stage and reflection only -- no SPIR-V copy
    virtual std::shared_ptr<IIr77Return const> GetShaderReflection(Ir77PVShaderHandle const& handle, Ir77PVShaderStage& stage,
                                                                   Ir77PVShaderReflection& reflection) = 0;

    // Stage create infos for a pipeline, in the order given. Fails on any unknown handle.
    virtual std::shared_ptr<IIr77Return const> GetPipelineShaderStageInfos(std::vector<VkPipelineShaderStageCreateInfo>& infos,
                                                                           std::vector<Ir77PVShaderHandle> const& handles) = 0;

    virtual ~IIr77PVShader() = default;
}* pIIr77PVShader;
}  // namespace NSIr77PeregrineV
