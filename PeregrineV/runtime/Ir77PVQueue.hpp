#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <vector>
#include <cstring>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77PVDevice.hpp"
#include "../interface/IIr77PVSurface.hpp"
#include "../interface/IIr77PVQueue.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77PVQueue : public Ir77Enlisted, public IIr77PVQueue, public std::enable_shared_from_this<Ir77PVQueue> {
   public:
    Ir77PVQueue() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

   public:
    std::shared_ptr<IIr77Return const> MemberOfUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVQueue>(uid);

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

        else if (iid == &GUIDIr77PVQueue)
            obj = std::shared_ptr<Ir77PVQueue>(shared_from_this(), static_cast<Ir77PVQueue*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context) {
        m_context = context;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> InitQueueFamilyProps() {
        VkPhysicalDevice device = GetPhysicalDevice();
        VkSurfaceKHR surface = GetSurface();

        m_queue_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &m_queue_count, nullptr);

        m_family_prop_list.resize(m_queue_count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &m_queue_count, m_family_prop_list.data());

        for (uint32_t i = 0; i < m_queue_count; ++i) {
            if (m_family_prop_list[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) m_graphics_family = i;

            VkBool32 present_support = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
            if (present_support) m_present_family = i;

            if (m_graphics_family != UINT32_MAX && m_present_family != UINT32_MAX) break;
        }

        if (m_graphics_family == UINT32_MAX || m_present_family == UINT32_MAX) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: required queue families not found.");
        }
    }

    std::shared_ptr<IIr77Return const> InitQueueInfos() {
        auto add_queue = [&](uint32_t family) {
            VkDeviceQueueCreateInfo qi{};
            qi.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            qi.queueFamilyIndex = family;
            qi.queueCount = 1;
            qi.pQueuePriorities = &m_priority;
            m_queue_create_info_list.push_back(qi);
        };

        add_queue(m_graphics_family);
        if (m_present_family != m_graphics_family) add_queue(m_present_family);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CreateResources() { return Ir77RETURN<Ir77OperationSucceeded>(); }

    std::shared_ptr<IIr77Return const> GetDeviceQueueCreateInfo(std::vector<VkDeviceQueueCreateInfo>& info_list) {
        info_list = m_queue_create_info_list;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetQueue(VkQueue& m_queue) { return Ir77RETURN<Ir77OperationSucceeded>(); }

    std::shared_ptr<IIr77Return const> Submit(std::shared_ptr<IIr77PVCommandBuffer const>& cmd) { return Ir77RETURN<Ir77OperationSucceeded>(); }

    std::shared_ptr<IIr77Return const> WaitIdle() { return Ir77RETURN<Ir77OperationSucceeded>(); }

    std::shared_ptr<IIr77Return const> Present(std::shared_ptr<IIr77PVSurface const>& surface) { return Ir77RETURN<Ir77OperationSucceeded>(); }

    std::shared_ptr<IIr77Return const> GetQueueType(Ir77PVQueueType& type) const { return Ir77RETURN<Ir77OperationSucceeded>(); }

   private:
    VkPhysicalDevice GetPhysicalDevice();

    VkSurfaceKHR GetSurface();

   private:
    std::shared_ptr<IIr77Enlisted> m_context;

    uint32_t m_queue_count = 0;

    std::vector<VkQueueFamilyProperties> m_family_prop_list;

    uint32_t m_graphics_family{UINT32_MAX};

    float m_priority = 1.0f;

    std::vector<VkDeviceQueueCreateInfo> m_queue_create_info_list;

    uint32_t m_present_family{UINT32_MAX};

    /*********************************************** */

    VkQueue m_queue{VK_NULL_HANDLE};

    VkQueue m_gfx_queue{VK_NULL_HANDLE};

    VkQueue m_present_queue{VK_NULL_HANDLE};
};
}  // namespace NSIr77PeregrineV