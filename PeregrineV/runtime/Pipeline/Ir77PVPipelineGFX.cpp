#include "Ir77PVPipelineGFX.hpp"

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include "../../dictionary/IDIIr77PeregrineV.hpp"
#include "../../dictionary/IDIr77PVContext.hpp"

#include "../../interface/IIr77PeregrineV.hpp"

#include "../../interface/IIr77PVSwapchain.hpp"
#include "../../interface/IIr77PVDevice.hpp"
#include "../../interface/IIr77PVLayout.hpp"
#include "../../interface/IIr77PVRenderPass.hpp"
#include "../../interface/IIr77PVShader.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {
std::uint32_t Ir77PVPipelineGFX::CurrentDevice() {
    std::shared_ptr<IIr77PeregrineV> context = QueryAs<IIr77PeregrineV>(&GUIDIIr77PeregrineV, m_context.get());

    std::uint32_t index;
    context->CurrentDevice(index);

    return index;
}

std::vector<Ir77PVSwapchainInfo> Ir77PVPipelineGFX::GetSwapchainInfos() {
    std::shared_ptr<IIr77PeregrineV> context = QueryAs<IIr77PeregrineV>(&GUIDIIr77PeregrineV, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    context->GetMemberByID(ID_SWAPCHAIN, enlisted);

    std::shared_ptr<IIr77PVSwapchain> swapchain = QueryAs<IIr77PVSwapchain>(&GUIDIIr77PVSwapchain, enlisted.get());

    std::vector<Ir77PVSwapchainInfo> swapchain_infos;
    swapchain->GetSwapchainInfos(swapchain_infos);

    return swapchain_infos;
}

VkDevice Ir77PVPipelineGFX::GetDevice() {
    std::shared_ptr<IIr77PeregrineV> context = QueryAs<IIr77PeregrineV>(&GUIDIIr77PeregrineV, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    context->GetMemberByID(ID_DEVICE, enlisted);

    std::shared_ptr<IIr77PVDevice> device = QueryAs<IIr77PVDevice>(&GUIDIIr77PVDevice, enlisted.get());

    VkDevice vk_device;
    device->GetVkDevice(&vk_device);

    return vk_device;
}

VkRenderPass Ir77PVPipelineGFX::GetRenderPass() {
    std::shared_ptr<IIr77PeregrineV> context = QueryAs<IIr77PeregrineV>(&GUIDIIr77PeregrineV, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    context->GetMemberByID(ID_RENDER_PASS_COLOR, enlisted);

    std::shared_ptr<IIr77PVRenderPass> render_pass = QueryAs<IIr77PVRenderPass>(&GUIDIIr77PVRenderPass, enlisted.get());

    VkRenderPass vk_render_pass;
    render_pass->GetRenderPass(&vk_render_pass);

    return vk_render_pass;
}

std::vector<VkPipelineShaderStageCreateInfo> Ir77PVPipelineGFX::GetPipelineShaderStageInfos() {
    std::shared_ptr<IIr77PeregrineV> context = QueryAs<IIr77PeregrineV>(&GUIDIIr77PeregrineV, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    context->GetMemberByID(ID_SHADER, enlisted);

    std::shared_ptr<IIr77PVShader> shaders = QueryAs<IIr77PVShader>(&GUIDIIr77PVShader, enlisted.get());

    std::vector<VkPipelineShaderStageCreateInfo> create_info;
    shaders->GetPipelineShaderStageInfos(create_info, {ID_SHADER_VERT, ID_SHADER_FRAG});

    return create_info;
}
}  // namespace NSIr77PeregrineV