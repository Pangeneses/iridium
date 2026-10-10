#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <cstdint>
#include <memory>
#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVDevice.hpp"
#include "IIr77PVLayoutRay.hpp"

#include "../types/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// Ray-tracing pipeline (VK_KHR_ray_tracing_pipeline) and the shader binding table built from it.
// Needs the device extensions VK_KHR_ray_tracing_pipeline, VK_KHR_acceleration_structure, VK_KHR_deferred_host_operations,
// the bufferDeviceAddress feature, and a VMA allocator created with VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT.
typedef struct IIr77PVPipelineRay : virtual public IIr77Enlisted {
    IIr77PVPipelineRay() = default;

    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    // Allocates the SBT buffer
    virtual std::shared_ptr<IIr77Return const> SetAllocator(VmaAllocator allocator) = 0;

    virtual std::shared_ptr<IIr77Return const> SetLayout(std::shared_ptr<IIr77PVLayoutRay> layout) = 0;

    virtual std::shared_ptr<IIr77Return const> SetStages(std::vector<VkPipelineShaderStageCreateInfo> const& stages) = 0;

    // Exactly one RayGen group; any number of Miss / Hit / Callable. SBT records follow list order within each region,
    // so the Nth Hit group is hit-record N and the Nth Miss group is miss index N.
    virtual std::shared_ptr<IIr77Return const> SetGroups(std::vector<Ir77PVRayGroup> const& groups) = 0;

    // Clamped to the device's maxRayRecursionDepth. 1 = raygen may trace, hit shaders may not.
    virtual std::shared_ptr<IIr77Return const> SetMaxRecursion(std::uint32_t const& depth) = 0;

    virtual std::shared_ptr<IIr77Return const> CreatePipeline() = 0;

    virtual std::shared_ptr<IIr77Return const> GetPipeline(VkPipeline* pipeline) = 0;

    virtual std::shared_ptr<IIr77Return const> GetLayout(std::shared_ptr<IIr77PVLayoutRay>& layout) = 0;

    // The four regions vkCmdTraceRaysKHR takes. An unused region comes back zeroed.
    virtual std::shared_ptr<IIr77Return const> GetShaderBindingTable(VkStridedDeviceAddressRegionKHR* raygen, VkStridedDeviceAddressRegionKHR* miss,
                                                                     VkStridedDeviceAddressRegionKHR* hit, VkStridedDeviceAddressRegionKHR* callable) = 0;

    virtual ~IIr77PVPipelineRay() = default;
}* pIIr77PVPipelineRay;
}  // namespace NSIr77PeregrineV