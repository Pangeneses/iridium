#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <memory>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../interface/IIr77PVInstance.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77PVInstance : public Ir77Enlisted, public IIr77PVInstance, public std::enable_shared_from_this<Ir77PVInstance> {
   public:
    Ir77PVInstance() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    ~Ir77PVInstance() {
      vkDestroyInstance(m_instance, nullptr);
    }

   public:
    std::shared_ptr<IIr77Return const> MemberOfUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVInstance>(uid);

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

        else if (iid == &GUIDIr77PVInstance)
            obj = std::shared_ptr<Ir77PVInstance>(shared_from_this(), static_cast<Ir77PVInstance*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    static constexpr const char* VALIDATION_LAYERS[] = {"VK_LAYER_KHRONOS_validation"};

    std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context) {
        m_context = context;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> InitAppInfo() {
        m_application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        m_application_info.pApplicationName = "Iridium";
        m_application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        m_application_info.pEngineName = "Iridium";
        m_application_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        m_application_info.apiVersion = VK_API_VERSION_1_3;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

#ifdef NDEBUG
    static constexpr bool ENABLE_VALIDATION = false;
#else
    static constexpr bool ENABLE_VALIDATION = true;
#endif

    std::shared_ptr<IIr77Return const> InitExtensions() {
        m_sdl_ext_count = 0;

        const char* const* sdl_exts = SDL_Vulkan_GetInstanceExtensions(&m_sdl_ext_count);

        m_extensions = std::vector<const char*>(sdl_exts, sdl_exts + m_sdl_ext_count);

        if (ENABLE_VALIDATION) m_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> InitCreateInfo() {
        m_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        m_create_info.pApplicationInfo = &m_application_info;
        m_create_info.enabledExtensionCount = static_cast<uint32_t>(m_extensions.size());
        m_create_info.ppEnabledExtensionNames = m_extensions.data();
        if (ENABLE_VALIDATION) {
            m_create_info.enabledLayerCount = 1;
            m_create_info.ppEnabledLayerNames = VALIDATION_LAYERS;
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> InitCreateInstance() {
        if (vkCreateInstance(&m_create_info, nullptr, &m_instance) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: vkCreateInstance failed.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   public:
    std::shared_ptr<IIr77Return const> GetVkInstance(VkInstance* instance) {
        *instance = m_instance;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    private:
    std::shared_ptr<IIr77Enlisted> m_context;

    VkApplicationInfo m_application_info;

    VkInstanceCreateInfo m_create_info;

    uint32_t m_sdl_ext_count;

    std::vector<const char*> m_extensions;

    VkInstance m_instance{VK_NULL_HANDLE};
};
}  // namespace NSIr77PeregrineV