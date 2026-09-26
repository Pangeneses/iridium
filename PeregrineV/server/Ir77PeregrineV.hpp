#pragma once

#include <vk_mem_alloc.h>
#include <SDL3/SDL_video.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77PVInstance.hpp"
#include "../interface/IIr77PVDevice.hpp"
#include "../interface/IIr77PVSwapchain.hpp"
#include "../interface/IIr77PVRenderPass.hpp"
#include "../interface/IIr77PVPipeline.hpp"
#include "../interface/IIr77PVLayout.hpp"
#include "../interface/IIr77PVBuffer.hpp"
#include "../interface/IIr77PVDescriptorSet.hpp"
#include "../interface/IIr77PVOverlay.hpp"
#include "../interface/IIr77PVTexture.hpp"
#include "../interface/IIr77PVCmdBuffer.hpp"

#include "../runtime/Ir77PVInstance.hpp"
#include "../runtime/Ir77PVDevice.hpp"
#include "../runtime/Ir77PVSwapchain.hpp"
#include "../runtime/Ir77PVRenderPass.hpp"
#include "../runtime/Ir77PVPipeline.hpp"
#include "../runtime/Ir77PVLayout.hpp"
#include "../runtime/Ir77PVBuffer.hpp"
#include "../runtime/Ir77PVDescriptorSet.hpp"
#include "../runtime/Ir77PVOverlay.hpp"
#include "../runtime/Ir77PVTexture.hpp"

#include "Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// Frame-level buffers every window owns, one Dynamic copy per frame in flight.
// Mesh vertex/index buffers are per-asset and live in m_buffers_vertex / m_buffers_index (filled by Ir77PVAsset).
enum class Ir77PVBufferSlot : std::uint8_t { Camera, Lights, Instances, ShadowMatrices, Bones, Indirect };

// Default textures bound to image slots that have no real texture yet
enum class Ir77PVPlaceholderKind : std::uint8_t { White, Normal };

class Ir77PeregrineV : public Ir77Enlisted, public std::enable_shared_from_this<Ir77PeregrineV> {
   public:
    Ir77PeregrineV() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    // Everything holding VMA memory must die before its allocator, so the maps are released
    // explicitly here instead of after this body runs (member destruction happens after the allocator is gone).
    ~Ir77PeregrineV() {
        for (auto& [device_id, device] : m_devices) {
            VkDevice vk_device{VK_NULL_HANDLE};
            device->GetDevice(&vk_device);
            if (vk_device != VK_NULL_HANDLE) vkDeviceWaitIdle(vk_device);
        }

        m_command_buffers.clear();
        m_descriptor_sets_global.clear();
        m_descriptor_sets_pass.clear();
        m_buffers_frame.clear();
        m_buffers_vertex.clear();
        m_buffers_index.clear();
        m_overlays.clear();
        m_pipelines.clear();
        m_layouts.clear();

        m_placeholders.clear();

        for (auto& [device_id, allocator] : m_allocators) vmaDestroyAllocator(allocator);
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

    // -------------------------------------------------------------------------------------------------------------------------------------
    // instance / device / surface -- unchanged order
    // -------------------------------------------------------------------------------------------------------------------------------------
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
        for (std::size_t i = 0; i < m_device_count; i++) {
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

        std::vector<SDL_Window*> const& device_windows = m_windows.at(m_current_device);

        if (device_windows.size() > MAX_WINDOWS) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PeregrineV: too many windows.");

        std::vector<std::shared_ptr<IIr77PVSwapchain>> surfaces{};
        for (std::size_t i = 0; i < device_windows.size(); i++) {
            auto surface = std::static_pointer_cast<IIr77PVSwapchain>(std::make_shared<Ir77PVSwapchain>());

            surface->SetInstance(m_instance);

            surface->SetDevice(m_devices.at(m_current_device));

            surface->CreateSurface(device_windows.at(i));

            surfaces.push_back(surface);
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

        VmaAllocator allocator{VK_NULL_HANDLE};
        if (vmaCreateAllocator(&allocator_info, &allocator) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PeregrineV: vmaCreateAllocator failed.");
        }

        m_allocators.emplace(m_current_device, allocator);

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Allocator.");
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // layouts -- one per kind, shared by every window on the device
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> CreateLayouts() {
        std::uint32_t const windows = static_cast<std::uint32_t>(m_windows.at(m_current_device).size());

        // Every pool allocates `max_count` sets of each of its set layouts.
        // Static also serves the per-window global + pass sets and every material; Skinned serves bones sets.
        std::uint32_t const frame_sets = windows * MAX_FRAMES_IN_FLIGHT;

        std::map<Ir77PVLayoutKind, std::uint32_t> const capacities{
            {Ir77PVLayoutKind::Static, std::max(frame_sets, MATERIAL_CAPACITY)},
            {Ir77PVLayoutKind::Skinned, std::max(frame_sets, SKELETON_CAPACITY)},
            {Ir77PVLayoutKind::Shadow, frame_sets},
            {Ir77PVLayoutKind::ShadowSkinned, frame_sets},
            {Ir77PVLayoutKind::CEF, std::max<std::uint32_t>(windows, 1)},
        };

        std::map<Ir77PVLayoutKind, std::shared_ptr<IIr77PVLayout>> layouts{};

        for (auto const& [kind, capacity] : capacities) {
            auto layout = std::static_pointer_cast<IIr77PVLayout>(std::make_shared<Ir77PVLayout>());

            layout->SetDevice(m_devices.at(m_current_device));

            if (layout->SetKind(kind)->ID() != &GUIDIr77OperationSucceeded ||
                layout->DefineDescriptorSetLayout()->ID() != &GUIDIr77OperationSucceeded ||
                layout->DefineDescriptorPool(capacity)->ID() != &GUIDIr77OperationSucceeded ||
                layout->DefinePipelineLayout()->ID() != &GUIDIr77OperationSucceeded) {
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PeregrineV: layout creation failed.");
            }

            layouts.emplace(kind, layout);
        }

        m_layouts[m_current_device] = layouts;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Layouts.");
    }
    // Main pass (swapchain color + depth). The color format must equal the swapchain's, so window 0's
    // surface format is resolved first. Every window on the device must share this format.
    std::shared_ptr<IIr77Return const> CreateRenderPass() {
        auto const& swapchain = m_swapchains.at(m_current_device).at(0);

        if (swapchain->QuerySwapchainSupport()->ID() != &GUIDIr77OperationSucceeded ||
            swapchain->SwapSurfaceFormat()->ID() != &GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PeregrineV: surface format query failed.");

        VkSurfaceFormatKHR surface_format{};
        swapchain->GetSurfaceFormat(surface_format);

        auto render_pass = std::static_pointer_cast<IIr77PVRenderPass>(std::make_shared<Ir77PVRenderPass>());

        render_pass->SetInstance(m_instance);

        render_pass->SetDevice(m_devices.at(m_current_device));

        render_pass->SetKind(Ir77PVRenderPassKind::Main);

        render_pass->SetColorFormat(surface_format.format);

        if (render_pass->CreateRenderPass()->ID() != &GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PeregrineV: main render pass failed.");

        m_render_pass[m_current_device] = render_pass;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Render Pass.");
    }

    // Depth-only pass for shadow maps. Once this exists, CreatePipelines builds the Shadow kinds.
    std::shared_ptr<IIr77Return const> CreateShadowRenderPass() {
        auto render_pass = std::static_pointer_cast<IIr77PVRenderPass>(std::make_shared<Ir77PVRenderPass>());

        render_pass->SetInstance(m_instance);

        render_pass->SetDevice(m_devices.at(m_current_device));

        render_pass->SetKind(Ir77PVRenderPassKind::Shadow);

        if (render_pass->CreateRenderPass()->ID() != &GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PeregrineV: shadow render pass failed.");

        m_render_pass_shadow[m_current_device] = render_pass;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Shadow Render Pass.");
    }
    std::shared_ptr<IIr77Return const> CreateSwapchains() {
        auto& swapchains = m_swapchains.at(m_current_device);

        std::vector<std::uint32_t> frames(swapchains.size(), 0);

        for (auto& swapchain : swapchains) {
            swapchain->SetRenderPass(m_render_pass.at(m_current_device));

            swapchain->QuerySwapchainSupport();

            swapchain->SwapSurfaceFormat();

            swapchain->PresentMode();

            swapchain->SurfaceCapabilities();

            swapchain->DefineSwapchain();

            swapchain->InitSwapchainImages();

            swapchain->DefineImageView();

            swapchain->DefineFramebuffers();
        }

        m_current_frames[m_current_device] = frames;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Swapchains.");
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // placeholder texture -- lets every image binding be written before real textures exist
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> CreatePlaceholderTexture() {
        VkDevice device;
        m_devices.at(m_current_device)->GetDevice(&device);

        VkQueue queue{VK_NULL_HANDLE};
        std::uint32_t family{0};
        if (!FindPresentQueue(&queue, &family)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PeregrineV: no presentation queue for placeholder.");

        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        pool_info.queueFamilyIndex = family;

        VkCommandPool pool{VK_NULL_HANDLE};
        if (vkCreateCommandPool(device, &pool_info, nullptr, &pool) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PeregrineV: placeholder command pool failed.");

        // white: albedo / ORM / AO / emissive / env / shadow -- neutral for multiply and "fully lit"
        // flat normal: tangent-space (0, 0, 1) encoded as (128, 128, 255)
        std::map<Ir77PVPlaceholderKind, std::uint32_t> const values{
            {Ir77PVPlaceholderKind::White, 0xFFFFFFFF},
            {Ir77PVPlaceholderKind::Normal, 0xFFFF8080},
        };

        std::map<Ir77PVPlaceholderKind, std::shared_ptr<IIr77PVTexture>> placeholders{};

        for (auto const& [kind, rgba] : values) {
            auto texture = std::static_pointer_cast<IIr77PVTexture>(std::make_shared<Ir77PVTexture>());

            texture->SetDevice(m_devices.at(m_current_device));
            texture->SetAllocator(m_allocators.at(m_current_device));
            texture->SetKind(Ir77PVTextureKind::Data);

            if (texture->CreateSolid(pool, queue, rgba)->ID() != &GUIDIr77OperationSucceeded) {
                vkDestroyCommandPool(device, pool, nullptr);
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PeregrineV: placeholder texture failed.");
            }

            placeholders.emplace(kind, texture);
        }

        vkDestroyCommandPool(device, pool, nullptr);

        m_placeholders[m_current_device] = placeholders;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Placeholder Texture.");
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // frame buffers -- one set of slots per window, Dynamic, one copy per frame in flight
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> CreateFrameBuffers(Ir77PVFrameSizes const& sizes = Ir77PVFrameSizes{}) {
        struct SlotSpec {
            Ir77PVBufferSlot slot{Ir77PVBufferSlot::Camera};
            VkBufferUsageFlags usage{0};
            VkDeviceSize size{0};
        };

        std::vector<SlotSpec> const specs{
            {Ir77PVBufferSlot::Camera, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizes.camera},
            {Ir77PVBufferSlot::Lights, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizes.lights},
            {Ir77PVBufferSlot::Instances, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizes.instances},
            {Ir77PVBufferSlot::ShadowMatrices, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizes.shadow_matrices},
            {Ir77PVBufferSlot::Bones, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizes.bones},
            {Ir77PVBufferSlot::Indirect, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizes.indirect},
        };

        std::size_t const windows = m_windows.at(m_current_device).size();

        std::vector<std::map<Ir77PVBufferSlot, std::shared_ptr<IIr77PVBuffer>>> per_window(windows);

        for (std::size_t w = 0; w < windows; w++) {
            for (auto const& spec : specs) {
                auto buffer = std::static_pointer_cast<IIr77PVBuffer>(std::make_shared<Ir77PVBuffer>());

                buffer->SetDevice(m_devices.at(m_current_device));

                buffer->SetAllocator(m_allocators.at(m_current_device));

                buffer->SetUsage(spec.usage);

                buffer->SetMode(Ir77PVBufferMode::Dynamic);

                if (buffer->CreateResources(spec.size, MAX_FRAMES_IN_FLIGHT)->ID() != &GUIDIr77OperationSucceeded) {
                    return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PeregrineV: frame buffer creation failed.");
                }

                per_window[w].emplace(spec.slot, buffer);
            }
        }

        m_buffers_frame[m_current_device] = per_window;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Frame Buffers.");
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // descriptor sets -- set 0 global and set 1 pass per window, allocated from the Static layout.
    // Identical BindingsGlobal() / BindingsPass() keep them compatible with Skinned and Shadow layouts.
    // Requires CreateLayouts, CreatePlaceholderTexture, CreateFrameBuffers.
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> CreateDescriptorSets() {
        auto const& layout = m_layouts.at(m_current_device).at(Ir77PVLayoutKind::Static);
        auto const& frame_buffers = m_buffers_frame.at(m_current_device);

        VkDescriptorImageInfo const placeholder = GetPlaceholderImageInfo();
        if (placeholder.imageView == VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PeregrineV: CreatePlaceholderTexture not called.");

        std::vector<std::shared_ptr<IIr77PVDescriptorSet>> globals{};
        std::vector<std::shared_ptr<IIr77PVDescriptorSet>> passes{};

        for (auto const& slots : frame_buffers) {
            // set 0 -- camera, lights, instances, env map
            auto global = std::static_pointer_cast<IIr77PVDescriptorSet>(std::make_shared<Ir77PVDescriptorSet>());

            global->SetDevice(m_devices.at(m_current_device));
            global->SetLayout(layout, 0);

            if (global->CreateSets(MAX_FRAMES_IN_FLIGHT)->ID() != &GUIDIr77OperationSucceeded) {
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PeregrineV: global set allocation failed.");
            }

            global->BindBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, slots.at(Ir77PVBufferSlot::Camera));
            global->BindBuffer(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, slots.at(Ir77PVBufferSlot::Lights));
            global->BindBuffer(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, slots.at(Ir77PVBufferSlot::Instances));
            global->BindImage(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, placeholder);  // env / IBL

            if (global->Write()->ID() != &GUIDIr77OperationSucceeded) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PeregrineV: global set write failed.");

            // set 1 -- shadow map, shadow matrices
            auto pass = std::static_pointer_cast<IIr77PVDescriptorSet>(std::make_shared<Ir77PVDescriptorSet>());

            pass->SetDevice(m_devices.at(m_current_device));
            pass->SetLayout(layout, 1);

            if (pass->CreateSets(MAX_FRAMES_IN_FLIGHT)->ID() != &GUIDIr77OperationSucceeded) {
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PeregrineV: pass set allocation failed.");
            }

            pass->BindImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, placeholder);  // shadow map
            pass->BindBuffer(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, slots.at(Ir77PVBufferSlot::ShadowMatrices));

            if (pass->Write()->ID() != &GUIDIr77OperationSucceeded) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PeregrineV: pass set write failed.");

            globals.push_back(global);
            passes.push_back(pass);
        }

        m_descriptor_sets_global[m_current_device] = globals;
        m_descriptor_sets_pass[m_current_device] = passes;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Descriptor Sets.");
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // pipelines -- per window, per kind. Shadow kinds are skipped until a shadow render pass exists.
    // Only request kinds whose shaders are loaded (Skinned / Shadow shaders don't exist yet).
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> CreatePipelines(std::vector<Ir77PVPipelineKind> const& kinds = {Ir77PVPipelineKind::Static,
                                                                                                         Ir77PVPipelineKind::Transparent,
                                                                                                         Ir77PVPipelineKind::CEF}) {
        std::size_t const windows = m_windows.at(m_current_device).size();

        if (windows > MAX_WINDOWS) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PeregrineV: too many windows.");

        bool const has_shadow_pass = m_render_pass_shadow.count(m_current_device) > 0;

        std::vector<std::map<Ir77PVPipelineKind, std::shared_ptr<IIr77PVPipeline>>> per_window(windows);

        for (std::size_t w = 0; w < windows; w++) {
            for (auto const kind : kinds) {
                bool const shadow = kind == Ir77PVPipelineKind::Shadow || kind == Ir77PVPipelineKind::ShadowSkinned;
                if (shadow && !has_shadow_pass) continue;

                auto ipipeline = std::static_pointer_cast<IIr77PVPipeline>(std::make_shared<Ir77PVPipeline>());

                ipipeline->SetInstance(m_instance);

                ipipeline->SetDevice(m_devices.at(m_current_device));

                ipipeline->SetSwapchain(m_swapchains.at(m_current_device).at(w));

                ipipeline->SetRenderPass(shadow ? m_render_pass_shadow.at(m_current_device) : m_render_pass.at(m_current_device));

                ipipeline->SetLayout(m_layouts.at(m_current_device).at(LayoutKindFor(kind)));

                ipipeline->SetShader(m_shaders.at(m_current_device));

                ipipeline->SetKind(kind);

                if (ipipeline->CreatePipeline()->ID() != &GUIDIr77OperationSucceeded) {
                    return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PeregrineV: pipeline creation failed.");
                }

                per_window[w].emplace(kind, ipipeline);
            }
        }

        m_pipelines[m_current_device] = per_window;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Pipelines.");
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // lookups for Paint / Asset / CEF
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> GetLayout(Ir77PVLayoutKind const& kind, std::shared_ptr<IIr77PVLayout>& layout) {
        auto const device = m_layouts.find(m_current_device);
        if (device == m_layouts.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PeregrineV: no layouts for device.");

        auto const found = device->second.find(kind);
        if (found == device->second.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PeregrineV: layout kind not created.");

        layout = found->second;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetPipeline(std::size_t const& window, Ir77PVPipelineKind const& kind, std::shared_ptr<IIr77PVPipeline>& pipeline) {
        auto const device = m_pipelines.find(m_current_device);
        if (device == m_pipelines.end() || window >= device->second.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PeregrineV: no pipelines for window.");

        auto const found = device->second[window].find(kind);
        if (found == device->second[window].end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PeregrineV: pipeline kind not created.");

        pipeline = found->second;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetFrameBuffer(std::size_t const& window, Ir77PVBufferSlot const& slot, std::shared_ptr<IIr77PVBuffer>& buffer) {
        auto const device = m_buffers_frame.find(m_current_device);
        if (device == m_buffers_frame.end() || window >= device->second.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PeregrineV: no frame buffers for window.");

        buffer = device->second[window].at(slot);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    VkDescriptorImageInfo GetPlaceholderImageInfo(Ir77PVPlaceholderKind const& kind = Ir77PVPlaceholderKind::White) const {
        VkDescriptorImageInfo info{};

        auto const device = m_placeholders.find(m_current_device);
        if (device == m_placeholders.end()) return info;

        auto const found = device->second.find(kind);
        if (found == device->second.end()) return info;

        found->second->GetImageInfo(&info);

        return info;
    }

   private:
    static Ir77PVLayoutKind LayoutKindFor(Ir77PVPipelineKind const& kind) {
        switch (kind) {
            case Ir77PVPipelineKind::Skinned:
                return Ir77PVLayoutKind::Skinned;
            case Ir77PVPipelineKind::Shadow:
                return Ir77PVLayoutKind::Shadow;
            case Ir77PVPipelineKind::ShadowSkinned:
                return Ir77PVLayoutKind::ShadowSkinned;
            case Ir77PVPipelineKind::CEF:
                return Ir77PVLayoutKind::CEF;
            case Ir77PVPipelineKind::Static:
            case Ir77PVPipelineKind::Transparent:
            default:
                return Ir77PVLayoutKind::Static;
        }
    }

    bool FindPresentQueue(VkQueue* queue, std::uint32_t* family) {
        std::vector<Ir77PVQueueFamily> queue_families{};
        m_devices.at(m_current_device)->GetQueueFamilies(queue_families);

        for (std::size_t i = 0; i < queue_families.size(); i++) {
            if (queue_families[i].presentation == VK_TRUE) {
                *queue = queue_families[i].queue;
                *family = static_cast<std::uint32_t>(i);
                return true;
            }
        }

        return false;
    }

   private:
    friend class Ir77PVPaint;
    friend class Ir77PVAsset;
    friend class Ir77PVCEF;

    static constexpr std::size_t MAX_WINDOWS = 8;

    static constexpr std::uint32_t MATERIAL_CAPACITY = 256;

    static constexpr std::uint32_t SKELETON_CAPACITY = 64;

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

    /*************************** passes *********************************/
    std::map<std::uint64_t, std::vector<std::shared_ptr<IIr77PVSwapchain>>> m_swapchains;

    std::map<std::uint64_t, std::shared_ptr<IIr77PVRenderPass>> m_render_pass;

    std::map<std::uint64_t, std::shared_ptr<IIr77PVRenderPass>> m_render_pass_shadow;

    /*************************** layouts / pipelines ********************/
    // [device][kind]
    std::map<std::uint64_t, std::map<Ir77PVLayoutKind, std::shared_ptr<IIr77PVLayout>>> m_layouts;

    // [device][window][kind]
    std::map<std::uint64_t, std::vector<std::map<Ir77PVPipelineKind, std::shared_ptr<IIr77PVPipeline>>>> m_pipelines;

    /*************************** buffers / sets *************************/
    // [device][window][slot]
    std::map<std::uint64_t, std::vector<std::map<Ir77PVBufferSlot, std::shared_ptr<IIr77PVBuffer>>>> m_buffers_frame;

    // [device][window]
    std::map<std::uint64_t, std::vector<std::shared_ptr<IIr77PVDescriptorSet>>> m_descriptor_sets_global;

    std::map<std::uint64_t, std::vector<std::shared_ptr<IIr77PVDescriptorSet>>> m_descriptor_sets_pass;

    // [device][mesh] -- Static buffers, filled by Ir77PVAsset
    std::map<std::uint64_t, std::vector<std::shared_ptr<IIr77PVBuffer>>> m_buffers_vertex;

    std::map<std::uint64_t, std::vector<std::shared_ptr<IIr77PVBuffer>>> m_buffers_index;

    // [device][kind]
    std::map<std::uint64_t, std::map<Ir77PVPlaceholderKind, std::shared_ptr<IIr77PVTexture>>> m_placeholders;

    /*************************** CEF ************************************/
    // [device][window]
    std::map<std::uint64_t, std::vector<std::shared_ptr<IIr77PVOverlay>>> m_overlays;

    /**********************************************************************/
};
}  // namespace NSIr77PeregrineV