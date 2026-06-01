#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <cstring>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77PVBuffer.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77Buffer : public Ir77Enlisted, public IIr77PVBuffer, public std::enable_shared_from_this<Ir77Buffer> {
   public:
    Ir77Buffer() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

   public:
    std::shared_ptr<IIr77Return const> MemberOfUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVBuffer>(uid);

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

        else if (iid == &GUIDIr77PVBuffer)
            obj = std::shared_ptr<Ir77Buffer>(shared_from_this(), static_cast<Ir77Buffer*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context) {
        m_context = context;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags mem_props) {
        std::uint64_t m_staging_size = m_swapchain_extent.width * m_swapchain_extent.height * 4;

        m_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        m_buffer_info.size = m_staging_size;
        m_buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        m_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(m_device, &m_buffer_info, nullptr, &m_staging_buf) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: vkCreateBuffer (staging) failed.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemoryRequirements() {
        vkGetBufferMemoryRequirements(m_device, m_staging_buf, &m_memory_requirements);

        m_memory_allocation_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        m_memory_allocation_info.allocationSize = m_memory_requirements.size;
        m_memory_allocation_info.memoryTypeIndex = UINT32_MAX;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> AllocateMemory() {
        auto props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        uint32_t type_filter = m_memory_requirements.memoryTypeBits;

        vkGetPhysicalDeviceMemoryProperties(m_physical_device, &m_phys_device_mem_props);
        for (uint32_t i = 0; i < m_phys_device_mem_props.memoryTypeCount; ++i) {
            if ((type_filter & (1 << i)) && (m_phys_device_mem_props.memoryTypes[i].propertyFlags & props) == props) {
                m_memory_allocation_info.memoryTypeIndex = i;
                break;  // inside the if
            }
        }

        if (m_memory_allocation_info.memoryTypeIndex == UINT32_MAX) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: no suitable staging memory type.");
        }

        if (vkAllocateMemory(m_device, &m_memory_allocation_info, nullptr, &m_staging_mem) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: vkAllocateMemory (staging) failed.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> BindBufferMemory() {
        vkBindBufferMemory(m_device, m_staging_buf, m_staging_mem, 0);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MapMemory() {
        vkMapMemory(m_device, m_staging_mem, 0, m_staging_size, 0, &m_staging_ptr);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }
    
   private:
    std::shared_ptr<IIr77Enlisted> m_context;

    VkBuffer m_buffer;

    VkBufferCreateInfo m_buffer_info;

    VkMemoryRequirements m_memory_requirements;

    VkMemoryAllocateInfo m_memory_allocation_info;

    VkPhysicalDeviceMemoryProperties m_phys_device_mem_props;

    VkDeviceSize m_device_size;

    void* m_staging_ptr{nullptr};
};
}  // namespace NSIr77PeregrineV