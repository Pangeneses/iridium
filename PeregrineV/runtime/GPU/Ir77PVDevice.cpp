#include "Ir77PVDevice.hpp"

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include "../../dictionary/IDIIr77PeregrineV.hpp"
#include "../../dictionary/IDIr77PVContext.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"

#include "../../interface/IIr77PeregrineV.hpp"

#include "../../interface/IIr77PVInstance.hpp"
#include "../../interface/IIr77PVQueue.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {
VkInstance Ir77PVDevice::GetInstance() {
    std::shared_ptr<IIr77PeregrineV> context = QueryAs<IIr77PeregrineV>(&GUIDIIr77PeregrineV, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    context->GetMemberByID(ID_INSTANCE, 0, enlisted);

    std::shared_ptr<IIr77PVInstance> instance = QueryAs<IIr77PVInstance>(&GUIDIIr77PVInstance, m_context.get());

    VkInstance vk_instance;
    instance->GetVkInstance(&vk_instance);

    return vk_instance;
}

Ir77PVQueueInfo Ir77PVDevice::GetQueueInfo() {
    std::shared_ptr<IIr77PeregrineV> context = QueryAs<IIr77PeregrineV>(&GUIDIIr77PeregrineV, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    context->GetMemberByID(ID_QUEUES, m_device_index, enlisted);

    std::shared_ptr<IIr77PVQueue> queue = QueryAs<IIr77PVQueue>(&GUIDIIr77PVQueue, m_context.get());

    Ir77PVQueueInfo queue_info;
    queue->GetQueueInfo(queue_info);

    return queue_info;
}
}  // namespace NSIr77PeregrineV