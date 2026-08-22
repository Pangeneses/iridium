#include "Ir77PVRPColor.hpp"

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include "../../dictionary/IDIr77PVContext.hpp"

#include "../../interface/IIr77PeregrineV.hpp"
#include "../../interface/IIr77PVDevice.hpp"
#include "../../interface/IIr77PVSwapchain.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {
std::uint32_t Ir77PVRenderPassColor::CurrentDevice() {
    std::shared_ptr<IIr77PeregrineV> context = QueryAs<IIr77PeregrineV>(&GUIDIIr77PeregrineV, m_context.get());

    std::uint32_t index;
    context->CurrentDevice(index);

    return index;
}

SwapchainSupportDetails Ir77PVRenderPassColor::GetSwapchainSupportDetails() {
    std::shared_ptr<IIr77PeregrineV> context = QueryAs<IIr77PeregrineV>(&GUIDIIr77PeregrineV, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    context->GetMemberByID(ID_SWAPCHAIN, enlisted);

    std::shared_ptr<IIr77PVSwapchain> swapchain = QueryAs<IIr77PVSwapchain>(&GUIDIIr77PVSwapchain, enlisted.get());

    SwapchainSupportDetails details;
    swapchain->GetSwapchainSupportDetails(details);

    return details;
}

VkDevice Ir77PVRenderPassColor::GetDevice() {
    std::shared_ptr<IIr77PeregrineV> context = QueryAs<IIr77PeregrineV>(&GUIDIIr77PeregrineV, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    context->GetMemberByID(ID_DEVICE, enlisted);

    std::shared_ptr<IIr77PVDevice> device = QueryAs<IIr77PVDevice>(&GUIDIIr77PVDevice, enlisted.get());

    VkDevice vk_device;
    device->GetVkDevice(&vk_device);

    return vk_device;
}
}  // namespace NSIr77PeregrineV