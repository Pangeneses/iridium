#pragma once
#include <SDL3/SDL_stdinc.h>
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

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

struct Ir77PVQueueFamily {
    VkQueue queue{VK_NULL_HANDLE};
    Ir77PVQueueType type{Ir77PVQueueType::Unknown};
    std::uint32_t index{UINT32_MAX};
    VkBool32 presentation{false};
};

struct IIr77PVPregrineV;
struct IIr77PVCmdBuffer;
struct IIr77PVSwapchain;

typedef struct IIr77PVQueue : virtual public IIr77Enlisted {
    IIr77PVQueue() = default;

    /************* RUNTIME *************/
    virtual std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context) = 0;

    virtual std::shared_ptr<IIr77Return const> InitQueueFamilyProps(std::uint32_t const& index) = 0;

    virtual std::shared_ptr<IIr77Return const> InitQueueCreateInfos() = 0;

    /************* GETTERS *************/
    virtual std::shared_ptr<IIr77Return const> GetQueueCreateInfos(std::vector<VkDeviceQueueCreateInfo>& info_list) = 0;

    virtual std::shared_ptr<IIr77Return const> GetQueueFamilies(std::vector<Ir77PVQueueFamily>& families) = 0;

    virtual ~IIr77PVQueue() = default;
}* PIr77PVQueue;
}  // namespace NSIr77PeregrineV