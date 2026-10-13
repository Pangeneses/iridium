#pragma once

#include <vulkan/vulkan.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

#include "../types/Ir77PVTypes.hpp"

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIr77PVContext.hpp"

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
// Ir77PVPipeline -- one graphics pipeline for one Ir77PVPipelineKind, against subpass 0 of the render pass it is given.
//
//   configure: SetDevice / SetRenderPass / SetLayout / SetShader / SetKind
//   create:    CreatePipeline -> DefineDynamicState -> DefineVertexInputState -> DefineInputAssemblyState -> DefineViewportState
//              -> DefineRasterizationState -> DefineMultisamplingState -> DefineDepthStencilState -> DefineColorBlendAttachment
//              -> DefineColorBlendState -> DefineShaderStages -> DefinePipeline
//
// Viewport and scissor are dynamic, so the pipeline does not depend on any swapchain extent.
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

    std::shared_ptr<IIr77Return const> SetKind(Ir77PVPipelineKind const& kind) {
        m_kind = kind;

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

    std::shared_ptr<IIr77Return const> GetKind(Ir77PVPipelineKind* kind) {
        *kind = m_kind;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // steps
    // -------------------------------------------------------------------------------------------------------------------------------------
   private:
    std::shared_ptr<IIr77Return const> DefineDynamicState() {
        m_dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

        m_dynamic_state_info = VkPipelineDynamicStateCreateInfo{};
        m_dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        m_dynamic_state_info.dynamicStateCount = static_cast<std::uint32_t>(m_dynamic_states.size());
        m_dynamic_state_info.pDynamicStates = m_dynamic_states.data();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Locations are shared across kinds: 0 position, 1 normal, 2 uv, 3 joints, 4 weights
    std::shared_ptr<IIr77Return const> DefineVertexInputState() {
        m_attribute_descriptions.clear();

        m_binding_description = VkVertexInputBindingDescription{};
        m_binding_description.binding = 0;
        m_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        switch (m_kind) {
            case Ir77PVPipelineKind::Static:
            case Ir77PVPipelineKind::Transparent:
            case Ir77PVPipelineKind::TransparentBack:
                m_binding_description.stride = sizeof(Ir77PVVertex);
                m_attribute_descriptions = {Attribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Ir77PVVertex, position)),
                                            Attribute(1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Ir77PVVertex, normal)),
                                            Attribute(2, VK_FORMAT_R32G32_SFLOAT, offsetof(Ir77PVVertex, uv))};
                break;

            case Ir77PVPipelineKind::Shadow:
                m_binding_description.stride = sizeof(Ir77PVVertex);
                m_attribute_descriptions = {Attribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Ir77PVVertex, position))};
                break;

            case Ir77PVPipelineKind::Skinned:
            case Ir77PVPipelineKind::SkinnedTransparent:
            case Ir77PVPipelineKind::SkinnedTransparentBack:
                m_binding_description.stride = sizeof(Ir77PVVertexSkinned);
                m_attribute_descriptions = {Attribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Ir77PVVertexSkinned, position)),
                                            Attribute(1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Ir77PVVertexSkinned, normal)),
                                            Attribute(2, VK_FORMAT_R32G32_SFLOAT, offsetof(Ir77PVVertexSkinned, uv)),
                                            Attribute(3, VK_FORMAT_R32G32B32A32_UINT, offsetof(Ir77PVVertexSkinned, joints)),
                                            Attribute(4, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Ir77PVVertexSkinned, weights))};
                break;

            case Ir77PVPipelineKind::ShadowSkinned:
                m_binding_description.stride = sizeof(Ir77PVVertexSkinned);
                m_attribute_descriptions = {Attribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Ir77PVVertexSkinned, position)),
                                            Attribute(3, VK_FORMAT_R32G32B32A32_UINT, offsetof(Ir77PVVertexSkinned, joints)),
                                            Attribute(4, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Ir77PVVertexSkinned, weights))};
                break;

            case Ir77PVPipelineKind::CEF:
                // fullscreen quad from gl_VertexIndex -- no vertex input
                break;

            case Ir77PVPipelineKind::Mesh:
                // task/mesh shaders fetch their own data -- no vertex input stage at all
                break;

            default:
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipeline: unknown pipeline kind.");
        }

        bool const has_input{!m_attribute_descriptions.empty()};

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

        switch (m_kind) {
            case Ir77PVPipelineKind::Shadow:
            case Ir77PVPipelineKind::ShadowSkinned:
                // front-face culling + bias to fight shadow acne
                m_rasterizer_state_info.cullMode = VK_CULL_MODE_FRONT_BIT;
                m_rasterizer_state_info.depthBiasEnable = VK_TRUE;
                m_rasterizer_state_info.depthBiasConstantFactor = 1.25f;
                m_rasterizer_state_info.depthBiasSlopeFactor = 1.75f;
                m_rasterizer_state_info.depthBiasClamp = 0.0f;
                break;

            case Ir77PVPipelineKind::TransparentBack:
            case Ir77PVPipelineKind::SkinnedTransparentBack:
                // first transparent pass draws back faces
                m_rasterizer_state_info.cullMode = VK_CULL_MODE_FRONT_BIT;
                break;

            case Ir77PVPipelineKind::CEF:
                // fullscreen quad -- winding irrelevant
                m_rasterizer_state_info.cullMode = VK_CULL_MODE_NONE;
                break;

            default:
                break;
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

    // Always provided: required whenever the subpass has a depth attachment, ignored when it doesn't
    std::shared_ptr<IIr77Return const> DefineDepthStencilState() {
        m_depth_stencil_info = VkPipelineDepthStencilStateCreateInfo{};
        m_depth_stencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        m_depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS;
        m_depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
        m_depth_stencil_info.stencilTestEnable = VK_FALSE;
        m_depth_stencil_info.minDepthBounds = 0.0f;
        m_depth_stencil_info.maxDepthBounds = 1.0f;

        if (m_kind == Ir77PVPipelineKind::CEF) {
            // overlay -- always on top
            m_depth_stencil_info.depthTestEnable = VK_FALSE;
            m_depth_stencil_info.depthWriteEnable = VK_FALSE;
        } else {
            // transparents test against opaque depth but don't occlude each other
            m_depth_stencil_info.depthTestEnable = VK_TRUE;
            m_depth_stencil_info.depthWriteEnable = IsTransparent() ? VK_FALSE : VK_TRUE;
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineColorBlendAttachment() {
        m_color_blend_attachment = VkPipelineColorBlendAttachmentState{};
        m_color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        m_color_blend_attachment.blendEnable = VK_FALSE;

        if (IsTransparent() || m_kind == Ir77PVPipelineKind::CEF) {
            m_color_blend_attachment.blendEnable = VK_TRUE;
            m_color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            m_color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            m_color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
            m_color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            m_color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            m_color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Depth-only shadow pass has no color attachment
    std::shared_ptr<IIr77Return const> DefineColorBlendState() {
        bool const has_color{!IsShadow()};

        m_color_blend_state_info = VkPipelineColorBlendStateCreateInfo{};
        m_color_blend_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        m_color_blend_state_info.logicOpEnable = VK_FALSE;
        m_color_blend_state_info.logicOp = VK_LOGIC_OP_COPY;
        m_color_blend_state_info.attachmentCount = has_color ? 1 : 0;
        m_color_blend_state_info.pAttachments = has_color ? &m_color_blend_attachment : nullptr;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineShaderStages() {
        m_shader_stages.clear();

        switch (m_kind) {
            case Ir77PVPipelineKind::Static:
            case Ir77PVPipelineKind::Transparent:
            case Ir77PVPipelineKind::TransparentBack:
                m_shader_stack->GetPipelineShaderStageInfos(m_shader_stages, {ID_SHADER_VERT, ID_SHADER_FRAG});
                break;

            case Ir77PVPipelineKind::Skinned:
            case Ir77PVPipelineKind::SkinnedTransparent:
            case Ir77PVPipelineKind::SkinnedTransparentBack:
                m_shader_stack->GetPipelineShaderStageInfos(m_shader_stages, {ID_SHADER_SKINNED_VERT, ID_SHADER_FRAG});
                break;

            case Ir77PVPipelineKind::Shadow:
                // depth-only: no fragment stage
                m_shader_stack->GetPipelineShaderStageInfos(m_shader_stages, {ID_SHADER_SHADOW_VERT});
                break;

            case Ir77PVPipelineKind::ShadowSkinned:
                m_shader_stack->GetPipelineShaderStageInfos(m_shader_stages, {ID_SHADER_SHADOW_SKINNED_VERT});
                break;

            case Ir77PVPipelineKind::Mesh:
                m_shader_stack->GetPipelineShaderStageInfos(m_shader_stages, {ID_SHADER_MESH_TASK, ID_SHADER_MESH, ID_SHADER_FRAG});
                break;

            case Ir77PVPipelineKind::CEF:
                m_shader_stack->GetPipelineShaderStageInfos(m_shader_stages, {ID_SHADER_CEF_VERT, ID_SHADER_CEF_FRAG});
                break;

            default:
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipeline: unknown pipeline kind.");
        }

        if (m_shader_stages.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipeline: no shader stages.");

        // frag.frag constant_id 0 (FLIP_BACK_FACES) -- off for transparent kinds, shader default (on) for everything else
        if (IsTransparent()) {
            m_flip_back_faces = VK_FALSE;
            m_spec_entry = VkSpecializationMapEntry{0, 0, sizeof(VkBool32)};
            m_spec_info = VkSpecializationInfo{1, &m_spec_entry, sizeof(VkBool32), &m_flip_back_faces};

            for (auto& stage : m_shader_stages)
                if (stage.stage == VK_SHADER_STAGE_FRAGMENT_BIT) stage.pSpecializationInfo = &m_spec_info;
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Mesh pipelines have no vertex input or input assembly stage
    std::shared_ptr<IIr77Return const> DefinePipeline() {
        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        VkPipelineLayout pipeline_layout{VK_NULL_HANDLE};
        m_pipeline_layout->GetPipelineLayout(&pipeline_layout);

        VkRenderPass render_pass{VK_NULL_HANDLE};
        m_render_pass->GetRenderPass(&render_pass);

        if (pipeline_layout == VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipeline: pipeline layout not created.");
        if (render_pass == VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipeline: render pass not created.");

        bool const mesh{m_kind == Ir77PVPipelineKind::Mesh};

        VkGraphicsPipelineCreateInfo pipeline_info{};
        pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
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
        pipeline_info.renderPass = render_pass;
        pipeline_info.subpass = 0;
        pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_info.basePipelineIndex = -1;

        VkPipeline pipeline{VK_NULL_HANDLE};
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipeline: vkCreateGraphicsPipelines failed.");

        m_pipeline = pipeline;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // internals
    // -------------------------------------------------------------------------------------------------------------------------------------
   private:
    bool IsShadow() const { return m_kind == Ir77PVPipelineKind::Shadow || m_kind == Ir77PVPipelineKind::ShadowSkinned; }

    bool IsTransparent() const {
        return m_kind == Ir77PVPipelineKind::Transparent || m_kind == Ir77PVPipelineKind::TransparentBack ||
               m_kind == Ir77PVPipelineKind::SkinnedTransparent || m_kind == Ir77PVPipelineKind::SkinnedTransparentBack;
    }

    // all attributes come from binding 0
    VkVertexInputAttributeDescription Attribute(std::uint32_t const location, VkFormat const format, std::uint32_t const offset) const {
        return VkVertexInputAttributeDescription{location, 0, format, offset};
    }

   private:
    std::shared_ptr<IIr77PVDevice> m_device{};

    std::shared_ptr<IIr77PVRenderPass> m_render_pass{};

    std::shared_ptr<IIr77PVLayout> m_pipeline_layout{};

    std::shared_ptr<IIr77PVShader> m_shader_stack{};

    Ir77PVPipelineKind m_kind{Ir77PVPipelineKind::Static};

    VkPipeline m_pipeline{VK_NULL_HANDLE};

    // create-info state -- members because the create info points into them
    std::vector<VkDynamicState> m_dynamic_states{};

    VkPipelineDynamicStateCreateInfo m_dynamic_state_info{};

    VkVertexInputBindingDescription m_binding_description{};

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