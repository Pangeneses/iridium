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

#include "../interface/IIr77PVAllocation.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77PVAllocation : public Ir77Enlisted, public IIr77PVAllocation, public std::enable_shared_from_this<Ir77PVAllocation> {
   public:
    Ir77PVAllocation() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }
        m_enlisted = std::chrono::system_clock::now();
    }

   public:
    std::shared_ptr<IIr77Return const> MemberOfUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVAllocation>(uid);

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

        else if (iid == &GUIDIr77PVAllocation)
            obj = std::shared_ptr<Ir77PVAllocation>(shared_from_this(), static_cast<Ir77PVAllocation*>(this));

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
        VkMemoryPropertyFlags properties) {

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
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAllocation: no suitable memory type found.");

        VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = requirements.size;
        alloc_info.memoryTypeIndex = type_index;

        if (vkAllocateMemory(device, &alloc_info, nullptr, &m_memory) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAllocation: vkAllocateMemory failed.");

        m_size = requirements.size;
        m_memory_type_index = type_index;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> BindBuffer(VkBuffer buffer) {
        VkDevice device = GetDevice();

        if (vkBindBufferMemory(device, buffer, m_memory, m_offset) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAllocation: vkBindBufferMemory failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> BindImage(VkImage image) {
        VkDevice device = GetDevice();

        if (vkBindImageMemory(device, image, m_memory, m_offset) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAllocation: vkBindImageMemory failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> Map(void** ptr) {
        VkDevice device = GetDevice();

        if (vkMapMemory(device, m_memory, m_offset, m_size, 0, ptr) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAllocation: vkMapMemory failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> Unmap() {
        VkDevice device = GetDevice();

        vkUnmapMemory(device, m_memory);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> Free() {
        VkDevice device = GetDevice();

        vkFreeMemory(device, m_memory, nullptr);

        m_memory = VK_NULL_HANDLE;
        m_size = 0;
        m_offset = 0;
        m_memory_type_index = UINT32_MAX;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetMemory(VkDeviceMemory* memory) {
        *memory = m_memory;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetOffset(VkDeviceSize* offset) {
        *offset = m_offset;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetSize(VkDeviceSize* size) {
        *size = m_size;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetMemoryTypeIndex(uint32_t* index) {
        *index = m_memory_type_index;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    VkPhysicalDevice GetPhysicalDevice();

    VkDevice GetDevice();
    
   private:
    std::shared_ptr<IIr77Enlisted> m_context;

    VkDeviceMemory m_memory{VK_NULL_HANDLE};

    VkDeviceSize m_size{0};

    VkDeviceSize m_offset{0};

    uint32_t m_memory_type_index{UINT32_MAX};
};

}  // namespace NSIr77PeregrineV