#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <string>
#include <set>
#include <vector>
#include <memory>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77PVDevice.hpp"
#include "../interface/IIr77PVQueue.hpp"

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
    std::shared_ptr<IIr77Return const> MemberOfUuid(std::shared_ptr<IIr77GUID const>& uid) const {
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

        else if (iid == &GUIDIr77PVDevice)
            obj = std::shared_ptr<Ir77PVDevice>(shared_from_this(), static_cast<Ir77PVDevice*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    const std::vector<const char*> DEVICE_EXTENSIONS = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context) {
        m_context = context;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> EnumeratePhysicalDevice() {
        VkInstance instance = GetInstance();

        std::uint32_t physical_device_count = 0;
        vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr);

        if (physical_device_count == 0) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: no Vulkan devices found.");
        }

        m_phys_devices.resize(physical_device_count);
        vkEnumeratePhysicalDevices(instance, &physical_device_count, m_phys_devices.data());

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CheckDeviceExtensionSupport(std::uint32_t index) {
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(m_phys_devices[index], nullptr, &extensionCount, nullptr);

        m_available_extensions.resize(extensionCount);
        vkEnumerateDeviceExtensionProperties(m_phys_devices[index], nullptr, &extensionCount, m_available_extensions[index].data());

        std::set<std::string> requiredExtensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());

        for (const auto& extension : m_available_extensions.at(index)) {
            requiredExtensions.erase(extension.extensionName);
        }

        if (!requiredExtensions.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: Vulkan Swap Chain not supported.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> InitCreateDeviceInfo() {
        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        queue_create_infos = GetQueueInfos();

        m_phys_device_features.samplerAnisotropy = VK_TRUE;
        m_phys_device_features.geometryShader = VK_TRUE;
        m_phys_device_features.multiDrawIndirect = VK_TRUE;
        m_phys_device_features.independentBlend = VK_TRUE;

        m_device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        m_device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
        m_device_create_info.pQueueCreateInfos = queue_create_infos.data();
        m_device_create_info.enabledExtensionCount = 1;
        m_device_create_info.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();
        m_device_create_info.pEnabledFeatures = &m_phys_device_features;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CreateDevice(std::uint32_t index) {
        std::vector<Ir77PVDeviceQueue> families = GetQueueFamilies();
        
        if (vkCreateDevice(m_phys_devices.at(index), &m_device_create_info, nullptr, &m_device) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: vkCreateDevice failed.");
        }

        for(Ir77PVDeviceQueue family : families) {
            vkGetDeviceQueue(m_device, family.index, 0, &family.queue);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   public:
    std::shared_ptr<IIr77Return const> GetPhysicalDeviceCount(std::uint32_t& indeces) {
        indeces = m_phys_devices.size();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetVkPhysicalDevice(VkPhysicalDevice* phys_device, std::uint32_t index) {
        *phys_device = m_phys_devices[index];

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetVkDevice(VkDevice* device) {
        *device = m_device;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    VkInstance GetInstance();

    std::vector<VkDeviceQueueCreateInfo> GetQueueInfos();

    std::vector<Ir77PVDeviceQueue>  GetQueueFamilies();

   private:
    std::shared_ptr<IIr77Enlisted> m_context;

    std::vector<VkPhysicalDevice> m_phys_devices;

    VkDevice m_device{VK_NULL_HANDLE};

    std::vector<std::vector<VkExtensionProperties>> m_available_extensions;

    VkPhysicalDeviceFeatures m_phys_device_features;

    VkPhysicalDeviceLimits m_phys_device_limits;

    VkDeviceCreateInfo m_device_create_info;
};
}  // namespace NSIr77PeregrineV