#pragma once

#include <vulkan/vulkan.h>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

typedef struct IIr77PVMemoryStrategy : virtual public IIr77Enlisted {
    IIr77PVMemoryStrategy() = default;

    virtual std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context) = 0;

    virtual std::shared_ptr<IIr77Return const> SelectMemoryType(VkMemoryRequirements const& requirements, VkMemoryPropertyFlags desired,
                                                                VkPhysicalDeviceMemoryProperties const& mem_props, uint32_t* type_index) = 0;

    virtual ~IIr77PVMemoryStrategy() = default;
}* PIr77PVMemoryStrategy;

}  // namespace NSIr77PeregrineV