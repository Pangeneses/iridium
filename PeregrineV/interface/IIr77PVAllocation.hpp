#pragma once

#include <vulkan/vulkan.h>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

typedef struct IIr77PVAllocation : virtual public IIr77Enlisted {
    IIr77PVAllocation() = default;

    virtual std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context) = 0;

    virtual std::shared_ptr<IIr77Return const> Allocate(
        VkMemoryRequirements const& requirements,
        VkMemoryPropertyFlags properties) = 0;

    virtual std::shared_ptr<IIr77Return const> BindBuffer(VkBuffer buffer) = 0;

    virtual std::shared_ptr<IIr77Return const> BindImage(VkImage image) = 0;

    virtual std::shared_ptr<IIr77Return const> Map(void** ptr) = 0;

    virtual std::shared_ptr<IIr77Return const> Unmap() = 0;

    virtual std::shared_ptr<IIr77Return const> Free() = 0;

    virtual std::shared_ptr<IIr77Return const> GetMemory(VkDeviceMemory* memory) = 0;

    virtual std::shared_ptr<IIr77Return const> GetOffset(VkDeviceSize* offset) = 0;

    virtual std::shared_ptr<IIr77Return const> GetSize(VkDeviceSize* size) = 0;

    virtual std::shared_ptr<IIr77Return const> GetMemoryTypeIndex(uint32_t* index) = 0;

    virtual std::shared_ptr<IIr77Return const> Allocate(VkMemoryRequirements const& requirements, VkMemoryPropertyFlags properties, VkDeviceMemory* memory) = 0;

    virtual std::shared_ptr<IIr77Return const> Free(VkDeviceMemory memory) = 0;

    virtual std::shared_ptr<IIr77Return const> SelectMemoryType(VkMemoryRequirements const& requirements, VkMemoryPropertyFlags desired,
                                                                VkPhysicalDeviceMemoryProperties const& mem_props, uint32_t* type_index) = 0;

    /************* GETTERS *************/
    virtual std::shared_ptr<IIr77Return const> GetPhysicalDeviceMemoryProperties(VkPhysicalDeviceMemoryProperties* props) = 0;
    virtual ~IIr77PVAllocation() = default;
}* PIr77PVAllocation;

}  // namespace NSIr77PeregrineV