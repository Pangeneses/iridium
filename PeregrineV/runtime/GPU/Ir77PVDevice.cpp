#include "Ir77PVDevice.hpp"

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include "../../dictionary/IDIIr77PeregrineV.hpp"
#include "../../dictionary/IDIr77PVContext.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"

#include "../../interface/IIr77PVContext.hpp"

#include "../../interface/IIr77PVInstance.hpp"
#include "../../interface/IIr77PVQueue.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {
std::uint32_t Ir77PVDevice::CurrentDevice() {
    std::shared_ptr<IIr77PVContext> context = QueryAs<IIr77PVContext>(&GUIDIIr77PVContext, m_context.get());

    std::uint32_t index;
    context->CurrentDevice(index);

    return index;
}

VkInstance Ir77PVDevice::GetInstance() {
    std::shared_ptr<IIr77PVContext> context = QueryAs<IIr77PVContext>(&GUIDIIr77PVContext, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    context->GetMemberByID(ID_INSTANCE, enlisted);

    std::shared_ptr<IIr77PVInstance> instance = QueryAs<IIr77PVInstance>(&GUIDIIr77PVInstance, m_context.get());

    VkInstance vk_instance;
    instance->GetVkInstance(&vk_instance);

    return vk_instance;
}

std::vector<Ir77PVQueueInfo> Ir77PVDevice::GetQueueInfos() {
    std::shared_ptr<IIr77PVContext> context = QueryAs<IIr77PVContext>(&GUIDIIr77PVContext, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    context->GetMemberByID(ID_INSTANCE, enlisted);

    std::shared_ptr<IIr77PVQueue> queue = QueryAs<IIr77PVQueue>(&GUIDIIr77PVQueue, m_context.get());

    std::vector<Ir77PVQueueInfo> queue_infos;
    queue->GetQueueInfos(queue_infos);

    return queue_infos;
}
}  // namespace NSIr77PeregrineV