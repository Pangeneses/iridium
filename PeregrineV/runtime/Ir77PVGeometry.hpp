#pragma once

#include <glm/glm.hpp>

#include <meshoptimizer.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "../types/Ir77PVTypes.hpp"

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/types/Ir77Types.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"
#include "../../Ir77RT/runtime/Ir77Return.hpp"

#include "../interface/IIr77PVDevice.hpp"
#include "../interface/IIr77PVBuffer.hpp"
#include "../interface/IIr77PVPipeline.hpp"
#include "../interface/IIr77PVPipelineCache.hpp"
#include "../interface/IIr77PVTransfer.hpp"
#include "../interface/IIr77PVMaterial.hpp"
#include "../interface/IIr77PVGeometry.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// =============================================================================================================================================
// Ir77PVGeometry -- meshes on the device and the draw items that reference them.
//
//   startup:   Ir77PVInit wires it -- SetDevice / SetTransfer / SetMaterials -> [Init builds the pipeline cache] -> SetPipelineCache
//   load:      UploadSingleMesh (one-off / procedural) / UploadScene (vertex path) / UploadMeshlets (mesh path)
//   frame:     Tick(frame) -> Make*DrawItem -> Paint::SetDraws
//   runtime:   SetSceneObjectVisible / SetSceneObjectInstances / SetMeshletObjectVisible / SetMeshletRecordVisible
//   compute:   GetSceneVertexSlot (vertices as a storage buffer); rays: GetSceneGeometry (what Acceleration builds BLASes from)
//   unload:    remove the draw items from Paint, then UnloadScene / UnloadMeshlets / UnloadSingleMeshBuffer (freed after the frames in flight)
// One device; it never sees Ir77PVInit. Every buffer is made, uploaded, slotted and released through Transfer; materials come from Materials.
//
// Every upload takes `wait`. true = blocks until the copy is done (load screens). false = returns at once; the copy runs in submission
// order ahead of later frames, and Transfer's Tick frees its staging when the GPU is finished with it.
// =============================================================================================================================================
class Ir77PVGeometry : public Ir77Enlisted, public IIr77PVGeometry, public std::enable_shared_from_this<Ir77PVGeometry> {
   public:
    Ir77PVGeometry() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    // Ir77PVInit releases this first of the three -- its buffers are shared with Transfer, which frees them after the device wait
    ~Ir77PVGeometry() {
        if (!m_device) return;

        VkDevice vk_device{VK_NULL_HANDLE};
        m_device->GetDevice(&vk_device);
        if (vk_device != VK_NULL_HANDLE) vkDeviceWaitIdle(vk_device);

        m_single_mesh_buffers.clear();
        m_scenes.clear();
        m_meshlet_scenes.clear();
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVGeometry>(uid);

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

        else if (iid == GUIDIIr77PVGeometry)
            obj = std::shared_ptr<IIr77PVGeometry>(shared_from_this(), static_cast<IIr77PVGeometry*>(this));

        else if (iid == GUIDIr77PVGeometry)
            obj = std::shared_ptr<Ir77PVGeometry>(shared_from_this(), static_cast<Ir77PVGeometry*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

    // =========================================================================================================================================
    // wiring -- Ir77PVInit sets these as each piece exists
    // =========================================================================================================================================
   public:
    std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) {
        m_device = device;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Uploads, buffers, bindless buffer slots and the graveyard -- every buffer here is made and released through it
    std::shared_ptr<IIr77Return const> SetTransfer(std::shared_ptr<IIr77PVTransfer> transfer) {
        m_transfer = transfer;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Material rows for the draw items
    std::shared_ptr<IIr77Return const> SetMaterials(std::shared_ptr<IIr77PVMaterial> materials) {
        m_materials = materials;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The device's pipeline cache -- engine roles and the draw item checks. After CreatePipelines.
    std::shared_ptr<IIr77Return const> SetPipelineCache(std::shared_ptr<IIr77PVPipelineCache> pipeline_cache) {
        m_pipeline_cache = pipeline_cache;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // per frame -- Ir77PVInit::Tick calls this once per frame, after Transfer's Tick and before Paint records, with the frame-in-flight
    // index Paint is about to record. Writes this frame's copy of every dirty indirect buffer (visibility / instance changes) -- one frame
    // copy per Tick until every copy matches.
    // =========================================================================================================================================
    std::shared_ptr<IIr77Return const> Tick(std::uint32_t const& frame) {
        if (frame >= MAX_FRAMES_IN_FLIGHT) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: frame index out of range.");

        for (auto& scene : m_scenes) {
            if (!scene.alive || scene.dirty == 0 || !scene.indirect) continue;

            VkDeviceSize const size = static_cast<VkDeviceSize>(scene.commands.size() * sizeof(VkDrawIndexedIndirectCommand));
            if (scene.indirect->Update(frame, scene.commands.data(), size)->ID() != GUIDIr77OperationSucceeded)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: scene indirect write failed.");

            scene.dirty--;
        }

        for (auto& slot : m_meshlet_scenes) {
            if (!slot.alive || slot.dirty == 0 || !slot.mesh.indirect) continue;

            VkDeviceSize const size = static_cast<VkDeviceSize>(slot.commands.size() * sizeof(VkDrawMeshTasksIndirectCommandEXT));
            if (slot.mesh.indirect->Update(frame, slot.commands.data(), size)->ID() != GUIDIr77OperationSucceeded)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: meshlet indirect write failed.");

            slot.dirty--;
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // single-mesh path -- one Static vertex + index buffer pair per mesh. procedural / debug geometry, one-off loads.
    // =========================================================================================================================================
    // One code-built mesh (gizmo, debug shape, procedural geometry) -> its own GPU vertex + index buffers. single_mesh_id names it for
    // GetSingleMesh / MakeDrawItem / UnloadSingleMesh. wait = true blocks until the copy is done; false lets it run ahead of later frames.
    std::shared_ptr<IIr77Return const> UploadSingleMesh(Ir77PVSingleMesh const& single_mesh, std::uint32_t& single_mesh_id, bool const& wait = true) {
        if (single_mesh.vertex_data.empty() || single_mesh.index_data.empty())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: single mesh has no vertices or no indices.");

        // sizes in bytes of what goes up
        VkDeviceSize const vertex_bytes = static_cast<VkDeviceSize>(single_mesh.vertex_data.size() * sizeof(Ir77PVVertex));
        VkDeviceSize const index_bytes = static_cast<VkDeviceSize>(single_mesh.index_data.size() * sizeof(std::uint32_t));

        // the two device-local buffers this mesh will live in
        Ir77PVSingleMeshBuffer single_mesh_buffer{};
        single_mesh_buffer.vertex = CreateStaticBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertex_bytes);
        single_mesh_buffer.index = CreateStaticBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, index_bytes);

        if (!single_mesh_buffer.vertex || !single_mesh_buffer.index)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: single mesh buffer creation failed.");

        // copy both through staging in one submission
        VkBuffer vertex_target{VK_NULL_HANDLE};
        VkBuffer index_target{VK_NULL_HANDLE};
        single_mesh_buffer.vertex->GetBuffer(0, &vertex_target);
        single_mesh_buffer.index->GetBuffer(0, &index_target);

        auto const uploaded =
            Upload({{vertex_target, single_mesh.vertex_data.data(), vertex_bytes}, {index_target, single_mesh.index_data.data(), index_bytes}}, wait);
        if (uploaded->ID() != GUIDIr77OperationSucceeded) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: single mesh upload failed.");

        // what a draw needs: how many indices, and their width (always 32-bit -- Ir77PVSingleMesh::index_data is uint32_t)
        single_mesh_buffer.index_count = static_cast<std::uint32_t>(single_mesh.index_data.size());
        single_mesh_buffer.index_type = VK_INDEX_TYPE_UINT32;

        // its position in the list is its id
        single_mesh_id = static_cast<std::uint32_t>(m_single_mesh_buffers.size());
        m_single_mesh_buffers.push_back(single_mesh_buffer);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetSingleMeshBuffer(std::uint32_t const& mesh_id, Ir77PVSingleMeshBuffer& mesh) {
        if (mesh_id >= m_single_mesh_buffers.size() || !m_single_mesh_buffers[mesh_id].vertex)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown mesh.");

        mesh = m_single_mesh_buffers[mesh_id];

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Draw item for mesh + material through an engine role on a window. Caller fills model / instances. Needs CreatePipelines first.
    std::shared_ptr<IIr77Return const> MakeSingleMeshDrawItem(std::size_t const& window, Ir77PVPipelineKind const& kind, std::uint32_t const& single_mesh_id,
                                                              std::uint32_t const& material_id, Ir77PVDrawItem& item) {
        Ir77PVPipelineHandle pipeline{IR77PV_NO_PIPELINE};

        auto const result = FindPipeline(kind, pipeline);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        return MakeSingleMeshDrawItem(window, pipeline, single_mesh_id, material_id, item);
    }

    // A single mesh (UploadSingleMesh) through any vertex-input pipeline handle (FindPipeline). One draw, one instance; the caller fills
    // item.model. Task / mesh pipelines take meshlets instead (MakeMeshletDrawItem).
    std::shared_ptr<IIr77Return const> MakeSingleMeshDrawItem(std::size_t const& window, Ir77PVPipelineHandle const& pipeline,
                                                              std::uint32_t const& single_mesh_id, std::uint32_t const& material_id, Ir77PVDrawItem& item) {
        Ir77PVSingleMeshBuffer single_mesh_buffer{};
        Ir77PVMaterialData material{};

        if (GetSingleMeshBuffer(single_mesh_id, single_mesh_buffer)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown single mesh.");
        if (GetMaterial(material_id, material)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown material.");

        item = Ir77PVDrawItem{};

        auto const pipeline_result = DefineItemPipeline(pipeline, false, false, item);
        if (pipeline_result->ID() != GUIDIr77OperationSucceeded) return pipeline_result;

        item.material = material.row;
        item.vertex = single_mesh_buffer.vertex;
        item.index = single_mesh_buffer.index;
        item.index_type = single_mesh_buffer.index_type;
        item.index_count = single_mesh_buffer.index_count;
        item.instance_count = 1;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The id stays reserved; GetMesh fails for it afterwards. Remove its draw items from Paint first.
    std::shared_ptr<IIr77Return const> UnloadSingleMeshBuffer(std::uint32_t const& mesh_id) {
        if (mesh_id >= m_single_mesh_buffers.size() || !m_single_mesh_buffers[mesh_id].vertex)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown mesh.");

        Ir77PVSingleMeshBuffer& mesh_buffer = m_single_mesh_buffers[mesh_id];
        Bury({mesh_buffer.vertex, mesh_buffer.index});
        mesh_buffer = Ir77PVSingleMeshBuffer{};

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // scene path (vertex) -- the client stacks every object into one Ir77PVSceneUpload and prebuilds one indirect command per object:
    //   firstIndex / indexCount   -> its slice of indices (indices stay local, 0-based)
    //   vertexOffset              -> where its vertices start
    //   instanceCount / firstInstance -> how many copies, and their slots in models[] (set 0 binding 2)
    // One draw item covers every command (MakeIndirectDrawItem); the vertex shader finds each command's material in a per-draw table (gl_DrawID).
    // The indirect buffer is Dynamic, so objects can be hidden / shown / resized per frame without re-uploading geometry.
    // =========================================================================================================================================
    std::shared_ptr<IIr77Return const> UploadScene(Ir77PVSceneUpload const& data, std::uint32_t& scene_id, bool const& wait = true) {
        if (data.vertices.empty() && data.vertices_skinned.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: empty scene.");

        Ir77PVSceneSlot scene{};
        std::vector<Ir77PVUploadEntry> entries{};

        if (!data.vertices.empty()) {
            VkDeviceSize const size = static_cast<VkDeviceSize>(data.vertices.size() * sizeof(Ir77PVVertex));
            scene.vertex = CreateStaticBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | RayInputUsage(), size);
            if (!scene.vertex) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: scene vertex buffer creation failed.");

            VkBuffer dst{VK_NULL_HANDLE};
            scene.vertex->GetBuffer(0, &dst);
            entries.push_back({dst, data.vertices.data(), size});

            scene.vertex_count = static_cast<std::uint32_t>(data.vertices.size());
        }

        if (!data.indices.empty()) {
            VkDeviceSize const size = static_cast<VkDeviceSize>(data.indices.size() * sizeof(std::uint32_t));
            scene.index = CreateStaticBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | RayInputUsage(), size);
            if (!scene.index) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: scene index buffer creation failed.");

            VkBuffer dst{VK_NULL_HANDLE};
            scene.index->GetBuffer(0, &dst);
            entries.push_back({dst, data.indices.data(), size});
        }

        if (!data.vertices_skinned.empty()) {
            VkDeviceSize const size = static_cast<VkDeviceSize>(data.vertices_skinned.size() * sizeof(Ir77PVVertexSkinned));
            scene.vertex_skinned = CreateStaticBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, size);
            if (!scene.vertex_skinned) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: scene skinned vertex buffer creation failed.");

            VkBuffer dst{VK_NULL_HANDLE};
            scene.vertex_skinned->GetBuffer(0, &dst);
            entries.push_back({dst, data.vertices_skinned.data(), size});
        }

        if (!data.indices_skinned.empty()) {
            VkDeviceSize const size = static_cast<VkDeviceSize>(data.indices_skinned.size() * sizeof(std::uint32_t));
            scene.index_skinned = CreateStaticBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, size);
            if (!scene.index_skinned) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: scene skinned index buffer creation failed.");

            VkBuffer dst{VK_NULL_HANDLE};
            scene.index_skinned->GetBuffer(0, &dst);
            entries.push_back({dst, data.indices_skinned.data(), size});
        }

        // indirect -- Dynamic, every frame copy written directly (not staged)
        if (!data.indirect_commands.empty()) {
            VkDeviceSize const size = static_cast<VkDeviceSize>(data.indirect_commands.size() * sizeof(VkDrawIndexedIndirectCommand));
            scene.indirect = CreateDynamicBuffer(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, size);
            if (!scene.indirect) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: scene indirect buffer creation failed.");

            for (std::uint32_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++) {
                if (scene.indirect->Update(frame, data.indirect_commands.data(), size)->ID() != GUIDIr77OperationSucceeded)
                    return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: scene indirect write failed.");
            }

            scene.commands = data.indirect_commands;
            scene.instance_counts.reserve(scene.commands.size());
            for (auto const& cmd : scene.commands) scene.instance_counts.push_back(cmd.instanceCount);
            scene.visible.assign(scene.commands.size(), 1);
        }

        if (Upload(entries, wait)->ID() != GUIDIr77OperationSucceeded) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: scene upload failed.");

        scene.alive = true;

        scene_id = static_cast<std::uint32_t>(m_scenes.size());
        m_scenes.push_back(std::move(scene));

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Draw item for commands [first_command, first_command + command_count) of a scene through an engine role
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
        if (!scene) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown scene.");

        std::shared_ptr<IIr77PVBuffer> const& vertex = skinned ? scene->vertex_skinned : scene->vertex;
        std::shared_ptr<IIr77PVBuffer> const& index = skinned ? scene->index_skinned : scene->index;

        if (!vertex || !index || !scene->indirect) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: scene channel empty.");

        if (static_cast<std::size_t>(first_command) + command_count > scene->commands.size())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: command range outside the scene.");

        Ir77PVMaterialData material{};
        if (GetMaterial(material_id, material)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown material.");

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
        if (!scene || command >= scene->commands.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown scene object.");

        scene->visible[command] = visible ? 1 : 0;
        scene->commands[command].instanceCount = visible ? scene->instance_counts[command] : 0;
        scene->dirty = MAX_FRAMES_IN_FLIGHT;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Change how many copies of an object draw -- spawn / despawn inside the models[] slots the client reserved for it
    // ([firstInstance, firstInstance + count) must not run into the next object's slots).
    std::shared_ptr<IIr77Return const> SetSceneObjectInstances(std::uint32_t const& scene_id, std::uint32_t const& command, std::uint32_t const& count) {
        Ir77PVSceneSlot* const scene = FindScene(scene_id);
        if (!scene || command >= scene->commands.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown scene object.");

        scene->instance_counts[command] = count;
        if (scene->visible[command]) scene->commands[command].instanceCount = count;
        scene->dirty = MAX_FRAMES_IN_FLIGHT;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // A scene's vertex buffer (skinned = the skinned channel) as a bindless storage buffer, so compute can read or write the vertices in place
    // (compute skinning reads the skinned stream and writes the posed one). Registered on the first call, the same slot after; UnloadScene
    // releases it.
    std::shared_ptr<IIr77Return const> GetSceneVertexSlot(std::uint32_t const& scene_id, bool const& skinned, std::uint32_t& slot) {
        Ir77PVSceneSlot* const scene = FindScene(scene_id);
        if (!scene) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown scene.");

        std::shared_ptr<IIr77PVBuffer> const& vertex = skinned ? scene->vertex_skinned : scene->vertex;
        std::uint32_t& registered = skinned ? scene->vertex_skinned_slot : scene->vertex_slot;

        if (!vertex) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: scene channel empty.");

        if (registered == BINDLESS_NO_SLOT) {
            auto const result = RegisterBuffer(vertex, registered);
            if (result->ID() != GUIDIr77OperationSucceeded) return result;
        }

        slot = registered;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // What Acceleration builds from: the scene's vertex / index buffers, its vertex count and its commands (instance counts whatever the
    // visibility). The buffers carry the build-input usage when the device has ray queries.
    std::shared_ptr<IIr77Return const> GetSceneGeometry(std::uint32_t const& scene_id, Ir77PVSceneGeometry& geometry) {
        Ir77PVSceneSlot* const scene = FindScene(scene_id);
        if (!scene) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown scene.");

        geometry = Ir77PVSceneGeometry{};
        geometry.vertex = scene->vertex;
        geometry.index = scene->index;
        geometry.vertex_count = scene->vertex_count;
        geometry.commands = scene->commands;
        geometry.instance_counts = scene->instance_counts;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Frees the scene's buffers once no frame in flight can read them. Remove its draw items from Paint first.
    std::shared_ptr<IIr77Return const> UnloadScene(std::uint32_t const& scene_id) {
        Ir77PVSceneSlot* const scene = FindScene(scene_id);
        if (!scene) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown scene.");

        ReleaseSlots(BINDLESS_BINDING_BUFFERS, {scene->vertex_slot, scene->vertex_skinned_slot});
        Bury({scene->vertex, scene->index, scene->vertex_skinned, scene->index_skinned, scene->indirect});
        *scene = Ir77PVSceneSlot{};

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // meshlet path -- Mesh pipeline kind only (VK_EXT_mesh_shader). Takes the same stacked Ir77PVSceneUpload as UploadScene.
    // Each indirect command is one object's range; meshlets are cut from that range only, so none ever spans two objects -- and from one material
// row's triangles at a time (triangle_materials), so each meshlet has exactly one material.
    // One draw record + one mesh command per instance. Static (non-skinned) vertices only. On a ray-query device the scene's index buffer is
    // kept as well -- GetMeshletGeometry, for the BLASes.
    // record_count = records written. Fails when the device has no mesh shading.
    // =========================================================================================================================================
    std::shared_ptr<IIr77Return const> UploadMeshlets(Ir77PVSceneUpload const& scene, std::uint32_t& meshlet_id, std::uint32_t& record_count,
                                                      bool const& wait = true) {
        if (scene.vertices.empty() || scene.indices.empty() || scene.indirect_commands.empty())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: empty scene.");

        if (!m_device) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: device not set.");

        bool mesh_shading{false};
        m_device->GetMeshShading(mesh_shading);

        if (!mesh_shading) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: mesh shading unsupported on this device.");

        if (scene.triangle_materials.size() != scene.indices.size() / 3)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: the mesh path needs one material row per triangle (triangle_materials).");

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
        std::vector<std::uint32_t> rows{};           // the object's material rows, first-seen order
        std::vector<std::uint32_t> group_indices{};  // the object's triangles that use one row

        for (std::size_t c = 0; c < scene.indirect_commands.size(); c++) {
            first_record.push_back(static_cast<std::uint32_t>(draws.size()));  // every command gets an entry, even when it draws nothing

            VkDrawIndexedIndirectCommand const& cmd = scene.indirect_commands[c];
            if (cmd.indexCount == 0) continue;

            // 1. this object's slice
            if (cmd.vertexOffset < 0 || static_cast<std::size_t>(cmd.firstIndex) + cmd.indexCount > scene.indices.size() ||
                static_cast<std::size_t>(cmd.vertexOffset) >= scene.vertices.size())
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: indirect command outside the scene buffers.");

            if (cmd.firstIndex % 3 != 0 || cmd.indexCount % 3 != 0)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: indirect command does not cover whole triangles.");

            std::uint32_t const* indices = &scene.indices[cmd.firstIndex];
            Ir77PVVertex const* vertices = &scene.vertices[static_cast<std::size_t>(cmd.vertexOffset)];

            // 2. vertices this object uses -- indices are local, so highest + 1
            std::size_t const local_count = static_cast<std::size_t>(*std::max_element(indices, indices + cmd.indexCount)) + 1;
            if (static_cast<std::size_t>(cmd.vertexOffset) + local_count > scene.vertices.size())
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: object indices run past the vertex buffer.");

            float const* positions = &vertices[0].position.x;
            std::uint32_t const first_triangle = cmd.firstIndex / 3;
            std::uint32_t const triangle_count = cmd.indexCount / 3;

            // 3. the object's material rows -- meshlets are cut from one row's triangles at a time, so every meshlet has exactly one material
            rows.clear();
            for (std::uint32_t t = 0; t < triangle_count; t++) {
                std::uint32_t const row = scene.triangle_materials[first_triangle + t];
                if (std::find(rows.begin(), rows.end(), row) == rows.end()) rows.push_back(row);
            }

            std::uint32_t const meshlet_base = static_cast<std::uint32_t>(meshlets.size());  // the object's meshlets stay contiguous

            for (std::uint32_t const row : rows) {
                // 4. this row's triangles
                group_indices.clear();
                for (std::uint32_t t = 0; t < triangle_count; t++) {
                    if (scene.triangle_materials[first_triangle + t] == row) group_indices.insert(group_indices.end(), indices + t * 3, indices + t * 3 + 3);
                }

                // 5. build on them only -- the builder can't see any other object or material
                std::size_t const max_meshlets = meshopt_buildMeshletsBound(group_indices.size(), MESHLET_MAX_VERTICES, MESHLET_MAX_TRIANGLES);
                built.assign(max_meshlets, meshopt_Meshlet{});
                local_vertices.assign(max_meshlets * MESHLET_MAX_VERTICES, 0);
                local_triangles.assign(max_meshlets * MESHLET_MAX_TRIANGLES * 3, 0);

                std::size_t const count =
                    meshopt_buildMeshlets(built.data(), local_vertices.data(), local_triangles.data(), group_indices.data(), group_indices.size(), positions,
                                          local_count, sizeof(Ir77PVVertex), MESHLET_MAX_VERTICES, MESHLET_MAX_TRIANGLES, MESHLET_CONE_WEIGHT);
                if (count == 0) continue;
                built.resize(count);

                // where this group starts in the shared arrays
                std::uint32_t const vertex_base = static_cast<std::uint32_t>(meshlet_vertices.size());
                std::uint32_t const triangle_base = static_cast<std::uint32_t>(meshlet_triangles.size());  // stays a 4-byte multiple

                // 6. optimize + bounds (object-local space, same space models[] transforms from)
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
                    out.material = row;
                    meshlets.push_back(out);
                }

                // 7. append -- local vertex index + vertexOffset = index into the full vertex buffer
                meshopt_Meshlet const& last = built.back();
                std::size_t const used_vertices = last.vertex_offset + last.vertex_count;
                std::size_t const used_triangles = last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3u);

                for (std::size_t j = 0; j < used_vertices; j++) meshlet_vertices.push_back(local_vertices[j] + static_cast<std::uint32_t>(cmd.vertexOffset));

                meshlet_triangles.insert(meshlet_triangles.end(), local_triangles.begin(), local_triangles.begin() + used_triangles);
            }

            std::uint32_t const object_meshlets = static_cast<std::uint32_t>(meshlets.size()) - meshlet_base;
            if (object_meshlets == 0) continue;

            // 8. this object = meshlets [meshlet_base, meshlet_base + object_meshlets). One record per instance, same meaning as the vertex path.
            std::uint32_t const groups = (object_meshlets + 31) / 32;  // TASK_GROUP_SIZE

            for (std::uint32_t inst = 0; inst < cmd.instanceCount; inst++) {
                Ir77PVMeshDraw draw{};
                draw.instance = cmd.firstInstance + inst;
                draw.meshlet_offset = meshlet_base;
                draw.meshlet_count = object_meshlets;

                draws.push_back(draw);
                commands.push_back(VkDrawMeshTasksIndirectCommandEXT{groups, 1, 1});
                groups_per_record.push_back(groups);
            }
        }

        first_record.push_back(static_cast<std::uint32_t>(draws.size()));  // end entry

        if (meshlets.empty() || draws.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: scene produced no meshlets or draws.");

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
        mesh.vertices = CreateStaticBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | RayInputUsage(), vertices_size);  // also the BLAS vertex input
        mesh.draws = CreateStaticBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, draws_size);

        if (!mesh.meshlets || !mesh.meshlet_vertices || !mesh.meshlet_triangles || !mesh.vertices || !mesh.draws)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: meshlet buffer creation failed.");

        // indirect -- Dynamic, so records can be hidden per frame ({0,1,1}). Written directly, not staged.
        mesh.indirect = CreateDynamicBuffer(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, indirect_size);
        if (!mesh.indirect) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: meshlet indirect buffer creation failed.");

        for (std::uint32_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++) {
            if (mesh.indirect->Update(frame, commands.data(), indirect_size)->ID() != GUIDIr77OperationSucceeded)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: meshlet indirect write failed.");
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

        std::vector<Ir77PVUploadEntry> entries{{meshlets_dst, meshlets.data(), meshlets_size},
                                               {indices_dst, meshlet_vertices.data(), indices_size},
                                               {triangles_dst, meshlet_triangles.data(), triangles_size},
                                               {vertices_dst, scene.vertices.data(), vertices_size},
                                               {draws_dst, draws.data(), draws_size}};

        // ray queries -- the scene's index buffer too, so Acceleration can build the static stream's BLASes (meshlets can't be built from)
        if (RayInputUsage() != 0) {
            VkDeviceSize const index_size = static_cast<VkDeviceSize>(scene.indices.size() * sizeof(std::uint32_t));

            slot.ray_index = CreateStaticBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | RayInputUsage(), index_size);
            if (!slot.ray_index) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: meshlet ray index buffer creation failed.");

            VkBuffer index_dst{VK_NULL_HANDLE};
            slot.ray_index->GetBuffer(0, &index_dst);
            entries.push_back({index_dst, scene.indices.data(), index_size});

            slot.vertex_count = static_cast<std::uint32_t>(scene.vertices.size());
            slot.scene_commands = scene.indirect_commands;
            slot.instance_counts.reserve(scene.indirect_commands.size());
            for (auto const& cmd : scene.indirect_commands) slot.instance_counts.push_back(cmd.instanceCount);
        }

        if (Upload(entries, wait)->ID() != GUIDIr77OperationSucceeded) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: meshlet upload failed.");

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

        meshlet_id = static_cast<std::uint32_t>(m_meshlet_scenes.size());
        m_meshlet_scenes.push_back(std::move(slot));

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // What Acceleration builds the mesh path's static stream from -- the same shape as GetSceneGeometry: the meshlet scene's vertices, the
    // scene's index buffer (kept only on ray-query devices) and its indirect commands
    std::shared_ptr<IIr77Return const> GetMeshletGeometry(std::uint32_t const& meshlet_id, Ir77PVSceneGeometry& geometry) {
        Ir77PVMeshletSlot* const slot = FindMeshlets(meshlet_id);
        if (!slot) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown meshlet mesh.");
        if (!slot->ray_index) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: meshlet mesh has no ray input (no ray queries).");

        geometry = Ir77PVSceneGeometry{};
        geometry.vertex = slot->mesh.vertices;
        geometry.index = slot->ray_index;
        geometry.vertex_count = slot->vertex_count;
        geometry.commands = slot->scene_commands;
        geometry.instance_counts = slot->instance_counts;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetMeshlets(std::uint32_t const& meshlet_id, Ir77PVMeshletMesh& mesh) {
        Ir77PVMeshletSlot* const slot = FindMeshlets(meshlet_id);
        if (!slot) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown meshlet mesh.");

        mesh = slot->mesh;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // First record of a scene command -- record = first_record + instance offset, for SetMeshletRecordVisible.
    std::shared_ptr<IIr77Return const> GetMeshletFirstRecord(std::uint32_t const& meshlet_id, std::uint32_t const& command, std::uint32_t& first_record) {
        Ir77PVMeshletSlot* const slot = FindMeshlets(meshlet_id);
        if (!slot || static_cast<std::size_t>(command) + 1 >= slot->first_record.size())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown meshlet object.");

        first_record = slot->first_record[command];

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
        Ir77PVMaterialData material{};

        if (GetMeshlets(meshlet_id, mesh)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown meshlet mesh.");
        if (GetMaterial(material_id, material)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown material.");

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

    std::shared_ptr<IIr77Return const> MakeMeshletIndirectDrawItem(std::size_t const& window, Ir77PVPipelineHandle const& pipeline,
                                                                   std::uint32_t const& meshlet_id, std::uint32_t const& material_id,
                                                                   std::uint32_t const& first_record, std::uint32_t const& record_count, Ir77PVDrawItem& item) {
        Ir77PVMeshletSlot* const slot = FindMeshlets(meshlet_id);
        if (!slot) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown meshlet mesh.");

        if (static_cast<std::size_t>(first_record) + record_count > slot->commands.size())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: record range outside the meshlet mesh.");

        auto const result = MakeMeshletDrawItem(window, pipeline, meshlet_id, material_id, item);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        item.indirect = slot->mesh.indirect;
        item.indirect_offset = static_cast<VkDeviceSize>(first_record) * sizeof(VkDrawMeshTasksIndirectCommandEXT);
        item.indirect_count = record_count;
        item.draw_base = first_record;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Same as MakeIndirectDrawItem but on the mesh path: commands [first_command, first_command + command_count) of the uploaded scene,
    // mapped to their records. Every meshlet carries its own material row, so the whole scene is one mesh draw.
    std::shared_ptr<IIr77Return const> MakeMeshletSceneDrawItem(std::size_t const& window, std::uint32_t const& meshlet_id, std::uint32_t const& material_id,
                                                                std::uint32_t const& first_command, std::uint32_t const& command_count, Ir77PVDrawItem& item) {
        Ir77PVPipelineHandle pipeline{IR77PV_NO_PIPELINE};

        auto const result = FindPipeline(Ir77PVPipelineKind::Mesh, pipeline);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        return MakeMeshletSceneDrawItem(window, pipeline, meshlet_id, material_id, first_command, command_count, item);
    }

    std::shared_ptr<IIr77Return const> MakeMeshletSceneDrawItem(std::size_t const& window, Ir77PVPipelineHandle const& pipeline,
                                                                std::uint32_t const& meshlet_id, std::uint32_t const& material_id,
                                                                std::uint32_t const& first_command, std::uint32_t const& command_count, Ir77PVDrawItem& item) {
        Ir77PVMeshletSlot* const slot = FindMeshlets(meshlet_id);
        if (!slot) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown meshlet mesh.");

        if (static_cast<std::size_t>(first_command) + command_count >= slot->first_record.size())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: command range outside the meshlet scene.");

        std::uint32_t const first = slot->first_record[first_command];
        std::uint32_t const end = slot->first_record[first_command + command_count];

        return MakeMeshletIndirectDrawItem(window, pipeline, meshlet_id, material_id, first, end - first, item);
    }

    // Hide / show every instance of one scene object on the mesh path. Applied to each frame copy by Tick.
    std::shared_ptr<IIr77Return const> SetMeshletObjectVisible(std::uint32_t const& meshlet_id, std::uint32_t const& command, bool const& visible) {
        Ir77PVMeshletSlot* const slot = FindMeshlets(meshlet_id);
        if (!slot || static_cast<std::size_t>(command) + 1 >= slot->first_record.size())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown meshlet object.");

        for (std::uint32_t r = slot->first_record[command]; r < slot->first_record[command + 1]; r++)
            slot->commands[r].groupCountX = visible ? slot->groups[r] : 0;

        slot->dirty = MAX_FRAMES_IN_FLIGHT;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Hide / show one instance (record). The record count is fixed at build time -- spawn capacity is reserved by uploading
    // extra instances and starting them hidden.
    std::shared_ptr<IIr77Return const> SetMeshletRecordVisible(std::uint32_t const& meshlet_id, std::uint32_t const& record, bool const& visible) {
        Ir77PVMeshletSlot* const slot = FindMeshlets(meshlet_id);
        if (!slot || record >= slot->commands.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown meshlet record.");

        slot->commands[record].groupCountX = visible ? slot->groups[record] : 0;
        slot->dirty = MAX_FRAMES_IN_FLIGHT;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Releases the meshlet buffer slots and frees the buffers once no frame in flight can read them. Remove its draw items from Paint first.
    std::shared_ptr<IIr77Return const> UnloadMeshlets(std::uint32_t const& meshlet_id) {
        Ir77PVMeshletSlot* const slot = FindMeshlets(meshlet_id);
        if (!slot) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown meshlet mesh.");

        Ir77PVMeshletMesh const& mesh = slot->mesh;
        ReleaseSlots(BINDLESS_BINDING_BUFFERS, std::vector<std::uint32_t>(mesh.slots.begin(), mesh.slots.end()));
        Bury({mesh.meshlets, mesh.meshlet_vertices, mesh.meshlet_triangles, mesh.vertices, mesh.draws, mesh.indirect, slot->ray_index});
        *slot = Ir77PVMeshletSlot{};

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // procedural -- no vertex buffer; the vertex shader builds its geometry from gl_VertexIndex / gl_InstanceIndex and storage buffers
    // =========================================================================================================================================
    // Draw item with no vertex buffer: vertex_count vertices (times instance_count) that the vertex shader builds from gl_VertexIndex /
    // gl_InstanceIndex and storage buffers. The pipeline's vertex shader must read no inputs. Caller fills model / user; for a GPU-driven
    // count set item.indirect (a storage buffer holding VkDrawIndirectCommands) and item.indirect_count.
    std::shared_ptr<IIr77Return const> MakeProceduralDrawItem(std::size_t const& window, Ir77PVPipelineHandle const& pipeline, std::uint32_t const& material_id,
                                                              std::uint32_t const& vertex_count, std::uint32_t const& instance_count, Ir77PVDrawItem& item) {
        Ir77PVMaterialData material{};
        if (GetMaterial(material_id, material)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown material.");

        item = Ir77PVDrawItem{};

        auto const pipeline_result = DefineItemPipeline(pipeline, false, true, item);
        if (pipeline_result->ID() != GUIDIr77OperationSucceeded) return pipeline_result;

        item.material = material.row;
        item.index_count = vertex_count;
        item.instance_count = instance_count;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    Ir77PVSceneSlot* FindScene(std::uint32_t const& scene_id) {
        if (scene_id >= m_scenes.size() || !m_scenes[scene_id].alive) return nullptr;

        return &m_scenes[scene_id];
    }

    Ir77PVMeshletSlot* FindMeshlets(std::uint32_t const& meshlet_id) {
        if (meshlet_id >= m_meshlet_scenes.size() || !m_meshlet_scenes[meshlet_id].alive) return nullptr;

        return &m_meshlet_scenes[meshlet_id];
    }

    // The handle for an engine role -- follows the manifest's "roles"
    std::shared_ptr<IIr77Return const> FindPipeline(Ir77PVPipelineKind const& kind, Ir77PVPipelineHandle& handle) {
        if (!m_pipeline_cache) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: CreatePipelines not called.");

        return m_pipeline_cache->FindPipeline(kind, handle);
    }

    // item.pipeline = handle, once it resolves now to the right path: mesh = a task / mesh pipeline, otherwise a vertex shader; procedural =
    // one that reads no vertex inputs. The command buffer re-resolves the handle every frame, so a later manifest reload still re-points the item.
    std::shared_ptr<IIr77Return const> DefineItemPipeline(Ir77PVPipelineHandle const& handle, bool const mesh, bool const procedural, Ir77PVDrawItem& item) {
        if (!m_pipeline_cache) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: CreatePipelines not called.");

        std::shared_ptr<IIr77PVPipeline> pipeline{};

        auto const result = m_pipeline_cache->GetPipeline(handle, pipeline);
        if (result->ID() != GUIDIr77OperationSucceeded) {
            std::string name{};
            m_pipeline_cache->GetPipelineName(handle, name);
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: pipeline '" + name + "' not available (manifest, shaders or pass missing).");
        }

        Ir77PVPipelineDesc desc{};
        pipeline->GetDesc(&desc);

        if (desc.IsMesh() != mesh)
            return Ir77RETURN<Ir77NotConfigured>(this, mesh ? "Ir77PVGeometry: a meshlet draw needs a task / mesh pipeline."
                                                            : "Ir77PVGeometry: a task / mesh pipeline takes a meshlet mesh -- use MakeMeshlet*DrawItem.");

        bool vertex_input{false};
        pipeline->GetVertexInput(vertex_input);

        if (procedural && vertex_input)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: a procedural draw needs a vertex shader that reads no inputs (layout locations).");

        item.pipeline = handle;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // A material's row -- through Materials
    std::shared_ptr<IIr77Return const> GetMaterial(std::uint32_t const& material_id, Ir77PVMaterialData& material) {
        if (!m_materials) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: materials not set.");

        return m_materials->GetMaterial(material_id, material);
    }

    // With ray queries on, scene geometry is also an acceleration-structure build input, read by GPU address
    VkBufferUsageFlags RayInputUsage() const {
        bool ray_query{false};
        if (m_device) m_device->GetRayQuery(ray_query);

        return ray_query ? VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR : 0;
    }

    // ---- through Transfer -- every buffer here is made, uploaded, slotted and released there --------------------------------------------

    std::shared_ptr<IIr77PVBuffer> CreateStaticBuffer(VkBufferUsageFlags const& usage, VkDeviceSize const& size) {
        std::shared_ptr<IIr77PVBuffer> buffer{};

        if (m_transfer) m_transfer->CreateStaticBuffer(usage, size, buffer);

        return buffer;
    }

    // Host-written every frame -- one copy per frame in flight, persistently mapped. Write with Update(frame, ...).
    std::shared_ptr<IIr77PVBuffer> CreateDynamicBuffer(VkBufferUsageFlags const& usage, VkDeviceSize const& size) {
        std::shared_ptr<IIr77PVBuffer> buffer{};

        if (m_transfer) m_transfer->CreateDynamicBuffer(usage, size, buffer);

        return buffer;
    }

    std::shared_ptr<IIr77Return const> Upload(std::vector<Ir77PVUploadEntry> const& entries, bool const& wait = true) {
        if (!m_transfer) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: transfer not set.");

        return m_transfer->Upload(entries, wait);
    }

    std::shared_ptr<IIr77Return const> RegisterBuffer(std::shared_ptr<IIr77PVBuffer> const& buffer, std::uint32_t& slot) {
        if (!m_transfer) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: transfer not set.");

        return m_transfer->RegisterBuffer(buffer, slot);
    }

    void ReleaseSlots(std::uint32_t const& binding, std::vector<std::uint32_t> const& slots) {
        if (m_transfer) m_transfer->ReleaseSlots(binding, slots);
    }

    void Bury(std::vector<std::shared_ptr<IIr77PVBuffer>> const& buffers) {
        if (m_transfer) m_transfer->Bury(buffers);
    }

   private:
    // wiring -- Ir77PVInit
    std::shared_ptr<IIr77PVDevice> m_device{};

    std::shared_ptr<IIr77PVTransfer> m_transfer{};

    std::shared_ptr<IIr77PVMaterial> m_materials{};

    std::shared_ptr<IIr77PVPipelineCache> m_pipeline_cache{};

    // owned
    std::vector<Ir77PVSingleMeshBuffer> m_single_mesh_buffers{};  // single-mesh path

    std::vector<Ir77PVSceneSlot> m_scenes{};  // scene path (vertex)

    std::vector<Ir77PVMeshletSlot> m_meshlet_scenes{};  // meshlet path
};
}  // namespace NSIr77PeregrineV