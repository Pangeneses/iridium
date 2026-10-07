#pragma once

#include <vulkan/vulkan.h>

#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <stdexcept>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"
#include "../../Ir77RT/dictionary/IDIr77RET.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77PVDevice.hpp"
#include "../interface/IIr77PVLayoutRay.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77PVLayoutRay : public Ir77Enlisted, public IIr77PVLayoutRay, public std::enable_shared_from_this<Ir77PVLayoutRay> {
   public:
    Ir77PVLayoutRay() {
        m_enlisted_uuid.Generate();

        m_enlisted = std::chrono::system_clock::now();
    }

    ~Ir77PVLayoutRay() {
        if (!m_device) return;

        VkDevice device;
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
        seat_shared_uuid<&GUIDIr77PVLayoutRay>(uid);

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

        else if (iid == GUIDIIr77PVLayoutRay)
            obj = std::shared_ptr<IIr77PVLayoutRay>(shared_from_this(), static_cast<IIr77PVLayoutRay*>(this));

        else if (iid == GUIDIr77PVLayoutRay)
            obj = std::shared_ptr<Ir77PVLayoutRay>(shared_from_this(), static_cast<Ir77PVLayoutRay*>(this));

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

    std::shared_ptr<IIr77Return const> SetBindings(std::uint32_t const& set, std::vector<VkDescriptorSetLayoutBinding> const& bindings) {
        if (bindings.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutRay: empty binding list.");

        if (m_bindings.size() <= set) m_bindings.resize(set + 1);

        m_bindings[set] = bindings;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetPushConstantSize(std::uint32_t const& size) {
        m_push_constant_size = size;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // creation
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> DefineDescriptorSetLayout() {
        if (!m_device) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutRay: device not set.");

        VkDevice device;
        m_device->GetDevice(&device);

        Destroy(device);

        if (m_bindings.empty()) m_bindings.push_back(DefaultBindings());

        m_set_layouts.assign(m_bindings.size(), VK_NULL_HANDLE);

        for (std::size_t set = 0; set < m_bindings.size(); set++) {
            // pipeline layouts take a contiguous array of set layouts -- a gap would need a null layout, which core Vulkan rejects
            if (m_bindings[set].empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutRay: set numbers must be contiguous from 0.");

            VkDescriptorSetLayoutCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            info.bindingCount = static_cast<std::uint32_t>(m_bindings[set].size());
            info.pBindings = m_bindings[set].data();

            if (vkCreateDescriptorSetLayout(device, &info, nullptr, &m_set_layouts[set]) != VK_SUCCESS)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutRay: vkCreateDescriptorSetLayout failed.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Define Ray Descriptor Set Layout.");
    }

    std::shared_ptr<IIr77Return const> DefineDescriptorPool(std::uint32_t const& capacity) {
        if (m_set_layouts.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutRay: DefineDescriptorSetLayout not called.");

        if (capacity == 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutRay: pool capacity is zero.");

        VkDevice device;
        m_device->GetDevice(&device);

        if (m_pool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, m_pool, nullptr);
            m_pool = VK_NULL_HANDLE;
        }

        // every binding of every set, times the number of sets of each layout the pool must hold
        std::map<VkDescriptorType, std::uint32_t> counts{};
        for (auto const& bindings : m_bindings)
            for (auto const& binding : bindings) counts[binding.descriptorType] += binding.descriptorCount * capacity;

        std::vector<VkDescriptorPoolSize> sizes{};
        sizes.reserve(counts.size());
        for (auto const& [type, count] : counts) sizes.push_back({type, count});

        VkDescriptorPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        info.maxSets = capacity * static_cast<std::uint32_t>(m_set_layouts.size());
        info.poolSizeCount = static_cast<std::uint32_t>(sizes.size());
        info.pPoolSizes = sizes.data();

        if (vkCreateDescriptorPool(device, &info, nullptr, &m_pool) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutRay: vkCreateDescriptorPool failed.");

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Define Ray Descriptor Pool.");
    }

    std::shared_ptr<IIr77Return const> DefinePipelineLayout() {
        if (m_set_layouts.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutRay: DefineDescriptorSetLayout not called.");

        VkDevice device;
        m_device->GetDevice(&device);

        if (m_pipeline_layout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, m_pipeline_layout, nullptr);
            m_pipeline_layout = VK_NULL_HANDLE;
        }

        VkPushConstantRange range{};
        range.stageFlags = kRayStages;
        range.offset = 0;
        range.size = m_push_constant_size;

        VkPipelineLayoutCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        info.setLayoutCount = static_cast<std::uint32_t>(m_set_layouts.size());
        info.pSetLayouts = m_set_layouts.data();
        info.pushConstantRangeCount = m_push_constant_size > 0 ? 1 : 0;
        info.pPushConstantRanges = m_push_constant_size > 0 ? &range : nullptr;

        if (vkCreatePipelineLayout(device, &info, nullptr, &m_pipeline_layout) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutRay: vkCreatePipelineLayout failed.");

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Define Ray Pipeline Layout.");
    }

    std::shared_ptr<IIr77Return const> AllocateSets(std::uint32_t const& set, std::uint32_t const& count, std::vector<VkDescriptorSet>& sets) {
        if (m_pool == VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutRay: DefineDescriptorPool not called.");

        if (set >= m_set_layouts.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutRay: set index out of range.");

        if (count == 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutRay: set count is zero.");

        VkDevice device;
        m_device->GetDevice(&device);

        std::vector<VkDescriptorSetLayout> const layouts(count, m_set_layouts[set]);

        VkDescriptorSetAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        info.descriptorPool = m_pool;
        info.descriptorSetCount = count;
        info.pSetLayouts = layouts.data();

        sets.assign(count, VK_NULL_HANDLE);

        if (vkAllocateDescriptorSets(device, &info, sets.data()) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutRay: set allocation failed -- raise the pool capacity.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // queries
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> GetPipelineLayout(VkPipelineLayout* layout) {
        *layout = m_pipeline_layout;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetDescriptorSetLayout(std::uint32_t const& set, VkDescriptorSetLayout* layout) {
        if (set >= m_set_layouts.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayoutRay: set index out of range.");

        *layout = m_set_layouts[set];

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetDescriptorPool(VkDescriptorPool* pool) {
        *pool = m_pool;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetSetCount(std::uint32_t* count) {
        *count = static_cast<std::uint32_t>(m_set_layouts.size());

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetPushConstantStages(VkShaderStageFlags* stages) {
        *stages = m_push_constant_size > 0 ? kRayStages : 0;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // internals
    // -------------------------------------------------------------------------------------------------------------------------------------
   private:
    static constexpr VkShaderStageFlags kRayStages = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
                                                     VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR |
                                                     VK_SHADER_STAGE_CALLABLE_BIT_KHR;

    // Set 0 when the caller gives none. Closest-hit sees the TLAS so it can fire shadow / reflection rays.
    static std::vector<VkDescriptorSetLayoutBinding> DefaultBindings() {
        VkShaderStageFlags const hit = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR;

        return {
            {0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, nullptr},  // TLAS
            {1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},                                            // output
            {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | hit, nullptr},      // camera
            {3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, hit, nullptr},  // per-instance geometry / material table
        };
    }

    void Destroy(VkDevice device) {
        if (m_pipeline_layout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, m_pipeline_layout, nullptr);
        if (m_pool != VK_NULL_HANDLE) vkDestroyDescriptorPool(device, m_pool, nullptr);

        for (auto const layout : m_set_layouts)
            if (layout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, layout, nullptr);

        m_pipeline_layout = VK_NULL_HANDLE;
        m_pool = VK_NULL_HANDLE;
        m_set_layouts.clear();
    }

   private:
    std::shared_ptr<IIr77PVDevice> m_device;

    // [set][binding]
    std::vector<std::vector<VkDescriptorSetLayoutBinding>> m_bindings{};

    std::vector<VkDescriptorSetLayout> m_set_layouts{};

    VkDescriptorPool m_pool{VK_NULL_HANDLE};

    VkPipelineLayout m_pipeline_layout{VK_NULL_HANDLE};

    std::uint32_t m_push_constant_size{0};
};
}  // namespace NSIr77PeregrineV