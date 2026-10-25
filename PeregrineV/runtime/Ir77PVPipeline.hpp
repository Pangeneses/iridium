#pragma once

#include <vulkan/vulkan.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../types/Ir77PVTypes.hpp"

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77PVPipeline.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// =============================================================================================================================================
// Ir77PVPipeline -- one graphics pipeline built from an Ir77PVPipelineDesc, for dynamic rendering into the attachment formats of the pass it
// is given. Normally created by Ir77PVPipelineCache, which keys pipelines by their description.
//
//   configure: SetDevice / SetRenderPass (attachment formats) / SetLayout / SetShader / SetDesc / [SetPipelineCache]
//   create:    CreatePipeline -> DefineDynamicState -> DefineVertexInputState -> DefineInputAssemblyState -> DefineViewportState
//              -> DefineRasterizationState -> DefineMultisamplingState -> DefineDepthStencilState -> DefineColorBlendAttachment
//              -> DefineColorBlendState -> DefineShaderStages -> DefinePipeline
//
// Dynamic: viewport, scissor, cull mode, front face, depth test / write / compare -- Ir77PVCmdBuffer sets them per pass and per draw from the
// description's policy. Baked: shader stages (registry handles), vertex input (from the vertex shader's reflection -- none when it reads no
// inputs), blend (opaque / alpha / additive), attachment formats, depth bias, the FLIP_BACK_FACES constant. DefineShaderStages checks each
// stage slot and the vertex -> fragment interface.
// =============================================================================================================================================
class Ir77PVPipeline : public Ir77Enlisted, public IIr77PVPipeline, public std::enable_shared_from_this<Ir77PVPipeline> {
   public:
    Ir77PVPipeline() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    ~Ir77PVPipeline() {
        if (!m_device || m_pipeline == VK_NULL_HANDLE) return;

        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        vkDestroyPipeline(device, m_pipeline, nullptr);
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVPipeline>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PeregrineV>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77PVPipeline)
            obj = std::shared_ptr<IIr77PVPipeline>(shared_from_this(), static_cast<IIr77PVPipeline*>(this));

        else if (iid == GUIDIr77PVPipeline)
            obj = std::shared_ptr<Ir77PVPipeline>(shared_from_this(), static_cast<Ir77PVPipeline*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // configuration
    // -------------------------------------------------------------------------------------------------------------------------------------
   public:
    std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) {
        m_device = device;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetRenderPass(std::shared_ptr<IIr77PVRenderPass> render_pass) {
        m_render_pass = render_pass;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetLayout(std::shared_ptr<IIr77PVLayout> pipeline_layout) {
        m_pipeline_layout = pipeline_layout;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetShader(std::shared_ptr<IIr77PVShader> shader_stack) {
        m_shader_stack = shader_stack;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetDesc(Ir77PVPipelineDesc const& desc) {
        m_desc = desc;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetPipelineCache(VkPipelineCache const& cache) {
        m_cache = cache;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // create
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> CreatePipeline() {
        if (m_pipeline != VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipeline: pipeline already created.");

        if (!m_device) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipeline: SetDevice not called.");
        if (!m_render_pass) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipeline: SetRenderPass not called.");
        if (!m_pipeline_layout) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipeline: SetLayout not called.");
        if (!m_shader_stack) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipeline: SetShader not called.");

        DefineDynamicState();

        auto result = DefineVertexInputState();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        DefineInputAssemblyState();

        DefineViewportState();

        DefineRasterizationState();

        DefineMultisamplingState();

        DefineDepthStencilState();

        DefineColorBlendAttachment();

        DefineColorBlendState();

        result = DefineShaderStages();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        result = DefinePipeline();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Pipeline.");
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // queries
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> GetPipeline(VkPipeline* pipeline) {
        *pipeline = m_pipeline;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetDesc(Ir77PVPipelineDesc* desc) {
        *desc = m_desc;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // false when the vertex shader reads no inputs (or there is none) -- draws need no vertex buffer
    std::shared_ptr<IIr77Return const> GetVertexInput(bool& has_input) {
        has_input = m_has_vertex_input;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // steps
    // -------------------------------------------------------------------------------------------------------------------------------------
   private:
    std::shared_ptr<IIr77Return const> DefineDynamicState() {
        m_dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT,          VK_DYNAMIC_STATE_SCISSOR,           VK_DYNAMIC_STATE_CULL_MODE,
                            VK_DYNAMIC_STATE_FRONT_FACE,        VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE, VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE,
                            VK_DYNAMIC_STATE_DEPTH_COMPARE_OP};

        m_dynamic_state_info = VkPipelineDynamicStateCreateInfo{};
        m_dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        m_dynamic_state_info.dynamicStateCount = static_cast<std::uint32_t>(m_dynamic_states.size());
        m_dynamic_state_info.pDynamicStates = m_dynamic_states.data();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // From the vertex shader's own input locations (reflected at load): 0 position, 1 normal, 2 uv, 3 joints, 4 weights, 5 tangent, all from binding 0.
    // Reading 3 or 4 means the skinned layout (Ir77PVVertexSkinned); otherwise Ir77PVVertex. No vertex shader (mesh) = no vertex input.
    std::shared_ptr<IIr77Return const> DefineVertexInputState() {
        m_attribute_descriptions.clear();

        m_binding_description = VkVertexInputBindingDescription{};
        m_binding_description.binding = 0;
        m_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::vector<std::uint32_t> locations{};

        if (m_desc.vertex != IR77PV_NO_SHADER) {
            Ir77PVShaderStage stage{Ir77PVShaderStage::Vertex};
            Ir77PVShaderReflection reflection{};

            auto const result = m_shader_stack->GetShaderReflection(m_desc.vertex, stage, reflection);
            if (result->ID() != GUIDIr77OperationSucceeded) return result;

            locations = reflection.inputs;
        }

        bool const skinned{std::find_if(locations.begin(), locations.end(), [](std::uint32_t const l) {
                               return l == VERTEX_LOCATION_JOINTS || l == VERTEX_LOCATION_WEIGHTS;
                           }) != locations.end()};

        m_binding_description.stride = skinned ? sizeof(Ir77PVVertexSkinned) : sizeof(Ir77PVVertex);

        for (auto const location : locations) {
            switch (location) {
                case VERTEX_LOCATION_POSITION:
                    m_attribute_descriptions.push_back(Attribute(location, VK_FORMAT_R32G32B32_SFLOAT,
                                                                 skinned ? offsetof(Ir77PVVertexSkinned, position) : offsetof(Ir77PVVertex, position)));
                    break;

                case VERTEX_LOCATION_NORMAL:
                    m_attribute_descriptions.push_back(Attribute(location, VK_FORMAT_R32G32B32_SFLOAT,
                                                                 skinned ? offsetof(Ir77PVVertexSkinned, normal) : offsetof(Ir77PVVertex, normal)));
                    break;

                case VERTEX_LOCATION_UV:
                    m_attribute_descriptions.push_back(Attribute(location, VK_FORMAT_R32G32_SFLOAT,
                                                                 skinned ? offsetof(Ir77PVVertexSkinned, uv) : offsetof(Ir77PVVertex, uv)));
                    break;

                case VERTEX_LOCATION_JOINTS:
                    m_attribute_descriptions.push_back(Attribute(location, VK_FORMAT_R32G32B32A32_UINT, offsetof(Ir77PVVertexSkinned, joints)));
                    break;

                case VERTEX_LOCATION_WEIGHTS:
                    m_attribute_descriptions.push_back(Attribute(location, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Ir77PVVertexSkinned, weights)));
                    break;

                case VERTEX_LOCATION_TANGENT:
                    m_attribute_descriptions.push_back(Attribute(location, VK_FORMAT_R32G32B32A32_SFLOAT,
                                                                 skinned ? offsetof(Ir77PVVertexSkinned, tangent) : offsetof(Ir77PVVertex, tangent)));
                    break;

                default:
                    return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipeline: vertex shader reads an unknown location " + std::to_string(location) + ".");
            }
        }

        bool const has_input{!m_attribute_descriptions.empty()};
        m_has_vertex_input = has_input;

        m_vertex_input_info = VkPipelineVertexInputStateCreateInfo{};
        m_vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        m_vertex_input_info.vertexBindingDescriptionCount = has_input ? 1 : 0;
        m_vertex_input_info.pVertexBindingDescriptions = has_input ? &m_binding_description : nullptr;
        m_vertex_input_info.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(m_attribute_descriptions.size());
        m_vertex_input_info.pVertexAttributeDescriptions = has_input ? m_attribute_descriptions.data() : nullptr;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineInputAssemblyState() {
        m_input_assembly = VkPipelineInputAssemblyStateCreateInfo{};
        m_input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        m_input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        m_input_assembly.primitiveRestartEnable = VK_FALSE;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Viewport and scissor are dynamic -- only the counts are fixed here
    std::shared_ptr<IIr77Return const> DefineViewportState() {
        m_viewport_state_info = VkPipelineViewportStateCreateInfo{};
        m_viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        m_viewport_state_info.viewportCount = 1;
        m_viewport_state_info.scissorCount = 1;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // cullMode / frontFace are dynamic (Ir77PVCmdBuffer::RecordDynamicState); the values here are placeholders. Depth bias stays baked.
    std::shared_ptr<IIr77Return const> DefineRasterizationState() {
        m_rasterizer_state_info = VkPipelineRasterizationStateCreateInfo{};
        m_rasterizer_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        m_rasterizer_state_info.depthClampEnable = VK_FALSE;
        m_rasterizer_state_info.rasterizerDiscardEnable = VK_FALSE;
        m_rasterizer_state_info.polygonMode = VK_POLYGON_MODE_FILL;
        m_rasterizer_state_info.lineWidth = 1.0f;
        m_rasterizer_state_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        m_rasterizer_state_info.cullMode = VK_CULL_MODE_BACK_BIT;
        m_rasterizer_state_info.depthBiasEnable = VK_FALSE;

        // bias to fight shadow acne
        if (m_desc.depth_bias) {
            m_rasterizer_state_info.depthBiasEnable = VK_TRUE;
            m_rasterizer_state_info.depthBiasConstantFactor = 1.25f;
            m_rasterizer_state_info.depthBiasSlopeFactor = 1.75f;
            m_rasterizer_state_info.depthBiasClamp = 0.0f;
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineMultisamplingState() {
        m_multisampling_state_info = VkPipelineMultisampleStateCreateInfo{};
        m_multisampling_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        m_multisampling_state_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        m_multisampling_state_info.sampleShadingEnable = VK_FALSE;
        m_multisampling_state_info.minSampleShading = 1.0f;
        m_multisampling_state_info.alphaToCoverageEnable = VK_FALSE;
        m_multisampling_state_info.alphaToOneEnable = VK_FALSE;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // depthTestEnable / depthWriteEnable / depthCompareOp are dynamic (Ir77PVCmdBuffer); only the fixed parts are set here
    std::shared_ptr<IIr77Return const> DefineDepthStencilState() {
        m_depth_stencil_info = VkPipelineDepthStencilStateCreateInfo{};
        m_depth_stencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        m_depth_stencil_info.depthTestEnable = VK_TRUE;
        m_depth_stencil_info.depthWriteEnable = VK_TRUE;
        m_depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS;
        m_depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
        m_depth_stencil_info.stencilTestEnable = VK_FALSE;
        m_depth_stencil_info.minDepthBounds = 0.0f;
        m_depth_stencil_info.maxDepthBounds = 1.0f;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineColorBlendAttachment() {
        m_color_blend_attachment = VkPipelineColorBlendAttachmentState{};
        m_color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        m_color_blend_attachment.blendEnable = VK_FALSE;

        if (m_desc.blend == Ir77PVBlend::Alpha) {
            m_color_blend_attachment.blendEnable = VK_TRUE;
            m_color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            m_color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            m_color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
            m_color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            m_color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            m_color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
        }

        // colour += src * src alpha; the target's alpha is left as it was
        if (m_desc.blend == Ir77PVBlend::Additive) {
            m_color_blend_attachment.blendEnable = VK_TRUE;
            m_color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            m_color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
            m_color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
            m_color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            m_color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            m_color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Depth-only passes have no color attachment
    std::shared_ptr<IIr77Return const> DefineColorBlendState() {
        bool has_color{false};
        m_render_pass->HasColor(&has_color);

        m_color_blend_state_info = VkPipelineColorBlendStateCreateInfo{};
        m_color_blend_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        m_color_blend_state_info.logicOpEnable = VK_FALSE;
        m_color_blend_state_info.logicOp = VK_LOGIC_OP_COPY;
        m_color_blend_state_info.attachmentCount = has_color ? 1 : 0;
        m_color_blend_state_info.pAttachments = has_color ? &m_color_blend_attachment : nullptr;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The description's stage handles, in pipeline order. Raster: vertex [+ fragment]. Mesh: [task +] mesh [+ fragment], no vertex.
    std::shared_ptr<IIr77Return const> DefineShaderStages() {
        m_shader_stages.clear();

        if (m_desc.IsMesh() && m_desc.vertex != IR77PV_NO_SHADER)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipeline: a description has either a vertex or a mesh stage, not both.");

        if (!m_desc.IsMesh() && m_desc.vertex == IR77PV_NO_SHADER)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipeline: the description has no vertex or mesh stage.");

        if (!m_desc.IsMesh() && m_desc.task != IR77PV_NO_SHADER)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipeline: a task stage needs a mesh stage.");

        auto result = DefineStageInterface();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        std::vector<Ir77PVShaderHandle> handles{};

        for (auto const handle : {m_desc.task, m_desc.mesh, m_desc.vertex, m_desc.fragment})
            if (handle != IR77PV_NO_SHADER) handles.push_back(handle);

        result = m_shader_stack->GetPipelineShaderStageInfos(m_shader_stages, handles);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        // frag.frag constant_id 0 (FLIP_BACK_FACES) -- only written when off; the shader default is on
        if (!m_desc.flip_back_faces) {
            m_flip_back_faces = VK_FALSE;
            m_spec_entry = VkSpecializationMapEntry{0, 0, sizeof(VkBool32)};
            m_spec_info = VkSpecializationInfo{1, &m_spec_entry, sizeof(VkBool32), &m_flip_back_faces};

            for (auto& stage : m_shader_stages)
                if (stage.stage == VK_SHADER_STAGE_FRAGMENT_BIT) stage.pSpecializationInfo = &m_spec_info;
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Each handle is the stage its slot names, and every location the fragment shader reads is written by the vertex / mesh shader
    std::shared_ptr<IIr77Return const> DefineStageInterface() {
        std::pair<Ir77PVShaderHandle, Ir77PVShaderStage> const slots[]{{m_desc.vertex, Ir77PVShaderStage::Vertex},
                                                                       {m_desc.fragment, Ir77PVShaderStage::Fragment},
                                                                       {m_desc.task, Ir77PVShaderStage::Task},
                                                                       {m_desc.mesh, Ir77PVShaderStage::Mesh}};

        Ir77PVShaderReflection producer{};  // vertex or mesh
        Ir77PVShaderReflection consumer{};  // fragment

        for (auto const& [handle, expected] : slots) {
            if (handle == IR77PV_NO_SHADER) continue;

            Ir77PVShaderStage stage{Ir77PVShaderStage::Vertex};
            Ir77PVShaderReflection reflection{};

            auto const result = m_shader_stack->GetShaderReflection(handle, stage, reflection);
            if (result->ID() != GUIDIr77OperationSucceeded) return result;

            if (stage != expected) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipeline: a description's stage slot holds a shader of another stage.");

            if (expected == Ir77PVShaderStage::Vertex || expected == Ir77PVShaderStage::Mesh) producer = reflection;
            if (expected == Ir77PVShaderStage::Fragment) consumer = reflection;
        }

        if (m_desc.fragment == IR77PV_NO_SHADER) return Ir77RETURN<Ir77OperationSucceeded>();

        for (auto const location : consumer.inputs)
            if (!std::binary_search(producer.outputs.begin(), producer.outputs.end(), location))
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipeline: the fragment shader reads location " + std::to_string(location) +
                                                               ", which the vertex / mesh shader doesn't write.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Mesh pipelines have no vertex input or input assembly stage. Dynamic rendering: the pass's formats go in VkPipelineRenderingCreateInfo.
    std::shared_ptr<IIr77Return const> DefinePipeline() {
        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        VkPipelineLayout pipeline_layout{VK_NULL_HANDLE};
        m_pipeline_layout->GetPipelineLayout(&pipeline_layout);

        if (pipeline_layout == VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipeline: pipeline layout not created.");

        VkFormat color_format{VK_FORMAT_UNDEFINED};
        VkFormat depth_format{VK_FORMAT_UNDEFINED};
        m_render_pass->GetColorFormat(&color_format);
        m_render_pass->GetDepthFormat(&depth_format);

        bool const has_color{color_format != VK_FORMAT_UNDEFINED};

        VkPipelineRenderingCreateInfo rendering_info{};
        rendering_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        rendering_info.colorAttachmentCount = has_color ? 1 : 0;
        rendering_info.pColorAttachmentFormats = has_color ? &color_format : nullptr;
        rendering_info.depthAttachmentFormat = depth_format;

        bool const mesh{m_desc.IsMesh()};

        VkGraphicsPipelineCreateInfo pipeline_info{};
        pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.pNext = &rendering_info;
        pipeline_info.stageCount = static_cast<std::uint32_t>(m_shader_stages.size());
        pipeline_info.pStages = m_shader_stages.data();
        pipeline_info.pVertexInputState = mesh ? nullptr : &m_vertex_input_info;
        pipeline_info.pInputAssemblyState = mesh ? nullptr : &m_input_assembly;
        pipeline_info.pViewportState = &m_viewport_state_info;
        pipeline_info.pRasterizationState = &m_rasterizer_state_info;
        pipeline_info.pMultisampleState = &m_multisampling_state_info;
        pipeline_info.pDepthStencilState = &m_depth_stencil_info;
        pipeline_info.pColorBlendState = &m_color_blend_state_info;
        pipeline_info.pDynamicState = &m_dynamic_state_info;
        pipeline_info.layout = pipeline_layout;
        pipeline_info.renderPass = VK_NULL_HANDLE;
        pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_info.basePipelineIndex = -1;

        VkPipeline pipeline{VK_NULL_HANDLE};
        if (vkCreateGraphicsPipelines(device, m_cache, 1, &pipeline_info, nullptr, &pipeline) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipeline: vkCreateGraphicsPipelines failed.");

        m_pipeline = pipeline;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // internals
    // -------------------------------------------------------------------------------------------------------------------------------------
   private:
    // all attributes come from binding 0
    VkVertexInputAttributeDescription Attribute(std::uint32_t const location, VkFormat const format, std::uint32_t const offset) const {
        return VkVertexInputAttributeDescription{location, 0, format, offset};
    }

   private:
    std::shared_ptr<IIr77PVDevice> m_device{};

    std::shared_ptr<IIr77PVRenderPass> m_render_pass{};

    std::shared_ptr<IIr77PVLayout> m_pipeline_layout{};

    std::shared_ptr<IIr77PVShader> m_shader_stack{};

    Ir77PVPipelineDesc m_desc{};

    VkPipelineCache m_cache{VK_NULL_HANDLE};

    VkPipeline m_pipeline{VK_NULL_HANDLE};

    // create-info state -- members because the create info points into them
    std::vector<VkDynamicState> m_dynamic_states{};

    VkPipelineDynamicStateCreateInfo m_dynamic_state_info{};

    VkVertexInputBindingDescription m_binding_description{};

    bool m_has_vertex_input{false};

    std::vector<VkVertexInputAttributeDescription> m_attribute_descriptions{};

    VkPipelineVertexInputStateCreateInfo m_vertex_input_info{};

    VkPipelineInputAssemblyStateCreateInfo m_input_assembly{};

    VkPipelineViewportStateCreateInfo m_viewport_state_info{};

    VkPipelineRasterizationStateCreateInfo m_rasterizer_state_info{};

    VkPipelineMultisampleStateCreateInfo m_multisampling_state_info{};

    VkPipelineDepthStencilStateCreateInfo m_depth_stencil_info{};

    VkPipelineColorBlendAttachmentState m_color_blend_attachment{};

    VkPipelineColorBlendStateCreateInfo m_color_blend_state_info{};

    std::vector<VkPipelineShaderStageCreateInfo> m_shader_stages{};

    VkBool32 m_flip_back_faces{VK_TRUE};

    VkSpecializationMapEntry m_spec_entry{};

    VkSpecializationInfo m_spec_info{};
};

}  // namespace NSIr77PeregrineV