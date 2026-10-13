#pragma once

#include <vk_mem_alloc.h>
#include <SDL3/SDL_video.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
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
#include "../runtime/Ir77PVTexture.hpp"

#include "../types/Ir77PVTypes.hpp"

#include "IDIr77PVContext.hpp"

#include "../types/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

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
        m_swapchains.clear();
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

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIr77PeregrineV)
            obj = std::shared_ptr<Ir77PeregrineV>(shared_from_this(), static_cast<Ir77PeregrineV*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> SetCurrentDevice(std::uint64_t const& device_id) {
        m_current_device = device_id;

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

    std::shared_ptr<IIr77Return const> EnumeratePhysicalDevices() {
        for (std::size_t i = 0; i < m_device_count; i++) {
            auto device = std::static_pointer_cast<IIr77PVDevice>(std::make_shared<Ir77PVDevice>());

            if (i == 0)
                m_devices.emplace(ID_DEVICE_001, device);
            else if (i == 1)
                m_devices.emplace(ID_DEVICE_002, device);
            else if (i == 2)
                m_devices.emplace(ID_DEVICE_003, device);
            else if (i == 3)
                m_devices.emplace(ID_DEVICE_004, device);
            else if (i == 4)
                m_devices.emplace(ID_DEVICE_005, device);
            else if (i == 5)
                m_devices.emplace(ID_DEVICE_006, device);
            else if (i == 6)
                m_devices.emplace(ID_DEVICE_007, device);
            else if (i == 7)
                m_devices.emplace(ID_DEVICE_008, device);

            device->SetInstance(m_instance);

            device->EnumeratePhysicalDevices(i);  // change to i later
        }

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Enumerate Devices.");
    }

    // picks and creates the device that drives the display. Candidates need VK_KHR_swapchain and a graphics queue that can present
    // to an SDL window; they're tried discrete > integrated > virtual > other > CPU, and the first whose logical device is created wins.
    // Multi-device allocation comes with in-app setup.
    std::shared_ptr<IIr77Return const> CreateDisplayDevice(std::vector<SDL_Window*> const& windows, std::uint64_t& device_id) {
        if (windows.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "CreateDisplayDevice: no window.");

        struct Candidate {
            std::uint64_t id{0};
            int rank{-1};
            std::string name{};
        };

        std::vector<Candidate> candidates{};

        for (auto const& [id, device] : m_devices) {
            Candidate c{};
            c.id = id;

            if (device->QueryDisplaySupport(c.rank, c.name)->ID() == GUIDIr77OperationSucceeded) candidates.push_back(c);
        }

        std::stable_sort(candidates.begin(), candidates.end(), [](Candidate const& a, Candidate const& b) { return a.rank > b.rank; });

        std::string skipped{};

        for (Candidate const& c : candidates) {
            m_current_device = c.id;
            m_windows[c.id] = windows;  // EnumerateDeviceQueues reads the device's first window

            auto result = EnumerateDeviceQueues();
            if (result->ID() == GUIDIr77OperationSucceeded) result = CreateLogicalDevices();

            if (result->ID() == GUIDIr77OperationSucceeded) {
                device_id = c.id;

                if (c.rank == 0)
                    return Ir77RETURN<Ir77OperationSucceeded>(
                        this, "CreateDisplayDevice: only a CPU device (" + c.name + ") -- software rendering, check the GPU driver." + skipped);

                return Ir77RETURN<Ir77OperationSucceeded>(this, "CreateDisplayDevice: " + c.name + skipped);
            }

            m_windows.erase(c.id);
            skipped += " [skipped " + c.name + "]";
        }

        m_current_device = 0;

        return Ir77RETURN<Ir77NotConfigured>(this, "CreateDisplayDevice: no device could drive the display." + skipped);
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
        auto const& device = m_devices.at(m_current_device);

        auto result = device->DefineQueueFamilyProps(m_windows.at(m_current_device).at(0));
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        result = device->DefineQueueCreateInfos();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Enumerate Device Queue.");
    }

    std::shared_ptr<IIr77Return const> CreateLogicalDevices() {
        auto const& device = m_devices.at(m_current_device);

        auto result = device->QueryDeviceExtensionSupport();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        result = device->DefineDeviceInfo();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        result = device->DefineDevice();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

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

        std::map<Ir77PVLayoutKind, std::uint32_t> capacities{
            {Ir77PVLayoutKind::Static, std::max(frame_sets, MATERIAL_CAPACITY)},
            {Ir77PVLayoutKind::Skinned, std::max(frame_sets, SKELETON_CAPACITY)},
            {Ir77PVLayoutKind::Shadow, frame_sets},
            {Ir77PVLayoutKind::ShadowSkinned, frame_sets},
            {Ir77PVLayoutKind::CEF, std::max<std::uint32_t>(windows, 1)},
        };

        bool mesh{false};
        m_devices.at(m_current_device)->GetMeshShading(mesh);

        if (mesh) capacities.emplace(Ir77PVLayoutKind::Mesh, std::max(frame_sets, MATERIAL_CAPACITY));

        std::map<Ir77PVLayoutKind, std::shared_ptr<IIr77PVLayout>> layouts{};

        for (auto const& [kind, capacity] : capacities) {
            auto layout = std::static_pointer_cast<IIr77PVLayout>(std::make_shared<Ir77PVLayout>());

            layout->SetDevice(m_devices.at(m_current_device));

            layout->SetMeshShading(mesh);

            if (layout->SetKind(kind)->ID() != GUIDIr77OperationSucceeded || layout->CreateLayout(capacity)->ID() != GUIDIr77OperationSucceeded) {
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PeregrineV: layout creation failed.");
            }

            layouts.emplace(kind, layout);
        }

        m_layouts[m_current_device] = layouts;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Layouts.");
    }

    // Creates one render pass of every kind for the current device and stores each in m_render_passes.
    // Main and Post take their color format from window 0's surface (every window on the device must
    // share it). Shadow and Offscreen need no surface.
    std::shared_ptr<IIr77Return const> CreateRenderPasses() {
        static std::array<Ir77PVRenderPassKind, 4> const kinds{Ir77PVRenderPassKind::Main, Ir77PVRenderPassKind::Shadow, Ir77PVRenderPassKind::Offscreen,
                                                               Ir77PVRenderPassKind::Post};

        for (auto const kind : kinds) {
            auto render_pass = std::static_pointer_cast<IIr77PVRenderPass>(std::make_shared<Ir77PVRenderPass>());

            render_pass->SetDevice(m_devices.at(m_current_device));

            render_pass->SetKind(kind);

            if (kind == Ir77PVRenderPassKind::Main || kind == Ir77PVRenderPassKind::Post) {
                auto const& swapchain = m_swapchains.at(m_current_device).at(0);

                VkSurfaceFormatKHR surface_format{};

                auto const format_result = swapchain->QuerySurfaceFormat(surface_format);
                if (format_result->ID() != GUIDIr77OperationSucceeded) return format_result;

                render_pass->SetColorFormat(surface_format.format);
            }

            if (render_pass->CreateRenderPass()->ID() != GUIDIr77OperationSucceeded)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PeregrineV: render pass failed.");

            m_render_passes[m_current_device][kind] = render_pass;
        }

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Render Pass.");
    }

    std::shared_ptr<IIr77Return const> CreateSwapchains() {
        auto& swapchains = m_swapchains.at(m_current_device);

        std::vector<std::uint32_t> frames(swapchains.size(), 0);

        auto const main_pass = m_render_passes.at(m_current_device).find(Ir77PVRenderPassKind::Main);
        if (main_pass == m_render_passes.at(m_current_device).end())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PeregrineV: main render pass must be created before swapchains.");

        for (auto& swapchain : swapchains) {
            swapchain->SetAllocator(m_allocators.at(m_current_device));

            swapchain->SetRenderPass(main_pass->second);

            if (swapchain->CreateSwapchain()->ID() != GUIDIr77OperationSucceeded)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: Rebuild failed.");
        }

        m_current_frames[m_current_device] = frames;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Swapchains.");
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // placeholder texture -- lets every image binding be written before real textures exist
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> CreatePlaceholderTexture() {
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

            auto const result = texture->CreatePlaceholder(rgba);
            if (result->ID() != GUIDIr77OperationSucceeded) return result;

            placeholders.emplace(kind, texture);
        }

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

                if (buffer->CreateResources(spec.size, MAX_FRAMES_IN_FLIGHT)->ID() != GUIDIr77OperationSucceeded) {
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

            if (global->CreateSets(MAX_FRAMES_IN_FLIGHT)->ID() != GUIDIr77OperationSucceeded) {
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PeregrineV: global set allocation failed.");
            }

            auto result = global->BindGlobal(slots, placeholder);  // env / IBL = placeholder until a real one exists
            if (result->ID() != GUIDIr77OperationSucceeded) return result;

            // set 1 -- shadow map, shadow matrices
            auto pass = std::static_pointer_cast<IIr77PVDescriptorSet>(std::make_shared<Ir77PVDescriptorSet>());

            pass->SetDevice(m_devices.at(m_current_device));
            pass->SetLayout(layout, 1);

            if (pass->CreateSets(MAX_FRAMES_IN_FLIGHT)->ID() != GUIDIr77OperationSucceeded) {
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PeregrineV: pass set allocation failed.");
            }

            result = pass->BindPass(slots, placeholder);  // shadow map = placeholder until the Shadow pass exists
            if (result->ID() != GUIDIr77OperationSucceeded) return result;

            globals.push_back(global);
            passes.push_back(pass);
        }

        m_descriptor_sets_global[m_current_device] = globals;
        m_descriptor_sets_pass[m_current_device] = passes;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Descriptor Sets.");
    }
    // -------------------------------------------------------------------------------------------------------------------------------------
    // pipelines -- per window, per kind. Each kind resolves its own render pass and layout via RenderPassKindFor / LayoutKindFor.
    // Kinds whose render pass hasn't been created yet (e.g. Shadow) are skipped.
    // Only request kinds whose shaders are loaded (Skinned / Shadow shaders don't exist yet).
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> CreatePipelines(std::vector<Ir77PVPipelineKind> const& kinds) {
        std::size_t const windows = m_windows.at(m_current_device).size();

        if (windows > MAX_WINDOWS) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PeregrineV: too many windows.");

        auto const& render_passes = m_render_passes.at(m_current_device);
        auto const& layouts = m_layouts.at(m_current_device);

        std::vector<std::map<Ir77PVPipelineKind, std::shared_ptr<IIr77PVPipeline>>> per_window(windows);

        for (std::size_t w = 0; w < windows; w++) {
            for (auto const kind : kinds) {
                auto const pass = render_passes.find(RenderPassKindFor(kind));
                if (pass == render_passes.end()) continue;  // that render pass kind doesn't exist

                auto const layout = layouts.find(LayoutKindFor(kind));
                if (layout == layouts.end()) continue;  // that layout kind wasn't built

                auto ipipeline = std::static_pointer_cast<IIr77PVPipeline>(std::make_shared<Ir77PVPipeline>());

                ipipeline->SetDevice(m_devices.at(m_current_device));
                ipipeline->SetRenderPass(pass->second);
                ipipeline->SetLayout(layout->second);
                ipipeline->SetShader(m_shaders.at(m_current_device));
                ipipeline->SetKind(kind);

                if (ipipeline->CreatePipeline()->ID() != GUIDIr77OperationSucceeded) {
                    continue;  // this kind's shader stage(s) aren't loaded -- skip it, not fatal
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
        if (device == m_pipelines.end() || window >= device->second.size())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PeregrineV: no pipelines for window.");

        auto const found = device->second[window].find(kind);
        if (found == device->second[window].end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PeregrineV: pipeline kind not created.");

        pipeline = found->second;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetFrameBuffer(std::size_t const& window, Ir77PVBufferSlot const& slot, std::shared_ptr<IIr77PVBuffer>& buffer) {
        auto const device = m_buffers_frame.find(m_current_device);
        if (device == m_buffers_frame.end() || window >= device->second.size())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PeregrineV: no frame buffers for window.");

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
            case Ir77PVPipelineKind::SkinnedTransparent:
            case Ir77PVPipelineKind::SkinnedTransparentBack:
                return Ir77PVLayoutKind::Skinned;

            case Ir77PVPipelineKind::Shadow:
                return Ir77PVLayoutKind::Shadow;

            case Ir77PVPipelineKind::ShadowSkinned:
                return Ir77PVLayoutKind::ShadowSkinned;

            case Ir77PVPipelineKind::Mesh:
                return Ir77PVLayoutKind::Mesh;

            case Ir77PVPipelineKind::CEF:
                return Ir77PVLayoutKind::CEF;

            case Ir77PVPipelineKind::Static:
            case Ir77PVPipelineKind::Transparent:
            default:
                return Ir77PVLayoutKind::Static;
        }
    }

    static Ir77PVRenderPassKind RenderPassKindFor(Ir77PVPipelineKind const kind) {
        switch (kind) {
            case Ir77PVPipelineKind::Shadow:

            case Ir77PVPipelineKind::ShadowSkinned:
                return Ir77PVRenderPassKind::Shadow;

            default:
                return Ir77PVRenderPassKind::Main;
        }
    }

   private:
    friend class Ir77PVPaint;
    friend class Ir77PVAsset;
    friend class Ir77PVCEF;

    std::shared_ptr<IIr77PVInstance> m_instance;

    std::uint32_t m_device_count{0};

    std::uint64_t m_current_device{0};

    std::map<std::uint64_t, std::shared_ptr<IIr77PVDevice>> m_devices;

    std::map<std::uint64_t, std::vector<std::uint32_t>> m_current_frames;

    std::map<std::uint64_t, std::vector<SDL_Window*>> m_windows;

    std::map<std::uint64_t, VmaAllocator> m_allocators;

    std::map<std::uint64_t, std::vector<std::shared_ptr<IIr77PVCmdBuffer>>> m_command_buffers;

    std::map<std::uint64_t, std::shared_ptr<IIr77PVShader>> m_shaders;

    std::map<std::uint64_t, std::vector<std::shared_ptr<IIr77Enlisted>>> m_compute;

    bool m_resize_in_progress{false};

    /*************************** passes *********************************/
    std::map<std::uint64_t, std::vector<std::shared_ptr<IIr77PVSwapchain>>> m_swapchains;

    std::map<std::size_t, std::map<Ir77PVRenderPassKind, std::shared_ptr<IIr77PVRenderPass>>> m_render_passes;

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