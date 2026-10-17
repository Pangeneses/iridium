#pragma once

#include <SDL3/SDL_video.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <meshoptimizer.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include "../types/Ir77PVTypes.hpp"

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77PVShader.hpp"

#include "Ir77PVInit.hpp"

#include "../runtime/Ir77PVBuffer.hpp"
#include "../runtime/Ir77PVDescriptorSet.hpp"
#include "../runtime/Ir77PVShader.hpp"
#include "../runtime/Ir77PVTexture.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// =============================================================================================================================================
// Ir77PVAsset -- the data binding point between a client (scene loader / game layer) and PeregrineV.
//
//   startup:   SetPeregrineV -> [SetShaderRoot] -> CreateShaders (every .spv in the root) -> (CreatePipelines) -> placeholder textures -> CreateDescriptorSets (bindless table)
//   load:      CreateTexture -> CreateMaterial / UploadMaterials -> UploadScene (vertex path) or UploadMeshlets (mesh path)
//   gpu data:  CreateStorageBuffer (compute writes it, shaders read it by slot) -> MakeProceduralDrawItem / Ir77PVDispatch -> DestroyStorageBuffer
//
// Bindless: textures get texture slots, materials are rows in the device's material table (holding texture slots), meshlet buffers get
// buffer slots. Draw items carry rows and slots; Ir77PVCmdBuffer turns them into push constants.
//   frame:     Tick(frame) -> camera / Instances via Paint -> Make*DrawItem per material run -> Paint::SetDraws
//   runtime:   SetSceneObjectVisible / SetSceneObjectInstances / SetMeshletObjectVisible / SetMeshletRecordVisible
//   unload:    remove the draw items from Paint, then UnloadScene / UnloadMeshlets / UnloadMesh (freed after the frames in flight)
//
// Every upload takes `wait`. true = blocks until the copy is done (load screens). false = returns at once; the copy runs in submission
// order ahead of later frames, and Tick frees its staging when the GPU is finished with it.
// =============================================================================================================================================
class Ir77PVAsset : public Ir77Enlisted, public std::enable_shared_from_this<Ir77PVAsset> {
   public:
    Ir77PVAsset() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    // m_context outlives this object (held by shared_ptr), so devices and allocators are still alive here
    ~Ir77PVAsset() {
        if (!m_context) return;

        for (auto const& [device_id, device] : m_context->m_devices) {
            VkDevice vk_device{VK_NULL_HANDLE};
            device->GetDevice(&vk_device);
            if (vk_device == VK_NULL_HANDLE) continue;

            vkDeviceWaitIdle(vk_device);

            // uploads still pending -- finished after the wait; each staging buffer frees itself
            auto const pending = m_pending.find(device_id);
            if (pending != m_pending.end()) pending->second.clear();

            // textures release their own image / view / sampler -- the device is idle and the allocator still alive here
            auto const textures = m_textures.find(device_id);
            if (textures != m_textures.end()) textures->second.clear();

            // storage buffers free themselves -- same reason
            auto const storage = m_storage_buffers.find(device_id);
            if (storage != m_storage_buffers.end()) storage->second.clear();
        }

        m_graveyard.clear();  // buffers are still listed in the context slots, which tear them down
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVAsset>(uid);

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

        else if (iid == GUIDIr77PVAsset)
            obj = std::shared_ptr<Ir77PVAsset>(shared_from_this(), static_cast<Ir77PVAsset*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> SetPVInit(std::shared_ptr<Ir77PVInit>& context) {
        m_context = context;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetShaderRoot(std::string const& root) {
        m_shader_root = root;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Camera for set 0 binding 0. Vulkan clip space is Y-down, so the projection is flipped.
    static Ir77PVCamera MakeCamera(glm::vec3 const& eye, glm::vec3 const& target, float const& fov_degrees, float const& aspect, float const& znear = 0.1f,
                                   float const& zfar = 100.0f) {
        Ir77PVCamera camera{};

        camera.eye = glm::vec4(eye, 1.0f);
        camera.view = glm::lookAtRH(eye, target, glm::vec3(0.0f, 1.0f, 0.0f));
        camera.proj = glm::perspectiveRH_ZO(glm::radians(fov_degrees), aspect, znear, zfar);
        camera.proj[1][1] *= -1.0f;
        camera.view_proj = camera.proj * camera.view;

        return camera;
    }

    // =========================================================================================================================================
    // per frame -- call once per frame, before Paint records, with the frame-in-flight index Paint is about to record.
    //   0. ages the bindless table's released slots
    //   1. drops the staging buffers of uploads whose fence has signalled
    //   2. writes this frame's copy of every dirty indirect buffer (visibility / instance changes)
    //   3. ages the graveyard and releases anything no frame in flight can still read
    // =========================================================================================================================================
    std::shared_ptr<IIr77Return const> Tick(std::uint32_t const& frame) {
        if (frame >= MAX_FRAMES_IN_FLIGHT) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: frame index out of range.");

        std::uint64_t const device_id = m_context->m_current_device;

        // 0. released slots become reusable after MAX_FRAMES_IN_FLIGHT Ticks
        m_context->m_descriptor_sets.at(device_id)->Tick();

        // 1. finished uploads -- dropping the staging buffer frees its memory, command buffer and fence
        std::erase_if(m_pending[device_id], [](std::shared_ptr<IIr77PVBuffer> const& staging) {
            bool finished{true};
            staging->IsUploadFinished(finished);
            return finished;
        });

        // 2. dirty indirect copies -- one frame copy per Tick until every copy matches
        for (auto& scene : m_scenes[device_id]) {
            if (!scene.alive || scene.dirty == 0 || !scene.indirect) continue;

            VkDeviceSize const size = static_cast<VkDeviceSize>(scene.commands.size() * sizeof(VkDrawIndexedIndirectCommand));
            if (scene.indirect->Update(frame, scene.commands.data(), size)->ID() != GUIDIr77OperationSucceeded)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: scene indirect write failed.");

            scene.dirty--;
        }

        for (auto& slot : m_meshlet_scenes[device_id]) {
            if (!slot.alive || slot.dirty == 0 || !slot.mesh.indirect) continue;

            VkDeviceSize const size = static_cast<VkDeviceSize>(slot.commands.size() * sizeof(VkDrawMeshTasksIndirectCommandEXT));
            if (slot.mesh.indirect->Update(frame, slot.commands.data(), size)->ID() != GUIDIr77OperationSucceeded)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: meshlet indirect write failed.");

            slot.dirty--;
        }

        // 3. graveyard
        auto& graves = m_graveyard[device_id];
        auto& vertex_slot = m_context->m_buffers_vertex[device_id];
        auto& index_slot = m_context->m_buffers_index[device_id];

        std::size_t keep{0};
        for (std::size_t i = 0; i < graves.size(); i++) {
            if (graves[i].frames_left > 0) {
                graves[i].frames_left--;
                if (keep != i) graves[keep] = std::move(graves[i]);  // self-move would empty the vectors
                keep++;
                continue;
            }

            for (auto const& buffer : graves[i].buffers) {
                std::erase(vertex_slot, buffer);
                std::erase(index_slot, buffer);
            }
        }
        graves.resize(keep);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // shaders
    // =========================================================================================================================================

    // Registers every .spv under the shader root, each under its file name without ".spv" ("vert", "skinned.vert", "heightmap.frag"); the
    // stage comes from the SPIR-V. A file that breaks the shader contract (Ir77PVTypes.hpp) -- or a task / mesh shader on a device without mesh
    // shading -- is skipped with a warning. Which shaders combine into which pipelines is the manifest's job: <shader root>/pipelines.json
    // becomes the device's manifest unless PeregrineV::SetPipelineManifest named one already. More shaders can be registered at any time
    // through the registry's LoadShader / AddShader.
    std::shared_ptr<IIr77Return const> CreateShaders() {
        std::error_code error{};
        if (!std::filesystem::is_directory(m_shader_root, error))
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: shader root not found: " + m_shader_root);

        auto shader = std::static_pointer_cast<IIr77PVShader>(std::make_shared<Ir77PVShader>());

        shader->SetDevice(m_context->m_devices.at(m_context->m_current_device));

        for (auto const& entry : std::filesystem::directory_iterator(m_shader_root, error)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".spv") continue;

            std::string const name{entry.path().stem().string()};  // "skinned.vert.spv" -> "skinned.vert"
            Ir77PVShaderHandle handle{IR77PV_NO_SHADER};

            if (shader->LoadShader(name, entry.path().string(), handle)->ID() != GUIDIr77OperationSucceeded)
                std::printf("[WARN] shader skipped: %s\n", entry.path().string().c_str());
        }

        if (error) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: cannot read the shader root: " + m_shader_root);

        std::filesystem::path const manifest = std::filesystem::path(m_shader_root) / "pipelines.json";
        if (std::filesystem::is_regular_file(manifest, error))
            m_context->m_pipeline_manifests.emplace(m_context->m_current_device, manifest.string());  // emplace -- an explicit one wins

        m_context->m_shaders[m_context->m_current_device] = shader;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Shaders.");
    }

    // Every engine role -- CreatePipelines warms the cache with them. A role the manifest leaves out, or points at an unavailable pipeline,
    // is skipped there (static and cef can't be: the manifest fails to load without them).
    std::shared_ptr<IIr77Return const> GetPipelineKinds(std::vector<Ir77PVPipelineKind>& kinds) {
        kinds = {Ir77PVPipelineKind::Static, Ir77PVPipelineKind::Skinned,     Ir77PVPipelineKind::Shadow, Ir77PVPipelineKind::ShadowSkinned,
                 Ir77PVPipelineKind::Transparent, Ir77PVPipelineKind::CEF, Ir77PVPipelineKind::Mesh,   Ir77PVPipelineKind::SkinnedTransparent};

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The handle a draw item carries for a manifest pipeline ("water") -- look it up once, reuse it for every item. Needs CreatePipelines.
    std::shared_ptr<IIr77Return const> FindPipeline(std::string const& name, Ir77PVPipelineHandle& handle) {
        auto const cache = m_context->m_pipeline_caches.find(m_context->m_current_device);
        if (cache == m_context->m_pipeline_caches.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: CreatePipelines not called.");

        return cache->second->FindPipeline(name, handle);
    }

    // The handle for an engine role -- follows the manifest's "roles"
    std::shared_ptr<IIr77Return const> FindPipeline(Ir77PVPipelineKind const& kind, Ir77PVPipelineHandle& handle) {
        auto const cache = m_context->m_pipeline_caches.find(m_context->m_current_device);
        if (cache == m_context->m_pipeline_caches.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: CreatePipelines not called.");

        return cache->second->FindPipeline(kind, handle);
    }

    // =========================================================================================================================================
    // storage buffers -- GPU data a compute pass writes and shaders read by slot (storage buffers, set 0 binding 1): height fields, particle
    // state, indirect commands. One copy shared by every frame in flight; the Compute pass waits for the previous frame's readers before
    // writing. Also usable as a draw's / dispatch's indirect buffer, or bound as vertex / index data.
    // =========================================================================================================================================

    // size bytes, filled from data (zeros when data is null). Blocking upload. buffer_id for Get / Upload / Destroy; slot for the shaders.
    std::shared_ptr<IIr77Return const> CreateStorageBuffer(VkDeviceSize const& size, void const* data, std::uint32_t& buffer_id, std::uint32_t& slot) {
        if (size == 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: storage buffer size is 0.");

        auto buffer = CreateStaticBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                             VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                         size);
        if (!buffer) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: storage buffer creation failed.");

        std::vector<std::uint8_t> zeros{};
        if (!data) zeros.assign(static_cast<std::size_t>(size), 0);

        VkBuffer dst{VK_NULL_HANDLE};
        buffer->GetBuffer(0, &dst);

        if (Upload({{dst, data ? data : zeros.data(), size}}, true)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: storage buffer upload failed.");

        auto const result = RegisterBuffer(buffer, slot);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        auto& buffers = m_storage_buffers[m_context->m_current_device];
        auto& slots = m_storage_slots[m_context->m_current_device];

        buffer_id = static_cast<std::uint32_t>(buffers.size());
        buffers.push_back(buffer);
        slots.push_back(slot);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The buffer itself -- for item.indirect / dispatch.indirect
    std::shared_ptr<IIr77Return const> GetStorageBuffer(std::uint32_t const& buffer_id, std::shared_ptr<IIr77PVBuffer>& buffer) {
        auto const found = m_storage_buffers.find(m_context->m_current_device);
        if (found == m_storage_buffers.end() || buffer_id >= found->second.size() || !found->second[buffer_id])
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown storage buffer.");

        buffer = found->second[buffer_id];

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // CPU rewrite of [offset, offset + size) -- resets, spawn parameters. The copy runs in submission order ahead of later frames.
    std::shared_ptr<IIr77Return const> UploadStorageBuffer(std::uint32_t const& buffer_id, void const* data, VkDeviceSize const& size, VkDeviceSize const& offset,
                                                           bool const& wait) {
        std::shared_ptr<IIr77PVBuffer> buffer{};

        auto const result = GetStorageBuffer(buffer_id, buffer);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        VkBuffer dst{VK_NULL_HANDLE};
        buffer->GetBuffer(0, &dst);

        return Upload({{dst, data, size, offset}}, wait);
    }

    // Releases the slot and frees the buffer once no frame in flight can read it. Remove the draws / dispatches using it first.
    std::shared_ptr<IIr77Return const> DestroyStorageBuffer(std::uint32_t const& buffer_id) {
        std::shared_ptr<IIr77PVBuffer> buffer{};

        auto const result = GetStorageBuffer(buffer_id, buffer);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        auto& slots = m_storage_slots[m_context->m_current_device];

        ReleaseSlots(BINDLESS_BINDING_BUFFERS, {slots[buffer_id]});
        Bury({buffer});

        m_storage_buffers[m_context->m_current_device][buffer_id].reset();
        slots[buffer_id] = BINDLESS_NO_SLOT;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Name of the GPU this context renders on -- for stats / logs
    std::shared_ptr<IIr77Return const> GetDeviceName(std::string& name) {
        VkPhysicalDeviceProperties properties{};
        m_context->m_devices.at(m_context->m_current_device)->GetDeviceProperties(properties);

        name = properties.deviceName;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // single-mesh path -- one Static vertex + index buffer pair per mesh. procedural / debug geometry, one-off loads.
    // =========================================================================================================================================
    std::shared_ptr<IIr77Return const> UploadMesh(Ir77PVInputBuffer const& input, std::uint32_t& mesh_id, bool const& wait = true) {
        if (input.vertex_data.empty() || input.index_data.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: empty mesh.");

        VkDeviceSize const vertex_size = static_cast<VkDeviceSize>(input.vertex_data.size() * sizeof(Ir77PVVertex));
        VkDeviceSize const index_size = static_cast<VkDeviceSize>(input.index_data.size() * sizeof(std::uint32_t));

        Ir77PVMesh mesh{};

        mesh.vertex = CreateStaticBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertex_size);
        mesh.index = CreateStaticBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, index_size);

        if (!mesh.vertex || !mesh.index) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: mesh buffer creation failed.");

        VkBuffer vertex_dst{VK_NULL_HANDLE};
        VkBuffer index_dst{VK_NULL_HANDLE};
        mesh.vertex->GetBuffer(0, &vertex_dst);
        mesh.index->GetBuffer(0, &index_dst);

        if (Upload({{vertex_dst, input.vertex_data.data(), vertex_size}, {index_dst, input.index_data.data(), index_size}}, wait)->ID() !=
            GUIDIr77OperationSucceeded) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: mesh upload failed.");
        }

        mesh.index_count = static_cast<std::uint32_t>(input.index_data.size());
        mesh.index_type = VK_INDEX_TYPE_UINT32;

        auto& meshes = m_meshes[m_context->m_current_device];
        mesh_id = static_cast<std::uint32_t>(meshes.size());
        meshes.push_back(mesh);

        // mirrored into the context slots so its teardown order covers them
        m_context->m_buffers_vertex[m_context->m_current_device].push_back(mesh.vertex);
        m_context->m_buffers_index[m_context->m_current_device].push_back(mesh.index);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetMesh(std::uint32_t const& mesh_id, Ir77PVMesh& mesh) {
        auto const found = m_meshes.find(m_context->m_current_device);
        if (found == m_meshes.end() || mesh_id >= found->second.size() || !found->second[mesh_id].vertex)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown mesh.");

        mesh = found->second[mesh_id];

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The id stays reserved; GetMesh fails for it afterwards. Remove its draw items from Paint first.
    std::shared_ptr<IIr77Return const> UnloadMesh(std::uint32_t const& mesh_id) {
        auto const found = m_meshes.find(m_context->m_current_device);
        if (found == m_meshes.end() || mesh_id >= found->second.size() || !found->second[mesh_id].vertex)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown mesh.");

        Ir77PVMesh& mesh = found->second[mesh_id];
        Bury({mesh.vertex, mesh.index});
        mesh = Ir77PVMesh{};

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // scene path (vertex) -- the client stacks every object into one Ir77PVSceneUpload and prebuilds one indirect command per object:
    //   firstIndex / indexCount   -> its slice of indices (indices stay local, 0-based)
    //   vertexOffset              -> where its vertices start
    //   instanceCount / firstInstance -> how many copies, and their slots in models[] (set 0 binding 2)
    // Sort objects by material so each material is one contiguous run of commands; one draw item per run (MakeIndirectDrawItem).
    // The indirect buffer is Dynamic, so objects can be hidden / shown / resized per frame without re-uploading geometry.
    // =========================================================================================================================================
    std::shared_ptr<IIr77Return const> UploadScene(Ir77PVSceneUpload const& data, std::uint32_t& scene_id, bool const& wait = true) {
        if (data.vertices.empty() && data.vertices_skinned.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: empty scene.");

        Ir77PVSceneSlot scene{};
        std::vector<Ir77PVUploadEntry> entries{};

        if (!data.vertices.empty()) {
            VkDeviceSize const size = static_cast<VkDeviceSize>(data.vertices.size() * sizeof(Ir77PVVertex));
            scene.vertex = CreateStaticBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, size);
            if (!scene.vertex) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: scene vertex buffer creation failed.");

            VkBuffer dst{VK_NULL_HANDLE};
            scene.vertex->GetBuffer(0, &dst);
            entries.push_back({dst, data.vertices.data(), size});
        }

        if (!data.indices.empty()) {
            VkDeviceSize const size = static_cast<VkDeviceSize>(data.indices.size() * sizeof(std::uint32_t));
            scene.index = CreateStaticBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, size);
            if (!scene.index) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: scene index buffer creation failed.");

            VkBuffer dst{VK_NULL_HANDLE};
            scene.index->GetBuffer(0, &dst);
            entries.push_back({dst, data.indices.data(), size});
        }

        if (!data.vertices_skinned.empty()) {
            VkDeviceSize const size = static_cast<VkDeviceSize>(data.vertices_skinned.size() * sizeof(Ir77PVVertexSkinned));
            scene.vertex_skinned = CreateStaticBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, size);
            if (!scene.vertex_skinned) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: scene skinned vertex buffer creation failed.");

            VkBuffer dst{VK_NULL_HANDLE};
            scene.vertex_skinned->GetBuffer(0, &dst);
            entries.push_back({dst, data.vertices_skinned.data(), size});
        }

        if (!data.indices_skinned.empty()) {
            VkDeviceSize const size = static_cast<VkDeviceSize>(data.indices_skinned.size() * sizeof(std::uint32_t));
            scene.index_skinned = CreateStaticBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, size);
            if (!scene.index_skinned) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: scene skinned index buffer creation failed.");

            VkBuffer dst{VK_NULL_HANDLE};
            scene.index_skinned->GetBuffer(0, &dst);
            entries.push_back({dst, data.indices_skinned.data(), size});
        }

        // indirect -- Dynamic, every frame copy written directly (not staged)
        if (!data.indirect_commands.empty()) {
            VkDeviceSize const size = static_cast<VkDeviceSize>(data.indirect_commands.size() * sizeof(VkDrawIndexedIndirectCommand));
            scene.indirect = CreateDynamicBuffer(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, size);
            if (!scene.indirect) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: scene indirect buffer creation failed.");

            for (std::uint32_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++) {
                if (scene.indirect->Update(frame, data.indirect_commands.data(), size)->ID() != GUIDIr77OperationSucceeded)
                    return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: scene indirect write failed.");
            }

            scene.commands = data.indirect_commands;
            scene.instance_counts.reserve(scene.commands.size());
            for (auto const& cmd : scene.commands) scene.instance_counts.push_back(cmd.instanceCount);
            scene.visible.assign(scene.commands.size(), 1);
        }

        if (Upload(entries, wait)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: scene upload failed.");

        scene.alive = true;

        std::uint64_t const device_id = m_context->m_current_device;

        auto& vertex_slot = m_context->m_buffers_vertex[device_id];
        auto& index_slot = m_context->m_buffers_index[device_id];
        if (scene.vertex) vertex_slot.push_back(scene.vertex);
        if (scene.vertex_skinned) vertex_slot.push_back(scene.vertex_skinned);
        if (scene.indirect) vertex_slot.push_back(scene.indirect);
        if (scene.index) index_slot.push_back(scene.index);
        if (scene.index_skinned) index_slot.push_back(scene.index_skinned);

        auto& scenes = m_scenes[device_id];
        scene_id = static_cast<std::uint32_t>(scenes.size());
        scenes.push_back(std::move(scene));

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Draw item for commands [first_command, first_command + command_count) of a scene -- one material run -- through an engine role
    std::shared_ptr<IIr77Return const> MakeIndirectDrawItem(std::size_t const& window, Ir77PVPipelineKind const& kind, std::uint32_t const& scene_id,
                                                            std::uint32_t const& material_id, std::uint32_t const& first_command,
                                                            std::uint32_t const& command_count, bool const& skinned, Ir77PVDrawItem& item) {
        Ir77PVPipelineHandle pipeline{IR77PV_NO_PIPELINE};

        auto const result = FindPipeline(kind, pipeline);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        return MakeIndirectDrawItem(window, pipeline, scene_id, material_id, first_command, command_count, skinned, item);
    }

    // The same through any pipeline handle (FindPipeline) -- its vertex shader must read the channel `skinned` selects
    std::shared_ptr<IIr77Return const> MakeIndirectDrawItem(std::size_t const& window, Ir77PVPipelineHandle const& pipeline, std::uint32_t const& scene_id,
                                                            std::uint32_t const& material_id, std::uint32_t const& first_command,
                                                            std::uint32_t const& command_count, bool const& skinned, Ir77PVDrawItem& item) {
        Ir77PVSceneSlot* const scene = FindScene(scene_id);
        if (!scene) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown scene.");

        std::shared_ptr<IIr77PVBuffer> const& vertex = skinned ? scene->vertex_skinned : scene->vertex;
        std::shared_ptr<IIr77PVBuffer> const& index = skinned ? scene->index_skinned : scene->index;

        if (!vertex || !index || !scene->indirect) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: scene channel empty.");

        if (static_cast<std::size_t>(first_command) + command_count > scene->commands.size())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: command range outside the scene.");

        Ir77PVMaterial material{};
        if (GetMaterial(material_id, material)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown material.");

        item = Ir77PVDrawItem{};

        auto const pipeline_result = DefineItemPipeline(pipeline, false, false, item);
        if (pipeline_result->ID() != GUIDIr77OperationSucceeded) return pipeline_result;

        item.material = material.row;
        item.vertex = vertex;
        item.index = index;
        item.index_type = VK_INDEX_TYPE_UINT32;
        item.indirect = scene->indirect;
        item.indirect_offset = static_cast<VkDeviceSize>(first_command) * sizeof(VkDrawIndexedIndirectCommand);
        item.indirect_count = command_count;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Hide / show one object (every instance of it). Applied to each frame copy by Tick.
    std::shared_ptr<IIr77Return const> SetSceneObjectVisible(std::uint32_t const& scene_id, std::uint32_t const& command, bool const& visible) {
        Ir77PVSceneSlot* const scene = FindScene(scene_id);
        if (!scene || command >= scene->commands.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown scene object.");

        scene->visible[command] = visible ? 1 : 0;
        scene->commands[command].instanceCount = visible ? scene->instance_counts[command] : 0;
        scene->dirty = MAX_FRAMES_IN_FLIGHT;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Change how many copies of an object draw -- spawn / despawn inside the models[] slots the client reserved for it
    // ([firstInstance, firstInstance + count) must not run into the next object's slots).
    std::shared_ptr<IIr77Return const> SetSceneObjectInstances(std::uint32_t const& scene_id, std::uint32_t const& command, std::uint32_t const& count) {
        Ir77PVSceneSlot* const scene = FindScene(scene_id);
        if (!scene || command >= scene->commands.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown scene object.");

        scene->instance_counts[command] = count;
        if (scene->visible[command]) scene->commands[command].instanceCount = count;
        scene->dirty = MAX_FRAMES_IN_FLIGHT;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Frees the scene's buffers once no frame in flight can read them. Remove its draw items from Paint first.
    std::shared_ptr<IIr77Return const> UnloadScene(std::uint32_t const& scene_id) {
        Ir77PVSceneSlot* const scene = FindScene(scene_id);
        if (!scene) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown scene.");

        Bury({scene->vertex, scene->index, scene->vertex_skinned, scene->index_skinned, scene->indirect});
        *scene = Ir77PVSceneSlot{};

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // meshlet path -- Mesh pipeline kind only (VK_EXT_mesh_shader). Takes the same stacked Ir77PVSceneUpload as UploadScene.
    // Each indirect command is one object's range; meshlets are cut from that range only, so none ever spans two objects.
    // One draw record + one mesh command per instance. Static (non-skinned) vertices only.
    // record_count = records written. Fails when the device has no mesh shading.
    // =========================================================================================================================================
    std::shared_ptr<IIr77Return const> UploadMeshlets(Ir77PVSceneUpload const& scene, std::uint32_t& meshlet_id, std::uint32_t& record_count,
                                                      bool const& wait = true) {
        if (scene.vertices.empty() || scene.indices.empty() || scene.indirect_commands.empty())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: empty scene.");

        bool mesh_shading{false};
        m_context->m_devices.at(m_context->m_current_device)->GetMeshShading(mesh_shading);

        if (!mesh_shading) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: mesh shading unsupported on this device.");

        // shared results -- every object appended here
        std::vector<Ir77PVMeshlet> meshlets{};
        std::vector<std::uint32_t> meshlet_vertices{};
        std::vector<std::uint8_t> meshlet_triangles{};
        std::vector<Ir77PVMeshDraw> draws{};
        std::vector<VkDrawMeshTasksIndirectCommandEXT> commands{};
        std::vector<std::uint32_t> groups_per_record{};
        std::vector<std::uint32_t> first_record{};
        first_record.reserve(scene.indirect_commands.size() + 1);

        // per-object scratch, reused
        std::vector<meshopt_Meshlet> built{};
        std::vector<std::uint32_t> local_vertices{};
        std::vector<std::uint8_t> local_triangles{};

        for (std::size_t c = 0; c < scene.indirect_commands.size(); c++) {
            first_record.push_back(static_cast<std::uint32_t>(draws.size()));  // every command gets an entry, even when it draws nothing

            VkDrawIndexedIndirectCommand const& cmd = scene.indirect_commands[c];
            if (cmd.indexCount == 0) continue;

            // 1. this object's slice
            if (cmd.vertexOffset < 0 || static_cast<std::size_t>(cmd.firstIndex) + cmd.indexCount > scene.indices.size() ||
                static_cast<std::size_t>(cmd.vertexOffset) >= scene.vertices.size())
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: indirect command outside the scene buffers.");

            std::uint32_t const* indices = &scene.indices[cmd.firstIndex];
            Ir77PVVertex const* vertices = &scene.vertices[static_cast<std::size_t>(cmd.vertexOffset)];

            // 2. vertices this object uses -- indices are local, so highest + 1
            std::size_t const local_count = static_cast<std::size_t>(*std::max_element(indices, indices + cmd.indexCount)) + 1;
            if (static_cast<std::size_t>(cmd.vertexOffset) + local_count > scene.vertices.size())
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: object indices run past the vertex buffer.");

            // 3. build on the slice only -- the builder can't see any other object
            std::size_t const max_meshlets = meshopt_buildMeshletsBound(cmd.indexCount, MESHLET_MAX_VERTICES, MESHLET_MAX_TRIANGLES);
            built.assign(max_meshlets, meshopt_Meshlet{});
            local_vertices.assign(max_meshlets * MESHLET_MAX_VERTICES, 0);
            local_triangles.assign(max_meshlets * MESHLET_MAX_TRIANGLES * 3, 0);

            float const* positions = &vertices[0].position.x;

            std::size_t const count =
                meshopt_buildMeshlets(built.data(), local_vertices.data(), local_triangles.data(), indices, cmd.indexCount, positions, local_count,
                                      sizeof(Ir77PVVertex), MESHLET_MAX_VERTICES, MESHLET_MAX_TRIANGLES, MESHLET_CONE_WEIGHT);
            if (count == 0) continue;
            built.resize(count);

            // where this object starts in the shared arrays
            std::uint32_t const meshlet_base = static_cast<std::uint32_t>(meshlets.size());
            std::uint32_t const vertex_base = static_cast<std::uint32_t>(meshlet_vertices.size());
            std::uint32_t const triangle_base = static_cast<std::uint32_t>(meshlet_triangles.size());  // stays a 4-byte multiple

            // 4. optimize + bounds (object-local space, same space models[] transforms from)
            for (std::size_t i = 0; i < count; i++) {
                meshopt_Meshlet const& m = built[i];

                meshopt_optimizeMeshlet(&local_vertices[m.vertex_offset], &local_triangles[m.triangle_offset], m.triangle_count, m.vertex_count);

                meshopt_Bounds const bounds = meshopt_computeMeshletBounds(&local_vertices[m.vertex_offset], &local_triangles[m.triangle_offset],
                                                                           m.triangle_count, positions, local_count, sizeof(Ir77PVVertex));

                Ir77PVMeshlet out{};
                out.sphere = {bounds.center[0], bounds.center[1], bounds.center[2], bounds.radius};
                out.cone = {bounds.cone_axis[0], bounds.cone_axis[1], bounds.cone_axis[2], bounds.cone_cutoff};
                out.vertex_offset = vertex_base + m.vertex_offset;  // shifted into the shared array
                out.triangle_offset = triangle_base + m.triangle_offset;
                out.vertex_count = m.vertex_count;
                out.triangle_count = m.triangle_count;
                meshlets.push_back(out);
            }

            // 5. append -- local vertex index + vertexOffset = index into the full vertex buffer
            meshopt_Meshlet const& last = built.back();
            std::size_t const used_vertices = last.vertex_offset + last.vertex_count;
            std::size_t const used_triangles = last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3u);

            for (std::size_t j = 0; j < used_vertices; j++) meshlet_vertices.push_back(local_vertices[j] + static_cast<std::uint32_t>(cmd.vertexOffset));

            meshlet_triangles.insert(meshlet_triangles.end(), local_triangles.begin(), local_triangles.begin() + used_triangles);

            // 6. this object = meshlets [meshlet_base, meshlet_base + count). One record per instance, same meaning as the vertex path.
            std::uint32_t const groups = (static_cast<std::uint32_t>(count) + 31) / 32;  // TASK_GROUP_SIZE

            for (std::uint32_t inst = 0; inst < cmd.instanceCount; inst++) {
                Ir77PVMeshDraw draw{};
                draw.instance = cmd.firstInstance + inst;
                draw.meshlet_offset = meshlet_base;
                draw.meshlet_count = static_cast<std::uint32_t>(count);

                draws.push_back(draw);
                commands.push_back(VkDrawMeshTasksIndirectCommandEXT{groups, 1, 1});
                groups_per_record.push_back(groups);
            }
        }

        first_record.push_back(static_cast<std::uint32_t>(draws.size()));  // end entry

        if (meshlets.empty() || draws.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: scene produced no meshlets or draws.");

        // 7. upload
        VkDeviceSize const meshlets_size = static_cast<VkDeviceSize>(meshlets.size() * sizeof(Ir77PVMeshlet));
        VkDeviceSize const indices_size = static_cast<VkDeviceSize>(meshlet_vertices.size() * sizeof(std::uint32_t));
        VkDeviceSize const triangles_size = static_cast<VkDeviceSize>(meshlet_triangles.size());
        VkDeviceSize const vertices_size = static_cast<VkDeviceSize>(scene.vertices.size() * sizeof(Ir77PVVertex));
        VkDeviceSize const draws_size = static_cast<VkDeviceSize>(draws.size() * sizeof(Ir77PVMeshDraw));
        VkDeviceSize const indirect_size = static_cast<VkDeviceSize>(commands.size() * sizeof(VkDrawMeshTasksIndirectCommandEXT));

        Ir77PVMeshletSlot slot{};
        Ir77PVMeshletMesh& mesh = slot.mesh;

        mesh.meshlets = CreateStaticBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, meshlets_size);
        mesh.meshlet_vertices = CreateStaticBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, indices_size);
        mesh.meshlet_triangles = CreateStaticBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, triangles_size);
        mesh.vertices = CreateStaticBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, vertices_size);
        mesh.draws = CreateStaticBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, draws_size);

        if (!mesh.meshlets || !mesh.meshlet_vertices || !mesh.meshlet_triangles || !mesh.vertices || !mesh.draws)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: meshlet buffer creation failed.");

        // indirect -- Dynamic, so records can be hidden per frame ({0,1,1}). Written directly, not staged.
        mesh.indirect = CreateDynamicBuffer(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, indirect_size);
        if (!mesh.indirect) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: meshlet indirect buffer creation failed.");

        for (std::uint32_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++) {
            if (mesh.indirect->Update(frame, commands.data(), indirect_size)->ID() != GUIDIr77OperationSucceeded)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: meshlet indirect write failed.");
        }

        VkBuffer meshlets_dst{VK_NULL_HANDLE};
        VkBuffer indices_dst{VK_NULL_HANDLE};
        VkBuffer triangles_dst{VK_NULL_HANDLE};
        VkBuffer vertices_dst{VK_NULL_HANDLE};
        VkBuffer draws_dst{VK_NULL_HANDLE};
        mesh.meshlets->GetBuffer(0, &meshlets_dst);
        mesh.meshlet_vertices->GetBuffer(0, &indices_dst);
        mesh.meshlet_triangles->GetBuffer(0, &triangles_dst);
        mesh.vertices->GetBuffer(0, &vertices_dst);
        mesh.draws->GetBuffer(0, &draws_dst);

        if (Upload(
                        {{meshlets_dst, meshlets.data(), meshlets_size},
                         {indices_dst, meshlet_vertices.data(), indices_size},
                         {triangles_dst, meshlet_triangles.data(), triangles_size},
                         {vertices_dst, scene.vertices.data(), vertices_size},
                         {draws_dst, draws.data(), draws_size}},
                        wait)
                ->ID() != GUIDIr77OperationSucceeded) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: meshlet upload failed.");
        }

        // 8. bindless slots -- order matches Ir77PVMeshletMesh::slots / Ir77PVDrawItem::meshlet_slots
        std::array<std::shared_ptr<IIr77PVBuffer>, MESHLET_SLOT_COUNT> const slot_buffers{mesh.meshlets, mesh.meshlet_vertices, mesh.meshlet_triangles,
                                                                                          mesh.vertices, mesh.draws};

        for (std::size_t i = 0; i < slot_buffers.size(); i++) {
            auto const result = RegisterBuffer(slot_buffers[i], mesh.slots[i]);
            if (result->ID() != GUIDIr77OperationSucceeded) {
                ReleaseSlots(BINDLESS_BINDING_BUFFERS, std::vector<std::uint32_t>(mesh.slots.begin(), mesh.slots.end()));
                return result;
            }
        }

        mesh.meshlet_count = static_cast<std::uint32_t>(meshlets.size());
        record_count = static_cast<std::uint32_t>(draws.size());

        slot.commands = std::move(commands);
        slot.groups = std::move(groups_per_record);
        slot.first_record = std::move(first_record);
        slot.alive = true;

        std::uint64_t const device_id = m_context->m_current_device;

        // mirrored into the context slots so its teardown order covers them
        auto& vertex_slot = m_context->m_buffers_vertex[device_id];
        vertex_slot.push_back(mesh.meshlets);
        vertex_slot.push_back(mesh.meshlet_vertices);
        vertex_slot.push_back(mesh.meshlet_triangles);
        vertex_slot.push_back(mesh.vertices);
        vertex_slot.push_back(mesh.draws);
        vertex_slot.push_back(mesh.indirect);

        auto& slots = m_meshlet_scenes[device_id];
        meshlet_id = static_cast<std::uint32_t>(slots.size());
        slots.push_back(std::move(slot));

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetMeshlets(std::uint32_t const& meshlet_id, Ir77PVMeshletMesh& mesh) {
        Ir77PVMeshletSlot* const slot = FindMeshlets(meshlet_id);
        if (!slot) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown meshlet mesh.");

        mesh = slot->mesh;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Direct (non-indirect) draw of meshlets [0, meshlet_count) -- single-object meshlet meshes only -- through the mesh role
    std::shared_ptr<IIr77Return const> MakeMeshletDrawItem(std::size_t const& window, std::uint32_t const& meshlet_id, std::uint32_t const& material_id,
                                                           Ir77PVDrawItem& item) {
        Ir77PVPipelineHandle pipeline{IR77PV_NO_PIPELINE};

        auto const result = FindPipeline(Ir77PVPipelineKind::Mesh, pipeline);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        return MakeMeshletDrawItem(window, pipeline, meshlet_id, material_id, item);
    }

    // The same through any task / mesh pipeline handle
    std::shared_ptr<IIr77Return const> MakeMeshletDrawItem(std::size_t const& window, Ir77PVPipelineHandle const& pipeline, std::uint32_t const& meshlet_id,
                                                           std::uint32_t const& material_id, Ir77PVDrawItem& item) {
        Ir77PVMeshletMesh mesh{};
        Ir77PVMaterial material{};

        if (GetMeshlets(meshlet_id, mesh)->ID() != GUIDIr77OperationSucceeded) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown meshlet mesh.");
        if (GetMaterial(material_id, material)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown material.");

        item = Ir77PVDrawItem{};

        auto const pipeline_result = DefineItemPipeline(pipeline, true, false, item);
        if (pipeline_result->ID() != GUIDIr77OperationSucceeded) return pipeline_result;

        item.material = material.row;
        item.meshlet_slots = mesh.slots;
        item.meshlet_offset = 0;
        item.meshlet_count = mesh.meshlet_count;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // One indirect draw for records [first_record, first_record + record_count).
    // indirect_offset and draw_base both start at first_record, so gl_DrawID 0..record_count-1 lines up with the records.
    std::shared_ptr<IIr77Return const> MakeMeshletIndirectDrawItem(std::size_t const& window, std::uint32_t const& meshlet_id, std::uint32_t const& material_id,
                                                                   std::uint32_t const& first_record, std::uint32_t const& record_count, Ir77PVDrawItem& item) {
        Ir77PVPipelineHandle pipeline{IR77PV_NO_PIPELINE};

        auto const result = FindPipeline(Ir77PVPipelineKind::Mesh, pipeline);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        return MakeMeshletIndirectDrawItem(window, pipeline, meshlet_id, material_id, first_record, record_count, item);
    }

    std::shared_ptr<IIr77Return const> MakeMeshletIndirectDrawItem(std::size_t const& window, Ir77PVPipelineHandle const& pipeline, std::uint32_t const& meshlet_id,
                                                                   std::uint32_t const& material_id, std::uint32_t const& first_record,
                                                                   std::uint32_t const& record_count, Ir77PVDrawItem& item) {
        Ir77PVMeshletSlot* const slot = FindMeshlets(meshlet_id);
        if (!slot) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown meshlet mesh.");

        if (static_cast<std::size_t>(first_record) + record_count > slot->commands.size())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: record range outside the meshlet mesh.");

        auto const result = MakeMeshletDrawItem(window, pipeline, meshlet_id, material_id, item);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        item.indirect = slot->mesh.indirect;
        item.indirect_offset = static_cast<VkDeviceSize>(first_record) * sizeof(VkDrawMeshTasksIndirectCommandEXT);
        item.indirect_count = record_count;
        item.draw_base = first_record;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Same as MakeIndirectDrawItem but on the mesh path: commands [first_command, first_command + command_count) of the uploaded scene,
    // mapped to their records. This is how a material run becomes one mesh draw.
    std::shared_ptr<IIr77Return const> MakeMeshletSceneDrawItem(std::size_t const& window, std::uint32_t const& meshlet_id, std::uint32_t const& material_id,
                                                                std::uint32_t const& first_command, std::uint32_t const& command_count, Ir77PVDrawItem& item) {
        Ir77PVPipelineHandle pipeline{IR77PV_NO_PIPELINE};

        auto const result = FindPipeline(Ir77PVPipelineKind::Mesh, pipeline);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        return MakeMeshletSceneDrawItem(window, pipeline, meshlet_id, material_id, first_command, command_count, item);
    }

    std::shared_ptr<IIr77Return const> MakeMeshletSceneDrawItem(std::size_t const& window, Ir77PVPipelineHandle const& pipeline, std::uint32_t const& meshlet_id,
                                                                std::uint32_t const& material_id, std::uint32_t const& first_command,
                                                                std::uint32_t const& command_count, Ir77PVDrawItem& item) {
        Ir77PVMeshletSlot* const slot = FindMeshlets(meshlet_id);
        if (!slot) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown meshlet mesh.");

        if (static_cast<std::size_t>(first_command) + command_count >= slot->first_record.size())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: command range outside the meshlet scene.");

        std::uint32_t const first = slot->first_record[first_command];
        std::uint32_t const end = slot->first_record[first_command + command_count];

        return MakeMeshletIndirectDrawItem(window, pipeline, meshlet_id, material_id, first, end - first, item);
    }

    // Hide / show every instance of one scene object on the mesh path. Applied to each frame copy by Tick.
    std::shared_ptr<IIr77Return const> SetMeshletObjectVisible(std::uint32_t const& meshlet_id, std::uint32_t const& command, bool const& visible) {
        Ir77PVMeshletSlot* const slot = FindMeshlets(meshlet_id);
        if (!slot || static_cast<std::size_t>(command) + 1 >= slot->first_record.size())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown meshlet object.");

        for (std::uint32_t r = slot->first_record[command]; r < slot->first_record[command + 1]; r++)
            slot->commands[r].groupCountX = visible ? slot->groups[r] : 0;

        slot->dirty = MAX_FRAMES_IN_FLIGHT;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Hide / show one instance (record). The record count is fixed at build time -- spawn capacity is reserved by uploading
    // extra instances and starting them hidden.
    std::shared_ptr<IIr77Return const> SetMeshletRecordVisible(std::uint32_t const& meshlet_id, std::uint32_t const& record, bool const& visible) {
        Ir77PVMeshletSlot* const slot = FindMeshlets(meshlet_id);
        if (!slot || record >= slot->commands.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown meshlet record.");

        slot->commands[record].groupCountX = visible ? slot->groups[record] : 0;
        slot->dirty = MAX_FRAMES_IN_FLIGHT;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // First record of a scene command -- record = first_record + instance offset, for SetMeshletRecordVisible.
    std::shared_ptr<IIr77Return const> GetMeshletFirstRecord(std::uint32_t const& meshlet_id, std::uint32_t const& command, std::uint32_t& first_record) {
        Ir77PVMeshletSlot* const slot = FindMeshlets(meshlet_id);
        if (!slot || static_cast<std::size_t>(command) + 1 >= slot->first_record.size())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown meshlet object.");

        first_record = slot->first_record[command];

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Releases the meshlet buffer slots and frees the buffers once no frame in flight can read them. Remove its draw items from Paint first.
    std::shared_ptr<IIr77Return const> UnloadMeshlets(std::uint32_t const& meshlet_id) {
        Ir77PVMeshletSlot* const slot = FindMeshlets(meshlet_id);
        if (!slot) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown meshlet mesh.");

        Ir77PVMeshletMesh const& mesh = slot->mesh;
        ReleaseSlots(BINDLESS_BINDING_BUFFERS, std::vector<std::uint32_t>(mesh.slots.begin(), mesh.slots.end()));
        Bury({mesh.meshlets, mesh.meshlet_vertices, mesh.meshlet_triangles, mesh.vertices, mesh.draws, mesh.indirect});
        *slot = Ir77PVMeshletSlot{};

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // textures -- built on Ir77PVTexture (the same class as the placeholders), so they get its kinds, mip chain and sampler handling.
    // srgb = true for color data (albedo, emissive) -> Color kind (sRGB); false for data maps (normal, ORM, AO) -> Data kind (UNORM).
    // Blocking: Ir77PVTexture uploads and waits on its own. Registered in the bindless table -- GetTextureSlot gives its slot.
    // =========================================================================================================================================
    std::shared_ptr<IIr77Return const> CreateTexture(std::uint8_t const* rgba, std::uint32_t const& width, std::uint32_t const& height, bool const& srgb,
                                                     std::uint32_t& texture_id) {
        if (rgba == nullptr || width == 0 || height == 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: empty texture.");

        std::uint64_t const device_id = m_context->m_current_device;

        VkCommandPool pool{VK_NULL_HANDLE};
        VkQueue queue{VK_NULL_HANDLE};

        auto const pool_result = m_context->m_devices.at(device_id)->GetUploadPool(pool, queue);
        if (pool_result->ID() != GUIDIr77OperationSucceeded) return pool_result;

        auto texture = std::static_pointer_cast<IIr77PVTexture>(std::make_shared<Ir77PVTexture>());

        texture->SetDevice(m_context->m_devices.at(device_id));
        texture->SetAllocator(m_context->m_allocators.at(device_id));
        texture->SetKind(srgb ? Ir77PVTextureKind::Color : Ir77PVTextureKind::Data);  // first -- resets format and sampler to the kind's defaults
        texture->SetSampler(Ir77PVSamplerDesc{});                                     // linear / repeat; anisotropy needs samplerAnisotropy on the device

        if (texture->CreateFromPixels(pool, queue, rgba, width, height, true)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: texture upload failed.");

        VkDescriptorImageInfo info{};
        texture->GetImageInfo(&info);

        std::uint32_t slot{BINDLESS_NO_SLOT};

        auto const result = m_context->m_descriptor_sets.at(device_id)->RegisterTexture(info.imageView, info.sampler, slot);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        auto& textures = m_textures[device_id];
        texture_id = static_cast<std::uint32_t>(textures.size());
        textures.push_back(texture);
        m_texture_slots[device_id].push_back(slot);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Bindless texture slot of a texture -- what material rows and shaders use
    std::shared_ptr<IIr77Return const> GetTextureSlot(std::uint32_t const& texture_id, std::uint32_t& slot) {
        auto const found = m_texture_slots.find(m_context->m_current_device);
        if (found == m_texture_slots.end() || texture_id >= found->second.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown texture.");

        slot = found->second[texture_id];

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // materials -- rows in the device's material table: constants + 5 texture slots (placeholders where no texture is given)
    // =========================================================================================================================================
    std::shared_ptr<IIr77Return const> CreateMaterial(Ir77PVMaterialConstants const& constants, std::uint32_t& material_id) {
        return CreateMaterial(constants, Ir77PVMaterialTextures{}, material_id);
    }

    std::shared_ptr<IIr77Return const> CreateMaterial(Ir77PVMaterialConstants const& constants, Ir77PVMaterialTextures const& textures,
                                                      std::uint32_t& material_id) {
        std::vector<std::uint32_t> ids{};
        auto const result = UploadMaterials({constants}, {textures}, ids);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        material_id = ids.front();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> UploadMaterials(std::vector<Ir77PVMaterialConstants> const& constants, std::vector<std::uint32_t>& material_ids) {
        return UploadMaterials(constants, std::vector<Ir77PVMaterialTextures>(constants.size()), material_ids);
    }

    // One staging copy for every new row. Rows are appended -- existing rows (which frames in flight may be reading) are never rewritten.
    std::shared_ptr<IIr77Return const> UploadMaterials(std::vector<Ir77PVMaterialConstants> const& constants,
                                                       std::vector<Ir77PVMaterialTextures> const& textures, std::vector<std::uint32_t>& material_ids) {
        if (constants.size() != textures.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: constants / textures count mismatch.");

        std::uint64_t const device_id = m_context->m_current_device;

        auto& materials = m_materials[device_id];

        if (materials.size() + constants.size() > MATERIAL_CAPACITY)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: material table full -- raise MATERIAL_CAPACITY.");

        std::vector<Ir77PVMaterialRow> rows(constants.size());

        for (std::size_t i = 0; i < constants.size(); i++) {
            auto const result = DefineMaterialRow(constants[i], textures[i], rows[i]);
            if (result->ID() != GUIDIr77OperationSucceeded) return result;
        }

        VkBuffer table{VK_NULL_HANDLE};
        m_context->m_material_tables.at(device_id)->GetBuffer(0, &table);

        Ir77PVUploadEntry entry{};
        entry.dst = table;
        entry.data = rows.data();
        entry.size = static_cast<VkDeviceSize>(rows.size() * sizeof(Ir77PVMaterialRow));
        entry.dst_offset = static_cast<VkDeviceSize>(materials.size() * sizeof(Ir77PVMaterialRow));

        if (Upload({entry})->ID() != GUIDIr77OperationSucceeded) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: material upload failed.");

        material_ids.clear();
        material_ids.reserve(rows.size());

        for (std::size_t i = 0; i < rows.size(); i++) {
            Ir77PVMaterial material{};
            material.row = static_cast<std::uint32_t>(materials.size());

            material_ids.push_back(material.row);
            materials.push_back(material);
        }

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Upload Materials.");
    }

    std::shared_ptr<IIr77Return const> GetMaterial(std::uint32_t const& material_id, Ir77PVMaterial& material) {
        auto const found = m_materials.find(m_context->m_current_device);
        if (found == m_materials.end() || material_id >= found->second.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown material.");

        material = found->second[material_id];

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Draw item with no vertex buffer: vertex_count vertices (times instance_count) that the vertex shader builds from gl_VertexIndex /
    // gl_InstanceIndex and storage buffers. The pipeline's vertex shader must read no inputs. Caller fills model / user; for a GPU-driven
    // count set item.indirect (a storage buffer holding VkDrawIndirectCommands) and item.indirect_count.
    std::shared_ptr<IIr77Return const> MakeProceduralDrawItem(std::size_t const& window, Ir77PVPipelineHandle const& pipeline, std::uint32_t const& material_id,
                                                              std::uint32_t const& vertex_count, std::uint32_t const& instance_count, Ir77PVDrawItem& item) {
        Ir77PVMaterial material{};
        if (GetMaterial(material_id, material)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown material.");

        item = Ir77PVDrawItem{};

        auto const pipeline_result = DefineItemPipeline(pipeline, false, true, item);
        if (pipeline_result->ID() != GUIDIr77OperationSucceeded) return pipeline_result;

        item.material = material.row;
        item.index_count = vertex_count;
        item.instance_count = instance_count;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Draw item for mesh + material through an engine role on a window. Caller fills model / instances. Needs CreatePipelines first.
    std::shared_ptr<IIr77Return const> MakeDrawItem(std::size_t const& window, Ir77PVPipelineKind const& kind, std::uint32_t const& mesh_id,
                                                    std::uint32_t const& material_id, Ir77PVDrawItem& item) {
        Ir77PVPipelineHandle pipeline{IR77PV_NO_PIPELINE};

        auto const result = FindPipeline(kind, pipeline);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        return MakeDrawItem(window, pipeline, mesh_id, material_id, item);
    }

    // The same through any vertex-input pipeline handle (FindPipeline) -- a task / mesh pipeline takes a meshlet mesh (MakeMeshletDrawItem)
    std::shared_ptr<IIr77Return const> MakeDrawItem(std::size_t const& window, Ir77PVPipelineHandle const& pipeline, std::uint32_t const& mesh_id,
                                                    std::uint32_t const& material_id, Ir77PVDrawItem& item) {
        Ir77PVMesh mesh{};
        Ir77PVMaterial material{};

        if (GetMesh(mesh_id, mesh)->ID() != GUIDIr77OperationSucceeded) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown mesh.");
        if (GetMaterial(material_id, material)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown material.");

        item = Ir77PVDrawItem{};

        auto const pipeline_result = DefineItemPipeline(pipeline, false, false, item);
        if (pipeline_result->ID() != GUIDIr77OperationSucceeded) return pipeline_result;

        item.material = material.row;
        item.vertex = mesh.vertex;
        item.index = mesh.index;
        item.index_type = mesh.index_type;
        item.index_count = mesh.index_count;
        item.instance_count = 1;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    Ir77PVSceneSlot* FindScene(std::uint32_t const& scene_id) {
        auto const found = m_scenes.find(m_context->m_current_device);
        if (found == m_scenes.end() || scene_id >= found->second.size() || !found->second[scene_id].alive) return nullptr;

        return &found->second[scene_id];
    }

    Ir77PVMeshletSlot* FindMeshlets(std::uint32_t const& meshlet_id) {
        auto const found = m_meshlet_scenes.find(m_context->m_current_device);
        if (found == m_meshlet_scenes.end() || meshlet_id >= found->second.size() || !found->second[meshlet_id].alive) return nullptr;

        return &found->second[meshlet_id];
    }

    // Held until MAX_FRAMES_IN_FLIGHT + 1 Ticks have passed -- every frame that could have recorded them has finished by then
    void Bury(std::vector<std::shared_ptr<IIr77PVBuffer>> const& buffers) {
        Ir77PVGrave grave{};
        for (auto const& buffer : buffers)
            if (buffer) grave.buffers.push_back(buffer);
        grave.frames_left = MAX_FRAMES_IN_FLIGHT + 1;

        m_graveyard[m_context->m_current_device].push_back(std::move(grave));
    }

    std::shared_ptr<IIr77PVBuffer> CreateStaticBuffer(VkBufferUsageFlags const& usage, VkDeviceSize const& size) {
        auto buffer = std::static_pointer_cast<IIr77PVBuffer>(std::make_shared<Ir77PVBuffer>());

        buffer->SetDevice(m_context->m_devices.at(m_context->m_current_device));

        buffer->SetAllocator(m_context->m_allocators.at(m_context->m_current_device));

        buffer->SetUsage(usage);

        buffer->SetMode(Ir77PVBufferMode::Static);

        if (buffer->CreateResources(size, 1)->ID() != GUIDIr77OperationSucceeded) return nullptr;

        return buffer;
    }

    // Host-written every frame -- one copy per frame in flight, persistently mapped. Write with Update(frame, ...).
    std::shared_ptr<IIr77PVBuffer> CreateDynamicBuffer(VkBufferUsageFlags const& usage, VkDeviceSize const& size) {
        auto buffer = std::static_pointer_cast<IIr77PVBuffer>(std::make_shared<Ir77PVBuffer>());

        buffer->SetDevice(m_context->m_devices.at(m_context->m_current_device));

        buffer->SetAllocator(m_context->m_allocators.at(m_context->m_current_device));

        buffer->SetUsage(usage);

        buffer->SetMode(Ir77PVBufferMode::Dynamic);

        if (buffer->CreateResources(size, MAX_FRAMES_IN_FLIGHT)->ID() != GUIDIr77OperationSucceeded) return nullptr;

        return buffer;
    }

    // item.pipeline = handle, once it resolves now to the right path: mesh = a task / mesh pipeline, otherwise a vertex shader; procedural =
    // one that reads no vertex inputs. The command buffer re-resolves the handle every frame, so a later manifest reload still re-points the item.
    std::shared_ptr<IIr77Return const> DefineItemPipeline(Ir77PVPipelineHandle const& handle, bool const mesh, bool const procedural, Ir77PVDrawItem& item) {
        auto const cache = m_context->m_pipeline_caches.find(m_context->m_current_device);
        if (cache == m_context->m_pipeline_caches.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: CreatePipelines not called.");

        std::shared_ptr<IIr77PVPipeline> pipeline{};

        auto const result = cache->second->GetPipeline(handle, pipeline);
        if (result->ID() != GUIDIr77OperationSucceeded) {
            std::string name{};
            cache->second->GetPipelineName(handle, name);
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: pipeline '" + name + "' not available (manifest, shaders or pass missing).");
        }

        Ir77PVPipelineDesc desc{};
        pipeline->GetDesc(&desc);

        if (desc.IsMesh() != mesh)
            return Ir77RETURN<Ir77NotConfigured>(this, mesh ? "Ir77PVAsset: a meshlet draw needs a task / mesh pipeline."
                                                            : "Ir77PVAsset: a task / mesh pipeline takes a meshlet mesh -- use MakeMeshlet*DrawItem.");

        bool vertex_input{false};
        pipeline->GetVertexInput(vertex_input);

        if (procedural && vertex_input)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: a procedural draw needs a vertex shader that reads no inputs (layout locations).");

        item.pipeline = handle;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // One material-table row: the constants plus a texture slot per map -- the texture's when given, a placeholder's otherwise
    std::shared_ptr<IIr77Return const> DefineMaterialRow(Ir77PVMaterialConstants const& constants, Ir77PVMaterialTextures const& textures,
                                                         Ir77PVMaterialRow& row) {
        std::uint32_t white{BINDLESS_NO_SLOT};
        std::uint32_t flat_normal{BINDLESS_NO_SLOT};

        if (m_context->GetPlaceholderSlot(Ir77PVPlaceholderKind::White, white)->ID() != GUIDIr77OperationSucceeded ||
            m_context->GetPlaceholderSlot(Ir77PVPlaceholderKind::Normal, flat_normal)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: placeholders not registered -- call CreateDescriptorSets first.");

        // albedo, normal, ORM, emissive, AO
        std::array<std::uint32_t, 5> const ids{textures.albedo, textures.normal, textures.orm, textures.emissive, textures.ao};
        std::array<std::uint32_t, 5> slots{white, flat_normal, white, white, white};

        for (std::size_t i = 0; i < ids.size(); i++) {
            if (ids[i] == IR77PV_NO_TEXTURE) continue;
            if (GetTextureSlot(ids[i], slots[i])->ID() != GUIDIr77OperationSucceeded)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: material references an unknown texture.");
        }

        row = Ir77PVMaterialRow{};
        row.constants = constants;
        row.textures0 = {slots[0], slots[1], slots[2], slots[3]};
        row.textures1 = {slots[4], 0, 0, 0};

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // One Static buffer copy -> a bindless buffer slot
    std::shared_ptr<IIr77Return const> RegisterBuffer(std::shared_ptr<IIr77PVBuffer> const& buffer, std::uint32_t& slot) {
        VkDescriptorBufferInfo info{};

        auto const result = buffer->GetBufferInfo(0, &info);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        return m_context->m_descriptor_sets.at(m_context->m_current_device)->RegisterBuffer(info.buffer, info.offset, info.range, slot);
    }

    // Unset slots (BINDLESS_NO_SLOT) are skipped
    void ReleaseSlots(std::uint32_t const& binding, std::vector<std::uint32_t> const& slots) {
        auto const& table = m_context->m_descriptor_sets.at(m_context->m_current_device);

        for (auto const slot : slots)
            if (slot != BINDLESS_NO_SLOT) table->Release(binding, slot);
    }

    // one Staging buffer per batch, on the device's upload pool. No-wait uploads stay in m_pending until Tick sees them finished.
    std::shared_ptr<IIr77Return const> Upload(std::vector<Ir77PVUploadEntry> const& entries, bool const& wait = true) {
        std::uint64_t const device_id = m_context->m_current_device;
        auto const& device = m_context->m_devices.at(device_id);

        VkCommandPool pool{VK_NULL_HANDLE};
        VkQueue queue{VK_NULL_HANDLE};

        auto result = device->GetUploadPool(pool, queue);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        auto staging = std::static_pointer_cast<IIr77PVBuffer>(std::make_shared<Ir77PVBuffer>());

        staging->SetDevice(device);
        staging->SetAllocator(m_context->m_allocators.at(device_id));
        staging->SetMode(Ir77PVBufferMode::Staging);

        result = staging->UploadBatch(pool, queue, entries, wait);

        if (result->ID() == GUIDIr77OperationSucceeded && !wait) m_pending[device_id].push_back(staging);

        return result;
    }

   private:
    std::shared_ptr<Ir77PVInit> m_context{nullptr};

    std::string m_shader_root{"/home/alpha/workspace/iridium/Shader"};

    std::map<std::uint64_t, std::vector<Ir77PVMesh>> m_meshes{};  // single-mesh path

    std::map<std::uint64_t, std::vector<Ir77PVSceneSlot>> m_scenes{};  // scene path (vertex)

    std::map<std::uint64_t, std::vector<Ir77PVMeshletSlot>> m_meshlet_scenes{};  // meshlet path

    std::map<std::uint64_t, std::vector<Ir77PVMaterial>> m_materials{};

    std::map<std::uint64_t, std::vector<std::shared_ptr<IIr77PVTexture>>> m_textures{};

    std::map<std::uint64_t, std::vector<std::uint32_t>> m_texture_slots{};  // [device][texture_id] -> bindless texture slot

    std::map<std::uint64_t, std::vector<std::shared_ptr<IIr77PVBuffer>>> m_storage_buffers{};  // [device][buffer_id] -- empty once destroyed

    std::map<std::uint64_t, std::vector<std::uint32_t>> m_storage_slots{};  // [device][buffer_id] -> bindless buffer slot

    std::map<std::uint64_t, std::vector<std::shared_ptr<IIr77PVBuffer>>> m_pending{};  // Staging buffers of uploads submitted with wait = false

    std::map<std::uint64_t, std::vector<Ir77PVGrave>> m_graveyard{};  // unloaded, waiting out the frames in flight
};
}  // namespace NSIr77PeregrineV