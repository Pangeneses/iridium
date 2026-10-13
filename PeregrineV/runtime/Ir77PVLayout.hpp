#pragma once

#include <vulkan/vulkan.h>

#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
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

// =============================================================================================================================================
// Ir77PVLayout -- descriptor set layouts, descriptor pool and pipeline layout for one graphics Ir77PVLayoutKind.
//
//   configure: SetDevice / SetMeshShading / SetKind (any order)
//   create:    CreateLayout -> DefineBindings -> DefineDescriptorSetLayout -> DefineDescriptorPool -> DefinePipelineLayout
//
// Sets 0-2 (global / pass / material) are identical across kinds so their descriptor sets can be shared between pipelines.
// =============================================================================================================================================
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

    // the owner waits for the device to go idle before releasing layouts
    ~Ir77PVLayout() {
        if (!m_device) return;

        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        Destroy(device);
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

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77PVLayout)
            obj = std::shared_ptr<IIr77PVLayout>(shared_from_this(), static_cast<IIr77PVLayout*>(this));

        else if (iid == GUIDIr77PVLayout)
            obj = std::shared_ptr<Ir77PVLayout>(shared_from_this(), static_cast<Ir77PVLayout*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // configuration
    // -------------------------------------------------------------------------------------------------------------------------------------
   public:
    std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) {
        m_device = device;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetKind(Ir77PVLayoutKind const& kind) {
        m_kind = kind;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Adds task + mesh stages to the geometry bindings of sets 0 and 1
    std::shared_ptr<IIr77Return const> SetMeshShading(bool const& enabled) {
        m_geometry_stages = enabled ? (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT) : VK_SHADER_STAGE_VERTEX_BIT;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // create
    // -------------------------------------------------------------------------------------------------------------------------------------
    // pool_capacity = how many of each set the pool can hand out; 0 = no pool (sets come from elsewhere). Replaces any previous layout.
    std::shared_ptr<IIr77Return const> CreateLayout(std::uint32_t const& pool_capacity) {
        if (!m_device) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayout: SetDevice not called.");

        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        Destroy(device);

        auto result = DefineBindings();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        result = DefineDescriptorSetLayout();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        if (pool_capacity > 0) {
            result = DefineDescriptorPool(pool_capacity);
            if (result->ID() != GUIDIr77OperationSucceeded) return result;
        }

        result = DefinePipelineLayout();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Layout.");
    }

    std::shared_ptr<IIr77Return const> AllocateSet(std::uint32_t const& set_index, VkDescriptorSet* set) {
        if (m_descriptor_pool == VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayout: layout has no descriptor pool.");
        if (set_index >= m_descriptor_layouts.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayout: set index out of range.");

        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = m_descriptor_pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &m_descriptor_layouts[set_index];

        if (vkAllocateDescriptorSets(device, &alloc_info, set) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayout: descriptor set allocation failed -- raise the pool capacity.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // queries
    // -------------------------------------------------------------------------------------------------------------------------------------
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

    // -------------------------------------------------------------------------------------------------------------------------------------
    // steps
    // -------------------------------------------------------------------------------------------------------------------------------------
   private:
    // Sets and push constant for the kind -- built here, not in SetKind, so SetMeshShading can come in any order
    std::shared_ptr<IIr77Return const> DefineBindings() {
        m_sets.clear();
        m_push = VkPushConstantRange{};
        m_push_count = 0;

        switch (m_kind) {
            case Ir77PVLayoutKind::Static:
                m_sets = {BindingsGlobal(), BindingsPass(), BindingsMaterial()};
                m_push = VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT, 0, SIZE_MAT4};
                m_push_count = 1;
                break;

            case Ir77PVLayoutKind::Skinned:
                m_sets = {BindingsGlobal(), BindingsPass(), BindingsMaterial(), BindingsBones()};
                m_push = VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT, 0, SIZE_MAT4};
                m_push_count = 1;
                break;

            case Ir77PVLayoutKind::Shadow:
                m_sets = {BindingsGlobal()};
                m_push = VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT, 0, SIZE_MAT4 + sizeof(std::uint32_t)};
                m_push_count = 1;
                break;

            case Ir77PVLayoutKind::ShadowSkinned:
                m_sets = {BindingsGlobal(), BindingsBones()};
                m_push = VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT, 0, SIZE_MAT4 + sizeof(std::uint32_t)};
                m_push_count = 1;
                break;

            // task / mesh / fragment -- geometry comes from set 3 storage buffers, not vertex input
            case Ir77PVLayoutKind::Mesh:
                m_sets = {BindingsGlobal(), BindingsPass(), BindingsMaterial(), BindingsMeshlet()};
                m_push = VkPushConstantRange{VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT, 0, sizeof(std::uint32_t)};
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
        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        m_descriptor_layouts.assign(m_sets.size(), VK_NULL_HANDLE);

        for (std::size_t i = 0; i < m_sets.size(); i++) {
            VkDescriptorSetLayoutCreateInfo layout_info{};
            layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layout_info.bindingCount = static_cast<std::uint32_t>(m_sets[i].size());
            layout_info.pBindings = m_sets[i].data();

            if (vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &m_descriptor_layouts[i]) != VK_SUCCESS)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayout: vkCreateDescriptorSetLayout failed at set " + std::to_string(i) + ".");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Every binding of every set, times the number of each set the pool must hold
    std::shared_ptr<IIr77Return const> DefineDescriptorPool(std::uint32_t const& pool_capacity) {
        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        std::map<VkDescriptorType, std::uint32_t> counts{};
        for (auto const& set : m_sets)
            for (auto const& binding : set) counts[binding.descriptorType] += binding.descriptorCount * pool_capacity;

        std::vector<VkDescriptorPoolSize> pool_sizes{};
        pool_sizes.reserve(counts.size());
        for (auto const& [type, count] : counts) pool_sizes.push_back(VkDescriptorPoolSize{type, count});

        VkDescriptorPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.poolSizeCount = static_cast<std::uint32_t>(pool_sizes.size());
        pool_info.pPoolSizes = pool_sizes.data();
        pool_info.maxSets = pool_capacity * static_cast<std::uint32_t>(m_sets.size());

        if (vkCreateDescriptorPool(device, &pool_info, nullptr, &m_descriptor_pool) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayout: vkCreateDescriptorPool failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefinePipelineLayout() {
        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        VkPipelineLayoutCreateInfo pipeline_layout_info{};
        pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_info.setLayoutCount = static_cast<std::uint32_t>(m_descriptor_layouts.size());
        pipeline_layout_info.pSetLayouts = m_descriptor_layouts.data();
        pipeline_layout_info.pushConstantRangeCount = m_push_count;
        pipeline_layout_info.pPushConstantRanges = m_push_count > 0 ? &m_push : nullptr;

        if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &m_pipeline_layout) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayout: vkCreatePipelineLayout failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // bindings
    // -------------------------------------------------------------------------------------------------------------------------------------
   private:
    VkDescriptorSetLayoutBinding Bind(std::uint32_t const binding, VkDescriptorType const type, VkShaderStageFlags const stages) const {
        return VkDescriptorSetLayoutBinding{binding, type, 1, stages, nullptr};
    }

    // Set 0 -- camera, lights, instances, env, bones. Shared by every kind, so the flags must be identical across them.
    std::vector<VkDescriptorSetLayoutBinding> BindingsGlobal() const {
        return {
            Bind(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_geometry_stages | VK_SHADER_STAGE_FRAGMENT_BIT),  // camera
            Bind(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT),                      // lights
            Bind(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_geometry_stages),                                 // instances
            Bind(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),              // env / IBL
            Bind(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_geometry_stages),                                 // bones: bases[8192] + palettes
        };
    }

    // Set 1 -- shadow map, shadow matrices
    std::vector<VkDescriptorSetLayoutBinding> BindingsPass() const {
        return {
            Bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),              // shadow map
            Bind(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_geometry_stages | VK_SHADER_STAGE_FRAGMENT_BIT),  // shadow matrices
        };
    }

    // Set 2 -- material constants + albedo, normal, ORM, emissive, AO
    std::vector<VkDescriptorSetLayoutBinding> BindingsMaterial() const {
        std::vector<VkDescriptorSetLayoutBinding> bindings{Bind(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)};

        for (std::uint32_t i = 1; i <= MATERIAL_TEXTURES; i++) bindings.push_back(Bind(i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT));

        return bindings;
    }

    // Set 3 -- skinned objects
    std::vector<VkDescriptorSetLayoutBinding> BindingsBones() const {
        return {Bind(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)};  // bone matrices
    }

    // Set 3 for Mesh -- meshoptimizer layout. Task reads meshlets for culling; mesh reads everything.
    std::vector<VkDescriptorSetLayoutBinding> BindingsMeshlet() const {
        VkShaderStageFlags const task_mesh{VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT};

        return {
            Bind(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, task_mesh),                     // meshlets (offsets, counts, bounds)
            Bind(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_MESH_BIT_EXT),  // meshlet vertex indices
            Bind(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_MESH_BIT_EXT),  // meshlet triangles (uint8 x3)
            Bind(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_MESH_BIT_EXT),  // vertices
            Bind(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, task_mesh),                     // TODO: label -- what the shaders read here
        };
    }

    // Set 0 -- CEF composite
    std::vector<VkDescriptorSetLayoutBinding> BindingsCEF() const {
        return {Bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)};  // browser texture
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // internals
    // -------------------------------------------------------------------------------------------------------------------------------------
   private:
    void Destroy(VkDevice device) {
        if (m_pipeline_layout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, m_pipeline_layout, nullptr);
        if (m_descriptor_pool != VK_NULL_HANDLE) vkDestroyDescriptorPool(device, m_descriptor_pool, nullptr);

        for (auto const layout : m_descriptor_layouts)
            if (layout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, layout, nullptr);

        m_pipeline_layout = VK_NULL_HANDLE;
        m_descriptor_pool = VK_NULL_HANDLE;
        m_descriptor_layouts.clear();
    }

   private:
    static constexpr std::uint32_t SIZE_MAT4 = sizeof(float) * 16;

    static constexpr std::uint32_t MATERIAL_TEXTURES = 5;

    std::shared_ptr<IIr77PVDevice> m_device{};

    Ir77PVLayoutKind m_kind{Ir77PVLayoutKind::Static};

    VkShaderStageFlags m_geometry_stages{VK_SHADER_STAGE_VERTEX_BIT};

    // [set][binding]
    std::vector<std::vector<VkDescriptorSetLayoutBinding>> m_sets{};

    VkPushConstantRange m_push{};

    std::uint32_t m_push_count{0};

    std::vector<VkDescriptorSetLayout> m_descriptor_layouts{};

    VkDescriptorPool m_descriptor_pool{VK_NULL_HANDLE};

    VkPipelineLayout m_pipeline_layout{VK_NULL_HANDLE};
};

}  // namespace NSIr77PeregrineV