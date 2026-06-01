#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <cstring>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

enum class Ir77PVQueueType : uint32_t {
    Graphics = 0,
    Compute = 1,
    Transfer = 2,
    Present = 3,
};

struct IIr77PVPregrineV;
struct IIr77PVCommandBuffer;
struct IIr77PVSurface;

typedef struct IIr77PVQueue : virtual public IIr77Enlisted {
    IIr77PVQueue() = default;

    /************* RUNTIME *************/
    virtual std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context) = 0;

    virtual std::shared_ptr<IIr77Return const> InitQueueFamilyProps() = 0;

    virtual std::shared_ptr<IIr77Return const> InitQueueInfos() = 0;

    virtual std::shared_ptr<IIr77Return const> CreateResources() = 0;

    /************* GETTERS *************/
    virtual std::shared_ptr<IIr77Return const> GetDeviceQueueCreateInfo(std::vector<VkDeviceQueueCreateInfo>& info_list) = 0;

    virtual std::shared_ptr<IIr77Return const> GetGraphicsFamily(uint32_t& gfx_family) = 0;

    virtual std::shared_ptr<IIr77Return const> GetPresentFamily(uint32_t& present_family) = 0;

    virtual ~IIr77PVQueue() = default;
}* PIr77PVQueue;
}  // namespace NSIr77PeregrineV