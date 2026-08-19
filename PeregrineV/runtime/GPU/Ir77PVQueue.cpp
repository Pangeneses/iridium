#include "Ir77PVQueue.hpp"

#include "../../dictionary/IDIIr77PeregrineV.hpp"
#include "../../dictionary/IDIr77PVContext.hpp"

#include "../../Ir77RT/interface//IIr77Enlisted.hpp"

#include "../../interface/IIr77PVContext.hpp"

#include "../../interface/IIr77PVDevice.hpp"
#include "../../interface/IIr77PVSwapchain.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {
VkPhysicalDevice Ir77PVQueue::GetPhysicalDevice(std::uint32_t const& index) {
    std::shared_ptr<IIr77PVContext> context = QueryAs<IIr77PVContext>(&GUIDIIr77PVContext, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    context->GetMemberByID(ID_INSTANCE, enlisted);

    std::shared_ptr<IIr77PVDevice> device = QueryAs<IIr77PVDevice>(&GUIDIIr77PVDevice, enlisted.get());

    VkPhysicalDevice vk_phys_device;
    device->GetVkPhysicalDevice(&vk_phys_device, index);

    return vk_phys_device;
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