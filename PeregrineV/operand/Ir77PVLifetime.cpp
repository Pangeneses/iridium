#include "Ir77PVLifetime.hpp"
#include <memory>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"

#include "../../Ir77RT/operand//Ir77MPVMOP.hpp"

#include "../runtime/GPU/Ir77PVInstance.hpp"
#include "../runtime/GPU/Ir77PVDevice.hpp"
#include "../runtime/GPU/Ir77PVSwapchain.hpp"
#include "../runtime/Pipeline Layout/Ir77PVLayoutStd.hpp"
#include "../runtime/GPU/Ir77PVRenderPass.hpp"
#include "../runtime/Pipeline/Ir77PVPipelineGFX.hpp"
#include "../runtime/GPU/Ir77PVCmdBuffer.hpp"

#include "../runtime/Pipeline/Ir77PVPipelineGFX.hpp"

#include "../runtime/Pipeline Layout//Ir77PVLayoutStd.hpp"

#include "../runtime/Assets/Ir77PVShader.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

std::shared_ptr<IIr77Return const> Ir77PVLifetime::CreateDeviceInterface(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
    auto lhs_mutable = std::const_pointer_cast<IIr77Operand>(lhs);

    auto context = QueryAs<IIr77PeregrineV>(&GUIDIIr77PeregrineV, lhs_mutable.get());

    if (!context.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid PeregrineV Operand.");

    context->CreateInstance();
    context->EnumeratePhysicalDevices();

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PVLifetime::CreateSwapchains(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
    auto lhs_mutable = std::const_pointer_cast<IIr77Operand>(lhs);

    auto context = QueryAs<IIr77PeregrineV>(&GUIDIIr77PeregrineV, lhs_mutable.get());

    if (!context.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid PeregrineV Operand.");

    context->CreateSurfaces();
    context->EnumerateDeviceQueues();
    context->CreateLogicalDevices();
    context->CreateLayout();
    context->CreateRenderPass();
    context->CreateSwapchains();
    context->CreatePipelineGFX();
    context->CreateCommandBuffers();
    context->CreateShaders();

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PVLifetime::CreateInstance() {
    auto context = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

    m_instance = std::make_shared<Ir77PVInstance>();

    std::shared_ptr<IIr77PVInstance> instance = std::reinterpret_pointer_cast<IIr77PVInstance>(m_instance);

    instance->DefineAppInfo();

    instance->DefineExtensions();

    instance->DefineCreateInfo();

    instance->DefineCreateInstance();

    std::uint32_t device_count;
    instance->QueryDeviceCount(device_count);

    std::shared_ptr<Ir77MPVMOP<Ir77UInt32>> count = std::make_shared<Ir77MPVMOP<Ir77UInt32>>();

    count->Set(device_count);

    m_operand.at(2) = std::reinterpret_pointer_cast<IIr77Enlisted>(count);

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PVLifetime::EnumeratePhysicalDevices() {
    auto device = std::reinterpret_pointer_cast<IIr77PVDevice>(std::make_shared<Ir77PVDevice>());

    m_devices.emplace(m_current_device, device);

    device->SetInstance(m_instance);

    device->EnumeratePhysicalDevices();

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PVLifetime::CreateSurfaces() {
    std::vector<SDL_Window*> windows = m_windows.at(m_current_device);

    if (windows.size() > 8) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: too many windows.");

    std::vector<std::shared_ptr<IIr77PVSwapchain>> swapchains;
    for (int i = 0; i < windows.size(); i++) {
        auto swapchain = std::reinterpret_pointer_cast<IIr77PVSwapchain>(std::make_shared<Ir77PVSwapchain>());

        swapchains.push_back(swapchain);

        swapchains.back()->SetInstance(m_instance);

        swapchains.back()->SetDevice(m_devices.at(m_current_device));

        swapchains.back()->CreateSurface(windows.at(i));
    }

    m_swapchains.emplace(m_current_device, swapchains);

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PVLifetime::EnumerateDeviceQueues() {
    m_devices.at(m_current_device)->DefineQueueFamilyProps(m_windows.at(m_current_device).at(0));

    m_devices.at(m_current_device)->DefineQueueCreateInfos();

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PVLifetime::CreateLogicalDevices() {
    m_devices.at(m_current_device)->CheckDeviceExtensionSupport();

    m_devices.at(m_current_device)->DefineDeviceInfo() = 0;

    m_devices.at(m_current_device)->CreateDevice() = 0;

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PVLifetime::CreateLayout() {
    auto layout = std::reinterpret_pointer_cast<IIr77PVLayout>(std::make_shared<Ir77PVLayoutStd>());

    m_pipeline_layouts.emplace(m_current_device, layout);

    layout->SetDevice(m_devices.at(m_current_device));

    layout->CreatePipelineLayout();

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PVLifetime::CreateRenderPass() {
    auto render_pass = std::reinterpret_pointer_cast<IIr77PVRenderPass>(std::make_shared<Ir77PVRenderPass>());

    m_render_pass.emplace(m_current_device, render_pass);

    render_pass->SetInstance(m_instance);

    render_pass->SetDevice(m_devices.at(m_current_device));

    render_pass->DefineColorAttachment(m_windows.at(m_current_device).at(0));

    render_pass->DefineColorAttachmentRef();

    render_pass->DefineSubpass();

    render_pass->DefineRenderPass();

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PVLifetime::CreateSwapchains() {
    for (int i = 0; i < m_swapchains.at(m_current_device).size(); i++) {
        m_swapchains.at(m_current_device).at(i)->SetRenderPass(m_render_pass.at(m_current_device));

        m_swapchains.at(m_current_device).at(i)->QuerySwapchainSupport();

        m_swapchains.at(m_current_device).at(i)->SwapSurfaceFormat();

        m_swapchains.at(m_current_device).at(i)->PresentMode();

        m_swapchains.at(m_current_device).at(i)->SurfaceCapabilities();

        m_swapchains.at(m_current_device).at(i)->InitSwapchainInfo();

        m_swapchains.at(m_current_device).at(i)->CreateSwapchain();

        m_swapchains.at(m_current_device).at(i)->InitSwapchainImages();

        m_swapchains.at(m_current_device).at(i)->CreateImageView();

        m_swapchains.at(m_current_device).at(i)->CreateFramebuffers();
    }

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PVLifetime::CreatePipelineGFX() {
    std::vector<SDL_Window*> windows = m_windows.at(m_current_device);

    if (windows.size() > 8) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: too many windows.");

    std::vector<std::shared_ptr<IIr77PVPipeline>> pipelines;
    for (int i = 0; i < windows.size(); i++) {
        auto pipeline = std::reinterpret_pointer_cast<IIr77PVPipeline>(std::make_shared<Ir77PVPipelineGFX>());

        pipeline->SetInstance(m_instance);

        pipeline->SetDevice(m_devices.at(m_current_device));

        pipeline->SetSwapchain(m_swapchains.at(m_current_device).at(i));

        pipeline->SetRenderPass(m_render_pass.at(m_current_device));

        pipeline->SetLayout(m_pipeline_layouts.at(m_current_device));

        pipeline->SetShader(m_shaders.at(m_current_device));

        pipeline->CreatePipeline();

        pipelines.push_back(pipeline);
    }

    m_pipelines.emplace(m_current_device, pipelines);

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PVLifetime::CreateCommandBuffers() {
    std::vector<SDL_Window*> windows = m_windows.at(m_current_device);

    if (windows.size() > 8) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: too many windows.");

    std::vector<std::shared_ptr<IIr77PVCmdBuffer>> cmd_buffers;
    for (int i = 0; i < windows.size(); i++) {
        auto cmd_buffer = std::reinterpret_pointer_cast<IIr77PVCmdBuffer>(std::make_shared<Ir77PVCmdBuffer>());

        cmd_buffer->SetInstance(m_instance);

        cmd_buffer->SetDevice(m_devices.at(m_current_device));

        cmd_buffer->SetSwapchain(m_swapchains.at(m_current_device).at(i));

        cmd_buffer->SetRenderPass(m_render_pass.at(m_current_device));

        cmd_buffer->SetPipeline(m_pipelines.at(m_current_device).at(i));

        cmd_buffer->DefineCommandPool();

        cmd_buffer->RecordCommands();

        cmd_buffers.push_back(cmd_buffer);
    }

    m_command_buffers.emplace(m_current_device, cmd_buffers);

    return Ir77RETURN<Ir77OperationSucceeded>();
}
}  // namespace NSIr77PeregrineV