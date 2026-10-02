#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <memory>
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

    ~Ir77PVPipelineCPT() {
        if (!m_device) return;

        VkDevice device;
        m_device->GetDevice(&device);

        vkDeviceWaitIdle(device);

        if (m_pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, m_pipeline, nullptr);
        }
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

    IIr77GUID* const QueryInterface(IIr77GUID const* iid, std::shared_ptr<void>& obj) {
        if (iid == &GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == &GUIDIIr77PVPipelineCPT)
            obj = std::shared_ptr<IIr77PVPipelineCPT>(shared_from_this(), static_cast<IIr77PVPipelineCPT*>(this));

        else if (iid == &GUIDIr77PVPipelineCPT)
            obj = std::shared_ptr<Ir77PVPipelineCPT>(shared_from_this(), static_cast<Ir77PVPipelineCPT*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

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

    std::shared_ptr<IIr77Return const> CreatePipeline() {
        if (!m_device) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCPT: device not set.");
        if (!m_layout) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCPT: layout not set.");
        if (!m_shader_stack) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCPT: shader stack not set.");

        VkDevice device;
        m_device->GetDevice(&device);

        VkPipelineLayout pipeline_layout{VK_NULL_HANDLE};
        m_layout->GetPipelineLayout(&pipeline_layout);

        if (pipeline_layout == VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCPT: layout has no VkPipelineLayout -- call its DefinePipelineLayout first.");

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

        VkComputePipelineCreateInfo compute_info{};
        compute_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        compute_info.stage = shader_stages[0];
        compute_info.layout = pipeline_layout;
        compute_info.basePipelineHandle = VK_NULL_HANDLE;
        compute_info.basePipelineIndex = -1;

        if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &compute_info, nullptr, &m_pipeline) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCPT: vkCreateComputePipelines failed.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Compute Pipeline.");
    }

    std::shared_ptr<IIr77Return const> GetPipeline(VkPipeline* pipeline) {
        *pipeline = m_pipeline;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetKind(Ir77PVComputeKind* kind) {
        *kind = m_kind;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    std::shared_ptr<IIr77PVDevice> m_device{nullptr};

    std::shared_ptr<IIr77PVLayoutCPT> m_layout{nullptr};

    std::shared_ptr<IIr77PVShader> m_shader_stack{nullptr};

    Ir77PVComputeKind m_kind{Ir77PVComputeKind::Culling};

    VkPipeline m_pipeline{VK_NULL_HANDLE};
};

}  // namespace NSIr77PeregrineV