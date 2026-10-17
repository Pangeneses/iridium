#pragma once

#include <vk_mem_alloc.h>
#include <SDL3/SDL_video.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77PVInstance.hpp"
#include "../interface/IIr77PVDevice.hpp"
#include "../interface/IIr77PVSwapchain.hpp"
#include "../interface/IIr77PVRenderPass.hpp"
#include "../interface/IIr77PVPipeline.hpp"
#include "../interface/IIr77PVPipelineCache.hpp"
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
#include "../runtime/Ir77PVPipelineCache.hpp"
#include "../runtime/Ir77PVLayout.hpp"
#include "../runtime/Ir77PVBuffer.hpp"
#include "../runtime/Ir77PVDescriptorSet.hpp"
#include "../runtime/Ir77PVTexture.hpp"

#include "../types/Ir77PVTypes.hpp"

#include "../types/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77PVInit : public Ir77Enlisted, public std::enable_shared_from_this<Ir77PVInit> {
   public:
    Ir77PVInit() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    // Everything holding VMA memory must die before its allocator, so the maps are released
    // explicitly here instead of after this body runs (member destruction happens after the allocator is gone).
    ~Ir77PVInit() {
        for (auto& [device_id, device] : m_devices) {
            VkDevice vk_device{VK_NULL_HANDLE};
            device->GetDevice(&vk_device);
            if (vk_device != VK_NULL_HANDLE) vkDeviceWaitIdle(vk_device);
        }

        m_command_buffers.clear();
        m_overlays.clear();
        m_frame_tables.clear();
        m_material_tables.clear();
        m_buffers_frame.clear();
        m_buffers_vertex.clear();
        m_buffers_index.clear();
        m_swapchains.clear();
        m_pipeline_caches.clear();  // saves the cache file, destroys every pipeline
        m_placeholders.clear();
        m_descriptor_sets.clear();  // after everything that registered slots in it
        m_layouts.clear();

        for (auto& [device_id, allocator] : m_allocators) vmaDestroyAllocator(allocator);
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVInit>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVInit>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIr77PVInit)
            obj = std::shared_ptr<Ir77PVInit>(shared_from_this(), static_cast<Ir77PVInit*>(this));

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

        if (device_windows.size() > MAX_WINDOWS) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: too many windows.");

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
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: vmaCreateAllocator failed.");
        }

        m_allocators.emplace(m_current_device, allocator);

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Allocator.");
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // bindless layout + table -- one of each per device, shared by every window and every pipeline
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> CreateLayouts() {
        auto layout = std::static_pointer_cast<IIr77PVLayout>(std::make_shared<Ir77PVLayout>());

        layout->SetDevice(m_devices.at(m_current_device));

        auto result = layout->CreateLayout();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        auto table = std::static_pointer_cast<IIr77PVDescriptorSet>(std::make_shared<Ir77PVDescriptorSet>());

        table->SetDevice(m_devices.at(m_current_device));

        table->SetLayout(layout);

        result = table->CreateDescriptorSet();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        m_layouts[m_current_device] = layout;
        m_descriptor_sets[m_current_device] = table;

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
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: render pass failed.");

            m_render_passes[m_current_device][kind] = render_pass;
        }

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Render Pass.");
    }

    std::shared_ptr<IIr77Return const> CreateSwapchains() {
        auto& swapchains = m_swapchains.at(m_current_device);

        std::vector<std::uint32_t> frames(swapchains.size(), 0);

        auto const main_pass = m_render_passes.at(m_current_device).find(Ir77PVRenderPassKind::Main);
        if (main_pass == m_render_passes.at(m_current_device).end())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: main render pass must be created before swapchains.");

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
            // every frame buffer is read through the bindless storage-buffer array
            {Ir77PVBufferSlot::Camera, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizes.camera},
            {Ir77PVBufferSlot::Lights, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizes.lights},
            {Ir77PVBufferSlot::Instances, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizes.instances},
            {Ir77PVBufferSlot::ShadowMatrices, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizes.shadow_matrices},
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
                    return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: frame buffer creation failed.");
                }

                per_window[w].emplace(spec.slot, buffer);
            }
        }

        m_buffers_frame[m_current_device] = per_window;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Frame Buffers.");
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // bindless registration -- placeholders (White = texture slot 0), the material table, and per window per frame the frame buffers
    // plus a frame table listing their slots. Requires CreateLayouts, CreatePlaceholderTexture, CreateFrameBuffers.
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> CreateDescriptorSets() {
        auto result = RegisterPlaceholders();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        result = CreateMaterialTable();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        result = RegisterFrameBuffers();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Descriptor Sets.");
    }
    // -------------------------------------------------------------------------------------------------------------------------------------
    // pipelines -- one Ir77PVPipelineCache per device, shared by every window: a pipeline is built the first time its description is asked
    // for, and the driver's compiled code is kept on disk between runs. Asking for the kinds here only warms the cache at startup; kinds
    // whose shaders or render pass are missing are skipped. Needs CreateLayouts, CreateRenderPasses and Asset::CreateShaders.
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> CreatePipelines(std::vector<Ir77PVPipelineKind> const& kinds) {
        auto const result = DefinePipelineCache();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        auto const& cache = m_pipeline_caches.at(m_current_device);

        for (auto const kind : kinds) {
            Ir77PVPipelineHandle handle{IR77PV_NO_PIPELINE};
            std::shared_ptr<IIr77PVPipeline> pipeline{};

            cache->FindPipeline(kind, handle);
            cache->GetPipeline(handle, pipeline);  // builds now, into the slot draw items will use; a role that can't resolve is skipped
        }

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Pipelines.");
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // lookups for Paint / Asset / CEF
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> GetLayout(std::shared_ptr<IIr77PVLayout>& layout) {
        auto const found = m_layouts.find(m_current_device);
        if (found == m_layouts.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: CreateLayouts not called.");

        layout = found->second;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetDescriptorSet(std::shared_ptr<IIr77PVDescriptorSet>& table) {
        auto const found = m_descriptor_sets.find(m_current_device);
        if (found == m_descriptor_sets.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: CreateLayouts not called.");

        table = found->second;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Frame slots of one window, one entry per frame in flight -- for its command buffer
    std::shared_ptr<IIr77Return const> GetFrameSlots(std::size_t const& window, std::array<Ir77PVFrameSlots, MAX_FRAMES_IN_FLIGHT>& slots) {
        auto const device = m_frame_slots.find(m_current_device);
        if (device == m_frame_slots.end() || window >= device->second.size())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: CreateDescriptorSets not called.");

        slots = device->second[window];

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetMaterialTableSlot(std::uint32_t& slot) {
        auto const found = m_material_table_slots.find(m_current_device);
        if (found == m_material_table_slots.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: CreateDescriptorSets not called.");

        slot = found->second;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetPlaceholderSlot(Ir77PVPlaceholderKind const& kind, std::uint32_t& slot) {
        auto const device = m_placeholder_slots.find(m_current_device);
        if (device == m_placeholder_slots.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: CreateDescriptorSets not called.");

        auto const found = device->second.find(kind);
        if (found == device->second.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: placeholder kind not registered.");

        slot = found->second;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // A manifest pipeline's handle -- what draw items carry
    std::shared_ptr<IIr77Return const> FindPipeline(std::string const& name, Ir77PVPipelineHandle& handle) {
        auto const found = m_pipeline_caches.find(m_current_device);
        if (found == m_pipeline_caches.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: CreatePipelines not called.");

        return found->second->FindPipeline(name, handle);
    }

    // An engine role's handle -- follows the manifest's "roles"
    std::shared_ptr<IIr77Return const> FindPipeline(Ir77PVPipelineKind const& kind, Ir77PVPipelineHandle& handle) {
        auto const found = m_pipeline_caches.find(m_current_device);
        if (found == m_pipeline_caches.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: CreatePipelines not called.");

        return found->second->FindPipeline(kind, handle);
    }

    // The pipeline a handle resolves to under the current manifest -- every window on the device shares it
    std::shared_ptr<IIr77Return const> GetPipeline(Ir77PVPipelineHandle const& handle, std::shared_ptr<IIr77PVPipeline>& pipeline) {
        auto const found = m_pipeline_caches.find(m_current_device);
        if (found == m_pipeline_caches.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: CreatePipelines not called.");

        return found->second->GetPipeline(handle, pipeline);
    }

    // Before CreatePipelines: overrides <shader root>/pipelines.json. After: reloads now -- a bad file fails and the old manifest stays.
    std::shared_ptr<IIr77Return const> SetPipelineManifest(std::string const& path) {
        auto const cache = m_pipeline_caches.find(m_current_device);
        if (cache != m_pipeline_caches.end()) {
            auto const result = cache->second->LoadManifest(path);
            if (result->ID() != GUIDIr77OperationSucceeded) return result;
        }

        m_pipeline_manifests[m_current_device] = path;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetFrameBuffer(std::size_t const& window, Ir77PVBufferSlot const& slot, std::shared_ptr<IIr77PVBuffer>& buffer) {
        auto const device = m_buffers_frame.find(m_current_device);
        if (device == m_buffers_frame.end() || window >= device->second.size())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: no frame buffers for window.");

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
    // White first -- it must land in texture slot 0 (BINDLESS_PLACEHOLDER_TEXTURE), so unset texture slots sample white
    std::shared_ptr<IIr77Return const> RegisterPlaceholders() {
        auto const& table = m_descriptor_sets.at(m_current_device);

        auto const device = m_placeholders.find(m_current_device);
        if (device == m_placeholders.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: CreatePlaceholderTexture not called.");

        std::map<Ir77PVPlaceholderKind, std::uint32_t> slots{};

        for (auto const kind : {Ir77PVPlaceholderKind::White, Ir77PVPlaceholderKind::Normal}) {
            auto const found = device->second.find(kind);
            if (found == device->second.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: placeholder texture missing.");

            VkDescriptorImageInfo info{};
            found->second->GetImageInfo(&info);

            std::uint32_t slot{BINDLESS_NO_SLOT};

            auto const result = table->RegisterTexture(info.imageView, info.sampler, slot);
            if (result->ID() != GUIDIr77OperationSucceeded) return result;

            slots.emplace(kind, slot);
        }

        if (slots.at(Ir77PVPlaceholderKind::White) != BINDLESS_PLACEHOLDER_TEXTURE)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: white placeholder must be the first texture registered.");

        m_placeholder_slots[m_current_device] = slots;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // MATERIAL_CAPACITY rows of Ir77PVMaterialRow in one Static buffer -- Ir77PVAsset writes rows into it
    std::shared_ptr<IIr77Return const> CreateMaterialTable() {
        auto buffer = std::static_pointer_cast<IIr77PVBuffer>(std::make_shared<Ir77PVBuffer>());

        buffer->SetDevice(m_devices.at(m_current_device));

        buffer->SetAllocator(m_allocators.at(m_current_device));

        buffer->SetUsage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

        buffer->SetMode(Ir77PVBufferMode::Static);

        auto result = buffer->CreateResources(static_cast<VkDeviceSize>(MATERIAL_CAPACITY) * sizeof(Ir77PVMaterialRow), 1);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        VkDescriptorBufferInfo info{};
        buffer->GetBufferInfo(0, &info);

        std::uint32_t slot{BINDLESS_NO_SLOT};

        result = m_descriptor_sets.at(m_current_device)->RegisterBuffer(info.buffer, info.offset, info.range, slot);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        m_material_tables[m_current_device] = buffer;
        m_material_table_slots[m_current_device] = slot;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Per window, per frame in flight: every frame buffer's copy gets a slot, then a frame table row listing them gets its own slot
    std::shared_ptr<IIr77Return const> RegisterFrameBuffers() {
        auto const& table = m_descriptor_sets.at(m_current_device);
        auto const& frame_buffers = m_buffers_frame.at(m_current_device);

        std::uint32_t const white{m_placeholder_slots.at(m_current_device).at(Ir77PVPlaceholderKind::White)};

        std::vector<std::shared_ptr<IIr77PVBuffer>> frame_tables{};
        std::vector<std::array<Ir77PVFrameSlots, MAX_FRAMES_IN_FLIGHT>> frame_slots(frame_buffers.size());

        for (std::size_t w = 0; w < frame_buffers.size(); w++) {
            auto frame_table = std::static_pointer_cast<IIr77PVBuffer>(std::make_shared<Ir77PVBuffer>());

            frame_table->SetDevice(m_devices.at(m_current_device));

            frame_table->SetAllocator(m_allocators.at(m_current_device));

            frame_table->SetUsage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

            frame_table->SetMode(Ir77PVBufferMode::Dynamic);

            auto result = frame_table->CreateResources(sizeof(Ir77PVFrameTable), MAX_FRAMES_IN_FLIGHT);
            if (result->ID() != GUIDIr77OperationSucceeded) return result;

            for (std::uint32_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++) {
                Ir77PVFrameSlots& slots = frame_slots[w][frame];

                slots.values.camera = RegisterFrameCopy(table, frame_buffers[w].at(Ir77PVBufferSlot::Camera), frame);
                slots.values.lights = RegisterFrameCopy(table, frame_buffers[w].at(Ir77PVBufferSlot::Lights), frame);
                slots.values.instances = RegisterFrameCopy(table, frame_buffers[w].at(Ir77PVBufferSlot::Instances), frame);
                slots.values.shadow_matrices = RegisterFrameCopy(table, frame_buffers[w].at(Ir77PVBufferSlot::ShadowMatrices), frame);
                slots.values.bones = RegisterFrameCopy(table, frame_buffers[w].at(Ir77PVBufferSlot::Bones), frame);
                slots.values.env = white;         // until a real environment exists
                slots.values.shadow_map = white;  // until the Shadow pass exists

                if (slots.values.camera == BINDLESS_NO_SLOT || slots.values.lights == BINDLESS_NO_SLOT || slots.values.instances == BINDLESS_NO_SLOT ||
                    slots.values.shadow_matrices == BINDLESS_NO_SLOT || slots.values.bones == BINDLESS_NO_SLOT)
                    return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: frame buffer registration failed.");

                result = frame_table->Update(frame, &slots.values, sizeof(Ir77PVFrameTable));
                if (result->ID() != GUIDIr77OperationSucceeded) return result;

                slots.table = RegisterFrameCopy(table, frame_table, frame);
                if (slots.table == BINDLESS_NO_SLOT) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: frame table registration failed.");
            }

            frame_tables.push_back(frame_table);
        }

        m_frame_tables[m_current_device] = frame_tables;
        m_frame_slots[m_current_device] = frame_slots;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // One frame copy of a Dynamic buffer -> its bindless slot; BINDLESS_NO_SLOT on failure
    std::uint32_t RegisterFrameCopy(std::shared_ptr<IIr77PVDescriptorSet> const& table, std::shared_ptr<IIr77PVBuffer> const& buffer,
                                    std::uint32_t const& frame) {
        VkDescriptorBufferInfo info{};
        if (buffer->GetBufferInfo(frame, &info)->ID() != GUIDIr77OperationSucceeded) return BINDLESS_NO_SLOT;

        std::uint32_t slot{BINDLESS_NO_SLOT};
        if (table->RegisterBuffer(info.buffer, info.offset, info.range, slot)->ID() != GUIDIr77OperationSucceeded) return BINDLESS_NO_SLOT;

        return slot;
    }

    // The device's pipeline cache, wired to its layout, shaders and every render pass. A no-op once it exists.
    std::shared_ptr<IIr77Return const> DefinePipelineCache() {
        if (m_pipeline_caches.count(m_current_device) != 0) return Ir77RETURN<Ir77OperationSucceeded>();

        auto const shaders = m_shaders.find(m_current_device);
        if (shaders == m_shaders.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: shaders not loaded -- call Asset::CreateShaders first.");

        auto const passes = m_render_passes.find(m_current_device);
        if (passes == m_render_passes.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: CreateRenderPasses not called.");

        auto cache = std::static_pointer_cast<IIr77PVPipelineCache>(std::make_shared<Ir77PVPipelineCache>());

        cache->SetDevice(m_devices.at(m_current_device));

        cache->SetLayout(m_layouts.at(m_current_device));

        cache->SetShader(shaders->second);

        for (auto const& [kind, render_pass] : passes->second) cache->SetRenderPass(kind, render_pass);

        auto const result = cache->CreatePipelineCache();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        auto const manifest = m_pipeline_manifests.find(m_current_device);
        if (manifest == m_pipeline_manifests.end())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: no pipeline manifest -- put pipelines.json in the shader root or call SetPipelineManifest.");

        auto const loaded = cache->LoadManifest(manifest->second);

        if (loaded->ID() != GUIDIr77OperationSucceeded) return loaded;

        m_pipeline_caches[m_current_device] = cache;

        return Ir77RETURN<Ir77OperationSucceeded>();
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

    std::map<std::uint64_t, std::map<Ir77PVRenderPassKind, std::shared_ptr<IIr77PVRenderPass>>> m_render_passes;

    /*************************** layout / pipelines *********************/
    // [device] -- the one bindless layout and the one bindless table
    std::map<std::uint64_t, std::shared_ptr<IIr77PVLayout>> m_layouts;

    std::map<std::uint64_t, std::shared_ptr<IIr77PVDescriptorSet>> m_descriptor_sets;

    // [device] -- every pipeline, keyed by description
    std::map<std::uint64_t, std::shared_ptr<IIr77PVPipelineCache>> m_pipeline_caches;

    // [device] -- pipelines.json; Asset::CreateShaders fills it from the shader root unless SetPipelineManifest did
    std::map<std::uint64_t, std::string> m_pipeline_manifests;

    /*************************** buffers / slots ************************/
    // [device][window][slot]
    std::map<std::uint64_t, std::vector<std::map<Ir77PVBufferSlot, std::shared_ptr<IIr77PVBuffer>>>> m_buffers_frame;

    // [device][window] -- frame table buffer (Dynamic) and, per frame in flight, its slot + the slots it lists
    std::map<std::uint64_t, std::vector<std::shared_ptr<IIr77PVBuffer>>> m_frame_tables;

    std::map<std::uint64_t, std::vector<std::array<Ir77PVFrameSlots, MAX_FRAMES_IN_FLIGHT>>> m_frame_slots;

    // [device] -- material table (Static, MATERIAL_CAPACITY rows) and its slot
    std::map<std::uint64_t, std::shared_ptr<IIr77PVBuffer>> m_material_tables;

    std::map<std::uint64_t, std::uint32_t> m_material_table_slots;

    // [device][kind] -- texture slots of the placeholders (White is always BINDLESS_PLACEHOLDER_TEXTURE)
    std::map<std::uint64_t, std::map<Ir77PVPlaceholderKind, std::uint32_t>> m_placeholder_slots;

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
}  // namespace NSIr77PVInit