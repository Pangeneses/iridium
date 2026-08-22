#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <memory>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../../dictionary/IDIIr77PeregrineV.hpp"
#include "../../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../../interface/IIr77PVPipeline.hpp"
#include "../../interface/IIr77PVSwapchain.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77PVPipelineGFX : public Ir77Enlisted, public IIr77PVPipeline, public std::enable_shared_from_this<Ir77PVPipelineGFX> {
   public:
    Ir77PVPipelineGFX() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    ~Ir77PVPipelineGFX() { vkDestroyPipeline(GetDevice(), m_pipeline, nullptr); }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberOfUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVPipelineGFX>(uid);

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

        else if (iid == &GUIDIr77PVPipelineGFX)
            obj = std::shared_ptr<Ir77PVPipelineGFX>(shared_from_this(), static_cast<Ir77PVPipelineGFX*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context) {
        m_context = context;

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

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineDynamicState() {
        m_dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

        m_dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        m_dynamic_state_info.dynamicStateCount = static_cast<uint32_t>(m_dynamic_states.size());
        m_dynamic_state_info.pDynamicStates = m_dynamic_states.data();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineVertexInputState() {
        m_vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        m_vertex_input_info.vertexBindingDescriptionCount = 0;
        m_vertex_input_info.pVertexBindingDescriptions = nullptr;
        m_vertex_input_info.vertexAttributeDescriptionCount = 0;
        m_vertex_input_info.pVertexAttributeDescriptions = nullptr;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineInputAssemblyState() {
        m_input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        m_input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        m_input_assembly.primitiveRestartEnable = VK_FALSE;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineViewportState() {
        std::uint32_t index = CurrentDevice();



        SwapchainSupportDetails details = GetSwapchainSupportDetails();

        m_viewport.x = 0.0f;
        m_viewport.y = 0.0f;
        m_viewport.width = (float)details.swapchain_extent.width;
        m_viewport.height = (float)details.swapchain_extent.height;
        m_viewport.minDepth = 0.0f;
        m_viewport.maxDepth = 1.0f;

        m_scissor.offset = {0, 0};
        m_scissor.extent = details.swapchain_extent;

        // dynamic
        m_viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        m_viewport_state_info.viewportCount = 1;
        // m_viewport_state.pViewports = &viewport;
        m_viewport_state_info.scissorCount = 1;
        // m_viewport_state.pScissors = &scissor;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineRasterizationState() {
        m_rasterizer_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        m_rasterizer_state_info.depthClampEnable = VK_FALSE;
        m_rasterizer_state_info.rasterizerDiscardEnable = VK_FALSE;
        m_rasterizer_state_info.polygonMode = VK_POLYGON_MODE_FILL;
        m_rasterizer_state_info.lineWidth = 1.0f;
        m_rasterizer_state_info.cullMode = VK_CULL_MODE_BACK_BIT;
        m_rasterizer_state_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
        m_rasterizer_state_info.depthBiasEnable = VK_FALSE;
        m_rasterizer_state_info.depthBiasConstantFactor = 0.0f;  // Optional
        m_rasterizer_state_info.depthBiasClamp = 0.0f;           // Optional
        m_rasterizer_state_info.depthBiasSlopeFactor = 0.0f;     // Optional

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineMultisamplingState() {
        m_multisampling_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        m_multisampling_state_info.sampleShadingEnable = VK_FALSE;
        m_multisampling_state_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        m_multisampling_state_info.minSampleShading = 1.0f;           // Optional
        m_multisampling_state_info.pSampleMask = nullptr;             // Optional
        m_multisampling_state_info.alphaToCoverageEnable = VK_FALSE;  // Optional
        m_multisampling_state_info.alphaToOneEnable = VK_FALSE;       // Optional

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineDepthStencilState() { return Ir77RETURN<Ir77OperationSucceeded>(); }

    std::shared_ptr<IIr77Return const> DefineColorBlendAttachment() {
        m_color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        m_color_blend_attachment.blendEnable = VK_TRUE;
        m_color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        m_color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        m_color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
        m_color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        m_color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        m_color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineColorBlendState() {
        m_color_blend_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        m_color_blend_state_info.logicOpEnable = VK_FALSE;
        m_color_blend_state_info.logicOp = VK_LOGIC_OP_COPY;  // Optional
        m_color_blend_state_info.attachmentCount = 1;
        m_color_blend_state_info.pAttachments = &m_color_blend_attachment;
        m_color_blend_state_info.blendConstants[0] = 0.0f;  // Optional
        m_color_blend_state_info.blendConstants[1] = 0.0f;  // Optional
        m_color_blend_state_info.blendConstants[2] = 0.0f;  // Optional
        m_color_blend_state_info.blendConstants[3] = 0.0f;  // Optional

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefinePipeline() {
        VkDevice device = GetDevice();
        VkPipelineLayout layout = GetLayout();
        VkRenderPass render_pass = GetRenderPass();
        std::vector<VkPipelineShaderStageCreateInfo> shader_stages = GetPipelineShaderStageInfos();

        m_pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        m_pipeline_info.stageCount = 2;
        m_pipeline_info.pStages = shader_stages.data();
        m_pipeline_info.pVertexInputState = &m_vertex_input_info;
        m_pipeline_info.pInputAssemblyState = &m_input_assembly;
        m_pipeline_info.pViewportState = &m_viewport_state_info;
        m_pipeline_info.pRasterizationState = &m_rasterizer_state_info;
        m_pipeline_info.pMultisampleState = &m_multisampling_state_info;
        m_pipeline_info.pDepthStencilState = nullptr;  // Optional
        m_pipeline_info.pColorBlendState = &m_color_blend_state_info;
        m_pipeline_info.pDynamicState = &m_dynamic_state_info;
        m_pipeline_info.layout = layout;
        m_pipeline_info.renderPass = render_pass;
        m_pipeline_info.subpass = 0;
        m_pipeline_info.basePipelineHandle = VK_NULL_HANDLE;  // Optional
        m_pipeline_info.basePipelineIndex = -1;               // Optional

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &m_pipeline_info, nullptr, &m_pipeline) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: VkPipeline failed.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    std::uint32_t CurrentDevice();

    std::vector<Ir77PVSwapchainInfo> GetSwapchainInfos();

    VkDevice GetDevice();

    VkPipelineLayout GetLayout();

    VkRenderPass GetRenderPass();

    std::vector<VkPipelineShaderStageCreateInfo> GetPipelineShaderStageInfos();

   private:
    std::shared_ptr<IIr77Enlisted> m_context;

    std::vector<VkDynamicState> m_dynamic_states{};

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