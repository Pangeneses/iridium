#pragma once

#include <vulkan/vulkan.h>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

typedef struct IIr77PVMemory : virtual public IIr77Enlisted {
    IIr77PVMemory() = default;

    /************* RUNTIME *************/
    virtual std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context) = 0;

    virtual std::shared_ptr<IIr77Return const> Allocate(VkMemoryRequirements const& requirements, VkMemoryPropertyFlags properties, VkDeviceMemory* memory) = 0;

    virtual std::shared_ptr<IIr77Return const> Free(VkDeviceMemory memory) = 0;

    /************* GETTERS *************/
    virtual std::shared_ptr<IIr77Return const> GetPhysicalDeviceMemoryProperties(VkPhysicalDeviceMemoryProperties* props) = 0;

    virtual ~IIr77PVMemory() = default;
}* PIr77PVMemory;

}  // namespace NSIr77PeregrineV