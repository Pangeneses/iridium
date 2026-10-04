#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <map>
#include <memory>
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

    ~Ir77PVLayoutCPT() {
        if (!m_device) return;

        VkDevice device;
        m_device->GetDevice(&device);

        vkDeviceWaitIdle(device);

        if (m_pipeline_layout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, m_pipeline_layout, nullptr);
        }

        if (m_descriptor_pool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, m_descriptor_pool, nullptr);
        }

        for (auto& layout : m_descriptor_layouts) {
            if (layout != VK_NULL_HANDLE) {
                vkDestroyDescriptorSetLayout(device, layout, nullptr);
            }
        }
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
            obj = std::shared_ptr<IIr77PVLayoutCPT>(shared_from_this(), static_cast<Ir77PVLayoutCPT*>(this));

        else if (iid == GUIDIr77PVLayoutCPT)
            obj = std::shared_ptr<Ir77PVLayoutCPT>(shared_from_this(), static_cast<Ir77PVLayoutCPT*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) {
        m_device = device;
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // ---------------------------------------------------------------------
    // Per-kind configuration: descriptor sets, push constants, workgroup size
    // ---------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> SetKind(Ir77PVComputeKind const& kind) {
        m_kind = kind;

        m_sets.clear();
        m_push = VkPushConstantRange{};
        m_push_count = 0;
        m_debug_name.clear();
        m_local_size_x = 1;
        m_local_size_y = 1;
        m_local_size_z = 1;

        switch (m_kind) {
            case Ir77PVComputeKind::Culling: {
                m_debug_name = "Culling";

                m_sets = BindingsCulling();

                // Frustum planes travel as a push constant (cheap, changes every frame);
                // binding 1 in BindingsCulling is for any auxiliary per-object culling data
                // that doesn't fit a push constant, not a duplicate of this.
                m_push = VkPushConstantRange{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float) * 16};
                m_push_count = 1;

                m_local_size_x = 64;
                break;
            }

            case Ir77PVComputeKind::SkinningUpdate: {
                m_debug_name = "SkinningUpdate";

                m_sets = BindingsSkinningUpdate();

                m_push = VkPushConstantRange{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(std::uint32_t)};
                m_push_count = 1;

                m_local_size_x = 128;
                break;
            }

            case Ir77PVComputeKind::MorphUpdate: {
                m_debug_name = "MorphUpdate";

                m_sets = BindingsMorphUpdate();

                m_push_count = 0;

                m_local_size_x = 64;
                break;
            }

            case Ir77PVComputeKind::ClothSim: {
                m_debug_name = "ClothSim";

                m_sets = BindingsClothSim();

                m_push = VkPushConstantRange{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float) * 4};
                m_push_count = 1;

                m_local_size_x = 32;
                break;
            }

            default:
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutCPT: unknown compute kind.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // ---------------------------------------------------------------------
    // Descriptor set layouts
    // ---------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> DefineDescriptorSetLayout() {
        if (m_sets.empty()) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutCPT: compute kind has no descriptor sets.");
        }

        VkDevice device;
        m_device->GetDevice(&device);

        m_descriptor_layouts.assign(m_sets.size(), VK_NULL_HANDLE);

        for (std::size_t i = 0; i < m_sets.size(); ++i) {
            VkDescriptorSetLayoutCreateInfo layout_info{};
            layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layout_info.bindingCount = static_cast<std::uint32_t>(m_sets[i].size());
            layout_info.pBindings = m_sets[i].data();

            if (vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &m_descriptor_layouts[i]) != VK_SUCCESS) {
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutCPT: descriptor set layout failed.");
            }
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // ---------------------------------------------------------------------
    // Descriptor pool
    // ---------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> DefineDescriptorPool(std::uint32_t const& max_count) {
        VkDevice device;
        m_device->GetDevice(&device);

        std::map<VkDescriptorType, std::uint32_t> counts{};
        for (auto const& set : m_sets) {
            for (auto const& binding : set) {
                counts[binding.descriptorType] += binding.descriptorCount * max_count;
            }
        }

        std::vector<VkDescriptorPoolSize> pool_sizes{};
        for (auto const& [type, count] : counts) {
            pool_sizes.push_back(VkDescriptorPoolSize{type, count});
        }

        VkDescriptorPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.poolSizeCount = static_cast<std::uint32_t>(pool_sizes.size());
        pool_info.pPoolSizes = pool_sizes.data();
        pool_info.maxSets = max_count * static_cast<std::uint32_t>(m_sets.size());

        if (vkCreateDescriptorPool(device, &pool_info, nullptr, &m_descriptor_pool) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutCPT: descriptor pool failed.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // ---------------------------------------------------------------------
    // Pipeline layout
    // ---------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> DefinePipelineLayout() {
        if (m_descriptor_layouts.empty()) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutCPT: DefineDescriptorSetLayout not called.");
        }

        if (m_push_count > 0 && m_push.size == 0) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutCPT: invalid push constant size.");
        }

        VkDevice device;
        m_device->GetDevice(&device);

        VkPipelineLayoutCreateInfo pipeline_layout_info{};
        pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_info.setLayoutCount = static_cast<std::uint32_t>(m_descriptor_layouts.size());
        pipeline_layout_info.pSetLayouts = m_descriptor_layouts.data();
        pipeline_layout_info.pushConstantRangeCount = m_push_count;
        pipeline_layout_info.pPushConstantRanges = m_push_count ? &m_push : nullptr;

        if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &m_pipeline_layout) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutCPT: VkPipelineLayout failed.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // ---------------------------------------------------------------------
    // Descriptor set allocation
    // ---------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> AllocateSet(std::uint32_t const& set_index, VkDescriptorSet* set) {
        if (set_index >= m_descriptor_layouts.size()) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutCPT: set index out of range.");
        }

        VkDevice device;
        m_device->GetDevice(&device);

        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = m_descriptor_pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &m_descriptor_layouts[set_index];

        if (vkAllocateDescriptorSets(device, &alloc_info, set) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutCPT: descriptor set allocation failed.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // ---------------------------------------------------------------------
    // Accessors
    // ---------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> GetKind(Ir77PVComputeKind* kind) {
        *kind = m_kind;
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetSetCount(std::uint32_t* count) {
        *count = static_cast<std::uint32_t>(m_descriptor_layouts.size());
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetDescriptorSetLayout(std::uint32_t const& set_index, VkDescriptorSetLayout* layout) {
        if (set_index >= m_descriptor_layouts.size()) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutCPT: set index out of range.");
        }

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

    // x, y, z the shader's local_size_x/y/z must match; also what the dispatch-count math
    // (groupCount = ceil(itemCount / local_size)) should divide by
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

   private:
    static VkDescriptorSetLayoutBinding Bind(std::uint32_t binding, VkDescriptorType type, VkShaderStageFlags stages) {
        VkDescriptorSetLayoutBinding b{};
        b.binding = binding;
        b.descriptorType = type;
        b.descriptorCount = 1;
        b.stageFlags = stages;
        b.pImmutableSamplers = nullptr;
        return b;
    }

    // ---------------------------------------------------------------------
    // Compute layouts per kind -- each returns one or more descriptor sets,
    // each set a list of bindings. Graphics bindings (camera/material/CEF/etc)
    // live in Ir77PVLayout, not here.
    // ---------------------------------------------------------------------
    static std::vector<std::vector<VkDescriptorSetLayoutBinding>> BindingsCulling() {
        return {{
            Bind(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT),  // AABBs
            Bind(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT),  // per-object culling data
            Bind(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)   // output visibility / indirect commands
        }};
    }

    static std::vector<std::vector<VkDescriptorSetLayoutBinding>> BindingsSkinningUpdate() {
        return {{
            Bind(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT),  // bone matrices
            Bind(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT),  // weights
            Bind(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)   // output vertices
        }};
    }

    static std::vector<std::vector<VkDescriptorSetLayoutBinding>> BindingsMorphUpdate() {
        return {{
            Bind(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT),  // base mesh
            Bind(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT),  // morph targets
            Bind(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)   // output mesh
        }};
    }

    static std::vector<std::vector<VkDescriptorSetLayoutBinding>> BindingsClothSim() {
        return {{
            Bind(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT),  // particles
            Bind(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT),  // constraints
            Bind(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)   // output
        }};
    }

   private:
    std::shared_ptr<IIr77PVDevice> m_device{nullptr};

    std::uint32_t m_local_size_x{1};

    std::uint32_t m_local_size_y{1};

    std::uint32_t m_local_size_z{1};

    std::string m_debug_name{};

    Ir77PVComputeKind m_kind{Ir77PVComputeKind::Culling};

    std::vector<std::vector<VkDescriptorSetLayoutBinding>> m_sets{};

    std::vector<VkDescriptorSetLayout> m_descriptor_layouts{};

    VkDescriptorPool m_descriptor_pool{VK_NULL_HANDLE};

    VkPushConstantRange m_push{};

    std::uint32_t m_push_count{0};

    VkPipelineLayout m_pipeline_layout{VK_NULL_HANDLE};
};

}  // namespace NSIr77PeregrineV