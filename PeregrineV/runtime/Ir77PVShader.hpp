#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <fstream>
#include <map>
#include <memory>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PVContext.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"
#include "../../Ir77RT/runtime/Ir77Return.hpp"

#include "../interface/IIr77PVShader.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77PVShader : public Ir77Enlisted, public IIr77PVShader, public std::enable_shared_from_this<Ir77PVShader> {
   public:
    Ir77PVShader() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    ~Ir77PVShader() {
        VkDevice device;
        m_device->GetDevice(&device);
        vkDeviceWaitIdle(device);
        for (auto& [id, info] : m_shader_infos) {
            vkDestroyShaderModule(device, info.stage_create_info.module, nullptr);
        }
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVShader>(uid);

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

        else if (iid == &GUIDIr77PVShader)
            obj = std::shared_ptr<Ir77PVShader>(shared_from_this(), static_cast<Ir77PVShader*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) {
        m_device = device;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> LoadShader(std::string const& filename, std::uint64_t const& shader_id, Ir77PVShaderStage shader_stage) {
        std::ifstream shader_file(filename, std::ios::ate | std::ios::binary);

        if (!shader_file.is_open()) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Enlisted has been invalidated.");
        }

        size_t shader_file_size = (size_t)shader_file.tellg();
        std::vector<char> shader_buffer(shader_file_size);

        shader_file.seekg(0);
        shader_file.read(shader_buffer.data(), shader_file_size);

        shader_file.close();

        Ir77PVShaderInfo shader_info;
        shader_info.size = shader_file_size;
        shader_info.byte_code = shader_buffer;
        shader_info.stage = shader_stage;

        AddShader(shader_info, shader_id, shader_stage);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    std::shared_ptr<IIr77Return const> AddShader(Ir77PVShaderInfo& info, std::uint64_t const& shader_id, Ir77PVShaderStage shader_stage) {
        VkDevice device;
        m_device->GetDevice(&device);

        VkShaderModuleCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = info.size;
        create_info.pCode = reinterpret_cast<const uint32_t*>(info.byte_code.data());

        if (vkCreateShaderModule(device, &create_info, nullptr, &info.stage_create_info.module) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: vkCreateShaderModule failed.");
        }

        info.stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

        switch (info.stage) {
            case Ir77PVShaderStage::Vertex:
                info.stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case Ir77PVShaderStage::Fragment:
                info.stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            case Ir77PVShaderStage::Compute:
                info.stage_create_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
                break;
            case Ir77PVShaderStage::Geometry:
                info.stage_create_info.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
                break;
            case Ir77PVShaderStage::TessellationControl:
                info.stage_create_info.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
                break;
            case Ir77PVShaderStage::TessellationEvaluation:
                info.stage_create_info.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
                break;
            case Ir77PVShaderStage::Mesh:
                info.stage_create_info.stage = VK_SHADER_STAGE_MESH_BIT_EXT;
                break;
            case Ir77PVShaderStage::Task:
                info.stage_create_info.stage = VK_SHADER_STAGE_TASK_BIT_EXT;
                break;
            case Ir77PVShaderStage::RayGeneration:
                info.stage_create_info.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
                break;
            case Ir77PVShaderStage::RayMiss:
                info.stage_create_info.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
                break;
            case Ir77PVShaderStage::RayClosestHit:
                info.stage_create_info.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
                break;
            case Ir77PVShaderStage::RayAnyHit:
                info.stage_create_info.stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
                break;
            case Ir77PVShaderStage::RayIntersection:
                info.stage_create_info.stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
                break;
        }

        info.stage_create_info.pName = "main";

        m_shader_infos.emplace(shader_id, info);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetPipelineShaderStageInfos(std::vector<VkPipelineShaderStageCreateInfo>& infos,
                                                                   std::vector<std::uint64_t> const& id_list) {
        infos.clear();
        infos.reserve(id_list.size());

        for (auto const id : id_list) {
            auto const it = m_shader_infos.find(id);
            if (it == m_shader_infos.end()) {
                return Ir77RETURN<Ir77NotConfigured>(this, "required shader stage not loaded");
            }
            infos.push_back(it->second.stage_create_info);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    std::shared_ptr<IIr77PVDevice> m_device;

    std::uint32_t m_device_index;

    std::map<std::uint64_t, Ir77PVShaderInfo> m_shader_infos;
};
}  // namespace NSIr77PeregrineV
