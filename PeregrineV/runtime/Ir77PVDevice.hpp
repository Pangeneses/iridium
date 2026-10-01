#pragma once

#include <SDL3/SDL_video.h>
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <string>
#include <set>
#include <vector>
#include <memory>

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
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    ~Ir77PVDevice() { vkDestroyDevice(m_device, nullptr); }

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

    IIr77GUID* const QueryInterface(IIr77GUID const* iid, std::shared_ptr<void>& obj) {
        if (iid == &GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == &GUIDIIr77PVDevice)
            obj = std::shared_ptr<IIr77PVDevice>(shared_from_this(), static_cast<IIr77PVDevice*>(this));

        else if (iid == &GUIDIr77PVDevice)
            obj = std::shared_ptr<Ir77PVDevice>(shared_from_this(), static_cast<Ir77PVDevice*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    const std::vector<const char*> DEVICE_EXTENSIONS = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    std::shared_ptr<IIr77Return const> SetInstance(std::shared_ptr<IIr77PVInstance> instance) {
        m_instance = instance;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> EnumeratePhysicalDevices(std::uint32_t const& device_index) {
        VkInstance instance;
        m_instance->GetInstance(&instance);

        std::uint32_t physical_device_count = 0;
        vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr);

        if (physical_device_count == 0) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: no Vulkan devices found.");
        }

        std::vector<VkPhysicalDevice> phys_devices;
        phys_devices.resize(physical_device_count);

        vkEnumeratePhysicalDevices(instance, &physical_device_count, phys_devices.data());

        m_phys_device = phys_devices.at(device_index);

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

        std::vector<VkQueueFamilyProperties> fp;
        fp.resize(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(m_phys_device, &queue_family_count, fp.data());

        bool is_display = false;
        for (uint32_t i = 0; i < queue_family_count; ++i) {
            m_queue_families[i].family_properties = fp.at(i);

            if (fp[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) m_queue_families[i].type = IR77_GRAPHICS_BIT | m_queue_families[i].type;
            if (fp[i].queueFlags & VK_QUEUE_COMPUTE_BIT) m_queue_families[i].type = IR77_COMPUTE_BIT | m_queue_families[i].type;
            if (fp[i].queueFlags & VK_QUEUE_TRANSFER_BIT) m_queue_families[i].type = IR77_TRANSFER_BIT | m_queue_families[i].type;
            if (fp[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) m_queue_families[i].type = IR77_SPARSE_BIT | m_queue_families[i].type;
            if (fp[i].queueFlags & VK_QUEUE_PROTECTED_BIT) m_queue_families[i].type = IR77_PROTECTED_BIT | m_queue_families[i].type;
            if (fp[i].queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) m_queue_families[i].type = IR77_DECODE_BIT | m_queue_families[i].type;
            if (fp[i].queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR) m_queue_families[i].type = IR77_ENCODE_BIT | m_queue_families[i].type;

            VkBool32 present_support = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(m_phys_device, i, surface, &present_support);

            if (((m_queue_families[i].type & IR77_GRAPHICS_BIT) == IR77_GRAPHICS_BIT) && present_support == VK_TRUE) {
                m_queue_families[i].presentation = VK_TRUE;
            } else {
                m_queue_families[i].presentation = VK_FALSE;
            }
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineQueueCreateInfos() {
        float priority = 1.0f;

        for (std::size_t i = 0; i < m_queue_families.size(); i++) {
            VkDeviceQueueCreateInfo queue_create_info{};
            queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.pNext = nullptr;
            queue_create_info.flags = 0;
            queue_create_info.queueFamilyIndex = i;
            queue_create_info.queueCount = 1;
            queue_create_info.pQueuePriorities = &priority;
            m_queue_families[i].create_info = queue_create_info;
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    /************************************************************************************************************************************************************/

    std::shared_ptr<IIr77Return const> CheckDeviceExtensionSupport() {
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(m_phys_device, nullptr, &extensionCount, nullptr);

        m_available_extensions.resize(extensionCount);
        vkEnumerateDeviceExtensionProperties(m_phys_device, nullptr, &extensionCount, m_available_extensions.data());

        std::set<std::string> requiredExtensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());

        for (const auto& extension : m_available_extensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineDeviceInfo() {
        VkPhysicalDeviceFeatures supported{};
        vkGetPhysicalDeviceFeatures(m_phys_device, &supported);

        m_phys_device_features.samplerAnisotropy = supported.samplerAnisotropy;
        m_phys_device_features.geometryShader = supported.geometryShader;
        m_phys_device_features.multiDrawIndirect = supported.multiDrawIndirect;
        m_phys_device_features.independentBlend = supported.independentBlend;
        m_phys_device_features.drawIndirectFirstInstance = supported.drawIndirectFirstInstance;

        if (!supported.multiDrawIndirect) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDevice: multiDrawIndirect not supported.");
        if (!supported.drawIndirectFirstInstance) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDevice: drawIndirectFirstInstance not supported.");

        m_create_info.clear();
        for (std::size_t i = 0; i < m_queue_families.size(); i++) m_create_info.push_back(m_queue_families[i].create_info);

        m_device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        m_device_create_info.queueCreateInfoCount = static_cast<uint32_t>(m_create_info.size());
        m_device_create_info.pQueueCreateInfos = m_create_info.data();
        m_device_create_info.enabledExtensionCount = 1;
        m_device_create_info.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();
        m_device_create_info.pEnabledFeatures = &m_phys_device_features;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineDevice() {
        if (vkCreateDevice(m_phys_device, &m_device_create_info, nullptr, &m_device) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: vkCreateDevice failed.");
        }

        for (std::size_t i = 0; i < m_queue_families.size(); i++) {
            vkGetDeviceQueue(m_device, m_queue_families[i].create_info.queueFamilyIndex, 0, &m_queue_families[i].queue);
        }

        vkGetPhysicalDeviceProperties(m_phys_device, &m_phys_device_properties);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetPhysicalDevice(VkPhysicalDevice* phys_device) {
        *phys_device = m_phys_device;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetDeviceProperties(VkPhysicalDeviceProperties& phys_device_props) {
        phys_device_props = m_phys_device_properties;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetDeviceFeatures(VkPhysicalDeviceFeatures& phys_device_features) {
        phys_device_features = m_phys_device_features;

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
    std::shared_ptr<IIr77PVInstance> m_instance;

    VkPhysicalDevice m_phys_device{VK_NULL_HANDLE};

    VkPhysicalDeviceProperties m_phys_device_properties{};

    VkPhysicalDeviceFeatures m_phys_device_features{};

    VkPhysicalDeviceLimits m_phys_device_limits{};

    std::vector<Ir77PVQueueFamily> m_queue_families;

    std::vector<VkDeviceQueueCreateInfo> m_create_info;

    VkDevice m_device{VK_NULL_HANDLE};

    std::vector<VkExtensionProperties> m_available_extensions;

    VkDeviceCreateInfo m_device_create_info{};
};
}  // namespace NSIr77PeregrineV