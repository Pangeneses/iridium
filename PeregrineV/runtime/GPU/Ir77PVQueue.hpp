#pragma once

#include <SDL3/SDL_stdinc.h>
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../../interface/IIr77PVDevice.hpp"
#include "../../interface/IIr77PVQueue.hpp"

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

    ~Ir77PVQueue() {}

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

    std::shared_ptr<IIr77Return const> InitQueueFamilyProps(std::uint32_t const& index) {
        VkPhysicalDevice device = GetPhysicalDevice(index);
        VkSurfaceKHR surface = GetSurface();

        Uint32 queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

        m_family_props.resize(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, m_family_props.data());

        bool is_display = false;
        for (uint32_t i = 0; i < queue_family_count; ++i) {
            m_queue_families.push_back({});

            m_queue_families.back().index = i;

            if (m_family_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) m_queue_families.back().type = Ir77PVQueueType::Graphics;
            if (m_family_props[i].queueFlags & VK_QUEUE_COMPUTE_BIT) m_queue_families.back().type = Ir77PVQueueType::Compute;
            if (m_family_props[i].queueFlags & VK_QUEUE_TRANSFER_BIT) m_queue_families.back().type = Ir77PVQueueType::Transfer;
            if (m_family_props[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) m_queue_families.back().type = Ir77PVQueueType::Sparse;
            if (m_family_props[i].queueFlags & VK_QUEUE_PROTECTED_BIT) m_queue_families.back().type = Ir77PVQueueType::Protected;
            if (m_family_props[i].queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) m_queue_families.back().type = Ir77PVQueueType::Decode;
            if (m_family_props[i].queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR) m_queue_families.back().type = Ir77PVQueueType::Encode;

            VkBool32 present_support = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
            if (present_support) m_queue_families.back().presentation = VK_TRUE;

            if (m_queue_families.back().type == Ir77PVQueueType::Graphics && m_queue_families.back().presentation == VK_TRUE) is_display = true;
        }

        if (!is_display) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: required queue families not found.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> InitQueueCreateInfos() {
        float priority = 1.0f;

        for (int i = 0; i < m_family_props.size(); i++) {
            VkDeviceQueueCreateInfo queue_create_info{};
            queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.pNext = nullptr;
            queue_create_info.flags = m_family_props.at(i).queueFlags;
            queue_create_info.queueFamilyIndex = i;
            queue_create_info.queueCount = 1;
            queue_create_info.pQueuePriorities = &priority;
            m_queue_create_infos.push_back(queue_create_info);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   public:
    std::shared_ptr<IIr77Return const> GetQueueCreateInfos(std::vector<VkDeviceQueueCreateInfo>& info_list) {
        info_list = m_queue_create_infos;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetQueueFamilies(std::vector<Ir77PVQueueFamily>& families) {
        families = m_queue_families;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    VkPhysicalDevice GetPhysicalDevice(std::uint32_t const& index);

    VkSurfaceKHR GetSurface();

   private:
    std::shared_ptr<IIr77Enlisted> m_context;

    std::vector<VkQueueFamilyProperties> m_family_props;

    std::vector<VkDeviceQueueCreateInfo> m_queue_create_infos;

    std::vector<Ir77PVQueueFamily> m_queue_families;
};
}  // namespace NSIr77PeregrineV