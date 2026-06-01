#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <chrono>
#include <memory>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"

#include "../interface/IIr77PVMemory.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77PVMemory : public Ir77Enlisted, public IIr77PVMemory, public std::enable_shared_from_this<Ir77PVMemory> {
   public:
    Ir77PVMemory() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }
        m_enlisted = std::chrono::system_clock::now();
    }

   public:
    std::shared_ptr<IIr77Return const> MemberOfUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVMemory>(uid);

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

        else if (iid == &GUIDIr77PVMemory)
            obj = std::shared_ptr<Ir77PVMemory>(shared_from_this(), static_cast<Ir77PVMemory*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context) {
        m_context = context;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> Allocate(
        VkMemoryRequirements const& requirements,
        VkMemoryPropertyFlags properties,
        VkDeviceMemory* memory) {

        VkPhysicalDevice phys_device = GetPhysicalDevice();
        VkDevice device = GetDevice();

        VkPhysicalDeviceMemoryProperties mem_props{};
        vkGetPhysicalDeviceMemoryProperties(phys_device, &mem_props);

        uint32_t type_index = UINT32_MAX;
        for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i) {
            if ((requirements.memoryTypeBits & (1 << i)) &&
                (mem_props.memoryTypes[i].propertyFlags & properties) == properties) {
                type_index = i;
                break;
            }
        }

        if (type_index == UINT32_MAX)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVMemory: no suitable memory type found.");

        VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = requirements.size;
        alloc_info.memoryTypeIndex = type_index;

        if (vkAllocateMemory(device, &alloc_info, nullptr, memory) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVMemory: vkAllocateMemory failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> Free(VkDeviceMemory memory) {
        VkDevice device = GetDevice();

        vkFreeMemory(device, memory, nullptr);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetPhysicalDeviceMemoryProperties(
        VkPhysicalDeviceMemoryProperties* props) {

        VkPhysicalDevice phys_device = GetPhysicalDevice();
        vkGetPhysicalDeviceMemoryProperties(phys_device, props);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    VkPhysicalDevice GetPhysicalDevice();

    VkDevice GetDevice();
    
   private:
    std::shared_ptr<IIr77Enlisted> m_context;
};

}  // namespace NSIr77PeregrineV