#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
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
#include "../interface/IIr77PVPipelineRay.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77PVPipelineRay : public Ir77Enlisted, public IIr77PVPipelineRay, public std::enable_shared_from_this<Ir77PVPipelineRay> {
   public:
    Ir77PVPipelineRay() {
        m_enlisted_uuid.Generate();

        m_enlisted = std::chrono::system_clock::now();
    }

    ~Ir77PVPipelineRay() {
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
        seat_shared_uuid<&GUIDIr77PVPipelineRay>(uid);

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

        else if (iid == GUIDIIr77PVPipelineRay)
            obj = std::shared_ptr<IIr77PVPipelineRay>(shared_from_this(), static_cast<IIr77PVPipelineRay*>(this));

        else if (iid == GUIDIr77PVPipelineRay)
            obj = std::shared_ptr<Ir77PVPipelineRay>(shared_from_this(), static_cast<Ir77PVPipelineRay*>(this));

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

    std::shared_ptr<IIr77Return const> SetAllocator(VmaAllocator allocator) {
        m_allocator = allocator;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetLayout(std::shared_ptr<IIr77PVLayoutRay> layout) {
        m_layout = layout;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetStages(std::vector<VkPipelineShaderStageCreateInfo> const& stages) {
        m_stages = stages;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetGroups(std::vector<Ir77PVRayGroup> const& groups) {
        m_groups = groups;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetMaxRecursion(std::uint32_t const& depth) {
        m_max_recursion = depth;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // creation
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> CreatePipeline() {
        if (!m_device || m_allocator == VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineRay: device or allocator not set.");

        if (!m_layout) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineRay: layout not set.");

        if (m_stages.empty() || m_groups.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineRay: no stages or groups.");

        VkPipelineLayout layout{VK_NULL_HANDLE};
        m_layout->GetPipelineLayout(&layout);
        if (layout == VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineRay: layout has no pipeline layout -- call DefinePipelineLayout.");

        auto const validated = ValidateGroups();
        if (validated->ID() != GUIDIr77OperationSucceeded) return validated;

        VkDevice device;
        m_device->GetDevice(&device);

        Destroy(device);

        if (!LoadFunctions(device)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineRay: VK_KHR_ray_tracing_pipeline not enabled on the device.");

        QueryProperties();

        std::vector<VkRayTracingShaderGroupCreateInfoKHR> groups(m_groups.size());
        for (std::size_t i = 0; i < m_groups.size(); i++) {
            groups[i].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            groups[i].type = m_groups[i].type;
            groups[i].generalShader = m_groups[i].general;
            groups[i].closestHitShader = m_groups[i].closest_hit;
            groups[i].anyHitShader = m_groups[i].any_hit;
            groups[i].intersectionShader = m_groups[i].intersection;
        }

        VkRayTracingPipelineCreateInfoKHR info{};
        info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
        info.stageCount = static_cast<std::uint32_t>(m_stages.size());
        info.pStages = m_stages.data();
        info.groupCount = static_cast<std::uint32_t>(groups.size());
        info.pGroups = groups.data();
        info.maxPipelineRayRecursionDepth = std::min(std::max<std::uint32_t>(m_max_recursion, 1), m_properties.maxRayRecursionDepth);
        info.layout = layout;

        if (m_create_pipelines(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &info, nullptr, &m_pipeline) != VK_SUCCESS) {
            m_pipeline = VK_NULL_HANDLE;
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineRay: vkCreateRayTracingPipelinesKHR failed.");
        }

        auto const sbt = BuildShaderBindingTable(device);
        if (sbt->ID() != GUIDIr77OperationSucceeded) {
            Destroy(device);
            return sbt;
        }

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Ray Pipeline.");
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // queries
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> GetPipeline(VkPipeline* pipeline) {
        *pipeline = m_pipeline;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetLayout(std::shared_ptr<IIr77PVLayoutRay>& layout) {
        layout = m_layout;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetShaderBindingTable(VkStridedDeviceAddressRegionKHR* raygen, VkStridedDeviceAddressRegionKHR* miss,
                                                             VkStridedDeviceAddressRegionKHR* hit, VkStridedDeviceAddressRegionKHR* callable) {
        if (m_sbt == VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineRay: CreatePipeline not called.");

        *raygen = m_regions[static_cast<std::size_t>(Ir77PVRayRegion::RayGen)];
        *miss = m_regions[static_cast<std::size_t>(Ir77PVRayRegion::Miss)];
        *hit = m_regions[static_cast<std::size_t>(Ir77PVRayRegion::Hit)];
        *callable = m_regions[static_cast<std::size_t>(Ir77PVRayRegion::Callable)];

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // internals
    // -------------------------------------------------------------------------------------------------------------------------------------
   private:
    static constexpr std::size_t kRegionCount = 4;

    static VkDeviceSize AlignUp(VkDeviceSize const& value, VkDeviceSize const& alignment) {
        return alignment == 0 ? value : (value + alignment - 1) & ~(alignment - 1);
    }

    // Region / type / stage-index consistency, so a bad group fails here with a message instead of in the driver
    std::shared_ptr<IIr77Return const> ValidateGroups() const {
        std::uint32_t raygen_count{0};
        std::uint32_t const stage_count = static_cast<std::uint32_t>(m_stages.size());

        auto const in_range = [stage_count](std::uint32_t const& index) { return index == VK_SHADER_UNUSED_KHR || index < stage_count; };

        for (auto const& group : m_groups) {
            if (!in_range(group.general) || !in_range(group.closest_hit) || !in_range(group.any_hit) || !in_range(group.intersection))
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineRay: group references a stage index past the stage list.");

            bool const general = group.type == VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
            bool const hit_region = group.region == Ir77PVRayRegion::Hit;

            if (general == hit_region) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineRay: Hit region needs a hit-group type; RayGen / Miss / Callable need GENERAL.");

            if (general && group.general == VK_SHADER_UNUSED_KHR) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineRay: general group without a shader.");

            if (group.type == VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR && group.intersection == VK_SHADER_UNUSED_KHR)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineRay: procedural hit group without an intersection shader.");

            if (group.region == Ir77PVRayRegion::RayGen) raygen_count++;
        }

        if (raygen_count != 1) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineRay: exactly one RayGen group is required.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Extension commands -- not exported by the loader
    bool LoadFunctions(VkDevice device) {
        m_create_pipelines = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));
        m_get_handles = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR"));

        return m_create_pipelines != nullptr && m_get_handles != nullptr;
    }

    void QueryProperties() {
        VkPhysicalDevice physical{VK_NULL_HANDLE};
        m_device->GetPhysicalDevice(&physical);

        m_properties = VkPhysicalDeviceRayTracingPipelinePropertiesKHR{};
        m_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

        VkPhysicalDeviceProperties2 properties{};
        properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        properties.pNext = &m_properties;

        vkGetPhysicalDeviceProperties2(physical, &properties);
    }

    // Layout: [raygen][miss...][hit...][callable...], every region starting on shaderGroupBaseAlignment.
    // Raygen's stride must equal its size, so it gets a base-aligned stride; the others use the handle-aligned stride.
    std::shared_ptr<IIr77Return const> BuildShaderBindingTable(VkDevice device) {
        std::uint32_t const group_count = static_cast<std::uint32_t>(m_groups.size());
        VkDeviceSize const handle_size = m_properties.shaderGroupHandleSize;
        VkDeviceSize const handle_stride = AlignUp(handle_size, m_properties.shaderGroupHandleAlignment);
        VkDeviceSize const base_alignment = m_properties.shaderGroupBaseAlignment;

        std::vector<std::uint8_t> handles(static_cast<std::size_t>(group_count * handle_size));
        if (m_get_handles(device, m_pipeline, 0, group_count, handles.size(), handles.data()) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineRay: vkGetRayTracingShaderGroupHandlesKHR failed.");

        // group indices per region, in list order
        std::array<std::vector<std::uint32_t>, kRegionCount> by_region{};
        for (std::uint32_t i = 0; i < group_count; i++) by_region[static_cast<std::size_t>(m_groups[i].region)].push_back(i);

        std::array<VkDeviceSize, kRegionCount> strides{};
        std::array<VkDeviceSize, kRegionCount> sizes{};
        std::array<VkDeviceSize, kRegionCount> offsets{};
        VkDeviceSize total{0};

        for (std::size_t r = 0; r < kRegionCount; r++) {
            std::size_t const count = by_region[r].size();

            strides[r] = r == static_cast<std::size_t>(Ir77PVRayRegion::RayGen) ? AlignUp(handle_stride, base_alignment) : handle_stride;
            sizes[r] = count == 0 ? 0 : AlignUp(strides[r] * count, base_alignment);
            offsets[r] = total;

            total += sizes[r];
        }

        VkBufferCreateInfo buffer_info{};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = total;
        buffer_info.usage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo alloc_info{};
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
        alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo allocation{};

        // the table's base address must itself be base-aligned, not just the offsets inside it
        if (vmaCreateBufferWithAlignment(m_allocator, &buffer_info, &alloc_info, base_alignment, &m_sbt, &m_sbt_allocation, &allocation) != VK_SUCCESS) {
            m_sbt = VK_NULL_HANDLE;
            m_sbt_allocation = VK_NULL_HANDLE;
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineRay: SBT buffer creation failed.");
        }

        auto* const mapped = static_cast<std::uint8_t*>(allocation.pMappedData);

        for (std::size_t r = 0; r < kRegionCount; r++) {
            for (std::size_t j = 0; j < by_region[r].size(); j++) {
                std::uint8_t* const dst = mapped + offsets[r] + strides[r] * j;
                std::uint8_t const* const src = handles.data() + by_region[r][j] * handle_size;

                std::memcpy(dst, src, static_cast<std::size_t>(handle_size));
            }
        }

        vmaFlushAllocation(m_allocator, m_sbt_allocation, 0, VK_WHOLE_SIZE);

        VkBufferDeviceAddressInfo address_info{};
        address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        address_info.buffer = m_sbt;

        VkDeviceAddress const base = vkGetBufferDeviceAddress(device, &address_info);

        for (std::size_t r = 0; r < kRegionCount; r++) {
            // an empty region stays zeroed -- vkCmdTraceRaysKHR accepts that for unused miss / hit / callable
            m_regions[r] = VkStridedDeviceAddressRegionKHR{};

            if (sizes[r] == 0) continue;

            m_regions[r].deviceAddress = base + offsets[r];
            m_regions[r].stride = strides[r];
            m_regions[r].size = r == static_cast<std::size_t>(Ir77PVRayRegion::RayGen) ? strides[r] : sizes[r];
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    void Destroy(VkDevice device) {
        if (m_pipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, m_pipeline, nullptr);
        if (m_sbt != VK_NULL_HANDLE) vmaDestroyBuffer(m_allocator, m_sbt, m_sbt_allocation);

        m_pipeline = VK_NULL_HANDLE;
        m_sbt = VK_NULL_HANDLE;
        m_sbt_allocation = VK_NULL_HANDLE;
        m_regions = {};
    }

   private:
    std::shared_ptr<IIr77PVDevice> m_device;

    VmaAllocator m_allocator{VK_NULL_HANDLE};

    std::shared_ptr<IIr77PVLayoutRay> m_layout;

    std::vector<VkPipelineShaderStageCreateInfo> m_stages{};

    std::vector<Ir77PVRayGroup> m_groups{};

    std::uint32_t m_max_recursion{1};

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_properties{};

    PFN_vkCreateRayTracingPipelinesKHR m_create_pipelines{nullptr};

    PFN_vkGetRayTracingShaderGroupHandlesKHR m_get_handles{nullptr};

    VkPipeline m_pipeline{VK_NULL_HANDLE};

    VkBuffer m_sbt{VK_NULL_HANDLE};

    VmaAllocation m_sbt_allocation{VK_NULL_HANDLE};

    // indexed by Ir77PVRayRegion
    std::array<VkStridedDeviceAddressRegionKHR, kRegionCount> m_regions{};
};
}  // namespace NSIr77PeregrineV