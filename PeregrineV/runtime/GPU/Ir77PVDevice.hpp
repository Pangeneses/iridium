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

#include "../../dictionary/IDIIr77PeregrineV.hpp"
#include "../../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../../interface/IIr77PVDevice.hpp"

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

    ~Ir77PVDevice() {
            vkDestroyDevice(m_device.device, nullptr);
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

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

    std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context, std::uint32_t const& device_index) {
        m_context = context;

        m_device_index = device_index;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> EnumeratePhysicalDevices() {
        VkInstance instance = GetInstance();

        std::uint32_t physical_device_count = 0;
        vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr);

        if (physical_device_count == 0) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: no Vulkan devices found.");
        }

        std::vector<VkPhysicalDevice> phys_devices;
        phys_devices.resize(physical_device_count);

        vkEnumeratePhysicalDevices(instance, &physical_device_count, phys_devices.data());

        m_device.phys_device = phys_devices.at(m_device_index);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CheckDeviceExtensionSupport() {
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(m_device.phys_device, nullptr, &extensionCount, nullptr);

        m_device.available_extensions.resize(extensionCount);
        vkEnumerateDeviceExtensionProperties(m_device.phys_device, nullptr, &extensionCount, m_device.available_extensions.data());

        std::set<std::string> requiredExtensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());

        for (const auto& extension : m_device.available_extensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineDeviceInfo() {
        Ir77PVQueueInfo queue = GetQueueInfo();

        m_device.phys_device_features.samplerAnisotropy = VK_TRUE;
        m_device.phys_device_features.geometryShader = VK_TRUE;
        m_device.phys_device_features.multiDrawIndirect = VK_TRUE;
        m_device.phys_device_features.independentBlend = VK_TRUE;

        m_device.device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        m_device.device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue.create_infos.size());
        m_device.device_create_info.pQueueCreateInfos = queue.create_infos.data();
        m_device.device_create_info.enabledExtensionCount = 1;
        m_device.device_create_info.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();
        m_device.device_create_info.pEnabledFeatures = &m_device.phys_device_features;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CreateDevice() {
        Ir77PVQueueInfo queue = GetQueueInfo();

        if (vkCreateDevice(m_device.phys_device, &m_device.device_create_info, nullptr, &m_device.device) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: vkCreateDevice failed.");
        }

        for (int i = 0; i < queue.create_infos.size(); i++) {
            vkGetDeviceQueue(m_device.device, queue.create_infos.at(i).queueFamilyIndex, 0, &queue.queues.at(i));
        }

        vkGetPhysicalDeviceProperties(m_device.phys_device, &m_device.device_properties);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetDeviceInfo(Ir77PVDeviceInfo& device_info) {
        device_info = m_device;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    VkInstance GetInstance();

    Ir77PVQueueInfo GetQueueInfo();

   private:
    std::shared_ptr<IIr77Enlisted> m_context;

    std::uint32_t m_device_index;

    Ir77PVDeviceInfo m_device;
};
}  // namespace NSIr77PeregrineV