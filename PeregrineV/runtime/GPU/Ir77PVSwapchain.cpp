#include "Ir77PVSwapchain.hpp"

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include "../../dictionary/IDIIr77PeregrineV.hpp"
#include "../../dictionary/IDIr77PVContext.hpp"

#include "../../interface/IIr77PeregrineV.hpp"

#include "../../interface/IIr77PVInstance.hpp"
#include "../../interface/IIr77PVDevice.hpp"
#include "../../interface/IIr77PVQueue.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {
std::vector<Ir77PVWindowInfo> Ir77PVSwapchain::GetWindowInfos() {
    std::shared_ptr<IIr77PeregrineV> context = QueryAs<IIr77PeregrineV>(&GUIDIIr77PeregrineV, m_context.get());

    std::vector<Ir77PVWindowInfo> window_infos;
    context->GetWindowInfos(window_infos);

    return window_infos;
}

VkInstance Ir77PVSwapchain::GetInstance() {
    std::shared_ptr<IIr77PeregrineV> context = QueryAs<IIr77PeregrineV>(&GUIDIIr77PeregrineV, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    context->GetMemberByID(ID_INSTANCE, m_device_index, enlisted);

    std::shared_ptr<IIr77PVInstance> instance = QueryAs<IIr77PVInstance>(&GUIDIIr77PVInstance, m_context.get());

    VkInstance vk_instance;
    instance->GetVkInstance(&vk_instance);

    return vk_instance;
}

std::vector<Ir77PVDeviceInfo> Ir77PVSwapchain::GetDeviceInfos() {
    std::shared_ptr<IIr77PeregrineV> context = QueryAs<IIr77PeregrineV>(&GUIDIIr77PeregrineV, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    context->GetMemberByID(ID_DEVICE, m_device_index, enlisted);

    std::shared_ptr<IIr77PVDevice> device = QueryAs<IIr77PVDevice>(&GUIDIIr77PVDevice, enlisted.get());

    std::vector<Ir77PVDeviceInfo> device_infos;
    device->GetDeviceInfos(device_infos);

    return device_infos;
}

std::vector<Ir77PVQueueInfo> Ir77PVSwapchain::GetQueueInfos() {
    std::shared_ptr<IIr77PeregrineV> context = QueryAs<IIr77PeregrineV>(&GUIDIIr77PeregrineV, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    context->GetMemberByID(ID_QUEUES, m_device_index, enlisted);

    std::shared_ptr<IIr77PVQueue> queue = QueryAs<IIr77PVQueue>(&GUIDIIr77PVQueue, m_context.get());

    std::vector<Ir77PVQueueInfo> queue_infos;
    queue->GetQueueInfos(queue_infos);

    return queue_infos;
}

}  // namespace NSIr77PeregrineV