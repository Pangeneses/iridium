#include "Ir77PVAllocation.hpp"

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include "../dictionary/IDIIr77PeregrineV.hpp"

#include "../service/IIr77PeregrineV.hpp"

#include "../interface/IIr77PVDevice.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

VkPhysicalDevice Ir77PVAllocation::GetPhysicalDevice() {
    std::shared_ptr<IIr77PeregrineV> peregrine = QueryAs<IIr77PeregrineV>(&GUIDIIr77PeregrineV, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    peregrine->Context(ID_INSTANCE, enlisted);

    std::shared_ptr<IIr77PVDevice> device = QueryAs<IIr77PVDevice>(&GUIDIIr77PVDevice, enlisted.get());

    VkPhysicalDevice vk_phys_device;
    device->GetVkPhysicalDevice(&vk_phys_device);

    return vk_phys_device;
}

VkDevice Ir77PVAllocation::GetDevice() {
    std::shared_ptr<IIr77PeregrineV> peregrine = QueryAs<IIr77PeregrineV>(&GUIDIIr77PeregrineV, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    peregrine->Context(ID_INSTANCE, enlisted);

    std::shared_ptr<IIr77PVDevice> device = QueryAs<IIr77PVDevice>(&GUIDIIr77PVDevice, enlisted.get());

    VkDevice vk_device;
    device->GetVkDevice(&vk_device);

    return vk_device;
}
}  // namespace NSIr77PeregrineV