#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <map>
#include <memory>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../../dictionary/IDIIr77PeregrineV.hpp"
#include "../../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

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
    std::shared_ptr<IIr77Return const> MemberOfUuid(std::shared_ptr<IIr77GUID const>& uid) const {
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
    std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context) {
        m_context = context;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> AddShader(Ir77PVShaderInfo& info, std::shared_ptr<IIr77GUID const>& uid) {
        VkDevice device = GetDevice();

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = info.size;
        createInfo.pCode = info.byte_code.data();

        if (vkCreateShaderModule(device, &createInfo, nullptr, &info.shader_module) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: vkCreateShaderModule failed.");
        }

        shader_infos.emplace(reinterpret_cast<Ir77GUID const*>(uid.get()), info);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetShaderInfo(Ir77PVShaderInfo& info, std::shared_ptr<IIr77GUID const> uid) {
        info = shader_infos.at(reinterpret_cast<Ir77GUID const*>(uid.get()));

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    VkDevice GetDevice();

   private:
    std::shared_ptr<IIr77Enlisted> m_context;

    std::map<Ir77GUID const*, Ir77PVShaderInfo> shader_infos;
};
}  // namespace NSIr77PeregrineV