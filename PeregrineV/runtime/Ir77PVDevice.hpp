#pragma once

#include <SDL3/SDL_video.h>
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77PVDevice.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77PVDevice : public Ir77Enlisted, public IIr77PVDevice, public std::enable_shared_from_this<Ir77PVDevice> {
   public:
    Ir77PVDevice() {
        m_enlisted_uuid.Generate();

        m_enlisted = std::chrono::system_clock::now();
    }

    ~Ir77PVDevice() {
        if (m_device != VK_NULL_HANDLE) vkDestroyDevice(m_device, nullptr);
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVDevice>(uid);

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

        else if (iid == GUIDIIr77PVDevice)
            obj = std::shared_ptr<IIr77PVDevice>(shared_from_this(), static_cast<IIr77PVDevice*>(this));

        else if (iid == GUIDIr77PVDevice)
            obj = std::shared_ptr<Ir77PVDevice>(shared_from_this(), static_cast<Ir77PVDevice*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> SetInstance(std::shared_ptr<IIr77PVInstance> instance) {
        m_instance = instance;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> EnumeratePhysicalDevices(std::uint32_t const& device_index) {
        VkInstance instance;
        m_instance->GetInstance(&instance);

        std::uint32_t physical_device_count = 0;
        vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr);

        if (physical_device_count == 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDevice: no Vulkan devices found.");

        std::vector<VkPhysicalDevice> phys_devices(physical_device_count);
        vkEnumeratePhysicalDevices(instance, &physical_device_count, phys_devices.data());

        if (device_index >= physical_device_count) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDevice: device index out of range.");

        m_phys_device = phys_devices[device_index];

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    /************************************************************************************************************************************************************/

    std::shared_ptr<IIr77Return const> DefineQueueFamilyProps(SDL_Window* window) {
        VkInstance instance;
        m_instance->GetInstance(&instance);

        VkSurfaceKHR surface;
        if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface)) {
            std::string str{SDL_GetError()};
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDevice: SDL_Vulkan_CreateSurface failed: " + str);
        }

        std::uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_phys_device, &queue_family_count, nullptr);

        m_queue_families.resize(queue_family_count);

        std::vector<VkQueueFamilyProperties> fp(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(m_phys_device, &queue_family_count, fp.data());

        for (std::uint32_t i = 0; i < queue_family_count; ++i) {
            m_queue_families[i].family_properties = fp[i];

            if (fp[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) m_queue_families[i].type = IR77_GRAPHICS_BIT | m_queue_families[i].type;
            if (fp[i].queueFlags & VK_QUEUE_COMPUTE_BIT) m_queue_families[i].type = IR77_COMPUTE_BIT | m_queue_families[i].type;
            if (fp[i].queueFlags & VK_QUEUE_TRANSFER_BIT) m_queue_families[i].type = IR77_TRANSFER_BIT | m_queue_families[i].type;
            if (fp[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) m_queue_families[i].type = IR77_SPARSE_BIT | m_queue_families[i].type;
            if (fp[i].queueFlags & VK_QUEUE_PROTECTED_BIT) m_queue_families[i].type = IR77_PROTECTED_BIT | m_queue_families[i].type;
            if (fp[i].queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) m_queue_families[i].type = IR77_DECODE_BIT | m_queue_families[i].type;
            if (fp[i].queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR) m_queue_families[i].type = IR77_ENCODE_BIT | m_queue_families[i].type;

            VkBool32 present_support = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(m_phys_device, i, surface, &present_support);

            bool const graphics = (m_queue_families[i].type & IR77_GRAPHICS_BIT) == IR77_GRAPHICS_BIT;
            m_queue_families[i].presentation = (graphics && present_support == VK_TRUE) ? VK_TRUE : VK_FALSE;
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineQueueCreateInfos() {
        for (std::size_t i = 0; i < m_queue_families.size(); i++) {
            VkDeviceQueueCreateInfo queue_create_info{};
            queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.queueFamilyIndex = static_cast<std::uint32_t>(i);
            queue_create_info.queueCount = 1;
            queue_create_info.pQueuePriorities = &m_queue_priority;  // member -- must outlive vkCreateDevice
            m_queue_families[i].create_info = queue_create_info;
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    /************************************************************************************************************************************************************/

    // Builds the extension list from what the GPU actually exposes. Required ones fail the call; optional ones are added only when present.
    std::shared_ptr<IIr77Return const> CheckDeviceExtensionSupport() {
        std::uint32_t count = 0;
        vkEnumerateDeviceExtensionProperties(m_phys_device, nullptr, &count, nullptr);

        m_available_extensions.resize(count);
        vkEnumerateDeviceExtensionProperties(m_phys_device, nullptr, &count, m_available_extensions.data());

        if (!HasExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDevice: swapchain extension not supported.");

        m_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

        // optional -- kept only if DefineDeviceInfo also finds the features it needs
        if (HasExtension(VK_EXT_MESH_SHADER_EXTENSION_NAME)) m_extensions.push_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);
        if (HasExtension(VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME)) m_extensions.push_back(VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Features go through VkPhysicalDeviceFeatures2 in pNext, so extension feature structs can be chained after it
    std::shared_ptr<IIr77Return const> DefineDeviceInfo() {
        if (m_extensions.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDevice: CheckDeviceExtensionSupport not called.");

        // query: features2 -> mesh -> vulkan11
        VkPhysicalDeviceVulkan11Features v11_supported{};
        v11_supported.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;

        VkPhysicalDeviceMeshShaderFeaturesEXT mesh_supported{};
        mesh_supported.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
        mesh_supported.pNext = &v11_supported;

        VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT maintenance_supported{};
        maintenance_supported.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT;
        v11_supported.pNext = &maintenance_supported;  // query: features2 -> mesh -> vulkan11 -> maintenance

        VkPhysicalDeviceFeatures2 supported{};
        supported.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        supported.pNext = &mesh_supported;
        vkGetPhysicalDeviceFeatures2(m_phys_device, &supported);

        if (!supported.features.multiDrawIndirect) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDevice: multiDrawIndirect not supported.");
        if (!supported.features.drawIndirectFirstInstance) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDevice: drawIndirectFirstInstance not supported.");

        m_phys_device_features = VkPhysicalDeviceFeatures{};
        m_phys_device_features.samplerAnisotropy = supported.features.samplerAnisotropy;
        m_phys_device_features.geometryShader = supported.features.geometryShader;
        m_phys_device_features.multiDrawIndirect = supported.features.multiDrawIndirect;
        m_phys_device_features.independentBlend = supported.features.independentBlend;
        m_phys_device_features.drawIndirectFirstInstance = supported.features.drawIndirectFirstInstance;

        // mesh shading -- extension present, both stages supported, and gl_DrawID available; otherwise the extension is dropped entirely
        auto const mesh_ext = FindRequested(VK_EXT_MESH_SHADER_EXTENSION_NAME);

        m_mesh_shading = mesh_ext != m_extensions.end() && mesh_supported.meshShader == VK_TRUE && mesh_supported.taskShader == VK_TRUE &&
                         v11_supported.shaderDrawParameters == VK_TRUE;

        if (!m_mesh_shading && mesh_ext != m_extensions.end()) m_extensions.erase(mesh_ext);

        m_mesh_features = VkPhysicalDeviceMeshShaderFeaturesEXT{};
        m_mesh_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
        m_mesh_features.taskShader = m_mesh_shading ? VK_TRUE : VK_FALSE;
        m_mesh_features.meshShader = m_mesh_shading ? VK_TRUE : VK_FALSE;

        // swapchain maintenance -- extension present AND feature supported; otherwise dropped
        auto const maintenance_ext = FindRequested(VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME);

        m_swapchain_maintenance = maintenance_ext != m_extensions.end() && maintenance_supported.swapchainMaintenance1 == VK_TRUE;

        if (!m_swapchain_maintenance && maintenance_ext != m_extensions.end()) m_extensions.erase(maintenance_ext);

        m_maintenance_features = VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT{};
        m_maintenance_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT;
        m_maintenance_features.swapchainMaintenance1 = VK_TRUE;

        // enable: create info -> features2 -> vulkan11 -> [mesh] -> [maintenance]
        void* tail = nullptr;
        if (m_swapchain_maintenance) {
            m_maintenance_features.pNext = tail;
            tail = &m_maintenance_features;
        }
        if (m_mesh_shading) {
            m_mesh_features.pNext = tail;
            tail = &m_mesh_features;
        }

        m_vulkan11 = VkPhysicalDeviceVulkan11Features{};
        m_vulkan11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        m_vulkan11.shaderDrawParameters = v11_supported.shaderDrawParameters;
        m_vulkan11.pNext = tail;

        m_features2 = VkPhysicalDeviceFeatures2{};
        m_features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        m_features2.features = m_phys_device_features;
        m_features2.pNext = &m_vulkan11;

        m_create_info.clear();
        for (std::size_t i = 0; i < m_queue_families.size(); i++) m_create_info.push_back(m_queue_families[i].create_info);

        m_device_create_info = VkDeviceCreateInfo{};
        m_device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        m_device_create_info.pNext = &m_features2;
        m_device_create_info.pEnabledFeatures = nullptr;  // must be null when features come through VkPhysicalDeviceFeatures2
        m_device_create_info.queueCreateInfoCount = static_cast<std::uint32_t>(m_create_info.size());
        m_device_create_info.pQueueCreateInfos = m_create_info.data();
        m_device_create_info.enabledExtensionCount = static_cast<std::uint32_t>(m_extensions.size());
        m_device_create_info.ppEnabledExtensionNames = m_extensions.data();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineDevice() {
        if (vkCreateDevice(m_phys_device, &m_device_create_info, nullptr, &m_device) != VK_SUCCESS) {
            m_device = VK_NULL_HANDLE;
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDevice: vkCreateDevice failed.");
        }

        for (std::size_t i = 0; i < m_queue_families.size(); i++) {
            vkGetDeviceQueue(m_device, m_queue_families[i].create_info.queueFamilyIndex, 0, &m_queue_families[i].queue);
        }

        vkGetPhysicalDeviceProperties(m_phys_device, &m_phys_device_properties);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    /************************************************************************************************************************************************************/

    std::shared_ptr<IIr77Return const> GetPhysicalDevice(VkPhysicalDevice* phys_device) {
        *phys_device = m_phys_device;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetDeviceProperties(VkPhysicalDeviceProperties& phys_device_props) {
        phys_device_props = m_phys_device_properties;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Core features that were enabled (not merely supported)
    std::shared_ptr<IIr77Return const> GetDeviceFeatures(VkPhysicalDeviceFeatures& phys_device_features) {
        phys_device_features = m_phys_device_features;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetSwapchainMaintenance(bool& enabled) {
        enabled = m_swapchain_maintenance;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // true only when VK_EXT_mesh_shader is enabled with task + mesh stages
    std::shared_ptr<IIr77Return const> GetMeshShading(bool& enabled) {
        enabled = m_mesh_shading;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetDevice(VkDevice* device) {
        *device = m_device;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetQueueFamilies(std::vector<Ir77PVQueueFamily>& queue_family) {
        queue_family = m_queue_families;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    bool HasExtension(char const* name) const {
        return std::any_of(m_available_extensions.begin(), m_available_extensions.end(),
                           [name](VkExtensionProperties const& e) { return std::strcmp(e.extensionName, name) == 0; });
    }

    std::vector<char const*>::iterator FindRequested(char const* name) {
        return std::find_if(m_extensions.begin(), m_extensions.end(), [name](char const* e) { return std::strcmp(e, name) == 0; });
    }

   private:
    std::shared_ptr<IIr77PVInstance> m_instance;

    VkPhysicalDevice m_phys_device{VK_NULL_HANDLE};

    VkPhysicalDeviceProperties m_phys_device_properties{};

    VkPhysicalDeviceFeatures m_phys_device_features{};

    std::vector<Ir77PVQueueFamily> m_queue_families;

    std::vector<VkDeviceQueueCreateInfo> m_create_info;

    float m_queue_priority{1.0f};

    VkDevice m_device{VK_NULL_HANDLE};

    std::vector<VkExtensionProperties> m_available_extensions;

    // requested extensions -- pointers into this list are handed to vkCreateDevice, so it must outlive DefineDevice
    std::vector<char const*> m_extensions{};

    // feature chain -- members for the same reason
    VkPhysicalDeviceFeatures2 m_features2{};

    VkPhysicalDeviceMeshShaderFeaturesEXT m_mesh_features{};

    bool m_mesh_shading{false};

    bool m_swapchain_maintenance{false};

    VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT m_maintenance_features{};

    VkDeviceCreateInfo m_device_create_info{};

    VkPhysicalDeviceVulkan11Features m_vulkan11{};
};
}  // namespace NSIr77PeregrineV