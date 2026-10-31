#pragma once

#include <vulkan/vulkan.h>

#include <memory>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVDevice.hpp"
#include "IIr77PVLayout.hpp"
#include "IIr77PVShader.hpp"

#include "../types/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// One compute pipeline for one compute shader from the device's shader registry, built on the device's shared bindless layout
typedef struct IIr77PVPipelineCPT : virtual public IIr77Enlisted {
    IIr77PVPipelineCPT() = default;

    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    // The device's one bindless layout (Ir77PeregrineV::GetLayout)
    virtual std::shared_ptr<IIr77Return const> SetLayout(std::shared_ptr<IIr77PVLayout> layout) = 0;

    virtual std::shared_ptr<IIr77Return const> SetShader(std::shared_ptr<IIr77PVShader> shader_stack) = 0;

    // A registered compute shader -- e.g. FindShader("culling.comp", handle)
    virtual std::shared_ptr<IIr77Return const> SetComputeShader(Ir77PVShaderHandle const& handle) = 0;

    // optional -- the device's VkPipelineCache, so the driver reuses compiled code
    virtual std::shared_ptr<IIr77Return const> SetPipelineCache(VkPipelineCache const& cache) = 0;

    virtual std::shared_ptr<IIr77Return const> CreatePipeline() = 0;

    virtual std::shared_ptr<IIr77Return const> GetPipeline(VkPipeline* pipeline) = 0;

    virtual std::shared_ptr<IIr77Return const> GetComputeShader(Ir77PVShaderHandle* handle) = 0;

    virtual ~IIr77PVPipelineCPT() = default;
}* pIIr77PVPipelineCPT;
}  // namespace NSIr77PeregrineV
