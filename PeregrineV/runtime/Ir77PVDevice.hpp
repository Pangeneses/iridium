#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <memory>
#include <vector>
#include <cstring>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

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
    static constexpr const char* DEVICE_EXTENSIONS[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context) {
        m_context = context;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> EnumeratePhysicalDevice() {
        VkInstance instance = GetInstance();

        m_physical_device_count = 0;
        vkEnumeratePhysicalDevices(instance, &m_physical_device_count, nullptr);

        if (m_physical_device_count == 0) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: no Vulkan devices found.");
        }

        m_phys_devices.resize(m_physical_device_count);
        vkEnumeratePhysicalDevices(instance, &m_physical_device_count, m_phys_devices.data());

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> InitCreateDeviceInfo() {
        std::vector<VkDeviceQueueCreateInfo> info_list;
        info_list = GetQueueInfo();

        m_device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        m_device_create_info.queueCreateInfoCount = static_cast<uint32_t>(info_list.size());
        m_device_create_info.pQueueCreateInfos = info_list.data();
        m_device_create_info.enabledExtensionCount = 1;
        m_device_create_info.ppEnabledExtensionNames = DEVICE_EXTENSIONS;
        m_device_create_info.pEnabledFeatures = &m_phys_device_features;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CreateDevice() {
        if (vkCreateDevice(m_phys_devices.at(0), &m_device_create_info, nullptr, &m_device) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: vkCreateDevice failed.");
        }
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   public:
    std::shared_ptr<IIr77Return const> GetVkPhysicalDevice(VkPhysicalDevice* phys_device) {
        *phys_device = m_phys_devices[0];

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetVkDevice(VkDevice* device) {
        *device = m_device;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    VkInstance GetInstance();

    std::vector<VkDeviceQueueCreateInfo> GetQueueInfo();
    
   private:
    std::shared_ptr<IIr77Enlisted> m_context;

    uint32_t m_physical_device_count = 0;

    std::vector<VkPhysicalDevice> m_phys_devices;

    VkDeviceCreateInfo m_device_create_info;

    VkPhysicalDeviceFeatures m_phys_device_features;

    VkPhysicalDeviceLimits m_phys_device_limits;

    VkDevice m_device{VK_NULL_HANDLE};
};
}  // namespace NSIr77PeregrineV