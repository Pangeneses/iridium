#include "Ir77PeregrineV.hpp"
#include <memory>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"

#include "../runtime/GPU/Ir77PVInstance.hpp"
#include "../runtime/GPU/Ir77PVDevice.hpp"
#include "../runtime/GPU/Ir77PVSwapchain.hpp"
#include "../runtime/Pipeline Layout/Ir77PVLayoutStd.hpp"
#include "../runtime/GPU/Ir77PVRenderPass.hpp"
#include "../runtime/GPU/Ir77PVCmdBuffer.hpp"

#include "../runtime/Pipeline/Ir77PVPipelineGFX.hpp"

#include "../runtime/Pipeline Layout//Ir77PVLayoutStd.hpp"

#include "../runtime/Assets/Ir77PVShader.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

std::shared_ptr<IIr77Return const> Ir77PeregrineV::CreateDeviceInterface(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
    auto lhs_mutable = std::const_pointer_cast<IIr77Operand>(lhs);

    auto context = QueryAs<IIr77PeregrineV>(&GUIDIIr77PeregrineV, lhs_mutable.get());

    if (!context.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid PeregrineV Operand.");

    std::uint32_t device_count;
    context->CreateInstance(device_count);

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::DestroyDeviceInterface(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::AddWindow(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::DestroyWindow(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::CreatePipeline(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::DestroyInstance(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::UpdateShaders(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::ClearShaders(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::UploadVertexToBuffer(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::ClearVertexBuffer(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::UploadMaterialToBuffer(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::ClearMaterialBuffer(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::UploadComputeToBuffer(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::ClearComputeBuffer(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::CreateInstance(std::uint32_t& count) {
    auto context = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

    m_instance = std::make_shared<Ir77PVInstance>();

    std::shared_ptr<IIr77PVInstance> instance = std::reinterpret_pointer_cast<IIr77PVInstance>(m_instance);

    instance->DefineAppInfo();

    instance->DefineExtensions();

    instance->DefineCreateInfo();

    instance->DefineCreateInstance();

    instance->QueryDeviceCount(m_device_count);

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::EnumeratePhysicalDevices() {
    auto device = std::reinterpret_pointer_cast<IIr77PVDevice>(std::make_shared<Ir77PVDevice>());

    m_devices.emplace(m_current_device, device);

    device->SetInstance(m_instance);

    device->EnumeratePhysicalDevices();

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::CreateSurfaces() {
    std::vector<SDL_Window*> windows = m_windows.at(m_current_device);

    if (windows.size() > 8) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: too many windows.");

    std::vector<std::shared_ptr<IIr77PVSwapchain>> swapchains;
    for (int i = 0; i < windows.size(); i++) {
        auto swapchain = std::reinterpret_pointer_cast<IIr77PVSwapchain>(std::make_shared<Ir77PVSwapchain>());

        swapchains.push_back(swapchain);

        swapchains.back()->CreateSurface(windows.at(i));
    }

    m_swapchains.emplace(m_current_device, swapchains);

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::EnumerateDeviceQueues() {
    m_devices.at(m_current_device)->DefineQueueFamilyProps(m_windows.at(m_current_device).at(0));

    m_devices.at(m_current_device)->DefineQueueCreateInfos();

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::CreateLogicalDevices() {
    m_devices.at(m_current_device)->CheckDeviceExtensionSupport();

    m_devices.at(m_current_device)->DefineDeviceInfo() = 0;

    m_devices.at(m_current_device)->CreateDevice() = 0;

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::CreateLayout() {
    auto layout = std::reinterpret_pointer_cast<IIr77PVLayout>(std::make_shared<Ir77PVLayoutStd>());

    m_pipeline_layouts.emplace(m_current_device, layout);

    layout->CreatePipelineLayout();

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::CreateRenderPass() {
    auto render_pass = std::reinterpret_pointer_cast<IIr77PVRenderPass>(std::make_shared<Ir77PVRenderPass>());

    m_render_pass.emplace(m_current_device, render_pass);

    render_pass->DefineColorAttachment(m_windows.at(m_current_device).at(0));

    render_pass->DefineColorAttachmentRef();

    render_pass->DefineSubpass();

    render_pass->DefineRenderPass();

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::CreateSwapchains() {
    for (int i = 0; i < m_swapchains.at(m_current_device).size(); i++) {
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
}  // namespace NSIr77PeregrineV