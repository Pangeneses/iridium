#include "Ir77PVQueue.hpp"

#include "../../dictionary/IDIIr77PeregrineV.hpp"
#include "../../dictionary/IDIr77PVContext.hpp"

#include "../../Ir77RT/interface//IIr77Enlisted.hpp"

#include "../../interface/IIr77PeregrineV.hpp"

#include "../../interface/IIr77PVDevice.hpp"
#include "../../interface/IIr77PVSwapchain.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {
std::vector<Ir77PVDeviceInfo> Ir77PVQueue::GetDeviceInfos() {
    std::shared_ptr<IIr77PeregrineV> context = QueryAs<IIr77PeregrineV>(&GUIDIIr77PeregrineV, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    context->GetMemberByID(ID_DEVICE, enlisted);

    std::shared_ptr<IIr77PVDevice> device = QueryAs<IIr77PVDevice>(&GUIDIIr77PVDevice, enlisted.get());

    std::vector<Ir77PVDeviceInfo> device_infos;
    device->GetDeviceInfos(device_infos);

    return device_infos;
}

std::vector<Ir77PVSwapchainInfo> Ir77PVQueue::GetSwapchainInfos() {
    std::shared_ptr<IIr77PeregrineV> context = QueryAs<IIr77PeregrineV>(&GUIDIIr77PeregrineV, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    context->GetMemberByID(ID_SWAPCHAIN, enlisted);

    std::shared_ptr<IIr77PVSwapchain> swapchain = QueryAs<IIr77PVSwapchain>(&GUIDIIr77PVSwapchain, enlisted.get());

    std::vector<Ir77PVSwapchainInfo> swapchain_infos;
    swapchain->GetSwapchainInfos(swapchain_infos);

    return swapchain_infos;
}
}  // namespace NSIr77PeregrineV