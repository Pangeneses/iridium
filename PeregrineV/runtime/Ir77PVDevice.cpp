#include "Ir77PVDevice.hpp"

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PVContext.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"

#include "../interface/IIr77PVContext.hpp"

#include "../interface/IIr77PVInstance.hpp"
#include "../interface/IIr77PVQueue.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

VkInstance Ir77PVDevice::GetInstance() {
    std::shared_ptr<IIr77PVContext> context = QueryAs<IIr77PVContext>(&GUIDIIr77PVContext, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    context->GetMemberByID(ID_INSTANCE, enlisted);

    std::shared_ptr<IIr77PVInstance> instance = QueryAs<IIr77PVInstance>(&GUIDIIr77PVInstance, m_context.get());

    VkInstance vk_instance;
    instance->GetVkInstance(&vk_instance);

    return vk_instance;
}

std::vector<VkDeviceQueueCreateInfo> Ir77PVDevice::GetQueueInfo() {
    std::shared_ptr<IIr77PVContext> context = QueryAs<IIr77PVContext>(&GUIDIIr77PVContext, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    context->GetMemberByID(ID_INSTANCE, enlisted);

    std::shared_ptr<IIr77PVQueue> queue = QueryAs<IIr77PVQueue>(&GUIDIIr77PVQueue, m_context.get());

    std::vector<VkDeviceQueueCreateInfo> vk_device_infos;
    queue->GetQueueCreateInfos(vk_device_infos);

    return vk_device_infos;
}
}  // namespace NSIr77PeregrineV