#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "../server/Ir77PVTypes.hpp"

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
        if (!m_device) return;

        VkDevice device;
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

    IIr77GUID* const QueryInterface(IIr77GUID const* iid, std::shared_ptr<void>& obj) {
        if (iid == &GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == &GUIDIIr77PVPipeline)
            obj = std::shared_ptr<IIr77PVPipeline>(shared_from_this(), static_cast<IIr77PVPipeline*>(this));

        else if (iid == &GUIDIr77PVPipeline)
            obj = std::shared_ptr<Ir77PVPipeline>(shared_from_this(), static_cast<Ir77PVPipeline*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> SetInstance(std::shared_ptr<IIr77PVInstance> instance) {
        m_instance = instance;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) {
        m_device = device;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetSwapchain(std::shared_ptr<IIr77PVSwapchain> swapchain) {
        m_swapchain = swapchain;

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

    std::shared_ptr<IIr77Return const> CreatePipeline() {
        DefineDynamicState();

        DefineVertexInputState();

        DefineInputAssemblyState();

        DefineViewportState();

        DefineRasterizationState();

        DefineMultisamplingState();

        DefineDepthStencilState();

        DefineColorBlendAttachment();

        DefineColorBlendState();

        if (DefinePipeline()->ID() != &GUIDIr77OperationSucceeded) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipeline: DefinePipeline failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetPipeline(VkPipeline* pipeline) {
        *pipeline = m_pipeline;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetKind(Ir77PVPipelineKind* kind) {
        *kind = m_kind;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    bool IsShadow() const { return m_kind == Ir77PVPipelineKind::Shadow || m_kind == Ir77PVPipelineKind::ShadowSkinned; }

    std::shared_ptr<IIr77Return const> DefineDynamicState() {
        m_dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

        m_dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        m_dynamic_state_info.dynamicStateCount = static_cast<std::uint32_t>(m_dynamic_states.size());
        m_dynamic_state_info.pDynamicStates = m_dynamic_states.data();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    static VkVertexInputAttributeDescription Attribute(std::uint32_t location, VkFormat format, std::uint32_t offset) {
        return VkVertexInputAttributeDescription{location, 0, format, offset};
    }

    std::shared_ptr<IIr77Return const> DefineVertexInputState() {
        m_attribute_descriptions.clear();
        m_binding_description = VkVertexInputBindingDescription{};
        m_binding_description.binding = 0;
        m_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        // locations are shared across kinds: 0 position, 1 normal, 2 uv, 3 joints, 4 weights
        switch (m_kind) {
            case Ir77PVPipelineKind::Static:
            case Ir77PVPipelineKind::Transparent:
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
                // fullscreen quad generated from gl_VertexIndex -- no vertex input
                break;

            default:
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipeline: unknown pipeline kind.");
        }

        bool const has_input = !m_attribute_descriptions.empty();

        m_vertex_input_info = VkPipelineVertexInputStateCreateInfo{};
        m_vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        m_vertex_input_info.vertexBindingDescriptionCount = has_input ? 1 : 0;
        m_vertex_input_info.pVertexBindingDescriptions = has_input ? &m_binding_description : nullptr;
        m_vertex_input_info.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(m_attribute_descriptions.size());
        m_vertex_input_info.pVertexAttributeDescriptions = has_input ? m_attribute_descriptions.data() : nullptr;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineInputAssemblyState() {
        m_input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        m_input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        m_input_assembly.primitiveRestartEnable = VK_FALSE;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineViewportState() {
        // viewport + scissor are dynamic; extent kept only as a default for callers
        if (m_swapchain) {
            VkExtent2D swapchain_extent;
            m_swapchain->GetSwapchainExtents(swapchain_extent);

            m_viewport.x = 0.0f;
            m_viewport.y = 0.0f;
            m_viewport.width = static_cast<float>(swapchain_extent.width);
            m_viewport.height = static_cast<float>(swapchain_extent.height);
            m_viewport.minDepth = 0.0f;
            m_viewport.maxDepth = 1.0f;

            m_scissor.offset = {0, 0};
            m_scissor.extent = swapchain_extent;
        }

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
        m_rasterizer_state_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
        m_multisampling_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        m_multisampling_state_info.sampleShadingEnable = VK_FALSE;
        m_multisampling_state_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        m_multisampling_state_info.minSampleShading = 1.0f;
        m_multisampling_state_info.pSampleMask = nullptr;
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

        switch (m_kind) {
            case Ir77PVPipelineKind::Static:
            case Ir77PVPipelineKind::Skinned:
            case Ir77PVPipelineKind::Shadow:
            case Ir77PVPipelineKind::ShadowSkinned:
                m_depth_stencil_info.depthTestEnable = VK_TRUE;
                m_depth_stencil_info.depthWriteEnable = VK_TRUE;
                break;

            case Ir77PVPipelineKind::Transparent:
                // test against opaque depth, don't occlude other transparents
                m_depth_stencil_info.depthTestEnable = VK_TRUE;
                m_depth_stencil_info.depthWriteEnable = VK_FALSE;
                break;

            case Ir77PVPipelineKind::CEF:
                // overlay -- always on top
                m_depth_stencil_info.depthTestEnable = VK_FALSE;
                m_depth_stencil_info.depthWriteEnable = VK_FALSE;
                break;

            default:
                break;
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineColorBlendAttachment() {
        m_color_blend_attachment = VkPipelineColorBlendAttachmentState{};
        m_color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        switch (m_kind) {
            case Ir77PVPipelineKind::Transparent:
            case Ir77PVPipelineKind::CEF:
                m_color_blend_attachment.blendEnable = VK_TRUE;
                m_color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                m_color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                m_color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
                m_color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                m_color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                m_color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
                break;

            default:
                m_color_blend_attachment.blendEnable = VK_FALSE;
                break;
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineColorBlendState() {
        m_color_blend_state_info = VkPipelineColorBlendStateCreateInfo{};
        m_color_blend_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        m_color_blend_state_info.logicOpEnable = VK_FALSE;
        m_color_blend_state_info.logicOp = VK_LOGIC_OP_COPY;

        // depth-only shadow pass has no color attachment
        m_color_blend_state_info.attachmentCount = IsShadow() ? 0 : 1;
        m_color_blend_state_info.pAttachments = IsShadow() ? nullptr : &m_color_blend_attachment;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineShaderStages(std::vector<VkPipelineShaderStageCreateInfo>& shader_stages) {
        switch (m_kind) {
            case Ir77PVPipelineKind::Static:
            case Ir77PVPipelineKind::Transparent:
                m_shader_stack->GetPipelineShaderStageInfos(shader_stages, {ID_SHADER_VERT, ID_SHADER_FRAG});
                break;

            case Ir77PVPipelineKind::Skinned:
                m_shader_stack->GetPipelineShaderStageInfos(shader_stages, {ID_SHADER_SKINNED_VERT, ID_SHADER_FRAG});
                break;

            case Ir77PVPipelineKind::Shadow:
                // depth-only: no fragment stage
                m_shader_stack->GetPipelineShaderStageInfos(shader_stages, {ID_SHADER_SHADOW_VERT});
                break;

            case Ir77PVPipelineKind::ShadowSkinned:
                m_shader_stack->GetPipelineShaderStageInfos(shader_stages, {ID_SHADER_SHADOW_SKINNED_VERT});
                break;

            case Ir77PVPipelineKind::CEF:
                m_shader_stack->GetPipelineShaderStageInfos(shader_stages, {ID_SHADER_CEF_VERT, ID_SHADER_CEF_FRAG});
                break;

            default:
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipeline: unknown pipeline kind.");
        }

        if (shader_stages.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipeline: no shader stages.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefinePipeline() {
        VkDevice device;
        m_device->GetDevice(&device);

        VkPipelineLayout pipeline_layout;
        m_pipeline_layout->GetPipelineLayout(&pipeline_layout);

        VkRenderPass render_pass;
        m_render_pass->GetRenderPass(&render_pass);

        std::vector<VkPipelineShaderStageCreateInfo> shader_stages{};
        if (DefineShaderStages(shader_stages)->ID() != &GUIDIr77OperationSucceeded) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipeline: shader stages failed.");
        }

        m_pipeline_info = VkGraphicsPipelineCreateInfo{};
        m_pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        m_pipeline_info.stageCount = static_cast<std::uint32_t>(shader_stages.size());
        m_pipeline_info.pStages = shader_stages.data();
        m_pipeline_info.pVertexInputState = &m_vertex_input_info;
        m_pipeline_info.pInputAssemblyState = &m_input_assembly;
        m_pipeline_info.pViewportState = &m_viewport_state_info;
        m_pipeline_info.pRasterizationState = &m_rasterizer_state_info;
        m_pipeline_info.pMultisampleState = &m_multisampling_state_info;
        m_pipeline_info.pDepthStencilState = &m_depth_stencil_info;
        m_pipeline_info.pColorBlendState = &m_color_blend_state_info;
        m_pipeline_info.pDynamicState = &m_dynamic_state_info;
        m_pipeline_info.layout = pipeline_layout;
        m_pipeline_info.renderPass = render_pass;
        m_pipeline_info.subpass = 0;
        m_pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
        m_pipeline_info.basePipelineIndex = -1;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &m_pipeline_info, nullptr, &m_pipeline) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipeline: VkPipeline failed.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    std::shared_ptr<IIr77PVInstance> m_instance;

    std::shared_ptr<IIr77PVDevice> m_device;

    std::shared_ptr<IIr77PVSwapchain> m_swapchain;

    std::shared_ptr<IIr77PVRenderPass> m_render_pass;

    std::shared_ptr<IIr77PVLayout> m_pipeline_layout;

    std::shared_ptr<IIr77PVShader> m_shader_stack;

    Ir77PVPipelineKind m_kind{Ir77PVPipelineKind::Static};

    std::vector<VkDynamicState> m_dynamic_states{};

    VkVertexInputBindingDescription m_binding_description{};

    std::vector<VkVertexInputAttributeDescription> m_attribute_descriptions{};

    VkPipelineDynamicStateCreateInfo m_dynamic_state_info{};

    VkPipelineVertexInputStateCreateInfo m_vertex_input_info{};

    VkPipelineInputAssemblyStateCreateInfo m_input_assembly{};

    VkViewport m_viewport{};

    VkRect2D m_scissor{};

    VkPipelineViewportStateCreateInfo m_viewport_state_info{};

    VkPipelineRasterizationStateCreateInfo m_rasterizer_state_info{};

    VkPipelineMultisampleStateCreateInfo m_multisampling_state_info{};

    VkPipelineDepthStencilStateCreateInfo m_depth_stencil_info{};

    VkPipelineColorBlendAttachmentState m_color_blend_attachment{};

    VkPipelineColorBlendStateCreateInfo m_color_blend_state_info{};

    VkGraphicsPipelineCreateInfo m_pipeline_info{};

    VkPipeline m_pipeline{VK_NULL_HANDLE};
};
}  // namespace NSIr77PeregrineV