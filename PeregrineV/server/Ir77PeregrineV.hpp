#pragma once

#include <SDL3/SDL_video.h>
#include <map>
#include <memory>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "IIr77PeregrineV.hpp"

#include "../interface/IIr77PVInstance.hpp"
#include "../interface/IIr77PVDevice.hpp"
#include "../interface/IIr77PVSwapchain.hpp"
#include "../interface/IIr77PVLayout.hpp"
#include "../interface/IIr77PVRenderPass.hpp"
#include "../interface/IIr77PVPipeline.hpp"
#include "../interface/IIr77PVCmdBuffer.hpp"

#include "../runtime/GPU/Ir77PVInstance.hpp"
#include "../runtime/GPU/Ir77PVDevice.hpp"
#include "../runtime/GPU/Ir77PVSwapchain.hpp"
#include "../runtime/Pipeline Layout/Ir77PVLayoutStd.hpp"
#include "../runtime/GPU/Ir77PVRenderPass.hpp"
#include "../runtime/Pipeline/Ir77PVPipelineGFX.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77PeregrineV : public Ir77Enlisted, public IIr77PeregrineV, public std::enable_shared_from_this<Ir77PeregrineV> {
   public:
    Ir77PeregrineV() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PeregrineV>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PeregrineV>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    IIr77GUID* const QueryInterface(IIr77GUID const* iid, std::shared_ptr<void>& obj) {
        if (iid == &GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == &GUIDIIr77PeregrineV)
            obj = std::shared_ptr<IIr77PeregrineV>(shared_from_this(), static_cast<IIr77PeregrineV*>(this));

        else if (iid == &GUIDIr77PeregrineV)
            obj = std::shared_ptr<Ir77PeregrineV>(shared_from_this(), static_cast<Ir77PeregrineV*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> SetCurrentDevice(std::uint64_t const& device_id) {
        m_current_device = device_id;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CreateInstance() {
        auto instance = std::make_shared<Ir77PVInstance>();

        m_instance = std::static_pointer_cast<IIr77PVInstance>(instance);

        instance->DefineAppInfo();

        instance->DefineExtensions();

        instance->DefineCreateInfo();

        instance->DefineCreateInstance();

        instance->QueryDeviceCount(m_device_count);

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Instance.");
    }

    std::shared_ptr<IIr77Return const> EnumeratePhysicalDevices(std::map<std::uint64_t, std::shared_ptr<IIr77PVDevice>>& devices) {
        for (int i = 0; i < m_device_count; i++) {
            auto device = std::make_shared<Ir77PVDevice>();

            auto idevice = std::static_pointer_cast<IIr77PVDevice>(device);

            m_devices.emplace(m_current_device, idevice);

            idevice->SetInstance(m_instance);

            idevice->EnumeratePhysicalDevices(1);  // change to i later
        }

        devices = m_devices;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Enumerate Devices.");
    }

    std::shared_ptr<IIr77Return const> CreateSurfaces(std::map<std::uint64_t, std::vector<SDL_Window*>> const& windows) {
        m_windows = windows;

        std::vector<SDL_Window*> device_windows = m_windows.at(m_current_device);

        if (windows.size() > 8) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: too many windows.");

        std::vector<std::shared_ptr<IIr77PVSwapchain>> surfaces;
        for (int i = 0; i < device_windows.size(); i++) {
            auto surface = std::make_shared<Ir77PVSwapchain>();

            auto isurface = std::static_pointer_cast<IIr77PVSwapchain>(surface);

            surfaces.push_back(isurface);

            surfaces.back()->SetInstance(m_instance);

            surfaces.back()->SetDevice(m_devices.at(m_current_device));

            surfaces.back()->CreateSurface(device_windows.at(i));
        }

        m_swapchains.emplace(m_current_device, surfaces);

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Surfaces.");
    }

    std::shared_ptr<IIr77Return const> EnumerateDeviceQueues() {
        m_devices.at(m_current_device)->DefineQueueFamilyProps(m_windows.at(m_current_device).at(0));

        m_devices.at(m_current_device)->DefineQueueCreateInfos();

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Enumerate Device Queue.");
    }

    std::shared_ptr<IIr77Return const> CreateLogicalDevices() {
        m_devices.at(m_current_device)->CheckDeviceExtensionSupport();

        m_devices.at(m_current_device)->DefineDeviceInfo();

        m_devices.at(m_current_device)->DefineDevice();

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Logical Devices.");
    }

    std::shared_ptr<IIr77Return const> CreateLayout() {
        auto layout = std::static_pointer_cast<IIr77PVLayout>(std::make_shared<Ir77PVLayoutStd>());

        m_pipeline_layouts.emplace(m_current_device, layout);

        layout->SetDevice(m_devices.at(m_current_device));

        layout->DefinePipelineLayout();

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Layout.");
    }

    std::shared_ptr<IIr77Return const> CreateRenderPass() {
        auto render_pass = std::static_pointer_cast<IIr77PVRenderPass>(std::make_shared<Ir77PVRenderPass>());

        m_render_pass.emplace(m_current_device, render_pass);

        render_pass->SetInstance(m_instance);

        render_pass->SetDevice(m_devices.at(m_current_device));

        render_pass->DefineColorAttachment(m_windows.at(m_current_device).at(0));

        render_pass->DefineColorAttachmentRef();

        render_pass->DefineSubpass();

        render_pass->DefineRenderPass();

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Render Pass.");
    }

    std::shared_ptr<IIr77Return const> CreateSwapchains() {
        for (int i = 0; i < m_swapchains.at(m_current_device).size(); i++) {
            m_swapchains.at(m_current_device).at(i)->SetRenderPass(m_render_pass.at(m_current_device));

            m_swapchains.at(m_current_device).at(i)->QuerySwapchainSupport();

            m_swapchains.at(m_current_device).at(i)->SwapSurfaceFormat();

            m_swapchains.at(m_current_device).at(i)->PresentMode();

            m_swapchains.at(m_current_device).at(i)->SurfaceCapabilities();

            m_swapchains.at(m_current_device).at(i)->InitSwapchainInfo();

            m_swapchains.at(m_current_device).at(i)->DefineSwapchain();

            m_swapchains.at(m_current_device).at(i)->InitSwapchainImages();

            m_swapchains.at(m_current_device).at(i)->DefineImageView();

            m_swapchains.at(m_current_device).at(i)->DefineFramebuffers();
        }

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Swapchains.");
    }

    std::shared_ptr<IIr77Return const> CreatePipelineGFX() {
        std::vector<SDL_Window*> windows = m_windows.at(m_current_device);

        if (windows.size() > 8) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: too many windows.");

        std::vector<std::shared_ptr<IIr77PVPipeline>> pipelines;
        for (int i = 0; i < windows.size(); i++) {
            auto pipeline = std::static_pointer_cast<IIr77PVPipeline>(std::make_shared<Ir77PVPipelineGFX>());

            pipeline->SetInstance(m_instance);

            pipeline->SetDevice(m_devices.at(m_current_device));

            pipeline->SetSwapchain(m_swapchains.at(m_current_device).at(i));

            pipeline->SetRenderPass(m_render_pass.at(m_current_device));

            pipeline->SetLayout(m_pipeline_layouts.at(m_current_device));

            pipeline->SetShader(m_shaders.at(m_current_device));

            pipeline->DefinePipeline();

            pipelines.push_back(pipeline);
        }

        m_pipelines.emplace(m_current_device, pipelines);

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create GFX Pipeline.");
    }

   private:
    friend class Ir77PVPaint;
    friend class Ir77PVAsset;

    std::uint32_t m_device_count;

    std::uint64_t m_current_device;

    std::shared_ptr<IIr77PVInstance> m_instance;

    std::map<std::uint64_t, std::shared_ptr<IIr77PVDevice>> m_devices;

    std::map<std::uint64_t, std::vector<SDL_Window*>> m_windows;

    std::map<std::uint64_t, std::vector<std::shared_ptr<IIr77PVSwapchain>>> m_swapchains;

    std::map<std::uint64_t, std::shared_ptr<IIr77PVLayout>> m_pipeline_layouts;

    std::map<std::uint64_t, std::shared_ptr<IIr77PVRenderPass>> m_render_pass;

    std::map<std::uint64_t, std::vector<std::shared_ptr<IIr77PVPipeline>>> m_pipelines;

    std::map<std::uint64_t, std::vector<std::shared_ptr<IIr77PVCmdBuffer>>> m_command_buffers;

    std::map<std::uint64_t, std::shared_ptr<IIr77PVShader>> m_shaders;

    std::map<std::uint64_t, std::vector<std::shared_ptr<IIr77Enlisted>>> m_compute;
};
}  // namespace NSIr77PeregrineV