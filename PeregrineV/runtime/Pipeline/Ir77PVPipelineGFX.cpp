#include "Ir77PVPipelineGFX.hpp"

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include "../../dictionary/IDIIr77PeregrineV.hpp"
#include "../../dictionary/IDIr77PVContext.hpp"

#include "../../interface/IIr77PVContext.hpp"

#include "../../interface/IIr77PVSwapchain.hpp"
#include "../../interface/IIr77PVDevice.hpp"
#include "../../interface/IIr77PVLayout.hpp"
#include "../../interface/IIr77PVRenderPass.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {
std::uint32_t Ir77PVPipelineGFX::CurrentDevice() {
    std::shared_ptr<IIr77PVContext> context = QueryAs<IIr77PVContext>(&GUIDIIr77PVContext, m_context.get());

    std::uint32_t index;
    context->CurrentDevice(index);

    return index;
}

SwapchainSupportDetails Ir77PVPipelineGFX::GetSwapchainSupportDetails() {
    std::shared_ptr<IIr77PVContext> context = QueryAs<IIr77PVContext>(&GUIDIIr77PVContext, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    context->GetMemberByID(ID_SWAPCHAIN, enlisted);

    std::shared_ptr<IIr77PVSwapchain> swapchain = QueryAs<IIr77PVSwapchain>(&GUIDIIr77PVSwapchain, enlisted.get());

    SwapchainSupportDetails details;
    swapchain->GetSwapchainSupportDetails(details);

    return details;
}

VkDevice Ir77PVPipelineGFX::GetDevice() {
    std::shared_ptr<IIr77PVContext> context = QueryAs<IIr77PVContext>(&GUIDIIr77PeregrineV, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    context->GetMemberByID(ID_DEVICE, enlisted);

    std::shared_ptr<IIr77PVDevice> device = QueryAs<IIr77PVDevice>(&GUIDIIr77PVDevice, enlisted.get());

    VkDevice vk_device;
    device->GetVkDevice(&vk_device);

    return vk_device;
}

VkPipelineLayout Ir77PVPipelineGFX::GetLayout() {
    std::shared_ptr<IIr77PVContext> context = QueryAs<IIr77PVContext>(&GUIDIIr77PeregrineV, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    context->GetMemberByID(ID_LAYOUT_001, enlisted);

    std::shared_ptr<IIr77PVLayout> layout = QueryAs<IIr77PVLayout>(&GUIDIIr77PVLayout, enlisted.get());

    VkPipelineLayout vk_layout;
    layout->GetPipelineLayout(&vk_layout);

    return vk_layout;
}

VkRenderPass Ir77PVPipelineGFX::GetRenderPass() {
    std::shared_ptr<IIr77PVContext> context = QueryAs<IIr77PVContext>(&GUIDIIr77PeregrineV, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    context->GetMemberByID(ID_RENDER_PASS_COLOR, enlisted);

    std::shared_ptr<IIr77PVRenderPass> render_pass = QueryAs<IIr77PVRenderPass>(&GUIDIIr77PVRenderPass, enlisted.get());

    VkRenderPass vk_render_pass;
    render_pass->GetRenderPass(&vk_render_pass);

    return vk_render_pass;
}
}  // namespace NSIr77PeregrineV