#pragma once

#include <vk_mem_alloc.h>
#include <SDL3/SDL_video.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/types/Ir77Types.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"
#include "../../Ir77RT/runtime/Ir77Return.hpp"

#include "../interface/IIr77PVInit.hpp"
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
#include "../interface/IIr77PVShader.hpp"
#include "../interface/IIr77PVTransfer.hpp"
#include "../interface/IIr77PVMaterial.hpp"
#include "../interface/IIr77PVGeometry.hpp"
#include "../interface/IIr77PVAcceleration.hpp"
#include "../interface/IIr77PVWorld.hpp"
#include "../interface/IIr77PVPaint.hpp"
#include "../interface/IIr77PVCEF.hpp"

#include "../runtime/Ir77PVInstance.hpp"
#include "../runtime/Ir77PVDevice.hpp"
#include "../runtime/Ir77PVSwapchain.hpp"
#include "../runtime/Ir77PVRenderPass.hpp"
#include "../runtime/Ir77PVPipelineCache.hpp"
#include "../runtime/Ir77PVLayout.hpp"
#include "../runtime/Ir77PVBuffer.hpp"
#include "../runtime/Ir77PVDescriptorSet.hpp"
#include "../runtime/Ir77PVTexture.hpp"
#include "../runtime/Ir77PVShader.hpp"
#include "../runtime/Ir77PVTransfer.hpp"
#include "../runtime/Ir77PVMaterial.hpp"
#include "../runtime/Ir77PVGeometry.hpp"
#include "../runtime/Ir77PVAcceleration.hpp"
#include "../runtime/Ir77PVWorld.hpp"
#include "../runtime/Ir77PVPaint.hpp"
#include "../runtime/Ir77PVCEF.hpp"

#include "../types/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// =============================================================================================================================================
// Ir77PVInit -- the renderer's lifetime controller. Owns the device and everything on it (swapchains, bindless layout + table, frame
// buffers, shadow atlas, shader registry, pipeline cache) and CEF, Paint, Transfer, Materials, Geometry, (with ray queries) Acceleration and
// World, which it creates, wires with their Set* calls as each piece exists, and destroys first. They never see Init -- everything they use is
// handed to them. Only the server holds Init.
//   SetWindows -> CreateRenderer (its Define* steps, in order) -> [content] -> CreateCommandBuffers ... DestroyRenderer (before SDL destroys
//   the windows)
//   per frame:  Tick(frame) -- Transfer (slots, staging, graveyard), then Geometry (dirty indirect copies)
//   hot shader: Shader::CompileShader -> ReloadShader (registry + pipeline cache + manifest -- the three things only Init holds together)
// =============================================================================================================================================
class Ir77PVInit : public Ir77Enlisted, public IIr77PVInit, public std::enable_shared_from_this<Ir77PVInit> {
   public:
    Ir77PVInit() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    // DestroyRenderer normally ran already; ReleaseDevices is a no-op the second time
    ~Ir77PVInit() {
        WaitIdle();

        ReleasePeers();

        ReleaseDevices();
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
        seat_shared_uuid<&GUIDIr77PeregrineV>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77PVInit)
            obj = std::shared_ptr<IIr77PVInit>(shared_from_this(), static_cast<IIr77PVInit*>(this));

        else if (iid == GUIDIr77PVInit)
            obj = std::shared_ptr<Ir77PVInit>(shared_from_this(), static_cast<Ir77PVInit*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

   public:
    // -------------------------------------------------------------------------------------------------------------------------------------
    // lifetime
    // -------------------------------------------------------------------------------------------------------------------------------------

    // The windows the display device must present to -- pointers, so a direct link, not an arg
    std::shared_ptr<IIr77Return const> SetWindows(std::vector<SDL_Window*> const& windows) {
        m_display_windows = windows;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Instance through pipelines, in the only order that works. CEF, Transfer and Materials are wired as soon as what they need exists;
    // Geometry once the pipeline cache is there; Paint waits for CreateCommandBuffers, when the overlays are there too.
    std::shared_ptr<IIr77Return const> CreateRenderer(std::string const& shader_root, std::uint64_t& device_id) {
        std::shared_ptr<IIr77Return const> result{};

        if (m_paint) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: renderer already created.");

        // the servers -- owned here; they never see Init
        m_cef = std::static_pointer_cast<IIr77PVCEF>(std::make_shared<Ir77PVCEF>());
        m_paint = std::static_pointer_cast<IIr77PVPaint>(std::make_shared<Ir77PVPaint>());
        m_transfer = std::static_pointer_cast<IIr77PVTransfer>(std::make_shared<Ir77PVTransfer>());
        m_materials = std::static_pointer_cast<IIr77PVMaterial>(std::make_shared<Ir77PVMaterial>());
        m_geometry = std::static_pointer_cast<IIr77PVGeometry>(std::make_shared<Ir77PVGeometry>());
        m_world = std::static_pointer_cast<IIr77PVWorld>(std::make_shared<Ir77PVWorld>());

        // instance / device -- instance and enumeration report through the return log; the device fails if they did
        DefineInstance();
        DefinePhysicalDevices();
        if (Failed(DefineDisplayDevice(m_display_windows, device_id), result)) return result;

        std::map<std::uint64_t, std::vector<SDL_Window*>> const windows{{device_id, m_display_windows}};

        if (Failed(DefineSurfaces(windows), result)) return result;
        if (Failed(DefineAllocator(), result)) return result;

        // layouts + passes (Main, Shadow, Offscreen, Post)
        if (Failed(DefineLayouts(), result)) return result;
        if (Failed(DefineRenderPasses(), result)) return result;
        if (Failed(DefineSwapchains(), result)) return result;

        // frame resources -- the placeholder texture and shadow map must exist before the descriptor sets register them
        if (Failed(DefinePlaceholderTexture(), result)) return result;
        if (Failed(DefineFrameBuffers(), result)) return result;
        if (Failed(DefineShadowMap(8192), result)) return result;
        if (Failed(DefineDescriptorSets(), result)) return result;

        auto const& device = m_devices.at(device_id);
        VmaAllocator const allocator = m_allocators.at(device_id);
        auto const& table = m_descriptor_sets.at(device_id);

        // CEF overlay -- one per window, on each window's swapchain
        m_cef->SetDevice(device);
        m_cef->SetAllocator(allocator);
        m_cef->SetSwapchains(m_swapchains.at(device_id));
        m_cef->SetDescriptorSet(table);
        if (Failed(m_cef->CreateOverlays(), result)) return result;

        // Transfer -- uploads, buffers, bindless buffer slots, the graveyard
        m_transfer->SetDevice(device);
        m_transfer->SetAllocator(allocator);
        m_transfer->SetDescriptorSet(table);

        // Materials -- textures and material rows, uploaded through Transfer
        m_materials->SetDevice(device);
        m_materials->SetAllocator(allocator);
        m_materials->SetDescriptorSet(table);
        m_materials->SetTransfer(m_transfer);
        m_materials->SetMaterialTable(m_material_tables.at(device_id));
        m_materials->SetPlaceholderSlots(m_placeholder_slots.at(device_id));

        // the shader registry -- every <name>.<stage>.spv in the shader root
        auto shader = std::static_pointer_cast<IIr77PVShader>(std::make_shared<Ir77PVShader>());
        shader->SetDevice(device);
        shader->SetShaderRoot(shader_root);
        if (Failed(shader->LoadShaders(), result)) return result;

        m_shaders[device_id] = shader;

        // the manifest -- an explicit SetPipelineManifest wins, else <shader root>/pipelines.json
        if (m_pipeline_manifests.count(device_id) == 0) {
            std::error_code error{};
            std::filesystem::path const manifest = std::filesystem::path(shader_root) / "pipelines.json";
            if (std::filesystem::is_regular_file(manifest, error)) m_pipeline_manifests[device_id] = manifest.string();
        }

        // pipelines need shaders, layouts and the Main pass
        std::vector<Ir77PVPipelineKind> kinds{};
        if (Failed(GetPipelineKinds(kinds), result)) return result;
        if (Failed(DefinePipelines(kinds), result)) return result;

        // Geometry -- buffers through Transfer, rows from Materials, roles and draw item checks from the pipeline cache
        m_geometry->SetDevice(device);
        m_geometry->SetTransfer(m_transfer);
        m_geometry->SetMaterials(m_materials);
        m_geometry->SetPipelineCache(m_pipeline_caches.at(device_id));

        // Acceleration -- ray-query devices only: BLASes over Geometry's scenes, a TLAS per frame, buffers through Transfer
        bool ray_query{false};
        device->GetRayQuery(ray_query);

        if (ray_query) {
            m_acceleration = std::static_pointer_cast<IIr77PVAcceleration>(std::make_shared<Ir77PVAcceleration>());
            m_acceleration->SetDevice(device);
            m_acceleration->SetTransfer(m_transfer);
            m_acceleration->SetGeometry(m_geometry);
            m_acceleration->SetDescriptorSet(table);
            if (Failed(m_acceleration->CreateAcceleration(), result)) return result;
        }

        // World -- assets and frames, over everything above; Paint is used from the first frame, once CreateCommandBuffers has wired it
        std::string device_name{};
        GetDeviceName(device_name);

        m_world->SetTransfer(m_transfer);
        m_world->SetMaterials(m_materials);
        m_world->SetGeometry(m_geometry);
        m_world->SetPipelineCache(m_pipeline_caches.at(device_id));
        m_world->SetPaint(m_paint);
        if (m_acceleration) m_world->SetAcceleration(m_acceleration);
        if (Failed(m_world->CreateWorld(device_name), result)) return result;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Ir77PVInit: renderer created.");
    }

    // Paint gets everything here -- the overlays and the pipeline cache both exist by now
    std::shared_ptr<IIr77Return const> CreateCommandBuffers() {
        std::shared_ptr<IIr77Return const> result{};

        if (!m_paint) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: CreateRenderer not called.");

        auto const cache = m_pipeline_caches.find(m_current_device);
        if (cache == m_pipeline_caches.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: DefinePipelines not called.");

        std::vector<std::shared_ptr<IIr77PVOverlay>> overlays{};
        if (m_cef) m_cef->GetOverlays(overlays);

        m_paint->SetDevice(m_devices.at(m_current_device));
        m_paint->SetWindows(m_windows.at(m_current_device));
        m_paint->SetSwapchains(m_swapchains.at(m_current_device));
        m_paint->SetRenderPasses(m_render_passes.at(m_current_device));
        m_paint->SetLayout(m_layouts.at(m_current_device));
        m_paint->SetDescriptorSet(m_descriptor_sets.at(m_current_device));
        m_paint->SetMaterialTableSlot(m_material_table_slots.at(m_current_device));
        m_paint->SetPipelineCache(cache->second);
        m_paint->SetFrameSlots(m_frame_slots.at(m_current_device));
        m_paint->SetFrameBuffers(m_buffers_frame.at(m_current_device));
        m_paint->SetFrameTables(m_frame_tables.at(m_current_device));
        m_paint->SetOverlays(overlays);
        if (m_acceleration) m_paint->SetAcceleration(m_acceleration);  // the frame's first recorded window builds the TLAS after its Compute pass

        auto const shadow_map = m_shadow_maps.find(m_current_device);
        if (shadow_map != m_shadow_maps.end()) m_paint->SetShadowMap(shadow_map->second);

        return m_paint->CreateCommandBuffers();
    }

    // Anything else still holding a peer (Scene, through the server) must have let go first, or they outlive the allocator
    std::shared_ptr<IIr77Return const> DestroyRenderer() {
        WaitIdle();

        ReleasePeers();

        ReleaseDevices();

        m_display_windows.clear();

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Ir77PVInit: renderer destroyed.");
    }

    std::shared_ptr<IIr77Return const> GetCEF(std::shared_ptr<IIr77PVCEF>& cef) {
        if (!m_cef) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: CreateRenderer not called.");

        cef = m_cef;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetPaint(std::shared_ptr<IIr77PVPaint>& paint) {
        if (!m_paint) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: CreateRenderer not called.");

        paint = m_paint;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetTransfer(std::shared_ptr<IIr77PVTransfer>& transfer) {
        if (!m_transfer) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: CreateRenderer not called.");

        transfer = m_transfer;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetMaterials(std::shared_ptr<IIr77PVMaterial>& materials) {
        if (!m_materials) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: CreateRenderer not called.");

        materials = m_materials;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetGeometry(std::shared_ptr<IIr77PVGeometry>& geometry) {
        if (!m_geometry) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: CreateRenderer not called.");

        geometry = m_geometry;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // NotConfigured on a device without ray queries -- check GetRayQuery first
    std::shared_ptr<IIr77Return const> GetAcceleration(std::shared_ptr<IIr77PVAcceleration>& acceleration) {
        if (!m_acceleration) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: no acceleration structures (no ray queries, or CreateRenderer not called).");

        acceleration = m_acceleration;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetWorld(std::shared_ptr<IIr77PVWorld>& world) {
        if (!m_world) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: CreateRenderer not called.");

        world = m_world;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The display device has ray queries -- and so an Acceleration
    std::shared_ptr<IIr77Return const> GetRayQuery(bool& enabled) {
        enabled = false;

        auto const device = m_devices.find(m_current_device);
        if (device == m_devices.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: no display device.");

        return device->second->GetRayQuery(enabled);
    }

    // The display device has mesh shading -- assets can take the mesh path
    std::shared_ptr<IIr77Return const> GetMeshShading(bool& enabled) {
        enabled = false;

        auto const device = m_devices.find(m_current_device);
        if (device == m_devices.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: no display device.");

        return device->second->GetMeshShading(enabled);
    }

    // The display device's shader registry -- CompileShader, LoadShader / AddShader, queries
    std::shared_ptr<IIr77Return const> GetShader(std::shared_ptr<IIr77PVShader>& shader) {
        auto const found = m_shaders.find(m_current_device);
        if (found == m_shaders.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: CreateRenderer not called.");

        shader = found->second;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // per frame / runtime
    // -------------------------------------------------------------------------------------------------------------------------------------

    // Once per frame, before Paint records, with the frame-in-flight index Paint is about to record. Transfer first (slot aging, finished
    // staging, the graveyard), then Geometry (this frame's copy of every dirty indirect buffer).
    std::shared_ptr<IIr77Return const> Tick(std::uint32_t const& frame) {
        std::shared_ptr<IIr77Return const> result{};

        if (!m_transfer || !m_geometry) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: CreateRenderer not called.");

        if (Failed(m_transfer->Tick(), result)) return result;

        return m_geometry->Tick(frame);
    }

    // Puts SPIR-V into the registry under name -- replacing the shader of that name (same handle) or adding it -- then rebuilds every pipeline
    // that used it and reloads the manifest, so pipelines naming a shader that wasn't registered before resolve now. Waits for the device to
    // go idle first: the old pipelines may still be recording. A shader that breaks the contract fails and changes nothing.
    std::shared_ptr<IIr77Return const> ReloadShader(std::string const& name, std::vector<std::uint32_t> const& spirv) {
        auto const shaders = m_shaders.find(m_current_device);
        if (shaders == m_shaders.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: CreateRenderer not called.");

        auto const cache = m_pipeline_caches.find(m_current_device);
        if (cache == m_pipeline_caches.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: DefinePipelines not called.");

        auto const manifest = m_pipeline_manifests.find(m_current_device);
        if (manifest == m_pipeline_manifests.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: no pipeline manifest.");

        auto const& shader = shaders->second;
        auto const& pipeline_cache = cache->second;

        VkDevice device{VK_NULL_HANDLE};
        m_devices.at(m_current_device)->GetDevice(&device);
        vkDeviceWaitIdle(device);

        std::shared_ptr<IIr77Return const> result{};
        Ir77PVShaderHandle handle{IR77PV_NO_SHADER};

        if (shader->FindShader(name, handle)->ID() == GUIDIr77OperationSucceeded) {
            if (Failed(shader->ReplaceShader(handle, spirv), result)) return result;
            if (Failed(pipeline_cache->InvalidateShader(handle), result)) return result;
        } else {
            if (Failed(shader->AddShader(name, spirv, handle), result)) return result;
        }

        return pipeline_cache->LoadManifest(manifest->second);
    }

    // Name of the GPU this renderer runs on -- for stats / logs
    std::shared_ptr<IIr77Return const> GetDeviceName(std::string& name) {
        auto const device = m_devices.find(m_current_device);
        if (device == m_devices.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: no display device.");

        VkPhysicalDeviceProperties properties{};
        device->second->GetDeviceProperties(properties);

        name = properties.deviceName;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // pipelines
    // -------------------------------------------------------------------------------------------------------------------------------------

    // A manifest pipeline's handle -- what draw items carry
    std::shared_ptr<IIr77Return const> FindPipeline(std::string const& name, Ir77PVPipelineHandle& handle) {
        auto const found = m_pipeline_caches.find(m_current_device);
        if (found == m_pipeline_caches.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: DefinePipelines not called.");

        return found->second->FindPipeline(name, handle);
    }

    // An engine role's handle -- follows the manifest's "roles"
    std::shared_ptr<IIr77Return const> FindPipeline(Ir77PVPipelineKind const& kind, Ir77PVPipelineHandle& handle) {
        auto const found = m_pipeline_caches.find(m_current_device);
        if (found == m_pipeline_caches.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: DefinePipelines not called.");

        return found->second->FindPipeline(kind, handle);
    }

    // Before CreateRenderer: overrides <shader root>/pipelines.json. After: reloads now -- a bad file fails and the old manifest stays.
    std::shared_ptr<IIr77Return const> SetPipelineManifest(std::string const& path) {
        auto const cache = m_pipeline_caches.find(m_current_device);
        if (cache != m_pipeline_caches.end()) {
            auto const result = cache->second->LoadManifest(path);
            if (result->ID() != GUIDIr77OperationSucceeded) return result;
        }

        m_pipeline_manifests[m_current_device] = path;  // ReloadShader reloads this one from now on

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    // =========================================================================================================================================
    // CreateRenderer's steps, in the order it runs them
    // =========================================================================================================================================

    // -------------------------------------------------------------------------------------------------------------------------------------
    // instance / device / surface -- unchanged order
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> DefineInstance() {
        auto instance = std::make_shared<Ir77PVInstance>();

        m_instance = std::static_pointer_cast<IIr77PVInstance>(instance);

        instance->DefineAppInfo();

        instance->DefineExtensions();

        instance->DefineCreateInfo();

        instance->DefineCreateInstance();

        instance->QueryDeviceCount(m_device_count);

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Instance.");
    }

    std::shared_ptr<IIr77Return const> DefinePhysicalDevices() {
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

            device->SetPhysicalDevice(static_cast<std::uint32_t>(i));
        }

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Enumerate Devices.");
    }

    // picks and creates the device that drives the display. Candidates need VK_KHR_swapchain and a graphics queue that can present
    // to an SDL window; they're tried discrete > integrated > virtual > other > CPU, and the first whose logical device is created wins.
    // Multi-device allocation comes with in-app setup.
    std::shared_ptr<IIr77Return const> DefineDisplayDevice(std::vector<SDL_Window*> const& windows, std::uint64_t& device_id) {
        if (windows.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "DefineDisplayDevice: no window.");

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
            auto const& device = m_devices.at(c.id);

            if (device->CreateDevice()->ID() == GUIDIr77OperationSucceeded) {
                m_current_device = c.id;
                device_id = c.id;

                bool mesh_shading{false};
                bool ray_query{false};
                device->GetMeshShading(mesh_shading);
                device->GetRayQuery(ray_query);

                std::string const features = std::string{" (mesh shading "} + (mesh_shading ? "on" : "off") + ", ray queries " + (ray_query ? "on" : "off") + ")";

                if (c.rank == 0)
                    return Ir77RETURN<Ir77OperationSucceeded>(this, "DefineDisplayDevice: only a CPU device (" + c.name +
                                                                        ") -- software rendering, check the GPU driver." + features + skipped);

                return Ir77RETURN<Ir77OperationSucceeded>(this, "DefineDisplayDevice: " + c.name + features + skipped);
            }

            skipped += " [skipped " + c.name + "]";
        }

        m_current_device = 0;

        return Ir77RETURN<Ir77NotConfigured>(this, "DefineDisplayDevice: no device could drive the display." + skipped);
    }

    std::shared_ptr<IIr77Return const> DefineSurfaces(std::map<std::uint64_t, std::vector<SDL_Window*>> const& windows) {
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

    std::shared_ptr<IIr77Return const> DefineAllocator() {
        VkInstance instance;
        m_instance->GetInstance(&instance);

        VkPhysicalDevice phys_device;
        m_devices.at(m_current_device)->GetPhysicalDevice(&phys_device);

        VkDevice device;
        m_devices.at(m_current_device)->GetDevice(&device);

        // buffer device address -- acceleration-structure builds read geometry by GPU address; VMA must allocate memory that allows it
        bool device_address{false};
        m_devices.at(m_current_device)->GetBufferDeviceAddress(device_address);

        VmaAllocatorCreateInfo allocator_info{};
        allocator_info.flags = device_address ? VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT : 0;
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
    std::shared_ptr<IIr77Return const> DefineLayouts() {
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
    std::shared_ptr<IIr77Return const> DefineRenderPasses() {
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

    std::shared_ptr<IIr77Return const> DefineSwapchains() {
        auto& swapchains = m_swapchains.at(m_current_device);

        auto const main_pass = m_render_passes.at(m_current_device).find(Ir77PVRenderPassKind::Main);
        if (main_pass == m_render_passes.at(m_current_device).end())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: main render pass must be created before swapchains.");

        for (auto& swapchain : swapchains) {
            swapchain->SetAllocator(m_allocators.at(m_current_device));

            swapchain->SetRenderPass(main_pass->second);

            if (swapchain->CreateSwapchain()->ID() != GUIDIr77OperationSucceeded)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: Rebuild failed.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Swapchains.");
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // placeholder texture -- lets every image binding be written before real textures exist
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> DefinePlaceholderTexture() {
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
    // shadow map -- one depth texture per device, shared by every window: each window's Shadow pass rewrites it before its Main pass samples
    // it, all on one queue. Format from the Shadow render pass, so image and pipelines agree. Compare sampler, white border = lit.
    // After DefineRenderPasses, before DefineDescriptorSets (its slot goes into every frame table) and Paint::CreateCommandBuffers.
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> DefineShadowMap(std::uint32_t const& size = 2048) {
        std::shared_ptr<IIr77Return const> result{};

        auto const& passes = m_render_passes.at(m_current_device);
        auto const shadow_pass = passes.find(Ir77PVRenderPassKind::Shadow);
        if (shadow_pass == passes.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: shadow render pass must be created before the shadow map.");

        VkFormat format{VK_FORMAT_UNDEFINED};
        shadow_pass->second->GetDepthFormat(&format);
        if (format == VK_FORMAT_UNDEFINED) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: shadow render pass has no depth format.");

        VkCommandPool pool{VK_NULL_HANDLE};
        VkQueue queue{VK_NULL_HANDLE};
        if (Failed(m_devices.at(m_current_device)->GetUploadPool(pool, queue), result)) return result;

        auto texture = std::static_pointer_cast<IIr77PVTexture>(std::make_shared<Ir77PVTexture>());

        texture->SetDevice(m_devices.at(m_current_device));
        texture->SetAllocator(m_allocators.at(m_current_device));
        texture->SetKind(Ir77PVTextureKind::Depth);  // first -- compare sampler, clamp to border
        texture->SetFormat(format);

        if (Failed(texture->CreateDepth(pool, queue, size, size), result)) return result;

        m_shadow_maps[m_current_device] = texture;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Shadow Map.");
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // frame buffers -- one set of slots per window, Dynamic, one copy per frame in flight
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> DefineFrameBuffers(Ir77PVFrameSizes const& sizes = Ir77PVFrameSizes{}) {
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
    // plus a frame table listing their slots. Requires DefineLayouts, DefinePlaceholderTexture, DefineFrameBuffers.
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> DefineDescriptorSets() {
        auto result = RegisterPlaceholders();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        result = DefineMaterialTable();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        result = RegisterFrameBuffers();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Descriptor Sets.");
    }
    // -------------------------------------------------------------------------------------------------------------------------------------
    // pipelines -- one Ir77PVPipelineCache per device, shared by every window: a pipeline is built the first time its description is asked
    // for, and the driver's compiled code is kept on disk between runs. Asking for the kinds here only warms the cache at startup; kinds
    // whose shaders or render pass are missing are skipped. Needs DefineLayouts, DefineRenderPasses and the shader registry (CreateRenderer).
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> DefinePipelines(std::vector<Ir77PVPipelineKind> const& kinds) {
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

    // Every engine role -- DefinePipelines warms the cache with them. A role the manifest leaves out, or points at an unavailable pipeline,
    // is skipped there (static and cef can't be: the manifest fails to load without them).
    std::shared_ptr<IIr77Return const> GetPipelineKinds(std::vector<Ir77PVPipelineKind>& kinds) {
        kinds = {Ir77PVPipelineKind::Static,      Ir77PVPipelineKind::Skinned, Ir77PVPipelineKind::Shadow, Ir77PVPipelineKind::ShadowSkinned,
                 Ir77PVPipelineKind::Transparent, Ir77PVPipelineKind::CEF,     Ir77PVPipelineKind::Mesh,   Ir77PVPipelineKind::SkinnedTransparent};

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // teardown and helpers
    // =========================================================================================================================================

    // The peers, users first: World holds the rest; Paint and Geometry hold buffers Transfer made; Materials uploads through Transfer; CEF
    // last. Each waits for its device itself, and everything they hold dies before ReleaseDevices frees the allocator.
    void ReleasePeers() {
        m_world.reset();
        m_paint.reset();
        m_acceleration.reset();  // its BLASes read Geometry's buffers; its own buffers are Transfer's
        m_geometry.reset();
        m_materials.reset();
        m_transfer.reset();
        m_cef.reset();
    }

    // Every device idle -- before anything it may still be using is destroyed
    void WaitIdle() {
        for (auto& [device_id, device] : m_devices) {
            VkDevice vk_device{VK_NULL_HANDLE};
            device->GetDevice(&vk_device);
            if (vk_device != VK_NULL_HANDLE) vkDeviceWaitIdle(vk_device);
        }
    }

    // Everything Init holds, in dependency order. Whatever holds VMA memory dies before its allocator; render passes and shaders before
    // the device; surfaces (in the swapchains) before the instance. Safe to run twice.
    void ReleaseDevices() {
        WaitIdle();

        m_compute.clear();
        m_frame_tables.clear();
        m_material_tables.clear();
        m_buffers_frame.clear();
        m_swapchains.clear();
        m_pipeline_caches.clear();  // saves the cache file, destroys every pipeline
        m_placeholders.clear();
        m_descriptor_sets.clear();  // after everything that registered slots in it
        m_layouts.clear();
        m_shadow_maps.clear();

        for (auto& [device_id, allocator] : m_allocators) vmaDestroyAllocator(allocator);
        m_allocators.clear();

        m_render_passes.clear();
        m_shaders.clear();
        m_devices.clear();
        m_instance.reset();

        m_frame_slots.clear();
        m_material_table_slots.clear();
        m_placeholder_slots.clear();
        m_pipeline_manifests.clear();
        m_windows.clear();
        m_device_count = 0;
        m_current_device = 0;
    }

    // White first -- it must land in texture slot 0 (BINDLESS_PLACEHOLDER_TEXTURE), so unset texture slots sample white
    std::shared_ptr<IIr77Return const> RegisterPlaceholders() {
        auto const& table = m_descriptor_sets.at(m_current_device);

        auto const device = m_placeholders.find(m_current_device);
        if (device == m_placeholders.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: DefinePlaceholderTexture not called.");

        std::map<Ir77PVPlaceholderKind, std::uint32_t> slots{};

        for (auto const kind : {Ir77PVPlaceholderKind::White, Ir77PVPlaceholderKind::Normal}) {
            auto const found = device->second.find(kind);
            if (found == device->second.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: placeholder texture missing.");

            VkDescriptorImageInfo info{};
            found->second->GetImageInfo(&info);

            std::uint32_t slot{BINDLESS_NO_SLOT};

            auto const result = table->RegisterTexture(info.imageView, info.sampler, Ir77PVSlotKind::Texture, slot);
            if (result->ID() != GUIDIr77OperationSucceeded) return result;

            slots.emplace(kind, slot);
        }

        if (slots.at(Ir77PVPlaceholderKind::White) != BINDLESS_PLACEHOLDER_TEXTURE)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: white placeholder must be the first texture registered.");

        m_placeholder_slots[m_current_device] = slots;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // MATERIAL_CAPACITY rows of Ir77PVMaterialRow in one Static buffer -- Ir77PVAsset writes rows into it
    std::shared_ptr<IIr77Return const> DefineMaterialTable() {
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

        result = m_descriptor_sets.at(m_current_device)->RegisterBuffer(info.buffer, info.offset, info.range, Ir77PVSlotKind::MaterialTable, slot);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        m_material_tables[m_current_device] = buffer;
        m_material_table_slots[m_current_device] = slot;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Per window, per frame in flight: every frame buffer's copy gets a slot -- the camera buffer one per view, VIEW_STRIDE apart -- then one
    // frame table per view, VIEW_STRIDE apart in the frame table buffer, each listing its view's camera and the shared rest, gets its own slot
    std::shared_ptr<IIr77Return const> RegisterFrameBuffers() {
        auto const& table = m_descriptor_sets.at(m_current_device);
        auto const& frame_buffers = m_buffers_frame.at(m_current_device);

        std::uint32_t const white{m_placeholder_slots.at(m_current_device).at(Ir77PVPlaceholderKind::White)};

        // the shadow map's slot -- registered once, every window and frame reads the same image; white when DefineShadowMap wasn't called
        std::uint32_t shadow_map{white};

        auto const shadow = m_shadow_maps.find(m_current_device);
        if (shadow != m_shadow_maps.end()) {
            VkDescriptorImageInfo info{};
            shadow->second->GetImageInfo(&info);  // imageLayout = DEPTH_STENCIL_READ_ONLY_OPTIMAL

            auto const shadow_result = table->RegisterTexture(info.imageView, info.sampler, Ir77PVSlotKind::ShadowMap, shadow_map, info.imageLayout);
            if (shadow_result->ID() != GUIDIr77OperationSucceeded) return shadow_result;
        }

        std::vector<std::shared_ptr<IIr77PVBuffer>> frame_tables{};
        std::vector<std::array<Ir77PVFrameSlots, MAX_FRAMES_IN_FLIGHT>> frame_slots(frame_buffers.size());

        for (std::size_t w = 0; w < frame_buffers.size(); w++) {
            auto frame_table = std::static_pointer_cast<IIr77PVBuffer>(std::make_shared<Ir77PVBuffer>());

            frame_table->SetDevice(m_devices.at(m_current_device));

            frame_table->SetAllocator(m_allocators.at(m_current_device));

            frame_table->SetUsage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

            frame_table->SetMode(Ir77PVBufferMode::Dynamic);

            auto result = frame_table->CreateResources(VIEW_STRIDE * MAX_VIEWS, MAX_FRAMES_IN_FLIGHT);
            if (result->ID() != GUIDIr77OperationSucceeded) return result;

            for (std::uint32_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++) {
                Ir77PVFrameSlots& slots = frame_slots[w][frame];

                for (std::uint32_t view = 0; view < MAX_VIEWS; view++) {
                    slots.cameras[view] = RegisterFrameCopy(table, frame_buffers[w].at(Ir77PVBufferSlot::Camera), Ir77PVSlotKind::Camera, frame,
                                                            view * VIEW_STRIDE, VIEW_STRIDE);
                    if (slots.cameras[view] == BINDLESS_NO_SLOT) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: camera registration failed.");
                }

                slots.values.camera = slots.cameras[0];
                slots.values.lights = RegisterFrameCopy(table, frame_buffers[w].at(Ir77PVBufferSlot::Lights), Ir77PVSlotKind::Lights, frame);
                slots.values.instances = RegisterFrameCopy(table, frame_buffers[w].at(Ir77PVBufferSlot::Instances), Ir77PVSlotKind::Instances, frame);
                slots.values.shadow_matrices =
                    RegisterFrameCopy(table, frame_buffers[w].at(Ir77PVBufferSlot::ShadowMatrices), Ir77PVSlotKind::ShadowMatrices, frame);
                slots.values.bones = RegisterFrameCopy(table, frame_buffers[w].at(Ir77PVBufferSlot::Bones), Ir77PVSlotKind::Bones, frame);
                slots.values.env = white;  // until a real environment exists
                slots.values.shadow_map = shadow_map;

                if (slots.values.camera == BINDLESS_NO_SLOT || slots.values.lights == BINDLESS_NO_SLOT || slots.values.instances == BINDLESS_NO_SLOT ||
                    slots.values.shadow_matrices == BINDLESS_NO_SLOT || slots.values.bones == BINDLESS_NO_SLOT)
                    return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: frame buffer registration failed.");

                for (std::uint32_t view = 0; view < MAX_VIEWS; view++) {
                    Ir77PVFrameTable values = slots.values;
                    values.camera = slots.cameras[view];

                    result = frame_table->Update(frame, &values, sizeof(Ir77PVFrameTable), view * VIEW_STRIDE);
                    if (result->ID() != GUIDIr77OperationSucceeded) return result;

                    slots.tables[view] = RegisterFrameCopy(table, frame_table, Ir77PVSlotKind::FrameTable, frame, view * VIEW_STRIDE, VIEW_STRIDE);
                    if (slots.tables[view] == BINDLESS_NO_SLOT) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: frame table registration failed.");
                }
            }

            frame_tables.push_back(frame_table);
        }

        m_frame_tables[m_current_device] = frame_tables;
        m_frame_slots[m_current_device] = frame_slots;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // One frame copy of a Dynamic buffer -> its bindless slot, holding kind; BINDLESS_NO_SLOT on failure. offset / range pick a piece of the
    // copy (one view's camera or frame table); the default is the whole copy.
    std::uint32_t RegisterFrameCopy(std::shared_ptr<IIr77PVDescriptorSet> const& table, std::shared_ptr<IIr77PVBuffer> const& buffer,
                                    Ir77PVSlotKind const& kind, std::uint32_t const& frame, VkDeviceSize const& offset = 0,
                                    VkDeviceSize const& range = VK_WHOLE_SIZE) {
        VkDescriptorBufferInfo info{};
        if (buffer->GetBufferInfo(frame, &info)->ID() != GUIDIr77OperationSucceeded) return BINDLESS_NO_SLOT;

        info.offset += offset;
        if (range != VK_WHOLE_SIZE) info.range = range;

        std::uint32_t slot{BINDLESS_NO_SLOT};
        if (table->RegisterBuffer(info.buffer, info.offset, info.range, kind, slot)->ID() != GUIDIr77OperationSucceeded) return BINDLESS_NO_SLOT;

        return slot;
    }

    // The device's pipeline cache, wired to its layout, shaders and every render pass. A no-op once it exists.
    std::shared_ptr<IIr77Return const> DefinePipelineCache() {
        if (m_pipeline_caches.count(m_current_device) != 0) return Ir77RETURN<Ir77OperationSucceeded>();

        auto const shaders = m_shaders.find(m_current_device);
        if (shaders == m_shaders.end())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: shaders not loaded -- CreateRenderer loads them before the pipelines.");

        auto const passes = m_render_passes.find(m_current_device);
        if (passes == m_render_passes.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVInit: DefineRenderPasses not called.");

        auto cache = std::static_pointer_cast<IIr77PVPipelineCache>(std::make_shared<Ir77PVPipelineCache>());

        cache->SetDevice(m_devices.at(m_current_device));

        cache->SetLayout(m_layouts.at(m_current_device));

        cache->SetShader(shaders->second);

        for (auto const& [kind, render_pass] : passes->second) cache->SetRenderPass(kind, render_pass);

        auto const result = cache->CreatePipelineCache();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        auto const manifest = m_pipeline_manifests.find(m_current_device);
        if (manifest == m_pipeline_manifests.end())
            return Ir77RETURN<Ir77NotConfigured>(this,
                                                 "Ir77PVInit: no pipeline manifest -- put pipelines.json in the shader root or call SetPipelineManifest.");

        auto const loaded = cache->LoadManifest(manifest->second);

        if (loaded->ID() != GUIDIr77OperationSucceeded) return loaded;

        m_pipeline_caches[m_current_device] = cache;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    /*************************** servers ********************************/
    // owned here, released first -- they hold device resources this class frees after them
    std::shared_ptr<IIr77PVCEF> m_cef{};

    std::shared_ptr<IIr77PVPaint> m_paint{};

    std::shared_ptr<IIr77PVTransfer> m_transfer{};

    std::shared_ptr<IIr77PVMaterial> m_materials{};

    std::shared_ptr<IIr77PVGeometry> m_geometry{};

    std::shared_ptr<IIr77PVAcceleration> m_acceleration{};  // ray-query devices only

    std::shared_ptr<IIr77PVWorld> m_world{};  // assets and frames -- over all of the above

    std::vector<SDL_Window*> m_display_windows{};  // SetWindows -- what CreateRenderer puts the display device on

    /*************************** device *********************************/
    std::shared_ptr<IIr77PVInstance> m_instance;

    std::uint32_t m_device_count{0};

    std::uint64_t m_current_device{0};

    std::map<std::uint64_t, std::shared_ptr<IIr77PVDevice>> m_devices;

    std::map<std::uint64_t, std::vector<SDL_Window*>> m_windows;

    std::map<std::uint64_t, VmaAllocator> m_allocators;

    std::map<std::uint64_t, std::shared_ptr<IIr77PVShader>> m_shaders;  // [device] -- the registry CreateRenderer loads; the pipeline cache builds on it

    std::map<std::uint64_t, std::vector<std::shared_ptr<IIr77Enlisted>>> m_compute;

    /*************************** passes *********************************/
    std::map<std::uint64_t, std::vector<std::shared_ptr<IIr77PVSwapchain>>> m_swapchains;

    std::map<std::uint64_t, std::map<Ir77PVRenderPassKind, std::shared_ptr<IIr77PVRenderPass>>> m_render_passes;

    /*************************** layout / pipelines *********************/
    // [device] -- the one bindless layout and the one bindless table
    std::map<std::uint64_t, std::shared_ptr<IIr77PVLayout>> m_layouts;

    std::map<std::uint64_t, std::shared_ptr<IIr77PVDescriptorSet>> m_descriptor_sets;

    // [device] -- every pipeline, keyed by description
    std::map<std::uint64_t, std::shared_ptr<IIr77PVPipelineCache>> m_pipeline_caches;

    // [device] -- pipelines.json; CreateRenderer fills it from the shader root unless SetPipelineManifest did
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

    // [device][kind]
    std::map<std::uint64_t, std::map<Ir77PVPlaceholderKind, std::shared_ptr<IIr77PVTexture>>> m_placeholders;

    // [device] -- the sun's shadow map (depth, compare sampler)
    std::map<std::uint64_t, std::shared_ptr<IIr77PVTexture>> m_shadow_maps;

    /**********************************************************************/
};
}  // namespace NSIr77PeregrineV