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

struct Ir77PVQueueInfo {
    std::vector<Ir77PVQueueType> type{Ir77PVQueueType::Unknown};
    std::vector<VkQueueFamilyProperties> family_properties;
    std::vector<VkDeviceQueueCreateInfo> create_infos;
    std::vector<VkBool32> presentation;
    std::vector<VkQueue> queues;
};

typedef struct IIr77PVQueue : virtual public IIr77Enlisted {
    IIr77PVQueue() = default;

    /************* RUNTIME *************/
    virtual std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context) = 0;

    virtual std::shared_ptr<IIr77Return const> CreateQueues() = 0;

    /************* GETTERS *************/
    virtual std::shared_ptr<IIr77Return const> GetQueueInfos(std::vector<Ir77PVQueueInfo>& queue_infos) = 0;

    virtual ~IIr77PVQueue() = default;
}* PIr77PVQueue;
}  // namespace NSIr77PeregrineV