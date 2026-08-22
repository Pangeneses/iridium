#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <memory>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

typedef struct Ir77PVWindowInfo {
    SDL_Window* window{nullptr};
}* pIr77PVWindowInfo;

typedef struct Ir77PVDeviceInfo {
    VkPhysicalDevice phys_device;
    VkDevice device;
    VkPhysicalDeviceProperties device_properties;
    std::vector<VkExtensionProperties> available_extensions;
    VkPhysicalDeviceFeatures phys_device_features;
    VkPhysicalDeviceLimits phys_device_limits;
    VkDeviceCreateInfo device_create_info;
}* pIr77PVDeviceInfo;

enum class Ir77PVQueueType : uint32_t {
    Unknown = 0,
    Graphics = 1,
    Compute = 2,
    Transfer = 3,
    Sparse = 4,
    Protected = 5,
    Encode = 6,
    Decode = 7,
};

typedef struct Ir77PVQueueInfo {
    std::vector<Ir77PVQueueType> type{Ir77PVQueueType::Unknown};
    std::vector<VkQueueFamilyProperties> family_properties;
    std::vector<VkDeviceQueueCreateInfo> create_infos;
    std::vector<VkBool32> presentation;
    std::vector<VkQueue> queues;
}* pIr77PVQueueInfo;

typedef struct Ir77PVSwapchainInfo {
    VkSurfaceKHR surface{VK_NULL_HANDLE};
    VkSurfaceCapabilitiesKHR capabilities;    
    std::vector<VkSurfaceFormatKHR> formats;
    VkFormat format{VK_FORMAT_B8G8R8A8_SRGB};
    VkColorSpaceKHR color_space{VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};    
    std::vector<VkPresentModeKHR> present_modes;
    VkPresentModeKHR present_mode{VK_PRESENT_MODE_FIFO_KHR};    
    VkExtent2D swapchain_extent{};    
    VkSwapchainCreateInfoKHR swapchain_info{};
    VkSwapchainKHR swapchain{VK_NULL_HANDLE};
    VkImageViewCreateInfo image_view_create_info{};
    std::uint32_t imageCount{0};
    std::vector<VkImageView> swapchain_views;
    std::vector<VkImage> swapchain_images;
}* pIr77PVSwapchainInfo;

enum class Ir77PVPipelineType : uint32_t {
    Graphics = 0,
    Compute = 1,
    RayTrace = 2,
};

enum class Ir77PVShaderStage : uint32_t {
    Vertex = 0,
    Fragment = 1,
    Compute = 2,
    Geometry = 3,
    TessellationControl = 4,
    TessellationEvaluation = 5,
    Mesh = 6,
    Task = 7,
    RayGeneration = 8,
    RayMiss = 9,
    RayClosestHit = 10,
    RayAnyHit = 11,
    RayIntersection = 12,
};

typedef struct Ir77PVShaderInfo {
    VkPipelineShaderStageCreateInfo stage_create_info;
    Ir77PVShaderStage stage;
    std::vector<char> byte_code;
    std::uint32_t size;
}* pIr77PVShaderInfo;

typedef struct IIr77PeregrineV : virtual public IIr77Enlisted {
    IIr77PeregrineV() = default;

    virtual std::shared_ptr<IIr77Return const> CreateInstance(std::uint32_t& device_count) = 0;    

    virtual std::shared_ptr<IIr77Return const> GetMemberByID(std::uint64_t const& id, std::uint32_t const& device_index,
                                                             std::shared_ptr<IIr77Enlisted>& context) = 0;

    virtual ~IIr77PeregrineV() = default;
}* PIr77PVContext;

}  // namespace NSIr77PeregrineV