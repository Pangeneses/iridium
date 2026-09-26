#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77PVLayout.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77PVLayout : public Ir77Enlisted, public IIr77PVLayout, public std::enable_shared_from_this<Ir77PVLayout> {
   public:
    Ir77PVLayout() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    ~Ir77PVLayout() {
        if (!m_device) return;

        VkDevice device;
        m_device->GetDevice(&device);

        vkDeviceWaitIdle(device);

        vkDestroyPipelineLayout(device, m_pipeline_layout, nullptr);
        vkDestroyDescriptorPool(device, m_descriptor_pool, nullptr);

        for (auto& layout : m_descriptor_layouts) vkDestroyDescriptorSetLayout(device, layout, nullptr);
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVLayout>(uid);

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

        else if (iid == &GUIDIr77PVLayout)
            obj = std::shared_ptr<Ir77PVLayout>(shared_from_this(), static_cast<Ir77PVLayout*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) {
        m_device = device;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetKind(Ir77PVLayoutKind const& kind) {
        m_kind = kind;
        m_sets.clear();
        m_push = VkPushConstantRange{};
        m_push_count = 0;

        switch (m_kind) {
            case Ir77PVLayoutKind::Static:
                m_sets = {BindingsGlobal(), BindingsPass(), BindingsMaterial()};
                m_push = VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT, 0, SizeMat4};
                m_push_count = 1;
                break;

            case Ir77PVLayoutKind::Skinned:
                m_sets = {BindingsGlobal(), BindingsPass(), BindingsMaterial(), BindingsBones()};
                m_push = VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT, 0, SizeMat4};
                m_push_count = 1;
                break;

            case Ir77PVLayoutKind::Shadow:
                m_sets = {BindingsGlobal()};
                m_push = VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT, 0, SizeMat4 + sizeof(std::uint32_t)};
                m_push_count = 1;
                break;

            case Ir77PVLayoutKind::ShadowSkinned:
                m_sets = {BindingsGlobal(), BindingsBones()};
                m_push = VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT, 0, SizeMat4 + sizeof(std::uint32_t)};
                m_push_count = 1;
                break;

            case Ir77PVLayoutKind::CEF:
                m_sets = {BindingsCEF()};
                break;

            default:
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayout: unknown layout kind.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineDescriptorSetLayout() {
        VkDevice device;
        m_device->GetDevice(&device);

        m_descriptor_layouts.assign(m_sets.size(), VK_NULL_HANDLE);

        for (std::size_t i = 0; i < m_sets.size(); ++i) {
            VkDescriptorSetLayoutCreateInfo layout_info{};
            layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layout_info.bindingCount = static_cast<std::uint32_t>(m_sets[i].size());
            layout_info.pBindings = m_sets[i].data();

            if (vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &m_descriptor_layouts[i]) != VK_SUCCESS) {
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayout: descriptor set layout failed.");
            }
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineDescriptorPool(std::uint32_t const& max_count) {
        VkDevice device;
        m_device->GetDevice(&device);

        std::map<VkDescriptorType, std::uint32_t> counts{};
        for (auto const& set : m_sets)
            for (auto const& binding : set) counts[binding.descriptorType] += binding.descriptorCount * max_count;

        std::vector<VkDescriptorPoolSize> pool_sizes{};
        for (auto const& [type, count] : counts) pool_sizes.push_back(VkDescriptorPoolSize{type, count});

        VkDescriptorPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.poolSizeCount = static_cast<std::uint32_t>(pool_sizes.size());
        pool_info.pPoolSizes = pool_sizes.data();
        pool_info.maxSets = max_count * static_cast<std::uint32_t>(m_sets.size());

        if (vkCreateDescriptorPool(device, &pool_info, nullptr, &m_descriptor_pool) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayout: descriptor pool failed.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefinePipelineLayout() {
        if (m_descriptor_layouts.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayout: DefineDescriptorSetLayout not called.");

        VkDevice device;
        m_device->GetDevice(&device);

        VkPipelineLayoutCreateInfo pipeline_layout_info{};
        pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_info.setLayoutCount = static_cast<std::uint32_t>(m_descriptor_layouts.size());
        pipeline_layout_info.pSetLayouts = m_descriptor_layouts.data();
        pipeline_layout_info.pushConstantRangeCount = m_push_count;
        pipeline_layout_info.pPushConstantRanges = m_push_count ? &m_push : nullptr;

        if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &m_pipeline_layout) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayout: VkPipelineLayout failed.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> AllocateSet(std::uint32_t const& set_index, VkDescriptorSet* set) {
        if (set_index >= m_descriptor_layouts.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayout: set index out of range.");

        VkDevice device;
        m_device->GetDevice(&device);

        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = m_descriptor_pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &m_descriptor_layouts[set_index];

        if (vkAllocateDescriptorSets(device, &alloc_info, set) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayout: descriptor set allocation failed.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetKind(Ir77PVLayoutKind* kind) {
        *kind = m_kind;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetSetCount(std::uint32_t* count) {
        *count = static_cast<std::uint32_t>(m_descriptor_layouts.size());

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetDescriptorSetLayout(std::uint32_t const& set_index, VkDescriptorSetLayout* layout) {
        if (set_index >= m_descriptor_layouts.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayout: set index out of range.");

        *layout = m_descriptor_layouts[set_index];

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
    static constexpr std::uint32_t SizeMat4 = sizeof(float) * 16;

    static VkDescriptorSetLayoutBinding Bind(std::uint32_t binding, VkDescriptorType type, VkShaderStageFlags stages) {
        return VkDescriptorSetLayoutBinding{binding, type, 1, stages, nullptr};
    }

    // Set 0 -- per-frame, shared by Static/Skinned/Shadow (identical definition keeps them compatible)
    static std::vector<VkDescriptorSetLayoutBinding> BindingsGlobal() {
        return {Bind(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),  // camera
                Bind(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT),                               // lights
                Bind(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),                                 // instances
                Bind(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)};                      // env / IBL
    }

    // Set 1 -- per-pass
    static std::vector<VkDescriptorSetLayoutBinding> BindingsPass() {
        return {Bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),  // shadow map
                Bind(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)};           // shadow matrices
    }

    // Set 2 -- per-material
    static std::vector<VkDescriptorSetLayoutBinding> BindingsMaterial() {
        std::vector<VkDescriptorSetLayoutBinding> bindings{Bind(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)};  // material constants

        for (std::uint32_t i = 1; i <= 5; ++i)  // albedo, normal, ORM, emissive, AO
            bindings.push_back(Bind(i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT));

        return bindings;
    }

    // Set 3 -- skinned objects
    static std::vector<VkDescriptorSetLayoutBinding> BindingsBones() {
        return {Bind(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)};  // bone matrices
    }

    // Set 0 -- CEF composite
    static std::vector<VkDescriptorSetLayoutBinding> BindingsCEF() {
        return {Bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)};  // browser texture
    }

   private:
    std::shared_ptr<IIr77PVDevice> m_device;

    Ir77PVLayoutKind m_kind{Ir77PVLayoutKind::Static};

    std::vector<std::vector<VkDescriptorSetLayoutBinding>> m_sets{};

    std::vector<VkDescriptorSetLayout> m_descriptor_layouts{};

    VkDescriptorPool m_descriptor_pool{VK_NULL_HANDLE};

    VkPushConstantRange m_push{};

    std::uint32_t m_push_count{0};

    VkPipelineLayout m_pipeline_layout{VK_NULL_HANDLE};
};
}  // namespace NSIr77PeregrineV