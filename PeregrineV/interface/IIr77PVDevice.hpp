#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>
#include <string>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVInstance.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// One physical device and, once chosen, its logical device -- extensions, features, queues, the upload and transfer pools.
//   SetInstance -> SetPhysicalDevice(index) -> QueryDisplaySupport (Init ranks the candidates) -> CreateDevice -> Get*
typedef struct IIr77PVDevice : virtual public IIr77Enlisted {
    IIr77PVDevice() = default;

    // ---- setup -- Ir77PVInit -----------------------------------------------------------------------------------------------------------------

    virtual std::shared_ptr<IIr77Return const> SetInstance(std::shared_ptr<IIr77PVInstance> instance) = 0;

    // The instance's index-th physical device
    virtual std::shared_ptr<IIr77Return const> SetPhysicalDevice(std::uint32_t const& index) = 0;

    // Can it drive a display, before any logical device exists: VK_KHR_swapchain and a graphics family that can present to an SDL window.
    // rank 4 discrete, 3 integrated, 2 virtual, 1 other, 0 CPU; fails (rank -1) when it can't.
    virtual std::shared_ptr<IIr77Return const> QueryDisplaySupport(int& rank, std::string& name) = 0;

    // The logical device: one queue per family, the required extensions and features plus every optional one the GPU has (mesh shading,
    // ray queries, buffer device address). Fails when a requirement is missing.
    virtual std::shared_ptr<IIr77Return const> CreateDevice() = 0;

    // ---- the device -- valid after CreateDevice ----------------------------------------------------------------------------------------------

    virtual std::shared_ptr<IIr77Return const> GetPhysicalDevice(VkPhysicalDevice* physical_device) = 0;

    virtual std::shared_ptr<IIr77Return const> GetDevice(VkDevice* device) = 0;

    virtual std::shared_ptr<IIr77Return const> GetDeviceProperties(VkPhysicalDeviceProperties& properties) = 0;

    // VK_EXT_mesh_shader enabled with task + mesh stages
    virtual std::shared_ptr<IIr77Return const> GetMeshShading(bool& enabled) = 0;

    // acceleration structures + ray queries enabled (VK_KHR_acceleration_structure / ray_query / deferred_host_operations + features)
    virtual std::shared_ptr<IIr77Return const> GetRayQuery(bool& enabled) = 0;

    // VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT is usable -- the allocator is created to match
    virtual std::shared_ptr<IIr77Return const> GetBufferDeviceAddress(bool& enabled) = 0;

    // scratch alignment and build limits for BLAS / TLAS -- zeros when ray queries are off
    virtual std::shared_ptr<IIr77Return const> GetAccelerationStructureProperties(VkPhysicalDeviceAccelerationStructurePropertiesKHR& properties) = 0;

    // descriptor-indexing limits -- Ir77PVLayout sizes the bindless arrays from these
    virtual std::shared_ptr<IIr77Return const> GetVulkan12Properties(VkPhysicalDeviceVulkan12Properties& properties) = 0;

    // ---- queues and pools -- valid after CreateDevice ----------------------------------------------------------------------------------------

    // first graphics family that can present
    virtual std::shared_ptr<IIr77Return const> GetPresentQueue(VkQueue& queue, std::uint32_t& family) = 0;

    // transient pool on the present family for one-shot uploads
    virtual std::shared_ptr<IIr77Return const> GetUploadPool(VkCommandPool& pool, VkQueue& queue) = 0;

    // first transfer-only family (copy engine); the present queue when the GPU has none -- compare families to know if ownership transfers apply
    virtual std::shared_ptr<IIr77Return const> GetTransferQueue(VkQueue& queue, std::uint32_t& family) = 0;

    // transient pool on the transfer family, for streaming uploads -- never the upload pool
    virtual std::shared_ptr<IIr77Return const> GetTransferPool(VkCommandPool& pool, VkQueue& queue) = 0;

    virtual ~IIr77PVDevice() = default;
}* pIIr77PVDevice;
}  // namespace NSIr77PeregrineV