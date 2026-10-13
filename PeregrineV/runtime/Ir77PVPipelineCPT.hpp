#pragma once

#include <vulkan/vulkan.h>

#include <chrono>
#include <memory>
#include <stdexcept>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PVContext.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77PVPipelineCPT.hpp"
#include "../interface/IIr77PVLayoutCPT.hpp"
#include "../interface/IIr77PVShader.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// =============================================================================================================================================
// Ir77PVPipelineCPT -- one compute pipeline for one Ir77PVComputeKind.
//
//   configure: SetDevice / SetLayout / SetShader / SetKind
//   create:    CreatePipeline -> DefineShaderStage -> DefinePipeline
// =============================================================================================================================================
class Ir77PVPipelineCPT : public Ir77Enlisted, public IIr77PVPipelineCPT, public std::enable_shared_from_this<Ir77PVPipelineCPT> {
   public:
    Ir77PVPipelineCPT() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument const&) {
            throw;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    // the owner waits for the device to go idle before releasing pipelines
    ~Ir77PVPipelineCPT() {
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
        seat_shared_uuid<&GUIDIr77PVPipelineCPT>(uid);

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

        else if (iid == GUIDIIr77PVPipelineCPT)
            obj = std::shared_ptr<IIr77PVPipelineCPT>(shared_from_this(), static_cast<IIr77PVPipelineCPT*>(this));

        else if (iid == GUIDIr77PVPipelineCPT)
            obj = std::shared_ptr<Ir77PVPipelineCPT>(shared_from_this(), static_cast<Ir77PVPipelineCPT*>(this));

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

    std::shared_ptr<IIr77Return const> SetLayout(std::shared_ptr<IIr77PVLayoutCPT> layout) {
        m_layout = layout;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetShader(std::shared_ptr<IIr77PVShader> shader_stack) {
        m_shader_stack = shader_stack;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetKind(Ir77PVComputeKind const& kind) {
        m_kind = kind;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // create
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> CreatePipeline() {
        if (m_pipeline != VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCPT: pipeline already created.");

        if (!m_device) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCPT: SetDevice not called.");
        if (!m_layout) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCPT: SetLayout not called.");
        if (!m_shader_stack) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCPT: SetShader not called.");

        auto result = DefineShaderStage();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        result = DefinePipeline();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Compute Pipeline.");
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // queries
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> GetPipeline(VkPipeline* pipeline) {
        *pipeline = m_pipeline;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetKind(Ir77PVComputeKind* kind) {
        *kind = m_kind;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // steps
    // -------------------------------------------------------------------------------------------------------------------------------------
   private:
    // Exactly one compute stage per kind
    std::shared_ptr<IIr77Return const> DefineShaderStage() {
        std::vector<VkPipelineShaderStageCreateInfo> shader_stages{};

        switch (m_kind) {
            case Ir77PVComputeKind::Culling:
                m_shader_stack->GetPipelineShaderStageInfos(shader_stages, {ID_SHADER_CULLING_COMP});
                break;

            case Ir77PVComputeKind::SkinningUpdate:
                m_shader_stack->GetPipelineShaderStageInfos(shader_stages, {ID_SHADER_SKINNING_COMP});
                break;

            case Ir77PVComputeKind::MorphUpdate:
                m_shader_stack->GetPipelineShaderStageInfos(shader_stages, {ID_SHADER_MORPH_COMP});
                break;

            case Ir77PVComputeKind::ClothSim:
                m_shader_stack->GetPipelineShaderStageInfos(shader_stages, {ID_SHADER_CLOTH_COMP});
                break;

            default:
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCPT: unknown compute kind.");
        }

        if (shader_stages.size() != 1) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCPT: expected exactly one compute shader stage.");

        if (shader_stages[0].stage != VK_SHADER_STAGE_COMPUTE_BIT)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCPT: shader stage is not a compute stage.");

        m_shader_stage = shader_stages[0];

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefinePipeline() {
        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        VkPipelineLayout pipeline_layout{VK_NULL_HANDLE};
        m_layout->GetPipelineLayout(&pipeline_layout);

        if (pipeline_layout == VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCPT: pipeline layout not created.");

        VkComputePipelineCreateInfo compute_info{};
        compute_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        compute_info.stage = m_shader_stage;
        compute_info.layout = pipeline_layout;
        compute_info.basePipelineHandle = VK_NULL_HANDLE;
        compute_info.basePipelineIndex = -1;

        VkPipeline pipeline{VK_NULL_HANDLE};
        if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &compute_info, nullptr, &pipeline) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCPT: vkCreateComputePipelines failed.");

        m_pipeline = pipeline;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    std::shared_ptr<IIr77PVDevice> m_device{};

    std::shared_ptr<IIr77PVLayoutCPT> m_layout{};

    std::shared_ptr<IIr77PVShader> m_shader_stack{};

    Ir77PVComputeKind m_kind{Ir77PVComputeKind::Culling};

    VkPipelineShaderStageCreateInfo m_shader_stage{};

    VkPipeline m_pipeline{VK_NULL_HANDLE};
};

}  // namespace NSIr77PeregrineV