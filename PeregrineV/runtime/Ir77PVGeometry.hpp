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

#include "../types/Ir77Types.hpp"

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
//   load:      UploadBlock (vertex path) -> PlaceBlockCommands; UploadMeshlets (mesh path) -> CutMeshlets -> CreateMeshletBuffers ->
//              [PlaceMeshletRayInput] -> RegisterMeshletSlots
//   frame:     Tick(frame) -> Make*DrawItem -> Paint::SetDraws
//   runtime:   SetCommandVisible / SetCommandInstances / SetInstanceRange; SetMeshletCommandVisible / SetMeshletInstanceVisible /
//              SetMeshletInstanceRange
//   compute:   GetBlockVertexSlot (vertices as a storage buffer); rays: GetBlockGeometry (what Acceleration builds BLASes from)
//   unload:    remove the draw items from Paint, then UnloadBlock / UnloadMeshlets (freed after the frames in flight)
//   pools:     GetPoolUsage
//   culling:   SetCullBins (when the bins change) -> GetCullDispatches (Paint: every window, once per view) -> MakeCulledDrawItem per bin and view
// One device; it never sees Ir77PVInit. Every buffer is made, uploaded, slotted and released through Transfer; materials come from Materials.
//
// Blocks: an asset draws from 1 - 4 of them -- static, blend (its BLEND primitives, compacted), skinned, posed (the compute-skinned copy).
//
// Geometry pools: every block's vertices and indices go into four device buffers, all made by CreateGeometry -- static vertices, static
// indices, skinned vertices, skinned indices -- and its draw commands into two host-written ones, indexed and mesh-task, one copy per frame in
// flight (GEOMETRY_*_CAPACITY). A block takes a range of each it needs, first fit; what's left is free for whatever loads next, and an unloaded
// block's ranges come back once the frames in flight are done with them. Every draw binds the same buffers and reads the same command buffer.
// Beside the indexed command pool sits the command material table -- one material row per command, same index, device-local -- so a vertex
// shader finds any command's material from where it is in the pool: table[pc.user[5] + gl_DrawID], pc.user[4] = the table's slot. Every
// indexed draw item carries both; nothing per block. A command bounds table sits beside it the same way -- each command's object-space sphere.
//
// GPU culling (cull.comp): the caller groups vertex-path commands into bins -- one per pipeline and vertex stream -- with SetCullBins. Every
// frame, per view, the cull pass tests each command's instances against the view and compacts the survivors into the bin's list for that
// view (one command per instance, material rows beside them); MakeCulledDrawItem draws a list with vkCmdDrawIndexedIndirectCount. The cull
// pass reads this frame's copy of the command pool through the frame table (IR77_FRAME.commands), so hiding an object still works.
// Shaders that read a block's vertices by slot (compute skinning, the mesh path) get a slot over just its range, so their indices stay local.
//
// Instances: a command's instanceCount at upload is its capacity -- the models[] slots reserved for it, its places in the cull lists and its
// TLAS entries are all sized for that. What it draws is its live count, at most the capacity: SetCommandInstances per command,
// SetInstanceRange per models[] slot range (a mesh's instances, in every command that draws them). Spawning = raising the live count; the
// geometry and every list stay where they are.
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

        m_blocks.clear();
        m_meshlet_blocks.clear();
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

    // The device's pipeline cache -- engine roles and the draw item checks. After CreatePipelineCache.
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

        for (auto& block : m_blocks) {
            if (!block.alive || block.dirty == 0 || !block.indirect) continue;

            if (!WriteCommands(Ir77PVGeometryPool::Commands, block.ranges, block.commands.data(), frame))
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: block indirect write failed.");

            block.dirty--;
        }

        for (auto& slot : m_meshlet_blocks) {
            if (!slot.alive || slot.dirty == 0 || !slot.mesh.indirect) continue;

            if (!WriteCommands(Ir77PVGeometryPool::MeshCommands, slot.ranges, slot.commands.data(), frame))
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: meshlet indirect write failed.");

            slot.dirty--;
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // vertex path -- the client stacks every object of one stream (static or skinned) into one Ir77PVBlockUpload and prebuilds one
    // indirect command per object, local to the upload:
    //   firstIndex / indexCount   -> its slice of indices (indices stay local, 0-based)
    //   vertexOffset              -> where its vertices start
    //   instanceCount / firstInstance -> how many copies, and their slots in models[] (set 0 binding 2)
    // The upload lands in ranges of the stream's pools and the commands are rebased onto them (firstIndex / vertexOffset + the ranges'
    // starts), into the block's range of the command pool; command_materials (one material row per command) go to the same places in the
    // command material table. One draw item covers every command (MakeIndirectDrawItem); the vertex shader finds each command's material in
    // that table (gl_DrawID). The command pool is host-written per frame, so objects can be hidden / shown / resized per frame without
    // re-uploading geometry.
    // A pool too full for the upload fails it, naming the pool.
    // =========================================================================================================================================
    std::shared_ptr<IIr77Return const> UploadBlock(Ir77PVBlockUpload const& data, std::uint32_t& block_id, bool const& wait = true) {
        bool const skinned{!data.vertices_skinned.empty()};

        if (data.vertices.empty() && !skinned) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: empty block.");

        if (!data.command_materials.empty() && data.command_materials.size() != data.indirect_commands.size())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: command_materials needs one row per indirect command.");

        if (skinned && (!data.vertices.empty() || !data.indices.empty()))
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: a block is one stream -- static or skinned, not both.");

        std::shared_ptr<IIr77Return const> result{};

        Ir77PVGeometryPool const vertex_pool{skinned ? Ir77PVGeometryPool::SkinnedVertices : Ir77PVGeometryPool::Vertices};
        Ir77PVGeometryPool const index_pool{skinned ? Ir77PVGeometryPool::SkinnedIndices : Ir77PVGeometryPool::Indices};

        std::vector<std::uint32_t> const& indices = skinned ? data.indices_skinned : data.indices;
        std::size_t const vertex_count{skinned ? data.vertices_skinned.size() : data.vertices.size()};
        void const* const vertices = skinned ? static_cast<void const*>(data.vertices_skinned.data()) : static_cast<void const*>(data.vertices.data());

        Ir77PVBlockSlot block{};
        std::vector<Ir77PVUploadEntry> entries{};
        std::vector<std::uint32_t> rows{};  // the commands' material rows -- the upload entries point at them
        std::vector<glm::vec4> spheres{};   // the commands' bounding spheres -- the same

        // nothing reads the ranges until the upload is in, so a failure gives them straight back
        if (Failed(PlaceInPool(vertex_pool, vertex_count, vertices, block.ranges, entries), result) ||
            Failed(PlaceInPool(index_pool, indices.size(), indices.data(), block.ranges, entries), result) ||
            Failed(PlaceBlockCommands(data, skinned, block, rows, spheres, entries), result) || Failed(Upload(entries, wait), result)) {
            FreeRanges(block.ranges);
            return result;
        }

        Ir77PVGeometryRange const& vertex_range = block.ranges[PoolIndex(vertex_pool)];

        (skinned ? block.vertex_skinned : block.vertex) = m_pools[PoolIndex(vertex_pool)];
        if (block.ranges[PoolIndex(index_pool)].count > 0) (skinned ? block.index_skinned : block.index) = m_pools[PoolIndex(index_pool)];

        block.vertex_count = vertex_range.first + vertex_range.count;
        block.alive = true;

        block_id = static_cast<std::uint32_t>(m_blocks.size());
        m_blocks.push_back(std::move(block));

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Draw item for commands [first_command, first_command + command_count) of a block through an engine role
    std::shared_ptr<IIr77Return const> MakeIndirectDrawItem(std::size_t const& window, Ir77PVPipelineKind const& kind, std::uint32_t const& block_id,
                                                            std::uint32_t const& material_id, std::uint32_t const& first_command,
                                                            std::uint32_t const& command_count, bool const& skinned, Ir77PVDrawItem& item) {
        std::shared_ptr<IIr77Return const> result{};

        Ir77PVPipelineHandle pipeline{IR77PV_NO_PIPELINE};
        if (Failed(FindPipeline(kind, pipeline), result)) return result;

        return MakeIndirectDrawItem(window, pipeline, block_id, material_id, first_command, command_count, skinned, item);
    }

    // The same through any pipeline handle (FindPipeline) -- its vertex shader must read the channel `skinned` selects
    std::shared_ptr<IIr77Return const> MakeIndirectDrawItem(std::size_t const& window, Ir77PVPipelineHandle const& pipeline, std::uint32_t const& block_id,
                                                            std::uint32_t const& material_id, std::uint32_t const& first_command,
                                                            std::uint32_t const& command_count, bool const& skinned, Ir77PVDrawItem& item) {
        Ir77PVBlockSlot* const block = FindBlock(block_id);
        if (!block) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown block.");

        std::shared_ptr<IIr77PVBuffer> const& vertex = skinned ? block->vertex_skinned : block->vertex;
        std::shared_ptr<IIr77PVBuffer> const& index = skinned ? block->index_skinned : block->index;

        if (!vertex || !index || !block->indirect) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: block channel empty.");

        if (static_cast<std::size_t>(first_command) + command_count > block->commands.size())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: command range outside the block.");

        Ir77PVMaterialData material{};
        if (GetMaterial(material_id, material)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown material.");

        item = Ir77PVDrawItem{};

        std::shared_ptr<IIr77Return const> result{};
        if (Failed(ResolveItemPipeline(pipeline, false, false, item), result)) return result;

        item.material = material.row;
        item.vertex = vertex;
        item.index = index;
        item.index_type = VK_INDEX_TYPE_UINT32;
        std::uint32_t const first{block->ranges[PoolIndex(Ir77PVGeometryPool::Commands)].first + first_command};

        item.indirect = block->indirect;
        item.indirect_offset = static_cast<VkDeviceSize>(first) * sizeof(VkDrawIndexedIndirectCommand);
        item.indirect_count = command_count;

        // the vertex shaders' material lookup -- table[user[5] + gl_DrawID] (push user[4..7] = item.user)
        item.user = {m_command_materials_slot, first, 0u, 0u};

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Hide / show one object (every instance of it). Applied to each frame copy by Tick.
    std::shared_ptr<IIr77Return const> SetCommandVisible(std::uint32_t const& block_id, std::uint32_t const& command, bool const& visible) {
        Ir77PVBlockSlot* const block = FindBlock(block_id);
        if (!block || command >= block->commands.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown block object.");

        block->visible[command] = visible ? 1 : 0;
        block->commands[command].instanceCount = visible ? block->instance_counts[command] : 0;
        block->dirty = MAX_FRAMES_IN_FLIGHT;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // How many copies of one object draw -- spawn / despawn inside its capacity (its instanceCount at upload: the models[] slots, cull places
    // and TLAS entries reserved for it). A count past the capacity fails.
    std::shared_ptr<IIr77Return const> SetCommandInstances(std::uint32_t const& block_id, std::uint32_t const& command, std::uint32_t const& count) {
        Ir77PVBlockSlot* const block = FindBlock(block_id);
        if (!block || command >= block->commands.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown block object.");

        if (count > block->capacities[command])
            return Ir77RETURN<Ir77OutOfRange>(this, "Ir77PVGeometry: " + std::to_string(count) + " instances -- the object was uploaded with room for " +
                                                        std::to_string(block->capacities[command]) + ".");

        block->instance_counts[command] = count;
        if (block->visible[command]) block->commands[command].instanceCount = count;
        block->dirty = MAX_FRAMES_IN_FLIGHT;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The live instances of one models[] slot range -- slots [first_slot, first_slot + reserved) reserved, [first_slot, first_slot + live) drawn.
    // Every command of the block that starts in the range draws the live slots it covers: a command over the whole range (one primitive of a
    // mesh) draws live of them, a command per slot (the posed stream) draws 1 or 0. live past reserved fails.
    std::shared_ptr<IIr77Return const> SetInstanceRange(std::uint32_t const& block_id, std::uint32_t const& first_slot, std::uint32_t const& reserved,
                                                        std::uint32_t const& live) {
        Ir77PVBlockSlot* const block = FindBlock(block_id);
        if (!block) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown block.");
        if (live > reserved) return Ir77RETURN<Ir77OutOfRange>(this, "Ir77PVGeometry: more live instances than the range reserves.");

        std::uint64_t const live_end{static_cast<std::uint64_t>(first_slot) + live};
        std::uint64_t const range_end{static_cast<std::uint64_t>(first_slot) + reserved};

        for (std::size_t c = 0; c < block->commands.size(); c++) {
            std::uint32_t const start = block->commands[c].firstInstance;
            if (start < first_slot || start >= range_end) continue;

            std::uint32_t const count = live_end > start ? static_cast<std::uint32_t>(std::min<std::uint64_t>(live_end - start, block->capacities[c])) : 0u;

            block->instance_counts[c] = count;
            if (block->visible[c]) block->commands[c].instanceCount = count;
        }

        block->dirty = MAX_FRAMES_IN_FLIGHT;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // A block's vertices (skinned = the skinned channel) as a bindless storage buffer over just its range of the pool -- vertex 0 is its first --
    // so compute can read or write them in place with indices local to the block (compute skinning reads the skinned stream and writes the
    // posed one). Registered on the first call, the same slot after; UnloadBlock releases it.
    std::shared_ptr<IIr77Return const> GetBlockVertexSlot(std::uint32_t const& block_id, bool const& skinned, std::uint32_t& slot) {
        Ir77PVBlockSlot* const block = FindBlock(block_id);
        if (!block) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown block.");

        Ir77PVGeometryPool const pool{skinned ? Ir77PVGeometryPool::SkinnedVertices : Ir77PVGeometryPool::Vertices};

        std::shared_ptr<IIr77PVBuffer> const& vertex = skinned ? block->vertex_skinned : block->vertex;
        std::uint32_t& registered = skinned ? block->vertex_skinned_slot : block->vertex_slot;

        if (!vertex) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: block channel empty.");

        if (registered == BINDLESS_NO_SLOT) {
            Ir77PVGeometryRange const& range = block->ranges[PoolIndex(pool)];

            std::shared_ptr<IIr77Return const> result{};
            if (Failed(RegisterBuffer(vertex, Ir77PVSlotKind::Vertices, registered, static_cast<VkDeviceSize>(range.first) * PoolStride(pool),
                                      static_cast<VkDeviceSize>(range.count) * PoolStride(pool)),
                       result))
                return result;
        }

        slot = registered;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // An edit: vertices [first, first + count) of a block's vertex range (its own order) rewritten from data -- one staged copy, in submission
    // order ahead of the next frames, not waited on (a frame already in flight may still draw the old ones). The commands, their spheres and
    // the BLASes don't change here (UnboundBlock, Acceleration's refit).
    std::shared_ptr<IIr77Return const> UpdateBlockVertices(std::uint32_t const& block_id, bool const& skinned, std::uint32_t const& first,
                                                           std::uint32_t const& count, void const* data) {
        Ir77PVBlockSlot* const block = FindBlock(block_id);
        if (!block) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown block.");

        return UpdatePoolRange(skinned ? Ir77PVGeometryPool::SkinnedVertices : Ir77PVGeometryPool::Vertices, block->ranges, first, count, data);
    }

    // Its commands' bounding radius -1 -- what the cull pass never culls. Blocking; once per edited block.
    std::shared_ptr<IIr77Return const> UnboundBlock(std::uint32_t const& block_id) {
        Ir77PVBlockSlot* const block = FindBlock(block_id);
        if (!block) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown block.");

        Ir77PVGeometryRange const& range = block->ranges[PoolIndex(Ir77PVGeometryPool::Commands)];
        if (range.count == 0) return Ir77RETURN<Ir77OperationSucceeded>();

        if (!m_command_bounds) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: CreateGeometry not called.");

        std::vector<glm::vec4> const spheres(range.count, glm::vec4(0.0f, 0.0f, 0.0f, -1.0f));

        VkBuffer bounds_dst{VK_NULL_HANDLE};
        m_command_bounds->GetBuffer(0, &bounds_dst);

        return Upload({{bounds_dst, spheres.data(), static_cast<VkDeviceSize>(range.count) * sizeof(glm::vec4),
                        static_cast<VkDeviceSize>(range.first) * sizeof(glm::vec4)}},
                      true);
    }

    // The mesh path's UpdateBlockVertices -- its range of the vertex pool, the same order as the upload. The meshlets' bounds and cones stay
    // as they were cut, so the task shader may cull a part moved far from where it was.
    std::shared_ptr<IIr77Return const> UpdateMeshletVertices(std::uint32_t const& meshlet_id, std::uint32_t const& first, std::uint32_t const& count,
                                                             void const* data) {
        Ir77PVMeshletSlot* const slot = FindMeshletBlock(meshlet_id);
        if (!slot) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown meshlet block.");

        return UpdatePoolRange(Ir77PVGeometryPool::Vertices, slot->ranges, first, count, data);
    }

    // What Acceleration builds from: the block's vertex / index buffers (the pools), where its vertex range ends and its commands, rebased into
    // the pools, with every command's capacity -- one TLAS entry per reserved instance, live or not. The pools carry the build-input usage when
    // the device has ray queries.
    std::shared_ptr<IIr77Return const> GetBlockGeometry(std::uint32_t const& block_id, Ir77PVBlockGeometry& geometry) {
        Ir77PVBlockSlot* const block = FindBlock(block_id);
        if (!block) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown block.");

        geometry = Ir77PVBlockGeometry{};
        geometry.vertex = block->vertex;
        geometry.index = block->index;
        geometry.vertex_count = block->vertex_count;
        geometry.commands = block->commands;
        geometry.instance_counts = block->capacities;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Its vertex slots and its pool ranges (geometry and commands) -- released once no frame in flight can read them. Remove its draw items
    // from Paint first.
    std::shared_ptr<IIr77Return const> UnloadBlock(std::uint32_t const& block_id) {
        Ir77PVBlockSlot* const block = FindBlock(block_id);
        if (!block) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown block.");

        Bury({.slots = {{BINDLESS_BINDING_BUFFERS, block->vertex_slot}, {BINDLESS_BINDING_BUFFERS, block->vertex_skinned_slot}},
              .released = ReleaseRanges(block->ranges)});
        *block = Ir77PVBlockSlot{};

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // meshlet path -- Mesh pipeline kind only (VK_EXT_mesh_shader). Takes the same stacked Ir77PVBlockUpload as UploadBlock.
    // Each indirect command is one object's range; meshlets are cut from that range only, so none ever spans two objects -- and from one material
    // row's triangles at a time (triangle_materials), so each meshlet has exactly one material.
    // One draw record + one mesh command per instance. Static (non-skinned) vertices only, in a range of the vertex pool (its slot covers just
    // that range, so the meshlets' vertex indices stay local). On a ray-query device the block's indices go into the index pool as well --
    // GetMeshletGeometry, for the BLASes.
    // instance_count = instances written (one draw record each). Fails when the device has no mesh shading.
    // =========================================================================================================================================
    std::shared_ptr<IIr77Return const> UploadMeshlets(Ir77PVBlockUpload const& data, std::uint32_t& meshlet_id, std::uint32_t& instance_count,
                                                      bool const& wait = true) {
        if (data.vertices.empty() || data.indices.empty() || data.indirect_commands.empty())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: empty block.");

        if (!m_device) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: device not set.");

        bool mesh_shading{false};
        m_device->GetMeshShading(mesh_shading);

        if (!mesh_shading) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: mesh shading unsupported on this device.");

        if (data.triangle_materials.size() != data.indices.size() / 3)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: the mesh path needs one material row per triangle (triangle_materials).");

        std::shared_ptr<IIr77Return const> result{};

        Ir77PVMeshletBuild build{};
        if (Failed(CutMeshlets(data, build), result)) return result;

        Ir77PVMeshletSlot slot{};
        std::vector<Ir77PVUploadEntry> entries{};  // point into build and data -- both outlive the Upload

        // from here on a failure buries what was made: the buffers, any slots, the ranges -- an upload already submitted may still be reading them
        auto const fail = [&](std::shared_ptr<IIr77Return const> const& error) {
            Ir77PVMeshletMesh const& mesh = slot.mesh;
            Bury({.buffers = {mesh.meshlets, mesh.meshlet_vertices, mesh.meshlet_triangles, mesh.draws},
                  .slots = MeshletSlots(mesh),
                  .released = ReleaseRanges(slot.ranges)});
            return error;
        };

        if (Failed(CreateMeshletBuffers(build, slot.mesh, entries), result)) return fail(result);

        // the vertices -- a range of the vertex pool (also the BLAS vertex input)
        if (Failed(PlaceInPool(Ir77PVGeometryPool::Vertices, data.vertices.size(), data.vertices.data(), slot.ranges, entries), result)) return fail(result);

        slot.mesh.vertices = m_pools[PoolIndex(Ir77PVGeometryPool::Vertices)];

        if (RayInputUsage() != 0 && Failed(PlaceMeshletRayInput(data, slot, entries), result)) return fail(result);

        // the mesh commands -- one per instance, into the block's range of the mesh command pool, so instances can be hidden per frame ({0,1,1});
        // every frame copy written directly (not staged)
        if (Failed(ReserveInPool(Ir77PVGeometryPool::MeshCommands, build.commands.size(), slot.ranges), result)) return fail(result);

        slot.mesh.indirect = m_pools[PoolIndex(Ir77PVGeometryPool::MeshCommands)];

        for (std::uint32_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++)
            if (!WriteCommands(Ir77PVGeometryPool::MeshCommands, slot.ranges, build.commands.data(), frame))
                return fail(Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: meshlet indirect write failed."));

        if (Failed(Upload(entries, wait), result)) return fail(result);

        if (Failed(RegisterMeshletSlots(slot.mesh, slot.ranges[PoolIndex(Ir77PVGeometryPool::Vertices)]), result)) return fail(result);

        slot.mesh.meshlet_count = static_cast<std::uint32_t>(build.meshlets.size());
        instance_count = static_cast<std::uint32_t>(build.draws.size());

        slot.commands = std::move(build.commands);
        slot.groups = std::move(build.groups);
        slot.instance_slots = std::move(build.instance_slots);
        slot.first_instance = std::move(build.first_instance);
        slot.alive = true;

        meshlet_id = static_cast<std::uint32_t>(m_meshlet_blocks.size());
        m_meshlet_blocks.push_back(std::move(slot));

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // What Acceleration builds the mesh path's static stream from -- the same shape as GetBlockGeometry: the meshlet block's vertices, the
    // block's index buffer (kept only on ray-query devices) and its indirect commands
    std::shared_ptr<IIr77Return const> GetMeshletGeometry(std::uint32_t const& meshlet_id, Ir77PVBlockGeometry& geometry) {
        Ir77PVMeshletSlot* const slot = FindMeshletBlock(meshlet_id);
        if (!slot) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown meshlet block.");
        if (!slot->ray_index) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: meshlet block has no ray input (no ray queries).");

        geometry = Ir77PVBlockGeometry{};
        geometry.vertex = slot->mesh.vertices;
        geometry.index = slot->ray_index;
        geometry.vertex_count = slot->vertex_count;
        geometry.commands = slot->block_commands;
        geometry.instance_counts = slot->instance_counts;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // First instance of a block command -- instance = first_instance + instance offset, for SetMeshletInstanceVisible.
    std::shared_ptr<IIr77Return const> GetMeshletFirstInstance(std::uint32_t const& meshlet_id, std::uint32_t const& command, std::uint32_t& first_instance) {
        Ir77PVMeshletSlot* const slot = FindMeshletBlock(meshlet_id);
        if (!slot || static_cast<std::size_t>(command) + 1 >= slot->first_instance.size())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown meshlet object.");

        first_instance = slot->first_instance[command];

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Same as MakeIndirectDrawItem but on the mesh path: commands [first_command, first_command + command_count) of the uploaded block,
    // mapped to their instances. Every meshlet carries its own material row, so the whole block is one mesh draw.
    std::shared_ptr<IIr77Return const> MakeMeshletDrawItem(std::size_t const& window, std::uint32_t const& meshlet_id, std::uint32_t const& material_id,
                                                           std::uint32_t const& first_command, std::uint32_t const& command_count, Ir77PVDrawItem& item) {
        std::shared_ptr<IIr77Return const> result{};

        Ir77PVPipelineHandle pipeline{IR77PV_NO_PIPELINE};
        if (Failed(FindPipeline(Ir77PVPipelineKind::Mesh, pipeline), result)) return result;

        return MakeMeshletDrawItem(window, pipeline, meshlet_id, material_id, first_command, command_count, item);
    }

    std::shared_ptr<IIr77Return const> MakeMeshletDrawItem(std::size_t const& window, Ir77PVPipelineHandle const& pipeline, std::uint32_t const& meshlet_id,
                                                           std::uint32_t const& material_id, std::uint32_t const& first_command,
                                                           std::uint32_t const& command_count, Ir77PVDrawItem& item) {
        Ir77PVMeshletSlot* const slot = FindMeshletBlock(meshlet_id);
        if (!slot) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown meshlet block.");

        if (static_cast<std::size_t>(first_command) + command_count >= slot->first_instance.size())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: command range outside the meshlet block.");

        Ir77PVMaterialData material{};
        if (GetMaterial(material_id, material)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown material.");

        // the commands' instances -- indirect_offset (in the command pool) and draw_base (in the block's own draw records) both start at the
        // first, so gl_DrawID 0..n-1 lines up with them
        std::uint32_t const first = slot->first_instance[first_command];
        std::uint32_t const end = slot->first_instance[first_command + command_count];

        item = Ir77PVDrawItem{};

        std::shared_ptr<IIr77Return const> result{};
        if (Failed(ResolveItemPipeline(pipeline, true, false, item), result)) return result;

        item.material = material.row;
        item.meshlet_slots = slot->mesh.slots;
        item.meshlet_offset = 0;
        item.meshlet_count = slot->mesh.meshlet_count;
        item.indirect = slot->mesh.indirect;
        item.indirect_offset =
            static_cast<VkDeviceSize>(slot->ranges[PoolIndex(Ir77PVGeometryPool::MeshCommands)].first + first) * sizeof(VkDrawMeshTasksIndirectCommandEXT);
        item.indirect_count = end - first;
        item.draw_base = first;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Hide / show every instance of one block object on the mesh path. Applied to each frame copy by Tick.
    std::shared_ptr<IIr77Return const> SetMeshletCommandVisible(std::uint32_t const& meshlet_id, std::uint32_t const& command, bool const& visible) {
        Ir77PVMeshletSlot* const slot = FindMeshletBlock(meshlet_id);
        if (!slot || static_cast<std::size_t>(command) + 1 >= slot->first_instance.size())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown meshlet object.");

        for (std::uint32_t r = slot->first_instance[command]; r < slot->first_instance[command + 1]; r++)
            slot->commands[r].groupCountX = visible ? slot->groups[r] : 0;

        slot->dirty = MAX_FRAMES_IN_FLIGHT;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Hide / show one instance. The instance count is fixed at build time -- spawn capacity is reserved by uploading
    // extra instances and starting them hidden.
    std::shared_ptr<IIr77Return const> SetMeshletInstanceVisible(std::uint32_t const& meshlet_id, std::uint32_t const& instance, bool const& visible) {
        Ir77PVMeshletSlot* const slot = FindMeshletBlock(meshlet_id);
        if (!slot || instance >= slot->commands.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown meshlet instance.");

        slot->commands[instance].groupCountX = visible ? slot->groups[instance] : 0;
        slot->dirty = MAX_FRAMES_IN_FLIGHT;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The mesh path's SetInstanceRange -- every instance whose models[] slot is in [first_slot, first_slot + reserved) is shown when it's in
    // [first_slot, first_slot + live), hidden otherwise. Overrides SetMeshletInstanceVisible for those instances.
    std::shared_ptr<IIr77Return const> SetMeshletInstanceRange(std::uint32_t const& meshlet_id, std::uint32_t const& first_slot, std::uint32_t const& reserved,
                                                               std::uint32_t const& live) {
        Ir77PVMeshletSlot* const slot = FindMeshletBlock(meshlet_id);
        if (!slot) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown meshlet block.");
        if (live > reserved) return Ir77RETURN<Ir77OutOfRange>(this, "Ir77PVGeometry: more live instances than the range reserves.");

        std::uint64_t const live_end{static_cast<std::uint64_t>(first_slot) + live};
        std::uint64_t const range_end{static_cast<std::uint64_t>(first_slot) + reserved};

        for (std::size_t r = 0; r < slot->commands.size(); r++) {
            std::uint32_t const instance = slot->instance_slots[r];
            if (instance < first_slot || instance >= range_end) continue;

            slot->commands[r].groupCountX = instance < live_end ? slot->groups[r] : 0u;
        }

        slot->dirty = MAX_FRAMES_IN_FLIGHT;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Its meshlet buffers, their slots and its pool ranges -- released once no frame in flight can read them. Remove its draw items from Paint
    // first.
    std::shared_ptr<IIr77Return const> UnloadMeshlets(std::uint32_t const& meshlet_id) {
        Ir77PVMeshletSlot* const slot = FindMeshletBlock(meshlet_id);
        if (!slot) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: unknown meshlet block.");

        Ir77PVMeshletMesh const& mesh = slot->mesh;
        Bury({.buffers = {mesh.meshlets, mesh.meshlet_vertices, mesh.meshlet_triangles, mesh.draws},
              .slots = MeshletSlots(mesh),
              .released = ReleaseRanges(slot->ranges)});
        *slot = Ir77PVMeshletSlot{};

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // GPU culling
    // =========================================================================================================================================
    // The bins the cull pass fills -- each one's runs of commands (each frustum-tested or not) and whether only view 0 lists them. Every
    // command's capacity (whatever its live count or visibility) is a place in its bin's list per view; the bins together must fit GEOMETRY_CULL_CAPACITY,
    // and there are at most GEOMETRY_CULL_BINS. Call whenever the bins change -- the job list is remade and the old one released after the frames
    // in flight. Places are sized by capacity, so a live count changed later (SetCommandInstances / SetInstanceRange) always fits.
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

                Ir77PVBlockSlot* const block = FindBlock(run.block);
                if (!block || static_cast<std::size_t>(run.first_command) + run.command_count > block->commands.size())
                    return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: cull run outside its block.");

                std::uint32_t const base{block->ranges[PoolIndex(Ir77PVGeometryPool::Commands)].first};

                for (std::uint32_t c = run.first_command; c < run.first_command + run.command_count; c++) {
                    jobs.push_back(glm::uvec4(base + c, static_cast<std::uint32_t>(b) | (flags << 24), 0u, 0u));
                    places[b].y += block->capacities[c];  // live counts change; the places don't
                }
            }

            total += places[b].y;
            if (total > GEOMETRY_CULL_CAPACITY)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: the cull bins need more than GEOMETRY_CULL_CAPACITY places.");

            for (std::size_t j = first_job; j < jobs.size(); j++) {
                jobs[j].z = places[b].x;
                jobs[j].w = places[b].y;
            }
        }

        // the old list goes after the frames in flight; the new one in its own buffer, so no frame reads a half-written list
        if (!m_transfer) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: SetTransfer not called.");

        if (m_cull_jobs_slot != BINDLESS_NO_SLOT) m_transfer->DestroyStorageBuffer(m_cull_jobs_buffer);

        m_cull_jobs_slot = BINDLESS_NO_SLOT;
        m_cull_job_count = 0;
        m_cull_places = places;

        if (jobs.empty()) return Ir77RETURN<Ir77OperationSucceeded>();

        if (Failed(
                m_transfer->CreateStorageBuffer(static_cast<VkDeviceSize>(jobs.size() * sizeof(glm::uvec4)), jobs.data(), m_cull_jobs_buffer, m_cull_jobs_slot),
                result))
            return result;

        m_cull_job_count = static_cast<std::uint32_t>(jobs.size());

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The cull pass for one view -- zero the view's counts, then cull (nothing to cull: the zeroing alone). Recorded by every window, once per
    // view (Paint::SetViewDispatches); the command buffer fills in pc.user[7] = the view and pc.frame = its frame table. Fails without a
    // "cull.compute" pipeline in the manifest -- the caller draws unculled then.
    std::shared_ptr<IIr77Return const> GetCullDispatches(std::vector<Ir77PVDispatch>& dispatches) {
        dispatches.clear();

        if (!m_pipeline_cache) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: SetPipelineCache not called.");
        if (!m_cull_counts) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: CreateGeometry not called.");

        // a handle is issued for any name, in the manifest or not -- only resolving it says whether cull.compute exists
        std::shared_ptr<IIr77Return const> result{};

        Ir77PVPipelineHandle pipeline{IR77PV_NO_PIPELINE};
        if (Failed(m_pipeline_cache->FindPipeline(std::string{"cull.compute"}, pipeline), result)) return result;

        std::shared_ptr<IIr77PVPipelineCPT> compute{};
        if (m_pipeline_cache->GetComputePipeline(pipeline, compute)->ID() != GUIDIr77OperationSucceeded)
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
        cull.push.user = {m_cull_jobs_slot,     m_cull_job_count,      m_command_bounds_slot, m_command_materials_slot,
                          m_cull_commands_slot, m_cull_materials_slot, m_cull_counts_slot,    0u};
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

        std::shared_ptr<IIr77Return const> result{};
        if (Failed(ResolveItemPipeline(pipeline, false, false, item), result)) return result;

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

        std::shared_ptr<IIr77Return const> result{};
        if (Failed(ResolveItemPipeline(pipeline, false, true, item), result)) return result;

        item.material = material.row;
        item.index_count = vertex_count;
        item.instance_count = instance_count;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    Ir77PVBlockSlot* FindBlock(std::uint32_t const& block_id) {
        if (block_id >= m_blocks.size() || !m_blocks[block_id].alive) return nullptr;

        return &m_blocks[block_id];
    }

    Ir77PVMeshletSlot* FindMeshletBlock(std::uint32_t const& meshlet_id) {
        if (meshlet_id >= m_meshlet_blocks.size() || !m_meshlet_blocks[meshlet_id].alive) return nullptr;

        return &m_meshlet_blocks[meshlet_id];
    }

    // The handle for an engine role -- follows the manifest's "roles"
    std::shared_ptr<IIr77Return const> FindPipeline(Ir77PVPipelineKind const& kind, Ir77PVPipelineHandle& handle) {
        if (!m_pipeline_cache) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: SetPipelineCache not called.");

        return m_pipeline_cache->FindPipeline(kind, handle);
    }

    // item.pipeline = handle, once it resolves now to the right path: mesh = a task / mesh pipeline, otherwise a vertex shader; procedural =
    // one that reads no vertex inputs. The command buffer re-resolves the handle every frame, so a later manifest reload still re-points the item.
    std::shared_ptr<IIr77Return const> ResolveItemPipeline(Ir77PVPipelineHandle const& handle, bool const mesh, bool const procedural, Ir77PVDrawItem& item) {
        if (!m_pipeline_cache) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: SetPipelineCache not called.");

        std::shared_ptr<IIr77PVPipeline> pipeline{};

        if (m_pipeline_cache->GetPipeline(handle, pipeline)->ID() != GUIDIr77OperationSucceeded) {
            std::string name{};
            m_pipeline_cache->GetPipelineName(handle, name);
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: pipeline '" + name + "' not available (manifest, shaders or pass missing).");
        }

        Ir77PVPipelineDesc desc{};
        pipeline->GetDesc(&desc);

        if (desc.IsMesh() != mesh)
            return Ir77RETURN<Ir77NotConfigured>(this, mesh ? "Ir77PVGeometry: a meshlet draw needs a task / mesh pipeline."
                                                            : "Ir77PVGeometry: a task / mesh pipeline takes a meshlet block -- use MakeMeshletDrawItem.");

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

    // With ray queries on, block geometry is also an acceleration-structure build input, read by GPU address
    VkBufferUsageFlags RayInputUsage() const {
        bool ray_query{false};
        if (m_device) m_device->ImplementsRayQuery(ray_query);

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
                                                      VkDeviceSize const& offset = 0, VkDeviceSize const& range = VK_WHOLE_SIZE,
                                                      std::uint32_t const& copy = 0) {
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

    // ---- load steps -------------------------------------------------------------------------------------------------------------------------

    // UploadBlock: the commands rebased onto the block's vertex / index ranges (firstIndex / vertexOffset + their starts), into its range of the
    // command pool -- every frame copy written directly (not staged); their material rows (row 0 when none were given) and bounding spheres
    // into the command tables at the same places. rows / spheres are what those upload entries point at -- they must outlive the Upload.
    std::shared_ptr<IIr77Return const> PlaceBlockCommands(Ir77PVBlockUpload const& data, bool const skinned, Ir77PVBlockSlot& block,
                                                          std::vector<std::uint32_t>& rows, std::vector<glm::vec4>& spheres,
                                                          std::vector<Ir77PVUploadEntry>& entries) {
        if (data.indirect_commands.empty()) return Ir77RETURN<Ir77OperationSucceeded>();

        std::shared_ptr<IIr77Return const> result{};

        Ir77PVGeometryRange const vertex_range = block.ranges[PoolIndex(skinned ? Ir77PVGeometryPool::SkinnedVertices : Ir77PVGeometryPool::Vertices)];
        Ir77PVGeometryRange const index_range = block.ranges[PoolIndex(skinned ? Ir77PVGeometryPool::SkinnedIndices : Ir77PVGeometryPool::Indices)];

        block.commands = data.indirect_commands;

        for (VkDrawIndexedIndirectCommand& cmd : block.commands) {
            cmd.firstIndex += index_range.first;
            cmd.vertexOffset += static_cast<std::int32_t>(vertex_range.first);
        }

        if (Failed(ReserveInPool(Ir77PVGeometryPool::Commands, block.commands.size(), block.ranges), result)) return result;

        block.indirect = m_pools[PoolIndex(Ir77PVGeometryPool::Commands)];

        rows = data.command_materials;
        if (rows.empty()) rows.assign(block.commands.size(), 0);

        std::vector<std::uint32_t> const& indices = skinned ? data.indices_skinned : data.indices;

        spheres.reserve(data.indirect_commands.size());
        for (VkDrawIndexedIndirectCommand const& cmd : data.indirect_commands)
            spheres.push_back(skinned ? CommandSphere(data.vertices_skinned, indices, cmd) : CommandSphere(data.vertices, indices, cmd));

        if (Failed(PlaceCommandTables(block.ranges, rows, spheres, entries), result)) return result;

        for (std::uint32_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++)
            if (!WriteCommands(Ir77PVGeometryPool::Commands, block.ranges, block.commands.data(), frame))
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: block indirect write failed.");

        block.instance_counts.reserve(block.commands.size());
        for (auto const& cmd : block.commands) block.instance_counts.push_back(cmd.instanceCount);
        block.capacities = block.instance_counts;
        block.visible.assign(block.commands.size(), 1);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // What CutMeshlets makes from one block upload -- the meshlet buffers' contents and the per-instance mesh commands
    typedef struct Ir77PVMeshletBuild {
        std::vector<Ir77PVMeshlet> meshlets{};
        std::vector<std::uint32_t> meshlet_vertices{};              // local vertex index + the object's vertexOffset -- an index into the block's vertices
        std::vector<std::uint8_t> meshlet_triangles{};              // every group padded to a 4-byte multiple
        std::vector<Ir77PVMeshDraw> draws{};                        // one draw record per instance
        std::vector<VkDrawMeshTasksIndirectCommandEXT> commands{};  // one per instance, same index
        std::vector<std::uint32_t> groups{};                        // task groups per instance -- what a visible instance dispatches
        std::vector<std::uint32_t> instance_slots{};                // per instance -- its models[] slot
        std::vector<std::uint32_t> first_instance{};                // per block command, its first instance; plus an end entry
    }* pIr77PVMeshletBuild;

    // The mesh path on the CPU. Per indirect command (one object): its slice of the block, its material rows, and per row the meshlets cut from
    // that row's triangles only -- so none spans two objects or two materials -- optimized and bounded in object space; then one draw record and
    // one mesh command per instance. The object's meshlets stay contiguous.
    std::shared_ptr<IIr77Return const> CutMeshlets(Ir77PVBlockUpload const& data, Ir77PVMeshletBuild& build) {
        build = Ir77PVMeshletBuild{};
        build.first_instance.reserve(data.indirect_commands.size() + 1);

        // per-object scratch, reused
        std::vector<meshopt_Meshlet> built{};
        std::vector<std::uint32_t> local_vertices{};
        std::vector<std::uint8_t> local_triangles{};
        std::vector<std::uint32_t> rows{};           // the object's material rows, first-seen order
        std::vector<std::uint32_t> group_indices{};  // the object's triangles that use one row

        for (std::size_t c = 0; c < data.indirect_commands.size(); c++) {
            build.first_instance.push_back(static_cast<std::uint32_t>(build.draws.size()));  // every command gets an entry, even when it draws nothing

            VkDrawIndexedIndirectCommand const& cmd = data.indirect_commands[c];
            if (cmd.indexCount == 0) continue;

            // 1. this object's slice
            if (cmd.vertexOffset < 0 || static_cast<std::size_t>(cmd.firstIndex) + cmd.indexCount > data.indices.size() ||
                static_cast<std::size_t>(cmd.vertexOffset) >= data.vertices.size())
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: indirect command outside the block buffers.");

            if (cmd.firstIndex % 3 != 0 || cmd.indexCount % 3 != 0)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: indirect command does not cover whole triangles.");

            std::uint32_t const* indices = &data.indices[cmd.firstIndex];
            Ir77PVVertex const* vertices = &data.vertices[static_cast<std::size_t>(cmd.vertexOffset)];

            // 2. vertices this object uses -- indices are local, so highest + 1
            std::size_t const local_count = static_cast<std::size_t>(*std::max_element(indices, indices + cmd.indexCount)) + 1;
            if (static_cast<std::size_t>(cmd.vertexOffset) + local_count > data.vertices.size())
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: object indices run past the vertex buffer.");

            float const* positions = &vertices[0].position.x;
            std::uint32_t const first_triangle = cmd.firstIndex / 3;
            std::uint32_t const triangle_count = cmd.indexCount / 3;

            // 3. the object's material rows
            rows.clear();
            for (std::uint32_t t = 0; t < triangle_count; t++) {
                std::uint32_t const row = data.triangle_materials[first_triangle + t];
                if (std::find(rows.begin(), rows.end(), row) == rows.end()) rows.push_back(row);
            }

            std::uint32_t const meshlet_base = static_cast<std::uint32_t>(build.meshlets.size());

            for (std::uint32_t const row : rows) {
                // 4. this row's triangles
                group_indices.clear();
                for (std::uint32_t t = 0; t < triangle_count; t++) {
                    if (data.triangle_materials[first_triangle + t] == row) group_indices.insert(group_indices.end(), indices + t * 3, indices + t * 3 + 3);
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
                std::uint32_t const vertex_base = static_cast<std::uint32_t>(build.meshlet_vertices.size());
                std::uint32_t const triangle_base = static_cast<std::uint32_t>(build.meshlet_triangles.size());  // stays a 4-byte multiple

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
                    build.meshlets.push_back(out);
                }

                // 7. append -- local vertex index + vertexOffset = index into the full vertex buffer
                meshopt_Meshlet const& last = built.back();
                std::size_t const used_vertices = last.vertex_offset + last.vertex_count;
                std::size_t const used_triangles = last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3u);

                for (std::size_t v = 0; v < used_vertices; v++)
                    build.meshlet_vertices.push_back(local_vertices[v] + static_cast<std::uint32_t>(cmd.vertexOffset));

                build.meshlet_triangles.insert(build.meshlet_triangles.end(), local_triangles.begin(), local_triangles.begin() + used_triangles);
            }

            std::uint32_t const object_meshlets = static_cast<std::uint32_t>(build.meshlets.size()) - meshlet_base;
            if (object_meshlets == 0) continue;

            // 8. this object = meshlets [meshlet_base, meshlet_base + object_meshlets). One draw record per instance, same meaning as the vertex path.
            std::uint32_t const groups = (object_meshlets + MESHLET_TASK_GROUP_SIZE - 1) / MESHLET_TASK_GROUP_SIZE;

            for (std::uint32_t inst = 0; inst < cmd.instanceCount; inst++) {
                Ir77PVMeshDraw draw{};
                draw.instance = cmd.firstInstance + inst;
                draw.meshlet_offset = meshlet_base;
                draw.meshlet_count = object_meshlets;

                build.draws.push_back(draw);
                build.commands.push_back(VkDrawMeshTasksIndirectCommandEXT{groups, 1, 1});
                build.groups.push_back(groups);
                build.instance_slots.push_back(draw.instance);
            }
        }

        build.first_instance.push_back(static_cast<std::uint32_t>(build.draws.size()));  // end entry

        if (build.meshlets.empty() || build.draws.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: block produced no meshlets or draws.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The four meshlet buffers (meshlets, meshlet vertices, meshlet triangles, draw records) in mesh, and their four upload entries
    std::shared_ptr<IIr77Return const> CreateMeshletBuffers(Ir77PVMeshletBuild const& build, Ir77PVMeshletMesh& mesh, std::vector<Ir77PVUploadEntry>& entries) {
        VkDeviceSize const meshlets_size = static_cast<VkDeviceSize>(build.meshlets.size() * sizeof(Ir77PVMeshlet));
        VkDeviceSize const vertices_size = static_cast<VkDeviceSize>(build.meshlet_vertices.size() * sizeof(std::uint32_t));
        VkDeviceSize const triangles_size = static_cast<VkDeviceSize>(build.meshlet_triangles.size());
        VkDeviceSize const draws_size = static_cast<VkDeviceSize>(build.draws.size() * sizeof(Ir77PVMeshDraw));

        mesh.meshlets = CreateStaticBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, meshlets_size);
        mesh.meshlet_vertices = CreateStaticBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, vertices_size);
        mesh.meshlet_triangles = CreateStaticBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, triangles_size);
        mesh.draws = CreateStaticBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, draws_size);

        if (!mesh.meshlets || !mesh.meshlet_vertices || !mesh.meshlet_triangles || !mesh.draws)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: meshlet buffer creation failed.");

        VkBuffer meshlets_dst{VK_NULL_HANDLE};
        VkBuffer vertices_dst{VK_NULL_HANDLE};
        VkBuffer triangles_dst{VK_NULL_HANDLE};
        VkBuffer draws_dst{VK_NULL_HANDLE};
        mesh.meshlets->GetBuffer(0, &meshlets_dst);
        mesh.meshlet_vertices->GetBuffer(0, &vertices_dst);
        mesh.meshlet_triangles->GetBuffer(0, &triangles_dst);
        mesh.draws->GetBuffer(0, &draws_dst);

        entries.push_back({meshlets_dst, build.meshlets.data(), meshlets_size});
        entries.push_back({vertices_dst, build.meshlet_vertices.data(), vertices_size});
        entries.push_back({triangles_dst, build.meshlet_triangles.data(), triangles_size});
        entries.push_back({draws_dst, build.draws.data(), draws_size});

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Ray-query devices: the block's indices into the index pool as well, so Acceleration can build the static stream's BLASes (meshlets can't
    // be built from) -- and the upload's commands rebased onto the vertex and index ranges, one BLAS each (GetMeshletGeometry)
    std::shared_ptr<IIr77Return const> PlaceMeshletRayInput(Ir77PVBlockUpload const& data, Ir77PVMeshletSlot& slot, std::vector<Ir77PVUploadEntry>& entries) {
        std::shared_ptr<IIr77Return const> result{};
        if (Failed(PlaceInPool(Ir77PVGeometryPool::Indices, data.indices.size(), data.indices.data(), slot.ranges, entries), result)) return result;

        Ir77PVGeometryRange const& vertex_range = slot.ranges[PoolIndex(Ir77PVGeometryPool::Vertices)];
        Ir77PVGeometryRange const& index_range = slot.ranges[PoolIndex(Ir77PVGeometryPool::Indices)];

        slot.ray_index = m_pools[PoolIndex(Ir77PVGeometryPool::Indices)];
        slot.vertex_count = vertex_range.first + vertex_range.count;
        slot.block_commands = data.indirect_commands;

        for (VkDrawIndexedIndirectCommand& cmd : slot.block_commands) {
            cmd.firstIndex += index_range.first;
            cmd.vertexOffset += static_cast<std::int32_t>(vertex_range.first);
        }

        slot.instance_counts.reserve(data.indirect_commands.size());
        for (auto const& cmd : data.indirect_commands) slot.instance_counts.push_back(cmd.instanceCount);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The meshlet block's five bindless slots -- order matches Ir77PVMeshletMesh::slots / Ir77PVDrawItem::meshlet_slots. The vertices' slot
    // covers just the block's range of the pool, so the meshlets' vertex indices stay local.
    std::shared_ptr<IIr77Return const> RegisterMeshletSlots(Ir77PVMeshletMesh& mesh, Ir77PVGeometryRange const& vertex_range) {
        std::shared_ptr<IIr77Return const> result{};

        std::array<std::shared_ptr<IIr77PVBuffer>, MESHLET_SLOT_COUNT> const buffers{mesh.meshlets, mesh.meshlet_vertices, mesh.meshlet_triangles,
                                                                                     mesh.vertices, mesh.draws};
        std::array<Ir77PVSlotKind, MESHLET_SLOT_COUNT> const kinds{Ir77PVSlotKind::Meshlets, Ir77PVSlotKind::MeshletVertices, Ir77PVSlotKind::MeshletTriangles,
                                                                   Ir77PVSlotKind::Vertices, Ir77PVSlotKind::MeshDraws};

        for (std::size_t i = 0; i < buffers.size(); i++) {
            bool const vertices{kinds[i] == Ir77PVSlotKind::Vertices};

            VkDeviceSize const offset{vertices ? static_cast<VkDeviceSize>(vertex_range.first) * sizeof(Ir77PVVertex) : 0};
            VkDeviceSize const range{vertices ? static_cast<VkDeviceSize>(vertex_range.count) * sizeof(Ir77PVVertex) : VK_WHOLE_SIZE};

            if (Failed(RegisterBuffer(buffers[i], kinds[i], mesh.slots[i], offset, range), result)) return result;
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
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

        m_command_materials =
            CreateStaticBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, static_cast<VkDeviceSize>(GEOMETRY_COMMAND_CAPACITY) * sizeof(std::uint32_t));
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

        m_cull_commands =
            CreateStaticBuffer(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, places * sizeof(VkDrawIndexedIndirectCommand));
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

        entries.push_back({dst, data, static_cast<VkDeviceSize>(count) * PoolStride(pool), static_cast<VkDeviceSize>(ranges[p].first) * PoolStride(pool)});

        return result;
    }

    // count elements of data over [first, first + count) of a range already in the pool -- one staged copy, not waited on
    std::shared_ptr<IIr77Return const> UpdatePoolRange(Ir77PVGeometryPool const pool, std::array<Ir77PVGeometryRange, GEOMETRY_POOL_COUNT> const& ranges,
                                                       std::uint32_t const first, std::uint32_t const count, void const* data) {
        if (count == 0) return Ir77RETURN<Ir77OperationSucceeded>();
        if (!data) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVGeometry: no data.");

        std::size_t const p{PoolIndex(pool)};
        Ir77PVGeometryRange const& range = ranges[p];

        if (static_cast<std::uint64_t>(first) + count > range.count)
            return Ir77RETURN<Ir77OutOfRange>(this, std::string{"Ir77PVGeometry: "} + PoolName(pool) + " update past the block's range.");

        VkBuffer dst{VK_NULL_HANDLE};
        m_pools[p]->GetBuffer(0, &dst);

        return Upload({{dst, data, static_cast<VkDeviceSize>(count) * PoolStride(pool), static_cast<VkDeviceSize>(range.first + first) * PoolStride(pool)}}, false);
    }

    // A block's command material rows and bounding spheres (one each per command) into the command tables, at its command range (the indexed
    // command pool's) -- two more upload entries; rows / spheres must outlive the Upload.
    std::shared_ptr<IIr77Return const> PlaceCommandTables(std::array<Ir77PVGeometryRange, GEOMETRY_POOL_COUNT> const& ranges,
                                                          std::vector<std::uint32_t> const& rows, std::vector<glm::vec4> const& spheres,
                                                          std::vector<Ir77PVUploadEntry>& entries) {
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
        entries.push_back({bounds_dst, spheres.data(), static_cast<VkDeviceSize>(range.count) * sizeof(glm::vec4),
                           static_cast<VkDeviceSize>(range.first) * sizeof(glm::vec4)});

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

    // A block's commands into its range of a command pool, in one frame's copy -- false when the write fails
    bool WriteCommands(Ir77PVGeometryPool const pool, std::array<Ir77PVGeometryRange, GEOMETRY_POOL_COUNT> const& ranges, void const* commands,
                       std::uint32_t const frame) {
        std::size_t const p{PoolIndex(pool)};
        Ir77PVGeometryRange const& range = ranges[p];

        if (range.count == 0) return true;
        if (!m_pools[p]) return false;

        return m_pools[p]
                   ->Update(frame, commands, static_cast<VkDeviceSize>(range.count) * PoolStride(pool),
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

    // Ranges back to their pools now -- nothing on the GPU reads them (a failed upload). Unloads go through ReleaseRanges.
    void FreeRanges(std::array<Ir77PVGeometryRange, GEOMETRY_POOL_COUNT> const& ranges) {
        for (std::size_t p = 0; p < GEOMETRY_POOL_COUNT; p++)
            if (ranges[p].count > 0) m_pool_ranges[p].erase(ranges[p].first);
    }

    // What a grave runs to give a block's ranges back -- once no frame in flight can read them
    std::function<void()> ReleaseRanges(std::array<Ir77PVGeometryRange, GEOMETRY_POOL_COUNT> const& ranges) {
        return [owner = weak_from_this(), ranges]() {
            if (auto const geometry = owner.lock()) geometry->FreeRanges(ranges);
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
    std::array<std::shared_ptr<IIr77PVBuffer>, GEOMETRY_POOL_COUNT> m_pools{};  // [Ir77PVGeometryPool] -- all made by CreateGeometry

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

    std::vector<Ir77PVBlockSlot> m_blocks{};  // vertex path

    std::vector<Ir77PVMeshletSlot> m_meshlet_blocks{};  // meshlet path
};
}  // namespace NSIr77PeregrineV