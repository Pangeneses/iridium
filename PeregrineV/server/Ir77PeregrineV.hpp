#pragma once

#include <vk_mem_alloc.h>
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
#include "../runtime/Buffer/Ir77PVBufferVertex.hpp"
#include "../runtime/Pipeline Layout/Ir77PVLayoutUBO.hpp"
#include "../runtime/Buffer/Ir77PVBufferUBO.hpp"
#include "../runtime/GPU/Ir77PVRenderPass.hpp"
#include "../runtime/Pipeline/Ir77PVPipelineGFX.hpp"

#include "../runtime/Pipeline Layout/Ir77PVLayoutCEF.hpp"
#include "../runtime/Buffer/Ir77PVBufferCEF.hpp"
#include "../runtime/Pipeline/Ir77PVPipelineCEF.hpp"
#include "Ir77PVTypes.hpp"

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

    ~Ir77PeregrineV() {
        for (auto& [device_id, buffers] : m_buffer_cef) {
            for (auto& buf : buffers) {
                std::cerr << "[DEBUG] Ir77PVBufferCEF use_count=" << buf.use_count() << std::endl;
            }
        }

        for (auto& [device_id, allocator] : m_allocators) {
            vmaDestroyAllocator(allocator);
        }
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

    std::shared_ptr<IIr77Return const> IsResizing(bool& resizing) {
        resizing = m_resize_in_progress;

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

    std::shared_ptr<IIr77Return const> CreateAllocator() {
        VkInstance instance;
        m_instance->GetInstance(&instance);

        VkPhysicalDevice phys_device;
        m_devices.at(m_current_device)->GetPhysicalDevice(&phys_device);

        VkDevice device;
        m_devices.at(m_current_device)->GetDevice(&device);

        VmaAllocatorCreateInfo allocator_info{};
        allocator_info.instance = instance;
        allocator_info.physicalDevice = phys_device;
        allocator_info.device = device;
        allocator_info.vulkanApiVersion = VK_API_VERSION_1_3;

        VmaAllocator allocator;
        if (vmaCreateAllocator(&allocator_info, &allocator) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: vmaCreateAllocator failed.");
        }

        m_allocators.emplace(m_current_device, allocator);

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Allocator.");
    }

    std::shared_ptr<IIr77Return const> CreateLayoutUBO() {
        auto layout = std::static_pointer_cast<IIr77PVLayout>(std::make_shared<Ir77PVLayoutUBO>());

        layout->SetDevice(m_devices.at(m_current_device));

        layout->DefineDescriptorSetLayout();

        layout->DefineDescriptorPool(MAX_FRAMES_IN_FLIGHT);

        layout->DefinePipelineLayout();

        m_layouts_ubo.emplace(m_current_device, layout);

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
        std::vector<std::uint32_t> frames;
        frames.resize(m_swapchains.at(m_current_device).size());

        for (int i = 0; i < m_swapchains.at(m_current_device).size(); i++) {
            frames.at(i) = 0;

            m_swapchains.at(m_current_device).at(i)->SetRenderPass(m_render_pass.at(m_current_device));

            m_swapchains.at(m_current_device).at(i)->QuerySwapchainSupport();

            m_swapchains.at(m_current_device).at(i)->SwapSurfaceFormat();

            m_swapchains.at(m_current_device).at(i)->PresentMode();

            m_swapchains.at(m_current_device).at(i)->SurfaceCapabilities();

            m_swapchains.at(m_current_device).at(i)->DefineSwapchain();

            m_swapchains.at(m_current_device).at(i)->InitSwapchainImages();

            m_swapchains.at(m_current_device).at(i)->DefineImageView();

            m_swapchains.at(m_current_device).at(i)->DefineFramebuffers();
        }

        m_current_frames.emplace(m_current_device, frames);

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

            pipeline->SetLayout(m_layouts_ubo.at(m_current_device));

            pipeline->SetShader(m_shaders.at(m_current_device));

            pipeline->CreatePipeline();

            pipelines.push_back(pipeline);
        }

        m_pipelines_gfx.emplace(m_current_device, pipelines);

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create GFX Pipeline.");
    }

    std::shared_ptr<IIr77Return const> CreatePipelineCEF() {
        std::vector<SDL_Window*> windows = m_windows.at(m_current_device);

        if (windows.size() > 8) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: too many windows.");

        std::vector<std::shared_ptr<IIr77PVPipeline>> pipelines;
        for (int i = 0; i < windows.size(); i++) {
            auto pipeline = std::static_pointer_cast<IIr77PVPipeline>(std::make_shared<Ir77PVPipelineCEF>());

            pipeline->SetInstance(m_instance);

            pipeline->SetDevice(m_devices.at(m_current_device));

            pipeline->SetSwapchain(m_swapchains.at(m_current_device).at(i));

            pipeline->SetRenderPass(m_render_pass.at(m_current_device));

            pipeline->SetLayout(m_layouts_cef.at(m_current_device));

            pipeline->SetShader(m_shaders.at(m_current_device));

            pipeline->CreatePipeline();

            pipelines.push_back(pipeline);
        }

        m_pipelines_cef.emplace(m_current_device, pipelines);

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create CEF Pipeline.");
    }

   private:
    friend class Ir77PVPaint;
    friend class Ir77PVAsset;
    friend class Ir77PVCEF;

    std::uint32_t m_device_count{0};

    std::uint64_t m_current_device{0};

    std::map<std::uint64_t, std::vector<std::uint32_t>> m_current_frames;

    std::shared_ptr<IIr77PVInstance> m_instance;

    std::map<std::uint64_t, std::shared_ptr<IIr77PVDevice>> m_devices;

    std::map<std::uint64_t, std::vector<SDL_Window*>> m_windows;

    std::map<std::uint64_t, VmaAllocator> m_allocators;

    std::map<std::uint64_t, std::vector<std::shared_ptr<IIr77PVCmdBuffer>>> m_command_buffers;

    std::map<std::uint64_t, std::shared_ptr<IIr77PVShader>> m_shaders;

    std::map<std::uint64_t, std::vector<std::shared_ptr<IIr77Enlisted>>> m_compute;

    bool m_resize_in_progress{false};

    /*************************** GFX ***********************************/
    std::map<std::uint64_t, std::vector<std::shared_ptr<IIr77PVPipeline>>> m_pipelines_gfx;

    std::map<std::uint64_t, std::vector<std::shared_ptr<Ir77PVBufferVertex>>> m_buffer_vertex;

    std::map<std::uint64_t, std::shared_ptr<IIr77PVLayout>> m_layouts_ubo;

    std::map<std::uint64_t, std::vector<std::shared_ptr<Ir77PVBufferUBO>>> m_buffer_ubo;

    std::map<std::uint64_t, std::vector<std::shared_ptr<IIr77PVSwapchain>>> m_swapchains;

    std::map<std::uint64_t, std::shared_ptr<IIr77PVRenderPass>> m_render_pass;

    /*************************** CEF ***********************************/
    std::map<std::uint64_t, std::vector<std::shared_ptr<IIr77PVPipeline>>> m_pipelines_cef;

    std::map<std::uint64_t, std::shared_ptr<IIr77PVLayout>> m_layouts_cef;

    std::map<std::uint64_t, std::vector<std::shared_ptr<Ir77PVBufferCEF>>> m_buffer_cef;

    /**********************************************************************/
};
}  // namespace NSIr77PeregrineV