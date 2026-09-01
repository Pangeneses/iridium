#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <memory>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../../dictionary/IDIIr77PeregrineV.hpp"
#include "../../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../../interface/IIr77PVLayout.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77PVLayoutCEF : public Ir77Enlisted, public IIr77PVLayout, public std::enable_shared_from_this<Ir77PVLayoutCEF> {
   public:
    Ir77PVLayoutCEF() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    ~Ir77PVLayoutCEF() {
        VkDevice device;
        m_device->GetDevice(&device);

        vkDeviceWaitIdle(device);

        vkDestroyPipelineLayout(device, m_pipeline_layout, nullptr);
        vkDestroyDescriptorPool(device, m_descriptor_pool, nullptr);
        vkDestroyDescriptorSetLayout(device, m_descriptor_layout, nullptr);
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVLayoutCEF>(uid);

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

        else if (iid == &GUIDIIr77PVLayout)
            obj = std::shared_ptr<IIr77PVLayout>(shared_from_this(), static_cast<IIr77PVLayout*>(this));

        else if (iid == &GUIDIr77PVLayoutCEF)
            obj = std::shared_ptr<Ir77PVLayoutCEF>(shared_from_this(), static_cast<Ir77PVLayoutCEF*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) {
        m_device = device;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineDescriptorSetLayout() {
        VkDevice device;
        m_device->GetDevice(&device);

        VkDescriptorSetLayoutBinding binding{};
        binding.binding = 0;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        binding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = 1;
        layout_info.pBindings = &binding;

        if (vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &m_descriptor_layout) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutCEF: CEF descriptor set layout failed.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineDescriptorPool(std::uint32_t const& max_count) {
        VkDevice device;
        m_device->GetDevice(&device);

        VkDescriptorPoolSize pool_size{};
        pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pool_size.descriptorCount = max_count;

        VkDescriptorPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.poolSizeCount = 1;
        pool_info.pPoolSizes = &pool_size;
        pool_info.maxSets = max_count;

        if (vkCreateDescriptorPool(device, &pool_info, nullptr, &m_descriptor_pool) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutCEF: CEF descriptor pool failed.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefinePipelineLayout() {
        if (m_descriptor_layout == VK_NULL_HANDLE) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutCEF: descriptor set layout must be defined before pipeline layout.");
        }

        VkDevice device;
        m_device->GetDevice(&device);

        m_pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        m_pipeline_layout_info.setLayoutCount = 1;
        m_pipeline_layout_info.pSetLayouts = &m_descriptor_layout;
        m_pipeline_layout_info.pushConstantRangeCount = 0;
        m_pipeline_layout_info.pPushConstantRanges = nullptr;

        if (vkCreatePipelineLayout(device, &m_pipeline_layout_info, nullptr, &m_pipeline_layout) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutCEF: VkPipelineLayout failed.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> AllocateSet(VkDescriptorSet* set) {
        if (m_descriptor_pool == VK_NULL_HANDLE || m_descriptor_layout == VK_NULL_HANDLE) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutCEF: CEF descriptor pool/layout not initialized before AllocateSet.");
        }

        VkDevice device;
        m_device->GetDevice(&device);

        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = m_descriptor_pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &m_descriptor_layout;

        if (vkAllocateDescriptorSets(device, &alloc_info, set) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutCEF: CEF descriptor set allocation failed.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetDescriptorSetLayout(VkDescriptorSetLayout* layout) {
        *layout = m_descriptor_layout;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetDescriptorPool(VkDescriptorPool* pool) {
        *pool = m_descriptor_pool;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetPipelineLayout(VkPipelineLayout* pipeline_layout) {
        *pipeline_layout = m_pipeline_layout;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    std::shared_ptr<IIr77PVDevice> m_device;

    VkDescriptorSetLayout m_descriptor_layout{VK_NULL_HANDLE};

    VkDescriptorPool m_descriptor_pool{VK_NULL_HANDLE};

    VkPipelineLayoutCreateInfo m_pipeline_layout_info{};

    VkPipelineLayout m_pipeline_layout{VK_NULL_HANDLE};
};
}  // namespace NSIr77PeregrineV