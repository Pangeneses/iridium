#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <fstream>
#include <map>
#include <memory>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../../dictionary/IDIr77PeregrineV.hpp"
#include "../../dictionary/IDIr77PVContext.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"
#include "../../Ir77RT/runtime/Ir77Return.hpp"

#include "../../interface/IIr77PVShader.hpp"

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

    std::shared_ptr<IIr77Return const> ReadShader(std::string const& filename) {
        std::ifstream vert("/home/alpha/workspace/iridium/Shader/vert.spv", std::ios::ate | std::ios::binary);

        if (!vert.is_open()) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Enlisted has been invalidated.");
        }

        size_t vert_file_size = (size_t)vert.tellg();
        std::vector<char> vert_buffer(vert_file_size);

        vert.seekg(0);
        vert.read(vert_buffer.data(), vert_file_size);

        vert.close();

        Ir77PVShaderInfo vert_info;
        vert_info.size = vert_file_size;
        vert_info.byte_code = vert_buffer;
        vert_info.stage = Ir77PVShaderStage::Vertex;

        AddShader(vert_info, ID_SHADER_VERT);

        std::ifstream frag("/home/alpha/workspace/iridium/Shader/frag.spv", std::ios::ate | std::ios::binary);

        if (!frag.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t frag_file_size = (size_t)frag.tellg();
        std::vector<char> frag_buffer(frag_file_size);

        frag.seekg(0);
        frag.read(frag_buffer.data(), frag_file_size);

        frag.close();

        Ir77PVShaderInfo frag_info;
        frag_info.size = frag_file_size;
        frag_info.byte_code = frag_buffer;
        frag_info.stage = Ir77PVShaderStage::Fragment;

        AddShader(frag_info, ID_SHADER_FRAG);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> AddShader(Ir77PVShaderInfo& info, std::uint64_t const& id) {
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
        info.stage_create_info.stage = (info.stage == Ir77PVShaderStage::Vertex) ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT;
        info.stage_create_info.pName = "main";

        m_shader_infos.emplace(id, info);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetPipelineShaderStageInfos(std::vector<VkPipelineShaderStageCreateInfo>& infos,
                                                                   std::vector<std::uint64_t> const& id_list) {
        for (int i = 0; i < id_list.size(); i++) {
            infos.push_back(m_shader_infos.at(id_list[i]).stage_create_info);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    std::shared_ptr<IIr77PVDevice> m_device;

    std::uint32_t m_device_index;

    std::map<std::uint64_t, Ir77PVShaderInfo> m_shader_infos;
};
}  // namespace NSIr77PeregrineV