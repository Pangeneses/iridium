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

#include "../interface/IIr77PVLayoutCPT.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// =============================================================================================================================================
// Ir77PVLayoutCPT -- descriptor set layouts, descriptor pool and pipeline layout for one Ir77PVComputeKind, plus its workgroup size.
// Graphics bindings (camera / material / CEF) live in Ir77PVLayout.
//
//   configure: SetDevice / SetKind
//   create:    CreateLayout -> DefineBindings -> DefineDescriptorSetLayout -> DefineDescriptorPool -> DefinePipelineLayout
// =============================================================================================================================================
class Ir77PVLayoutCPT : public Ir77Enlisted, public IIr77PVLayoutCPT, public std::enable_shared_from_this<Ir77PVLayoutCPT> {
   public:
    Ir77PVLayoutCPT() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument const&) {
            throw;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    // the owner waits for the device to go idle before releasing layouts
    ~Ir77PVLayoutCPT() {
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
        seat_shared_uuid<&GUIDIr77PVLayoutCPT>(uid);

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

        else if (iid == GUIDIIr77PVLayoutCPT)
            obj = std::shared_ptr<IIr77PVLayoutCPT>(shared_from_this(), static_cast<IIr77PVLayoutCPT*>(this));

        else if (iid == GUIDIr77PVLayoutCPT)
            obj = std::shared_ptr<Ir77PVLayoutCPT>(shared_from_this(), static_cast<Ir77PVLayoutCPT*>(this));

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

    std::shared_ptr<IIr77Return const> SetKind(Ir77PVComputeKind const& kind) {
        m_kind = kind;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // create
    // -------------------------------------------------------------------------------------------------------------------------------------
    // pool_capacity = how many of each set the pool can hand out; 0 = no pool (sets come from elsewhere). Replaces any previous layout.
    std::shared_ptr<IIr77Return const> CreateLayout(std::uint32_t const& pool_capacity) {
        if (!m_device) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutCPT: SetDevice not called.");

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

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Compute Layout.");
    }

    std::shared_ptr<IIr77Return const> AllocateSet(std::uint32_t const& set_index, VkDescriptorSet* set) {
        if (m_descriptor_pool == VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutCPT: layout has no descriptor pool.");
        if (set_index >= m_descriptor_layouts.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutCPT: set index out of range.");

        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = m_descriptor_pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &m_descriptor_layouts[set_index];

        if (vkAllocateDescriptorSets(device, &alloc_info, set) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutCPT: descriptor set allocation failed -- raise the pool capacity.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // queries
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> GetKind(Ir77PVComputeKind* kind) {
        *kind = m_kind;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetSetCount(std::uint32_t* count) {
        *count = static_cast<std::uint32_t>(m_descriptor_layouts.size());

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetDescriptorSetLayout(std::uint32_t const& set_index, VkDescriptorSetLayout* layout) {
        if (set_index >= m_descriptor_layouts.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutCPT: set index out of range.");

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

    // Must match the shader's local_size_x/y/z; dispatch with groupCount = ceil(items / local_size)
    std::shared_ptr<IIr77Return const> GetLocalSize(std::uint32_t* x, std::uint32_t* y, std::uint32_t* z) {
        *x = m_local_size_x;
        *y = m_local_size_y;
        *z = m_local_size_z;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetDebugName(std::string* name) {
        *name = m_debug_name;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // steps
    // -------------------------------------------------------------------------------------------------------------------------------------
   private:
    // Sets, push constant, workgroup size and debug name for the kind
    std::shared_ptr<IIr77Return const> DefineBindings() {
        m_sets.clear();
        m_push = VkPushConstantRange{};
        m_push_count = 0;
        m_local_size_x = 1;
        m_local_size_y = 1;
        m_local_size_z = 1;
        m_debug_name.clear();

        switch (m_kind) {
            case Ir77PVComputeKind::Culling:
                m_debug_name = "Culling";
                m_sets = {BindingsCulling()};
                // frustum data travels as a push constant (changes every frame); binding 1 is auxiliary per-object culling data
                m_push = VkPushConstantRange{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float) * 16};
                m_push_count = 1;
                m_local_size_x = 64;
                break;

            case Ir77PVComputeKind::SkinningUpdate:
                m_debug_name = "SkinningUpdate";
                m_sets = {BindingsSkinningUpdate()};
                m_push = VkPushConstantRange{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(std::uint32_t)};
                m_push_count = 1;
                m_local_size_x = 128;
                break;

            case Ir77PVComputeKind::MorphUpdate:
                m_debug_name = "MorphUpdate";
                m_sets = {BindingsMorphUpdate()};
                m_local_size_x = 64;
                break;

            case Ir77PVComputeKind::ClothSim:
                m_debug_name = "ClothSim";
                m_sets = {BindingsClothSim()};
                m_push = VkPushConstantRange{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float) * 4};
                m_push_count = 1;
                m_local_size_x = 32;
                break;

            default:
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutCPT: unknown compute kind.");
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
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutCPT: vkCreateDescriptorSetLayout failed at set " + std::to_string(i) + ".");
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
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutCPT: vkCreateDescriptorPool failed.");

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
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutCPT: vkCreatePipelineLayout failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // bindings -- one set per kind, all compute-stage storage buffers
    // -------------------------------------------------------------------------------------------------------------------------------------
   private:
    VkDescriptorSetLayoutBinding Bind(std::uint32_t const binding, VkDescriptorType const type) const {
        return VkDescriptorSetLayoutBinding{binding, type, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};
    }

    std::vector<VkDescriptorSetLayoutBinding> BindingsCulling() const {
        return {
            Bind(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),  // AABBs
            Bind(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),  // per-object culling data
            Bind(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),  // output visibility / indirect commands
        };
    }

    std::vector<VkDescriptorSetLayoutBinding> BindingsSkinningUpdate() const {
        return {
            Bind(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),  // bone matrices
            Bind(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),  // weights
            Bind(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),  // output vertices
        };
    }

    std::vector<VkDescriptorSetLayoutBinding> BindingsMorphUpdate() const {
        return {
            Bind(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),  // base mesh
            Bind(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),  // morph targets
            Bind(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),  // output mesh
        };
    }

    std::vector<VkDescriptorSetLayoutBinding> BindingsClothSim() const {
        return {
            Bind(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),  // particles
            Bind(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),  // constraints
            Bind(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),  // output
        };
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
    std::shared_ptr<IIr77PVDevice> m_device{};

    Ir77PVComputeKind m_kind{Ir77PVComputeKind::Culling};

    std::string m_debug_name{};

    std::uint32_t m_local_size_x{1};

    std::uint32_t m_local_size_y{1};

    std::uint32_t m_local_size_z{1};

    // [set][binding]
    std::vector<std::vector<VkDescriptorSetLayoutBinding>> m_sets{};

    VkPushConstantRange m_push{};

    std::uint32_t m_push_count{0};

    std::vector<VkDescriptorSetLayout> m_descriptor_layouts{};

    VkDescriptorPool m_descriptor_pool{VK_NULL_HANDLE};

    VkPipelineLayout m_pipeline_layout{VK_NULL_HANDLE};
};

}  // namespace NSIr77PeregrineV