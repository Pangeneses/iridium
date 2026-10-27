#pragma once

#include <glm/glm.hpp>

#include <meshoptimizer.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
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
//   startup:   Ir77PVInit wires it -- SetDevice / SetTransfer / SetMaterials -> [Init builds the pipeline cache] -> SetPipelineCache ->
//              CreateGeometry (every pool and table) -> GetCommandSlots (Paint puts this frame's one in the frame table)
//   load:      UploadScene (vertex path) / UploadMeshlets (mesh path)
//   frame:     Tick(frame) -> Make*DrawItem -> Paint::SetDraws
//   runtime:   SetSceneObjectVisible / SetSceneObjectInstances / SetMeshletObjectVisible / SetMeshletRecordVisible
//   compute:   GetSceneVertexSlot (vertices as a storage buffer); rays: GetSceneGeometry (what Acceleration builds BLASes from)
//   unload:    remove the draw items from Paint, then UnloadScene / UnloadMeshlets (freed after the frames in flight)
//   pools:     GetPoolUsage
//   culling:   SetCullBins (when the bins change) -> GetCullDispatches (Paint: every window, once per view) -> MakeCulledDrawItem per bin and view
// One device; it never sees Ir77PVInit. Every buffer is made, uploaded, slotted and released through Transfer; materials come from Materials.
//
// Geometry pools: every scene's vertices and indices go into four device buffers made on first use -- static vertices, static indices,
// skinned vertices, skinned indices -- and its draw commands into two host-written ones, indexed and mesh-task, one copy per frame in flight
// (GEOMETRY_*_CAPACITY). A scene takes a range of each it needs, first fit; what's left is free for whatever loads next, and an unloaded
// scene's ranges come back once the frames in flight are done with them. Every draw binds the same buffers and reads the same command buffer.
// Beside the indexed command pool sits the command material table -- one material row per command, same index, device-local -- so a vertex
// shader finds any command's material from where it is in the pool: table[pc.user[5] + gl_DrawID], pc.user[4] = the table's slot. Every
// indexed draw item carries both; nothing per scene. A command bounds table sits beside it the same way -- each command's object-space sphere.
//
// GPU culling (cull.comp): the caller groups vertex-path commands into bins -- one per pipeline and vertex stream -- with SetCullBins. Every
// frame, per view, the cull pass tests each command's instances against the view and compacts the survivors into the bin's list for that
// view (one command per instance, material rows beside them); MakeCulledDrawItem draws a list with vkCmdDrawIndexedIndirectCount. The cull
// pass reads this frame's copy of the command pool through the frame table (IR77_FRAME.commands), so hiding an object still works.
// Shaders that read a scene's vertices by slot (compute skinning, the mesh path) get a slot over just its range, so their indices stay local.
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

        m_scenes.clear();
        m_meshlet_scenes.clear();
        m_pools = {};
        m_command_materials.reset();
        m_command_bounds.reset();
        m_cull_commands.reset();
        m_cull_materials.reset();
        m_cull_counts.reset();
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
    // create -- every pool, the command tables, the cull lists and their slots, up front. After the wiring.
    // =========================================================================================================================================
    std::shared_ptr<IIr77Return const> CreateGeometry() {
        std::shared_ptr<IIr77Return const> result{};

        if (!m_transfer) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: SetTransfer not called.");
        if (m_command_materials) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: geometry already created.");

        if (Failed(DefinePools(), result)) return result;
        if (Failed(DefineCommandTables(), result)) return result;
        if (Failed(DefineCullLists(), result)) return result;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Geometry.");
    }

    // The command pool's slot for each frame in flight -- what the frame table names (IR77_FRAME.commands) for the cull pass
    std::shared_ptr<IIr77Return const> GetCommandSlots(std::array<std::uint32_t, MAX_FRAMES_IN_FLIGHT>& slots) {
        slots = m_command_slots;

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

            if (!WriteCommands(Ir77PVGeometryPool::Commands, scene.ranges, scene.commands.data(), frame))
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: scene indirect write failed.");

            scene.dirty--;
        }

        for (auto& slot : m_meshlet_scenes) {
            if (!slot.alive || slot.dirty == 0 || !slot.mesh.indirect) continue;

            if (!WriteCommands(Ir77PVGeometryPool::MeshCommands, slot.ranges, slot.commands.data(), frame))
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: meshlet indirect write failed.");

            slot.dirty--;
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // scene path (vertex) -- the client stacks every object of one stream (static or skinned) into one Ir77PVSceneUpload and prebuilds one
    // indirect command per object, local to the upload:
    //   firstIndex / indexCount   -> its slice of indices (indices stay local, 0-based)
    //   vertexOffset              -> where its vertices start
    //   instanceCount / firstInstance -> how many copies, and their slots in models[] (set 0 binding 2)
    // The upload lands in ranges of the stream's pools and the commands are rebased onto them (firstIndex / vertexOffset + the ranges'
    // starts), into the scene's range of the command pool; command_materials (one material row per command) go to the same places in the
    // command material table. One draw item covers every command (MakeIndirectDrawItem); the vertex shader finds each command's material in
    // that table (gl_DrawID). The command pool is host-written per frame, so objects can be hidden / shown / resized per frame without
    // re-uploading geometry.
    // A pool too full for the upload fails it, naming the pool.
    // =========================================================================================================================================
    std::shared_ptr<IIr77Return const> UploadScene(Ir77PVSceneUpload const& data, std::uint32_t& scene_id, bool const& wait = true) {
        bool const skinned{!data.vertices_skinned.empty()};

        if (data.vertices.empty() && !skinned) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: empty scene.");

        if (!data.command_materials.empty() && data.command_materials.size() != data.indirect_commands.size())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: command_materials needs one row per indirect command.");

        if (skinned && (!data.vertices.empty() || !data.indices.empty()))
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: a scene is one stream -- static or skinned, not both.");

        std::shared_ptr<IIr77Return const> result{};

        Ir77PVGeometryPool const vertex_pool{skinned ? Ir77PVGeometryPool::SkinnedVertices : Ir77PVGeometryPool::Vertices};
        Ir77PVGeometryPool const index_pool{skinned ? Ir77PVGeometryPool::SkinnedIndices : Ir77PVGeometryPool::Indices};

        std::vector<std::uint32_t> const& indices = skinned ? data.indices_skinned : data.indices;
        std::size_t const vertex_count{skinned ? data.vertices_skinned.size() : data.vertices.size()};
        void const* const vertices = skinned ? static_cast<void const*>(data.vertices_skinned.data()) : static_cast<void const*>(data.vertices.data());

        Ir77PVSceneSlot scene{};
        std::vector<Ir77PVUploadEntry> entries{};
        std::vector<std::uint32_t> rows{};   // the commands' material rows -- read by Upload below
        std::vector<glm::vec4> spheres{};     // the commands' bounding spheres -- read by Upload below

        if (Failed(PlaceInPool(vertex_pool, vertex_count, vertices, scene.ranges, entries), result)) return result;

        if (Failed(PlaceInPool(index_pool, indices.size(), indices.data(), scene.ranges, entries), result)) {
            FreeGeometry(scene.ranges);
            return result;
        }

        Ir77PVGeometryRange const& vertex_range = scene.ranges[PoolIndex(vertex_pool)];
        Ir77PVGeometryRange const& index_range = scene.ranges[PoolIndex(index_pool)];

        (skinned ? scene.vertex_skinned : scene.vertex) = m_pools[PoolIndex(vertex_pool)];
        if (index_range.count > 0) (skinned ? scene.index_skinned : scene.index) = m_pools[PoolIndex(index_pool)];

        scene.vertex_count = vertex_range.first + vertex_range.count;

        // commands -- rebased onto the ranges, into the scene's range of the command pool; every frame copy written directly (not staged)
        if (!data.indirect_commands.empty()) {
            scene.commands = data.indirect_commands;

            for (VkDrawIndexedIndirectCommand& cmd : scene.commands) {
                cmd.firstIndex += index_range.first;
                cmd.vertexOffset += static_cast<std::int32_t>(vertex_range.first);
            }

            if (Failed(ReserveInPool(Ir77PVGeometryPool::Commands, scene.commands.size(), scene.ranges), result)) {
                FreeGeometry(scene.ranges);
                return result;
            }

            scene.indirect = m_pools[PoolIndex(Ir77PVGeometryPool::Commands)];

            // their material rows and bounding spheres -- the same places in the command tables (no rows given: row 0)
            rows = data.command_materials;
            if (rows.empty()) rows.assign(scene.commands.size(), 0);

            spheres.reserve(data.indirect_commands.size());
            for (VkDrawIndexedIndirectCommand const& cmd : data.indirect_commands)
                spheres.push_back(skinned ? CommandSphere(data.vertices_skinned, indices, cmd) : CommandSphere(data.vertices, indices, cmd));

            if (Failed(PlaceCommandTables(scene.ranges, rows, spheres, entries), result)) {
                FreeGeometry(scene.ranges);
                return result;
            }

            bool written{true};
            for (std::uint32_t frame = 0; written && frame < MAX_FRAMES_IN_FLIGHT; frame++)
                written = WriteCommands(Ir77PVGeometryPool::Commands, scene.ranges, scene.commands.data(), frame);

            if (!written) {
                FreeGeometry(scene.ranges);
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: scene indirect write failed.");
            }

            scene.instance_counts.reserve(scene.commands.size());
            for (auto const& cmd : scene.commands) scene.instance_counts.push_back(cmd.instanceCount);
            scene.visible.assign(scene.commands.size(), 1);
        }

        if (Upload(entries, wait)->ID() != GUIDIr77OperationSucceeded) {
            FreeGeometry(scene.ranges);
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: scene upload failed.");
        }

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
        std::uint32_t const first{scene->ranges[PoolIndex(Ir77PVGeometryPool::Commands)].first + first_command};

        item.indirect = scene->indirect;
        item.indirect_offset = static_cast<VkDeviceSize>(first) * sizeof(VkDrawIndexedIndirectCommand);
        item.indirect_count = command_count;

        // the vertex shaders' material lookup -- table[user[5] + gl_DrawID] (push user[4..7] = item.user)
        item.user = {m_command_materials_slot, first, 0u, 0u};

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

    // A scene's vertices (skinned = the skinned channel) as a bindless storage buffer over just its range of the pool -- vertex 0 is its first --
    // so compute can read or write them in place with indices local to the scene (compute skinning reads the skinned stream and writes the
    // posed one). Registered on the first call, the same slot after; UnloadScene releases it.
    std::shared_ptr<IIr77Return const> GetSceneVertexSlot(std::uint32_t const& scene_id, bool const& skinned, std::uint32_t& slot) {
        Ir77PVSceneSlot* const scene = FindScene(scene_id);
        if (!scene) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown scene.");

        Ir77PVGeometryPool const pool{skinned ? Ir77PVGeometryPool::SkinnedVertices : Ir77PVGeometryPool::Vertices};

        std::shared_ptr<IIr77PVBuffer> const& vertex = skinned ? scene->vertex_skinned : scene->vertex;
        std::uint32_t& registered = skinned ? scene->vertex_skinned_slot : scene->vertex_slot;

        if (!vertex) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: scene channel empty.");

        if (registered == BINDLESS_NO_SLOT) {
            Ir77PVGeometryRange const& range = scene->ranges[PoolIndex(pool)];

            auto const result = RegisterBuffer(vertex, Ir77PVSlotKind::Vertices, registered, static_cast<VkDeviceSize>(range.first) * PoolStride(pool),
                                               static_cast<VkDeviceSize>(range.count) * PoolStride(pool));
            if (result->ID() != GUIDIr77OperationSucceeded) return result;
        }

        slot = registered;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // What Acceleration builds from: the scene's vertex / index buffers (the pools), where its vertex range ends and its commands, rebased into
    // the pools (instance counts whatever the visibility). The pools carry the build-input usage when the device has ray queries.
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

    // Its vertex slots and its pool ranges (geometry and commands) -- released once no frame in flight can read them. Remove its draw items
    // from Paint first.
    std::shared_ptr<IIr77Return const> UnloadScene(std::uint32_t const& scene_id) {
        Ir77PVSceneSlot* const scene = FindScene(scene_id);
        if (!scene) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown scene.");

        Bury({.slots = {{BINDLESS_BINDING_BUFFERS, scene->vertex_slot}, {BINDLESS_BINDING_BUFFERS, scene->vertex_skinned_slot}},
              .released = ReleaseGeometry(scene->ranges)});
        *scene = Ir77PVSceneSlot{};

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // meshlet path -- Mesh pipeline kind only (VK_EXT_mesh_shader). Takes the same stacked Ir77PVSceneUpload as UploadScene.
    // Each indirect command is one object's range; meshlets are cut from that range only, so none ever spans two objects -- and from one material
// row's triangles at a time (triangle_materials), so each meshlet has exactly one material.
    // One draw record + one mesh command per instance. Static (non-skinned) vertices only, in a range of the vertex pool (its slot covers just
    // that range, so the meshlets' vertex indices stay local). On a ray-query device the scene's indices go into the index pool as well --
    // GetMeshletGeometry, for the BLASes.
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
        VkDeviceSize const draws_size = static_cast<VkDeviceSize>(draws.size() * sizeof(Ir77PVMeshDraw));

        Ir77PVMeshletSlot slot{};
        Ir77PVMeshletMesh& mesh = slot.mesh;

        mesh.meshlets = CreateStaticBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, meshlets_size);
        mesh.meshlet_vertices = CreateStaticBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, indices_size);
        mesh.meshlet_triangles = CreateStaticBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, triangles_size);
        mesh.draws = CreateStaticBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, draws_size);

        if (!mesh.meshlets || !mesh.meshlet_vertices || !mesh.meshlet_triangles || !mesh.draws)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: meshlet buffer creation failed.");

        VkBuffer meshlets_dst{VK_NULL_HANDLE};
        VkBuffer indices_dst{VK_NULL_HANDLE};
        VkBuffer triangles_dst{VK_NULL_HANDLE};
        VkBuffer draws_dst{VK_NULL_HANDLE};
        mesh.meshlets->GetBuffer(0, &meshlets_dst);
        mesh.meshlet_vertices->GetBuffer(0, &indices_dst);
        mesh.meshlet_triangles->GetBuffer(0, &triangles_dst);
        mesh.draws->GetBuffer(0, &draws_dst);

        std::vector<Ir77PVUploadEntry> entries{{meshlets_dst, meshlets.data(), meshlets_size},
                                               {indices_dst, meshlet_vertices.data(), indices_size},
                                               {triangles_dst, meshlet_triangles.data(), triangles_size},
                                               {draws_dst, draws.data(), draws_size}};

        std::shared_ptr<IIr77Return const> result{};

        // the vertices -- a range of the vertex pool (also the BLAS vertex input)
        if (Failed(PlaceInPool(Ir77PVGeometryPool::Vertices, scene.vertices.size(), scene.vertices.data(), slot.ranges, entries), result)) return result;

        Ir77PVGeometryRange const& vertex_range = slot.ranges[PoolIndex(Ir77PVGeometryPool::Vertices)];
        mesh.vertices = m_pools[PoolIndex(Ir77PVGeometryPool::Vertices)];

        // ray queries -- the scene's indices too, so Acceleration can build the static stream's BLASes (meshlets can't be built from); the
        // commands rebased onto both ranges
        if (RayInputUsage() != 0) {
            if (Failed(PlaceInPool(Ir77PVGeometryPool::Indices, scene.indices.size(), scene.indices.data(), slot.ranges, entries), result)) {
                FreeGeometry(slot.ranges);
                return result;
            }

            Ir77PVGeometryRange const& index_range = slot.ranges[PoolIndex(Ir77PVGeometryPool::Indices)];

            slot.ray_index = m_pools[PoolIndex(Ir77PVGeometryPool::Indices)];
            slot.vertex_count = vertex_range.first + vertex_range.count;
            slot.scene_commands = scene.indirect_commands;

            for (VkDrawIndexedIndirectCommand& cmd : slot.scene_commands) {
                cmd.firstIndex += index_range.first;
                cmd.vertexOffset += static_cast<std::int32_t>(vertex_range.first);
            }

            slot.instance_counts.reserve(scene.indirect_commands.size());
            for (auto const& cmd : scene.indirect_commands) slot.instance_counts.push_back(cmd.instanceCount);
        }

        // the mesh commands -- one per record, into the scene's range of the mesh command pool, so records can be hidden per frame ({0,1,1});
        // every frame copy written directly (not staged)
        if (Failed(ReserveInPool(Ir77PVGeometryPool::MeshCommands, commands.size(), slot.ranges), result)) {
            FreeGeometry(slot.ranges);
            return result;
        }

        mesh.indirect = m_pools[PoolIndex(Ir77PVGeometryPool::MeshCommands)];

        bool written{true};
        for (std::uint32_t frame = 0; written && frame < MAX_FRAMES_IN_FLIGHT; frame++)
            written = WriteCommands(Ir77PVGeometryPool::MeshCommands, slot.ranges, commands.data(), frame);

        if (!written || Upload(entries, wait)->ID() != GUIDIr77OperationSucceeded) {
            FreeGeometry(slot.ranges);
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: meshlet upload failed.");
        }

        // 8. bindless slots -- order matches Ir77PVMeshletMesh::slots / Ir77PVDrawItem::meshlet_slots
        std::array<std::shared_ptr<IIr77PVBuffer>, MESHLET_SLOT_COUNT> const slot_buffers{mesh.meshlets, mesh.meshlet_vertices, mesh.meshlet_triangles,
                                                                                          mesh.vertices, mesh.draws};
        std::array<Ir77PVSlotKind, MESHLET_SLOT_COUNT> const slot_kinds{Ir77PVSlotKind::Meshlets, Ir77PVSlotKind::MeshletVertices,
                                                                        Ir77PVSlotKind::MeshletTriangles, Ir77PVSlotKind::Vertices, Ir77PVSlotKind::MeshDraws};

        // the vertices' slot covers just the scene's range of the pool
        for (std::size_t i = 0; i < slot_buffers.size(); i++) {
            bool const vertices{slot_kinds[i] == Ir77PVSlotKind::Vertices};

            VkDeviceSize const offset{vertices ? static_cast<VkDeviceSize>(vertex_range.first) * sizeof(Ir77PVVertex) : 0};
            VkDeviceSize const range{vertices ? static_cast<VkDeviceSize>(vertex_range.count) * sizeof(Ir77PVVertex) : VK_WHOLE_SIZE};

            if (Failed(RegisterBuffer(slot_buffers[i], slot_kinds[i], mesh.slots[i], offset, range), result)) {
                Bury({.slots = MeshletSlots(mesh), .released = ReleaseGeometry(slot.ranges)});
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

    // First record of a scene command -- record = first_record + instance offset, for SetMeshletRecordVisible.
    std::shared_ptr<IIr77Return const> GetMeshletFirstRecord(std::uint32_t const& meshlet_id, std::uint32_t const& command, std::uint32_t& first_record) {
        Ir77PVMeshletSlot* const slot = FindMeshlets(meshlet_id);
        if (!slot || static_cast<std::size_t>(command) + 1 >= slot->first_record.size())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown meshlet object.");

        first_record = slot->first_record[command];

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

        Ir77PVMaterialData material{};
        if (GetMaterial(material_id, material)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown material.");

        // the commands' records -- indirect_offset (in the command pool) and draw_base (in the scene's own draw records) both start at the
        // first, so gl_DrawID 0..n-1 lines up with them
        std::uint32_t const first = slot->first_record[first_command];
        std::uint32_t const end = slot->first_record[first_command + command_count];

        item = Ir77PVDrawItem{};

        auto const pipeline_result = DefineItemPipeline(pipeline, true, false, item);
        if (pipeline_result->ID() != GUIDIr77OperationSucceeded) return pipeline_result;

        item.material = material.row;
        item.meshlet_slots = slot->mesh.slots;
        item.meshlet_offset = 0;
        item.meshlet_count = slot->mesh.meshlet_count;
        item.indirect = slot->mesh.indirect;
        item.indirect_offset = static_cast<VkDeviceSize>(slot->ranges[PoolIndex(Ir77PVGeometryPool::MeshCommands)].first + first) *
                               sizeof(VkDrawMeshTasksIndirectCommandEXT);
        item.indirect_count = end - first;
        item.draw_base = first;

        return Ir77RETURN<Ir77OperationSucceeded>();
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

    // Its meshlet buffers, their slots and its pool ranges -- released once no frame in flight can read them. Remove its draw items from Paint
    // first.
    std::shared_ptr<IIr77Return const> UnloadMeshlets(std::uint32_t const& meshlet_id) {
        Ir77PVMeshletSlot* const slot = FindMeshlets(meshlet_id);
        if (!slot) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown meshlet mesh.");

        Ir77PVMeshletMesh const& mesh = slot->mesh;
        Bury({.buffers = {mesh.meshlets, mesh.meshlet_vertices, mesh.meshlet_triangles, mesh.draws},
              .slots = MeshletSlots(mesh),
              .released = ReleaseGeometry(slot->ranges)});
        *slot = Ir77PVMeshletSlot{};

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // GPU culling
    // =========================================================================================================================================
    // The bins the cull pass fills -- each one's runs of commands (each frustum-tested or not) and whether only view 0 lists them. Every
    // command's instance count (whatever its visibility) is a place in its bin's list per view; the bins together must fit GEOMETRY_CULL_CAPACITY,
    // and there are at most GEOMETRY_CULL_BINS. Call whenever the bins change -- the job list is remade and the old one released after the frames
    // in flight. An instance count raised later than this (SetSceneObjectInstances) past what the bin was given is cut at the bin's end.
    std::shared_ptr<IIr77Return const> SetCullBins(std::vector<Ir77PVCullBin> const& bins) {
        std::shared_ptr<IIr77Return const> result{};

        if (bins.size() > GEOMETRY_CULL_BINS) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: more cull bins than GEOMETRY_CULL_BINS.");

        std::vector<glm::uvec4> jobs{};
        std::vector<glm::uvec2> places(bins.size(), glm::uvec2(0u));  // per bin: first place, places
        std::uint32_t total{0};

        for (std::size_t b = 0; b < bins.size(); b++) {
            Ir77PVCullBin const& bin = bins[b];

            places[b].x = total;

            std::size_t const first_job{jobs.size()};

            for (Ir77PVCullRun const& run : bin.runs) {
                std::uint32_t const flags{(run.test ? CULL_FLAG_TEST : 0u) | (bin.first_view_only ? CULL_FLAG_FIRST_VIEW_ONLY : 0u)};

                Ir77PVSceneSlot* const scene = FindScene(run.scene);
                if (!scene || static_cast<std::size_t>(run.first_command) + run.command_count > scene->commands.size())
                    return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: cull run outside its scene.");

                std::uint32_t const base{scene->ranges[PoolIndex(Ir77PVGeometryPool::Commands)].first};

                for (std::uint32_t c = run.first_command; c < run.first_command + run.command_count; c++) {
                    jobs.push_back(glm::uvec4(base + c, static_cast<std::uint32_t>(b) | (flags << 24), 0u, 0u));
                    places[b].y += scene->instance_counts[c];
                }
            }

            total += places[b].y;
            if (total > GEOMETRY_CULL_CAPACITY) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: the cull bins need more than GEOMETRY_CULL_CAPACITY places.");

            for (std::size_t j = first_job; j < jobs.size(); j++) {
                jobs[j].z = places[b].x;
                jobs[j].w = places[b].y;
            }
        }

        // the old list goes after the frames in flight; the new one in its own buffer, so no frame reads a half-written list
        if (m_cull_jobs_slot != BINDLESS_NO_SLOT) m_transfer->DestroyStorageBuffer(m_cull_jobs_buffer);

        m_cull_jobs_slot = BINDLESS_NO_SLOT;
        m_cull_job_count = 0;
        m_cull_places = places;

        if (jobs.empty()) return Ir77RETURN<Ir77OperationSucceeded>();

        if (Failed(m_transfer->CreateStorageBuffer(static_cast<VkDeviceSize>(jobs.size() * sizeof(glm::uvec4)), jobs.data(), m_cull_jobs_buffer, m_cull_jobs_slot),
                   result))
            return result;

        m_cull_job_count = static_cast<std::uint32_t>(jobs.size());

        return result;
    }

    // The cull pass for one view -- zero the view's counts, then cull (nothing to cull: the zeroing alone). Recorded by every window, once per
    // view (Paint::SetViewDispatches); the command buffer fills in pc.user[7] = the view and pc.frame = its frame table. Fails without a
    // "cull.compute" pipeline in the manifest -- the caller draws unculled then.
    std::shared_ptr<IIr77Return const> GetCullDispatches(std::vector<Ir77PVDispatch>& dispatches) {
        dispatches.clear();

        if (!m_pipeline_cache) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: CreatePipelines not called.");
        if (!m_cull_counts) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: CreateGeometry not called.");

        Ir77PVPipelineHandle pipeline{IR77PV_NO_PIPELINE};
        if (m_pipeline_cache->FindPipeline(std::string{"cull.compute"}, pipeline)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: no cull.compute pipeline -- drawing without GPU culling.");

        Ir77PVDispatch reset{};
        reset.pipeline = pipeline;
        reset.push.mode = 0;
        reset.push.user = {0u, 0u, 0u, 0u, 0u, 0u, m_cull_counts_slot, 0u};
        reset.groups = {(GEOMETRY_CULL_BINS + CULL_GROUP_SIZE - 1) / CULL_GROUP_SIZE, 1u, 1u};

        dispatches.push_back(reset);

        if (m_cull_job_count == 0) return Ir77RETURN<Ir77OperationSucceeded>();

        Ir77PVDispatch cull{};
        cull.pipeline = pipeline;
        cull.push.mode = 1;
        cull.push.user = {m_cull_jobs_slot, m_cull_job_count, m_command_bounds_slot, m_command_materials_slot, m_cull_commands_slot, m_cull_materials_slot,
                          m_cull_counts_slot, 0u};
        cull.groups = {(m_cull_job_count + CULL_GROUP_SIZE - 1) / CULL_GROUP_SIZE, 1u, 1u};

        dispatches.push_back(cull);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // One bin's culled list for one view, through pipeline -- drawn with vkCmdDrawIndexedIndirectCount from the pool's vertex / index buffers
    // (skinned = the skinned pair). Its material rows are the culled materials, read the usual way (pc.user[4] / [5] + gl_DrawID).
    std::shared_ptr<IIr77Return const> MakeCulledDrawItem(Ir77PVPipelineHandle const& pipeline, std::uint32_t const& bin, std::uint32_t const& view,
                                                          bool const& skinned, Ir77PVDrawItem& item) {
        if (bin >= m_cull_places.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown cull bin.");
        if (view >= MAX_VIEWS) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: view past MAX_VIEWS.");

        std::shared_ptr<IIr77PVBuffer> const& vertex = m_pools[PoolIndex(skinned ? Ir77PVGeometryPool::SkinnedVertices : Ir77PVGeometryPool::Vertices)];
        std::shared_ptr<IIr77PVBuffer> const& index = m_pools[PoolIndex(skinned ? Ir77PVGeometryPool::SkinnedIndices : Ir77PVGeometryPool::Indices)];

        if (!vertex || !index || !m_cull_commands) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: CreateGeometry not called.");

        item = Ir77PVDrawItem{};

        auto const pipeline_result = DefineItemPipeline(pipeline, false, false, item);
        if (pipeline_result->ID() != GUIDIr77OperationSucceeded) return pipeline_result;

        std::uint32_t const first{view * GEOMETRY_CULL_CAPACITY + m_cull_places[bin].x};

        item.vertex = vertex;
        item.index = index;
        item.index_type = VK_INDEX_TYPE_UINT32;
        item.indirect = m_cull_commands;
        item.indirect_offset = static_cast<VkDeviceSize>(first) * sizeof(VkDrawIndexedIndirectCommand);
        item.indirect_count = m_cull_places[bin].y;
        item.count = m_cull_counts;
        item.count_offset = static_cast<VkDeviceSize>(view * GEOMETRY_CULL_BINS + bin) * sizeof(std::uint32_t);
        item.user = {m_cull_materials_slot, first, 0u, 0u};

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // pools
    // =========================================================================================================================================
    // How much of a pool is in use and how much it holds, in elements (vertices, indices or commands). A pool not made yet: 0 used.
    std::shared_ptr<IIr77Return const> GetPoolUsage(Ir77PVGeometryPool const& pool, std::uint32_t& used, std::uint32_t& capacity) {
        used = 0;
        for (auto const& [first, count] : m_pool_ranges[PoolIndex(pool)]) used += count;

        capacity = PoolCapacity(pool);

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
                                                            : "Ir77PVGeometry: a task / mesh pipeline takes a meshlet scene -- use MakeMeshletSceneDrawItem.");

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

    std::shared_ptr<IIr77Return const> RegisterBuffer(std::shared_ptr<IIr77PVBuffer> const& buffer, Ir77PVSlotKind const& kind, std::uint32_t& slot,
                                                      VkDeviceSize const& offset = 0, VkDeviceSize const& range = VK_WHOLE_SIZE, std::uint32_t const& copy = 0) {
        if (!m_transfer) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: transfer not set.");

        return m_transfer->RegisterBuffer(buffer, kind, slot, offset, range, copy);
    }

    void Bury(Ir77PVGrave const& grave) {
        if (m_transfer) m_transfer->Bury(grave);
    }

    // A meshlet mesh's five buffer slots, as the graveyard takes them
    std::vector<Ir77PVSlot> MeshletSlots(Ir77PVMeshletMesh const& mesh) const {
        std::vector<Ir77PVSlot> slots{};
        for (std::uint32_t const slot : mesh.slots) slots.push_back({BINDLESS_BINDING_BUFFERS, slot});

        return slots;
    }

    // ---- create steps -----------------------------------------------------------------------------------------------------------------------

    // Every pool -- device-local for geometry, host-written per frame in flight for commands. The command pool's copies get a slot each.
    std::shared_ptr<IIr77Return const> DefinePools() {
        std::shared_ptr<IIr77Return const> result{};

        for (std::size_t p = 0; p < GEOMETRY_POOL_COUNT; p++) {
            Ir77PVGeometryPool const pool{static_cast<Ir77PVGeometryPool>(p)};
            VkDeviceSize const size{static_cast<VkDeviceSize>(PoolCapacity(pool)) * PoolStride(pool)};

            m_pools[p] = PoolHostWritten(pool) ? CreateDynamicBuffer(PoolUsage(pool), size) : CreateStaticBuffer(PoolUsage(pool), size);
            if (!m_pools[p]) return Ir77RETURN<Ir77NotConfigured>(this, std::string{"Ir77PVGeometry: the "} + PoolName(pool) + " pool couldn't be made.");
        }

        std::shared_ptr<IIr77PVBuffer> const& commands = m_pools[PoolIndex(Ir77PVGeometryPool::Commands)];

        for (std::uint32_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++)
            if (Failed(RegisterBuffer(commands, Ir77PVSlotKind::DrawCommands, m_command_slots[frame], 0, VK_WHOLE_SIZE, frame), result)) return result;

        return result;
    }

    // Beside the indexed command pool, same index -- each command's material row and bounding sphere
    std::shared_ptr<IIr77Return const> DefineCommandTables() {
        std::shared_ptr<IIr77Return const> result{};

        m_command_materials = CreateStaticBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, static_cast<VkDeviceSize>(GEOMETRY_COMMAND_CAPACITY) * sizeof(std::uint32_t));
        m_command_bounds = CreateStaticBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, static_cast<VkDeviceSize>(GEOMETRY_COMMAND_CAPACITY) * sizeof(glm::vec4));

        if (!m_command_materials || !m_command_bounds) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: command table creation failed.");

        if (Failed(RegisterBuffer(m_command_materials, Ir77PVSlotKind::CommandMaterials, m_command_materials_slot), result)) return result;

        return RegisterBuffer(m_command_bounds, Ir77PVSlotKind::CommandBounds, m_command_bounds_slot);
    }

    // What the cull pass writes -- per view, GEOMETRY_CULL_CAPACITY culled commands and their material rows, and GEOMETRY_CULL_BINS counts.
    // One copy shared by every frame in flight: the Compute pass waits for the previous frame's draws before it writes.
    std::shared_ptr<IIr77Return const> DefineCullLists() {
        std::shared_ptr<IIr77Return const> result{};

        VkDeviceSize const places{static_cast<VkDeviceSize>(MAX_VIEWS) * GEOMETRY_CULL_CAPACITY};

        m_cull_commands = CreateStaticBuffer(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, places * sizeof(VkDrawIndexedIndirectCommand));
        m_cull_materials = CreateStaticBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, places * sizeof(std::uint32_t));
        m_cull_counts = CreateStaticBuffer(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                           static_cast<VkDeviceSize>(MAX_VIEWS) * GEOMETRY_CULL_BINS * sizeof(std::uint32_t));

        if (!m_cull_commands || !m_cull_materials || !m_cull_counts) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: cull list creation failed.");

        if (Failed(RegisterBuffer(m_cull_commands, Ir77PVSlotKind::DrawCommands, m_cull_commands_slot), result)) return result;
        if (Failed(RegisterBuffer(m_cull_materials, Ir77PVSlotKind::CommandMaterials, m_cull_materials_slot), result)) return result;

        return RegisterBuffer(m_cull_counts, Ir77PVSlotKind::DrawCounts, m_cull_counts_slot);
    }

    // ---- geometry pools ---------------------------------------------------------------------------------------------------------------------

    // A fresh range of count elements in the pool (first fit) -- ranges[pool] is that range. count 0 takes nothing.
    std::shared_ptr<IIr77Return const> ReserveInPool(Ir77PVGeometryPool const pool, std::size_t const count,
                                                     std::array<Ir77PVGeometryRange, GEOMETRY_POOL_COUNT>& ranges) {
        if (count == 0) return Ir77RETURN<Ir77OperationSucceeded>();

        std::size_t const p{PoolIndex(pool)};

        if (!m_pools[p]) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: CreateGeometry not called.");

        std::uint32_t first{0};
        if (count > PoolCapacity(pool) || !AllocateRange(m_pool_ranges[p], static_cast<std::uint32_t>(count), PoolCapacity(pool), PoolAlign(pool), first))
            return Ir77RETURN<Ir77NotConfigured>(this, std::string{"Ir77PVGeometry: the "} + PoolName(pool) + " pool has no room for " + std::to_string(count) +
                                                           " more -- raise its GEOMETRY_*_CAPACITY.");

        ranges[p] = {first, static_cast<std::uint32_t>(count)};

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // count elements of data into a fresh range of a geometry pool, as one more upload entry -- ranges[pool] is that range
    std::shared_ptr<IIr77Return const> PlaceInPool(Ir77PVGeometryPool const pool, std::size_t const count, void const* data,
                                                   std::array<Ir77PVGeometryRange, GEOMETRY_POOL_COUNT>& ranges, std::vector<Ir77PVUploadEntry>& entries) {
        std::shared_ptr<IIr77Return const> result{};
        if (Failed(ReserveInPool(pool, count, ranges), result) || count == 0) return result;

        std::size_t const p{PoolIndex(pool)};

        VkBuffer dst{VK_NULL_HANDLE};
        m_pools[p]->GetBuffer(0, &dst);

        entries.push_back(
            {dst, data, static_cast<VkDeviceSize>(count) * PoolStride(pool), static_cast<VkDeviceSize>(ranges[p].first) * PoolStride(pool)});

        return result;
    }

    // A scene's command material rows and bounding spheres (one each per command) into the command tables, at its command range (the indexed
    // command pool's) -- two more upload entries; rows / spheres must outlive the Upload.
    std::shared_ptr<IIr77Return const> PlaceCommandTables(std::array<Ir77PVGeometryRange, GEOMETRY_POOL_COUNT> const& ranges, std::vector<std::uint32_t> const& rows,
                                                          std::vector<glm::vec4> const& spheres, std::vector<Ir77PVUploadEntry>& entries) {
        Ir77PVGeometryRange const& range = ranges[PoolIndex(Ir77PVGeometryPool::Commands)];
        if (range.count == 0) return Ir77RETURN<Ir77OperationSucceeded>();

        if (!m_command_materials || !m_command_bounds) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: CreateGeometry not called.");

        if (rows.size() != range.count || spheres.size() != range.count)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: one material row and one sphere per command.");

        VkBuffer materials_dst{VK_NULL_HANDLE};
        VkBuffer bounds_dst{VK_NULL_HANDLE};
        m_command_materials->GetBuffer(0, &materials_dst);
        m_command_bounds->GetBuffer(0, &bounds_dst);

        entries.push_back({materials_dst, rows.data(), static_cast<VkDeviceSize>(range.count) * sizeof(std::uint32_t),
                           static_cast<VkDeviceSize>(range.first) * sizeof(std::uint32_t)});
        entries.push_back(
            {bounds_dst, spheres.data(), static_cast<VkDeviceSize>(range.count) * sizeof(glm::vec4), static_cast<VkDeviceSize>(range.first) * sizeof(glm::vec4)});

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The sphere around one command's vertices, in object space: the centre of their box, the radius to the farthest. Nothing to bound (or
    // the command outside the upload) -> radius -1, which the cull pass never culls.
    template <typename Vertex>
    glm::vec4 CommandSphere(std::vector<Vertex> const& vertices, std::vector<std::uint32_t> const& indices, VkDrawIndexedIndirectCommand const& cmd) const {
        if (cmd.indexCount == 0 || cmd.vertexOffset < 0 || static_cast<std::size_t>(cmd.firstIndex) + cmd.indexCount > indices.size())
            return glm::vec4(0.0f, 0.0f, 0.0f, -1.0f);

        glm::vec3 low{std::numeric_limits<float>::max()};
        glm::vec3 high{std::numeric_limits<float>::lowest()};

        auto const position = [&](std::uint32_t const i, glm::vec3& out) {
            std::size_t const v{static_cast<std::size_t>(cmd.vertexOffset) + indices[cmd.firstIndex + i]};
            if (v >= vertices.size()) return false;

            out = vertices[v].position;
            return true;
        };

        glm::vec3 p{0.0f};
        for (std::uint32_t i = 0; i < cmd.indexCount; i++) {
            if (!position(i, p)) return glm::vec4(0.0f, 0.0f, 0.0f, -1.0f);

            low = glm::min(low, p);
            high = glm::max(high, p);
        }

        glm::vec3 const center{(low + high) * 0.5f};

        float radius{0.0f};
        for (std::uint32_t i = 0; i < cmd.indexCount; i++) {
            position(i, p);
            radius = std::max(radius, glm::length(p - center));
        }

        return glm::vec4(center, radius);
    }

    // A scene's commands into its range of a command pool, in one frame's copy -- false when the write fails
    bool WriteCommands(Ir77PVGeometryPool const pool, std::array<Ir77PVGeometryRange, GEOMETRY_POOL_COUNT> const& ranges, void const* commands,
                       std::uint32_t const frame) {
        std::size_t const p{PoolIndex(pool)};
        Ir77PVGeometryRange const& range = ranges[p];

        if (range.count == 0) return true;
        if (!m_pools[p]) return false;

        return m_pools[p]->Update(frame, commands, static_cast<VkDeviceSize>(range.count) * PoolStride(pool),
                                  static_cast<VkDeviceSize>(range.first) * PoolStride(pool))
                   ->ID() == GUIDIr77OperationSucceeded;
    }

    // First fit: count elements in [0, capacity) that no range in used covers, starting on a multiple of align. used maps first -> count.
    bool AllocateRange(std::map<std::uint32_t, std::uint32_t>& used, std::uint32_t const count, std::uint32_t const capacity, std::uint32_t const align,
                       std::uint32_t& first) const {
        auto const aligned = [align](std::uint64_t const at) { return (at + align - 1) / align * align; };

        std::uint64_t cursor{0};
        for (auto const& [start, size] : used) {
            if (aligned(cursor) + count <= start) break;
            cursor = static_cast<std::uint64_t>(start) + size;
        }

        cursor = aligned(cursor);
        if (cursor + count > capacity) return false;

        first = static_cast<std::uint32_t>(cursor);
        used.emplace(first, count);

        return true;
    }

    // Ranges back to their pools now -- nothing on the GPU reads them (a failed upload). Unloads go through ReleaseGeometry.
    void FreeGeometry(std::array<Ir77PVGeometryRange, GEOMETRY_POOL_COUNT> const& ranges) {
        for (std::size_t p = 0; p < GEOMETRY_POOL_COUNT; p++)
            if (ranges[p].count > 0) m_pool_ranges[p].erase(ranges[p].first);
    }

    // What a grave runs to give a scene's ranges back -- once no frame in flight can read them
    std::function<void()> ReleaseGeometry(std::array<Ir77PVGeometryRange, GEOMETRY_POOL_COUNT> const& ranges) {
        return [owner = weak_from_this(), ranges]() {
            if (auto const geometry = owner.lock()) geometry->FreeGeometry(ranges);
        };
    }

    std::size_t PoolIndex(Ir77PVGeometryPool const pool) const { return static_cast<std::size_t>(pool); }

    // bytes per element
    VkDeviceSize PoolStride(Ir77PVGeometryPool const pool) const {
        switch (pool) {
            case Ir77PVGeometryPool::Vertices:
                return sizeof(Ir77PVVertex);
            case Ir77PVGeometryPool::SkinnedVertices:
                return sizeof(Ir77PVVertexSkinned);
            case Ir77PVGeometryPool::Commands:
                return sizeof(VkDrawIndexedIndirectCommand);
            case Ir77PVGeometryPool::MeshCommands:
                return sizeof(VkDrawMeshTasksIndirectCommandEXT);
            default:
                return sizeof(std::uint32_t);
        }
    }

    // the command pools are written from the host, one copy per frame in flight; the rest are device-local and uploaded once
    bool PoolHostWritten(Ir77PVGeometryPool const pool) const { return pool == Ir77PVGeometryPool::Commands || pool == Ir77PVGeometryPool::MeshCommands; }

    std::uint32_t PoolCapacity(Ir77PVGeometryPool const pool) const {
        switch (pool) {
            case Ir77PVGeometryPool::Vertices:
                return GEOMETRY_VERTEX_CAPACITY;
            case Ir77PVGeometryPool::Indices:
                return GEOMETRY_INDEX_CAPACITY;
            case Ir77PVGeometryPool::SkinnedVertices:
                return GEOMETRY_SKINNED_VERTEX_CAPACITY;
            case Ir77PVGeometryPool::SkinnedIndices:
                return GEOMETRY_SKINNED_INDEX_CAPACITY;
            case Ir77PVGeometryPool::Commands:
                return GEOMETRY_COMMAND_CAPACITY;
            default:
                return GEOMETRY_MESH_COMMAND_CAPACITY;
        }
    }

    // vertex ranges can be their own storage-buffer slots, so they start on GEOMETRY_VERTEX_ALIGN
    std::uint32_t PoolAlign(Ir77PVGeometryPool const pool) const {
        return pool == Ir77PVGeometryPool::Vertices || pool == Ir77PVGeometryPool::SkinnedVertices ? GEOMETRY_VERTEX_ALIGN : 1;
    }

    // static vertices / indices are also BLAS build inputs on a ray-query device; every vertex pool is a storage buffer too (compute skinning,
    // the mesh path); the command pools are storage buffers as well, so a compute pass can write them (GPU culling)
    VkBufferUsageFlags PoolUsage(Ir77PVGeometryPool const pool) const {
        switch (pool) {
            case Ir77PVGeometryPool::Vertices:
                return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | RayInputUsage();
            case Ir77PVGeometryPool::Indices:
                return VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | RayInputUsage();
            case Ir77PVGeometryPool::SkinnedVertices:
                return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            case Ir77PVGeometryPool::SkinnedIndices:
                return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            default:
                return VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        }
    }

    char const* PoolName(Ir77PVGeometryPool const pool) const {
        switch (pool) {
            case Ir77PVGeometryPool::Vertices:
                return "static vertex";
            case Ir77PVGeometryPool::Indices:
                return "static index";
            case Ir77PVGeometryPool::SkinnedVertices:
                return "skinned vertex";
            case Ir77PVGeometryPool::SkinnedIndices:
                return "skinned index";
            case Ir77PVGeometryPool::Commands:
                return "draw command";
            default:
                return "mesh command";
        }
    }

   private:
    // wiring -- Ir77PVInit
    std::shared_ptr<IIr77PVDevice> m_device{};

    std::shared_ptr<IIr77PVTransfer> m_transfer{};

    std::shared_ptr<IIr77PVMaterial> m_materials{};

    std::shared_ptr<IIr77PVPipelineCache> m_pipeline_cache{};

    // owned
    std::array<std::shared_ptr<IIr77PVBuffer>, GEOMETRY_POOL_COUNT> m_pools{};  // [Ir77PVGeometryPool] -- made on first use

    std::array<std::map<std::uint32_t, std::uint32_t>, GEOMETRY_POOL_COUNT> m_pool_ranges{};  // [pool] first -> count, every range in use

    std::array<std::uint32_t, MAX_FRAMES_IN_FLIGHT> m_command_slots{BINDLESS_NO_SLOT, BINDLESS_NO_SLOT, BINDLESS_NO_SLOT};  // the command pool, per frame copy

    std::shared_ptr<IIr77PVBuffer> m_command_materials{};  // material row per indexed command, same index as the command pool

    std::uint32_t m_command_materials_slot{BINDLESS_NO_SLOT};

    std::shared_ptr<IIr77PVBuffer> m_command_bounds{};  // object-space sphere per indexed command, same index

    std::uint32_t m_command_bounds_slot{BINDLESS_NO_SLOT};

    // GPU culling -- the job list (SetCullBins), each bin's places, and the lists the cull pass writes (per view)
    std::uint32_t m_cull_jobs_buffer{0};

    std::uint32_t m_cull_jobs_slot{BINDLESS_NO_SLOT};

    std::uint32_t m_cull_job_count{0};

    std::vector<glm::uvec2> m_cull_places{};  // [bin] first place, places -- the same in every view's lists

    std::shared_ptr<IIr77PVBuffer> m_cull_commands{};

    std::uint32_t m_cull_commands_slot{BINDLESS_NO_SLOT};

    std::shared_ptr<IIr77PVBuffer> m_cull_materials{};

    std::uint32_t m_cull_materials_slot{BINDLESS_NO_SLOT};

    std::shared_ptr<IIr77PVBuffer> m_cull_counts{};

    std::uint32_t m_cull_counts_slot{BINDLESS_NO_SLOT};

    static constexpr std::uint32_t CULL_GROUP_SIZE{64};  // local_size_x in cull.comp

    static constexpr std::uint32_t CULL_FLAG_TEST{1};  // cull.comp's job flags

    static constexpr std::uint32_t CULL_FLAG_FIRST_VIEW_ONLY{2};

    std::vector<Ir77PVSceneSlot> m_scenes{};  // scene path (vertex)

    std::vector<Ir77PVMeshletSlot> m_meshlet_scenes{};  // meshlet path
};
}  // namespace NSIr77PeregrineV