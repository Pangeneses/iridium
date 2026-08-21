#include "Ir77PVCmdBuffer.hpp"

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include "../../dictionary/IDIIr77PeregrineV.hpp"
#include "../../dictionary/IDIr77PVContext.hpp"

#include "../../interface/IIr77PVContext.hpp"
#include "../../interface/IIr77PVDevice.hpp"
#include "../../interface/IIr77PVQueue.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {
VkDevice Ir77PVCmdBuffer::GetDevice() {
    std::shared_ptr<IIr77PVContext> context = QueryAs<IIr77PVContext>(&GUIDIIr77PeregrineV, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    context->GetMemberByID(ID_DEVICE, enlisted);

    std::shared_ptr<IIr77PVDevice> device = QueryAs<IIr77PVDevice>(&GUIDIIr77PVDevice, enlisted.get());

    VkDevice vk_device;
    device->GetVkDevice(&vk_device);

    return vk_device;
}

std::vector<Ir77PVQueueFamily> Ir77PVCmdBuffer::GetQueueFamilies() {
    std::shared_ptr<IIr77PVContext> context = QueryAs<IIr77PVContext>(&GUIDIIr77PVContext, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    context->GetMemberByID(ID_INSTANCE, enlisted);

    std::shared_ptr<IIr77PVQueue> queue = QueryAs<IIr77PVQueue>(&GUIDIIr77PVQueue, m_context.get());

    std::vector<Ir77PVQueueFamily> vk_queue_families;
    queue->GetQueueFamilies(vk_queue_families);

    return vk_queue_families;
}
}