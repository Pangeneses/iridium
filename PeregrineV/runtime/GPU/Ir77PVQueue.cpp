#include "Ir77PVQueue.hpp"

#include "../../dictionary/IDIIr77PeregrineV.hpp"
#include "../../dictionary/IDIr77PVContext.hpp"

#include "../../Ir77RT/interface//IIr77Enlisted.hpp"

#include "../../interface/IIr77PVContext.hpp"

#include "../../interface/IIr77PVDevice.hpp"
#include "../../interface/IIr77PVSwapchain.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {
std::uint32_t Ir77PVQueue::GetCurrentDevice() {
    std::shared_ptr<IIr77PVContext> context = QueryAs<IIr77PVContext>(&GUIDIIr77PVContext, m_context.get());

    std::uint32_t index;
    context->CurrentDevice(index);

    return index;
}

std::vector<Ir77PVDeviceInfo> Ir77PVQueue::GetDeviceInfos() {
    std::shared_ptr<IIr77PVContext> context = QueryAs<IIr77PVContext>(&GUIDIIr77PVContext, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    context->GetMemberByID(ID_DEVICE, enlisted);

    std::shared_ptr<IIr77PVDevice> device = QueryAs<IIr77PVDevice>(&GUIDIIr77PVDevice, enlisted.get());

    std::vector<Ir77PVDeviceInfo> device_infos;
    device->GetDeviceInfos(device_infos);

    return device_infos;
}

VkSurfaceKHR Ir77PVQueue::GetSurface() {
    std::shared_ptr<IIr77PVContext> context = QueryAs<IIr77PVContext>(&GUIDIIr77PVContext, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    context->GetMemberByID(ID_SWAPCHAIN, enlisted);

    std::shared_ptr<IIr77PVSwapchain> surface = QueryAs<IIr77PVSwapchain>(&GUIDIIr77PVSwapchain, enlisted.get());

    VkSurfaceKHR vk_surface;
    surface->GetSurface(&vk_surface);

    return vk_surface;
}
}  // namespace NSIr77PeregrineV