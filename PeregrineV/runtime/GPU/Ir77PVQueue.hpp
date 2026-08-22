#pragma once

#include <SDL3/SDL_stdinc.h>
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../../dictionary/IDIIr77PeregrineV.hpp"
#include "../../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../../interface/IIr77PVQueue.hpp"
#include "interface/IIr77PeregrineV.hpp"
#include "interface/IIr77PVSwapchain.hpp"
#include "../../interface/IIr77PVDevice.hpp"

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
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

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

        else if (iid == &GUIDIIr77PVQueue)
            obj = std::shared_ptr<IIr77PVQueue>(shared_from_this(), static_cast<IIr77PVQueue*>(this));

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

    std::shared_ptr<IIr77Return const> CreateQueues() {
        std::vector<Ir77PVDeviceInfo> device_infos = GetDeviceInfos();

        m_queues.resize(device_infos.size());
        for (int i = 0; i < device_infos.size(); i++) {
            DefineQueueFamilyProps(i);

            DefineQueueCreateInfos(i);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineQueueFamilyProps(std::uint32_t const& index) {
        Ir77PVQueueInfo info = m_queues.at(index);

        VkPhysicalDevice phys_device = GetDeviceInfos().at(index).phys_device;
        VkSurfaceKHR surface = GetSwapchainInfos().at(index).surface;

        Uint32 queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(phys_device, &queue_family_count, nullptr);

        std::vector<VkQueueFamilyProperties> fp;
        fp.resize(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(phys_device, &queue_family_count, fp.data());

        bool is_display = false;
        for (uint32_t i = 0; i < queue_family_count; ++i) {
            info.family_properties.push_back({});

            info.family_properties.back() = fp.at(i);

            if (fp[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) info.type.at(i) = Ir77PVQueueType::Graphics;
            if (fp[i].queueFlags & VK_QUEUE_COMPUTE_BIT) info.type.at(i) = Ir77PVQueueType::Compute;
            if (fp[i].queueFlags & VK_QUEUE_TRANSFER_BIT) info.type.at(i) = Ir77PVQueueType::Transfer;
            if (fp[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) info.type.at(i) = Ir77PVQueueType::Sparse;
            if (fp[i].queueFlags & VK_QUEUE_PROTECTED_BIT) info.type.at(i) = Ir77PVQueueType::Protected;
            if (fp[i].queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) info.type.at(i) = Ir77PVQueueType::Decode;
            if (fp[i].queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR) info.type.at(i) = Ir77PVQueueType::Encode;

            VkBool32 present_support = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(phys_device, i, surface, &present_support);

            if (present_support) info.presentation.at(i) = VK_TRUE;

            if (info.type.at(i) == Ir77PVQueueType::Graphics && present_support == VK_TRUE) {
                info.presentation.at(i) = VK_TRUE;
            } else {
                info.presentation.at(i) = VK_FALSE;
            }
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineQueueCreateInfos(std::uint32_t const& index) {
        Ir77PVQueueInfo info = m_queues.at(index);

        float priority = 1.0f;

        for (int i = 0; i < info.family_properties.size(); i++) {
            VkDeviceQueueCreateInfo queue_create_info{};
            queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.pNext = nullptr;
            queue_create_info.flags = info.family_properties.at(i).queueFlags;
            queue_create_info.queueFamilyIndex = i;
            queue_create_info.queueCount = 1;
            queue_create_info.pQueuePriorities = &priority;
            info.create_infos.push_back(queue_create_info);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   public:
    std::shared_ptr<IIr77Return const> GetQueueInfo(std::uint32_t const& index, Ir77PVQueueInfo& queue_info) {
        queue_info = m_queues.at(index);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    std::uint32_t GetCurrentDevice();

    std::vector<Ir77PVDeviceInfo> GetDeviceInfos();

    std::vector<Ir77PVSwapchainInfo> GetSwapchainInfos();

   private:
    std::shared_ptr<IIr77Enlisted> m_context;

    std::vector<Ir77PVQueueInfo> m_queues;
};
}  // namespace NSIr77PeregrineV