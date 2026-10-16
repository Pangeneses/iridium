#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVDevice.hpp"
#include "IIr77PVRenderPass.hpp"
#include "IIr77PVLayout.hpp"
#include "IIr77PVShader.hpp"

#include "../types/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// One graphics pipeline built from an Ir77PVPipelineDesc, for dynamic rendering into the attachment formats of the pass it is given.
// Normally created through Ir77PVPipelineCache, not directly.
typedef struct IIr77PVPipeline : virtual public IIr77Enlisted {
    IIr77PVPipeline() = default;

    // ---- configuration -----------------------------------------------------------------------------------------------------------------------
    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    // attachment formats of the pass the description names
    virtual std::shared_ptr<IIr77Return const> SetRenderPass(std::shared_ptr<IIr77PVRenderPass> render_pass) = 0;

    virtual std::shared_ptr<IIr77Return const> SetLayout(std::shared_ptr<IIr77PVLayout> pipeline_layout) = 0;

    virtual std::shared_ptr<IIr77Return const> SetShader(std::shared_ptr<IIr77PVShader> shader_stack) = 0;

    virtual std::shared_ptr<IIr77Return const> SetDesc(Ir77PVPipelineDesc const& desc) = 0;

    // optional -- the device's VkPipelineCache, so the driver reuses compiled code
    virtual std::shared_ptr<IIr77Return const> SetPipelineCache(VkPipelineCache const& cache) = 0;

    // ---- create ------------------------------------------------------------------------------------------------------------------------------
    virtual std::shared_ptr<IIr77Return const> CreatePipeline() = 0;

    // ---- queries -----------------------------------------------------------------------------------------------------------------------------
    virtual std::shared_ptr<IIr77Return const> GetPipeline(VkPipeline* pipeline) = 0;

    // what it was built from -- Ir77PVCmdBuffer reads the dynamic-state policy and the shader set from it
    virtual std::shared_ptr<IIr77Return const> GetDesc(Ir77PVPipelineDesc* desc) = 0;

    // false when the vertex shader reads no inputs -- its draws need no vertex buffer (gl_VertexIndex / storage buffers)
    virtual std::shared_ptr<IIr77Return const> GetVertexInput(bool& has_input) = 0;

    virtual ~IIr77PVPipeline() = default;
}* pIIr77PVPipeline;
}  // namespace NSIr77PeregrineV
