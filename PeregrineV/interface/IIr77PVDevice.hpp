#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVInstance.hpp"

#include "../types/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

static const std::uint8_t IR77_UNKNOWN__BIT = 0b00000000;
static const std::uint8_t IR77_GRAPHICS_BIT = 0b00000001;
static const std::uint8_t IR77_COMPUTE_BIT = 0b00000010;
static const std::uint8_t IR77_TRANSFER_BIT = 0b00000100;
static const std::uint8_t IR77_SPARSE_BIT = 0b00001000;
static const std::uint8_t IR77_PROTECTED_BIT = 0b00010000;
static const std::uint8_t IR77_ENCODE_BIT = 0b00100000;
static const std::uint8_t IR77_DECODE_BIT = 0b01000000;

typedef struct Ir77PVQueueFamily {
    std::uint8_t type{IR77_UNKNOWN__BIT};
    VkQueueFamilyProperties family_properties;
    VkDeviceQueueCreateInfo create_info;
    VkBool32 presentation{UINT32_MAX};
    VkQueue queue{VK_NULL_HANDLE};
}* pIr77PVQueueFamily;

typedef struct IIr77PVDevice : virtual public IIr77Enlisted {
    IIr77PVDevice() = default;

    virtual std::shared_ptr<IIr77Return const> SetInstance(std::shared_ptr<IIr77PVInstance> instance) = 0;

    virtual std::shared_ptr<IIr77Return const> EnumeratePhysicalDevices(std::uint32_t const& device_index) = 0;

    virtual std::shared_ptr<IIr77Return const> DefineQueueFamilyProps(SDL_Window* window) = 0;

    virtual std::shared_ptr<IIr77Return const> DefineQueueCreateInfos() = 0;

    virtual std::shared_ptr<IIr77Return const> QueryDeviceExtensionSupport() = 0;

    virtual std::shared_ptr<IIr77Return const> QueryDisplaySupport(int& rank, std::string& name) = 0;

    virtual std::shared_ptr<IIr77Return const> DefineDeviceInfo() = 0;

    virtual std::shared_ptr<IIr77Return const> DefineDevice() = 0;

    virtual std::shared_ptr<IIr77Return const> GetPhysicalDevice(VkPhysicalDevice* phys_device) = 0;

    virtual std::shared_ptr<IIr77Return const> GetDeviceProperties(VkPhysicalDeviceProperties& phys_device_props) = 0;

    virtual std::shared_ptr<IIr77Return const> GetDeviceFeatures(VkPhysicalDeviceFeatures& phys_device_features) = 0;

    virtual std::shared_ptr<IIr77Return const> GetMeshShading(bool& enabled) = 0;

    // acceleration structures + ray queries enabled (VK_KHR_acceleration_structure / ray_query / deferred_host_operations + features)
    virtual std::shared_ptr<IIr77Return const> GetRayQuery(bool& enabled) = 0;

    // VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT is usable -- the allocator is created to match
    virtual std::shared_ptr<IIr77Return const> GetBufferDeviceAddress(bool& enabled) = 0;

    // scratch alignment and build limits for BLAS / TLAS -- zeros when ray queries are off
    virtual std::shared_ptr<IIr77Return const> GetAccelerationStructureProperties(VkPhysicalDeviceAccelerationStructurePropertiesKHR& properties) = 0;

    virtual std::shared_ptr<IIr77Return const> GetVulkan12Properties(VkPhysicalDeviceVulkan12Properties& properties) = 0;

    virtual std::shared_ptr<IIr77Return const> GetDevice(VkDevice* device) = 0;

    virtual std::shared_ptr<IIr77Return const> GetQueueFamilies(std::vector<Ir77PVQueueFamily>& queue_family) = 0;

    virtual std::shared_ptr<IIr77Return const> GetPresentQueue(VkQueue& queue, std::uint32_t& family) = 0;

    virtual std::shared_ptr<IIr77Return const> GetUploadPool(VkCommandPool& pool, VkQueue& queue) = 0;

    // first transfer-only family (copy engine); the present queue when the GPU has none -- compare families to know if ownership transfers apply
    virtual std::shared_ptr<IIr77Return const> GetTransferQueue(VkQueue& queue, std::uint32_t& family) = 0;

    // transient pool on the transfer family, for streaming uploads -- never the upload pool
    virtual std::shared_ptr<IIr77Return const> GetTransferPool(VkCommandPool& pool, VkQueue& queue) = 0;

    virtual ~IIr77PVDevice() = default;
}* pIIr77PVDevice;
}  // namespace NSIr77PeregrineV
