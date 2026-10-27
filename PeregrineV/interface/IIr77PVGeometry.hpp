#pragma once

#include <vulkan/vulkan.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVDevice.hpp"
#include "IIr77PVPipelineCache.hpp"
#include "IIr77PVTransfer.hpp"
#include "IIr77PVMaterial.hpp"

#include "../types/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// Meshes on the device -- scene (vertex path) and meshlets (mesh path) -- and the draw items that reference them. Every buffer
// is made and released through Transfer; materials come from Material. Every upload takes `wait` -- true blocks, false runs ahead of
// later frames.
//   Ir77PVInit wires it: SetDevice / SetTransfer / SetMaterials -> [pipeline cache] -> SetPipelineCache -> CreateGeometry. Ir77PVInit::Tick
//   calls Tick(frame). GPU culling: SetCullBins -> GetCullDispatches (every window, once per view) -> MakeCulledDrawItem.
typedef struct IIr77PVGeometry : virtual public IIr77Enlisted {
    IIr77PVGeometry() = default;

    // ---- wiring -- Ir77PVInit sets these as each piece exists ---------------------------------------------------------------------------

    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    // Uploads, buffers, bindless buffer slots and the graveyard -- every buffer here is made and released through it
    virtual std::shared_ptr<IIr77Return const> SetTransfer(std::shared_ptr<IIr77PVTransfer> transfer) = 0;

    // Material rows for the draw items
    virtual std::shared_ptr<IIr77Return const> SetMaterials(std::shared_ptr<IIr77PVMaterial> materials) = 0;

    // The device's pipeline cache -- engine roles and the draw item checks. After CreatePipelines.
    virtual std::shared_ptr<IIr77Return const> SetPipelineCache(std::shared_ptr<IIr77PVPipelineCache> pipeline_cache) = 0;

    // Every pool, the command tables (material rows, bounding spheres), the cull lists and their slots -- after the wiring
    virtual std::shared_ptr<IIr77Return const> CreateGeometry() = 0;

    // The command pool's slot per frame in flight -- the frame table's commands field (what the cull pass reads)
    virtual std::shared_ptr<IIr77Return const> GetCommandSlots(std::array<std::uint32_t, MAX_FRAMES_IN_FLIGHT>& slots) = 0;

    // per frame -- Ir77PVInit::Tick calls this once per frame, after Transfer's Tick and before Paint records, with the frame-in-flight
    // index Paint is about to record. Writes this frame's copy of every dirty indirect buffer (visibility / instance changes) -- one frame
    // copy per Tick until every copy matches.
    virtual std::shared_ptr<IIr77Return const> Tick(std::uint32_t const& frame) = 0;

    // ---- scene path (vertex) -- one stacked upload, one indirect command per object, one draw item for the lot -------------------------------

    // scene path (vertex) -- the client stacks every object into one Ir77PVSceneUpload and prebuilds one indirect command per object:
    //   firstIndex / indexCount   -> its slice of indices (indices stay local, 0-based)
    //   vertexOffset              -> where its vertices start
    //   instanceCount / firstInstance -> how many copies, and their slots in models[] (set 0 binding 2)
    // One stream per upload (static or skinned). It lands in ranges of the geometry pools, the commands rebased onto them and kept in its range
    // of the command pool, their material rows (command_materials) at the same places in the command material table -- every scene draws from
    // the same buffers; a pool too full for it fails the upload. One draw item covers every command (MakeIndirectDrawItem) and carries the
    // table: the vertex shader reads table[pc.user[5] + gl_DrawID]. The command pool is host-written per frame, so objects can be hidden /
    // shown / resized per frame without re-uploading geometry.
    virtual std::shared_ptr<IIr77Return const> UploadScene(Ir77PVSceneUpload const& data, std::uint32_t& scene_id, bool const& wait = true) = 0;

    // Draw item for commands [first_command, first_command + command_count) of a scene through an engine role
    virtual std::shared_ptr<IIr77Return const> MakeIndirectDrawItem(std::size_t const& window, Ir77PVPipelineKind const& kind, std::uint32_t const& scene_id,
                                                                    std::uint32_t const& material_id, std::uint32_t const& first_command,
                                                                    std::uint32_t const& command_count, bool const& skinned, Ir77PVDrawItem& item) = 0;

    // The same through any pipeline handle (FindPipeline) -- its vertex shader must read the channel `skinned` selects
    virtual std::shared_ptr<IIr77Return const> MakeIndirectDrawItem(std::size_t const& window, Ir77PVPipelineHandle const& pipeline,
                                                                    std::uint32_t const& scene_id, std::uint32_t const& material_id,
                                                                    std::uint32_t const& first_command, std::uint32_t const& command_count, bool const& skinned,
                                                                    Ir77PVDrawItem& item) = 0;

    // Hide / show one object (every instance of it). Applied to each frame copy by Tick.
    virtual std::shared_ptr<IIr77Return const> SetSceneObjectVisible(std::uint32_t const& scene_id, std::uint32_t const& command, bool const& visible) = 0;

    // Change how many copies of an object draw -- spawn / despawn inside the models[] slots the client reserved for it
    // ([firstInstance, firstInstance + count) must not run into the next object's slots).
    virtual std::shared_ptr<IIr77Return const> SetSceneObjectInstances(std::uint32_t const& scene_id, std::uint32_t const& command,
                                                                       std::uint32_t const& count) = 0;

    // A scene's vertices (skinned = the skinned channel) as a bindless storage buffer over just its pool range -- vertex 0 is its first, so
    // compute reads / writes them in place with scene-local indices. Registered on the first call, the same slot after; UnloadScene releases it.
    virtual std::shared_ptr<IIr77Return const> GetSceneVertexSlot(std::uint32_t const& scene_id, bool const& skinned, std::uint32_t& slot) = 0;

    // What Acceleration builds from: the pools, where the scene's vertex range ends and its commands, rebased into the pools
    virtual std::shared_ptr<IIr77Return const> GetSceneGeometry(std::uint32_t const& scene_id, Ir77PVSceneGeometry& geometry) = 0;

    // Its vertex slots and pool ranges (geometry and commands) -- released once no frame in flight can read them. Remove its draw items from Paint first.
    virtual std::shared_ptr<IIr77Return const> UnloadScene(std::uint32_t const& scene_id) = 0;

    // ---- meshlet path -- Mesh pipeline kind only (VK_EXT_mesh_shader) ----------------------------------------------------------------------

    // meshlet path -- Mesh pipeline kind only (VK_EXT_mesh_shader). Takes the same stacked Ir77PVSceneUpload as UploadScene.
    // Each indirect command is one object's range; meshlets are cut from that range only, so none ever spans two objects -- and from one
    // material row's triangles at a time (triangle_materials), so each meshlet has exactly one material.
    // One draw record + one mesh command per instance. Static (non-skinned) vertices only.
    // record_count = records written. Fails when the device has no mesh shading.
    virtual std::shared_ptr<IIr77Return const> UploadMeshlets(Ir77PVSceneUpload const& scene, std::uint32_t& meshlet_id, std::uint32_t& record_count,
                                                              bool const& wait = true) = 0;

    // What Acceleration builds the mesh path's static stream from: the vertices, the scene's index buffer (ray-query devices only) and its
    // commands -- the same shape as GetSceneGeometry
    virtual std::shared_ptr<IIr77Return const> GetMeshletGeometry(std::uint32_t const& meshlet_id, Ir77PVSceneGeometry& geometry) = 0;

    // First record of a scene command -- record = first_record + instance offset, for SetMeshletRecordVisible.
    virtual std::shared_ptr<IIr77Return const> GetMeshletFirstRecord(std::uint32_t const& meshlet_id, std::uint32_t const& command,
                                                                     std::uint32_t& first_record) = 0;

    // Same as MakeIndirectDrawItem but on the mesh path: commands [first_command, first_command + command_count) of the uploaded scene,
    // mapped to their records. Every meshlet carries its own material row, so the whole scene is one mesh draw.
    virtual std::shared_ptr<IIr77Return const> MakeMeshletSceneDrawItem(std::size_t const& window, std::uint32_t const& meshlet_id,
                                                                        std::uint32_t const& material_id, std::uint32_t const& first_command,
                                                                        std::uint32_t const& command_count, Ir77PVDrawItem& item) = 0;

    virtual std::shared_ptr<IIr77Return const> MakeMeshletSceneDrawItem(std::size_t const& window, Ir77PVPipelineHandle const& pipeline,
                                                                        std::uint32_t const& meshlet_id, std::uint32_t const& material_id,
                                                                        std::uint32_t const& first_command, std::uint32_t const& command_count,
                                                                        Ir77PVDrawItem& item) = 0;

    // Hide / show every instance of one scene object on the mesh path. Applied to each frame copy by Tick.
    virtual std::shared_ptr<IIr77Return const> SetMeshletObjectVisible(std::uint32_t const& meshlet_id, std::uint32_t const& command, bool const& visible) = 0;

    // Hide / show one instance (record). The record count is fixed at build time -- spawn capacity is reserved by uploading
    // extra instances and starting them hidden.
    virtual std::shared_ptr<IIr77Return const> SetMeshletRecordVisible(std::uint32_t const& meshlet_id, std::uint32_t const& record, bool const& visible) = 0;

    // Its meshlet buffers, their slots and its pool ranges -- released once no frame in flight can read them. Remove its draw items from Paint
    // first.
    virtual std::shared_ptr<IIr77Return const> UnloadMeshlets(std::uint32_t const& meshlet_id) = 0;

    // ---- geometry pools ---------------------------------------------------------------------------------------------------------------------

    // How much of a pool is in use and how much it holds, in elements (vertices, indices or commands)
    virtual std::shared_ptr<IIr77Return const> GetPoolUsage(Ir77PVGeometryPool const& pool, std::uint32_t& used, std::uint32_t& capacity) = 0;

    // ---- GPU culling (cull.comp) ------------------------------------------------------------------------------------------------------------

    // The bins the cull pass fills -- runs of vertex-path commands, one bin per pipeline and stream. Whenever they change; the old job list is
    // released after the frames in flight. Every bin's instances together must fit GEOMETRY_CULL_CAPACITY; at most GEOMETRY_CULL_BINS bins.
    virtual std::shared_ptr<IIr77Return const> SetCullBins(std::vector<Ir77PVCullBin> const& bins) = 0;

    // Zero the view's counts, then cull -- every window records these once per view. Fails without a "cull.compute" pipeline.
    virtual std::shared_ptr<IIr77Return const> GetCullDispatches(std::vector<Ir77PVDispatch>& dispatches) = 0;

    // One bin's culled list for one view through pipeline -- vkCmdDrawIndexedIndirectCount from the pool buffers (skinned = the skinned pair)
    virtual std::shared_ptr<IIr77Return const> MakeCulledDrawItem(Ir77PVPipelineHandle const& pipeline, std::uint32_t const& bin, std::uint32_t const& view,
                                                                  bool const& skinned, Ir77PVDrawItem& item) = 0;

    // ---- procedural -- no vertex buffer ---------------------------------------------------------------------------------------------------------

    // Draw item with no vertex buffer: vertex_count vertices (times instance_count) that the vertex shader builds from gl_VertexIndex /
    // gl_InstanceIndex and storage buffers. The pipeline's vertex shader must read no inputs. Caller fills model / user; for a GPU-driven
    // count set item.indirect (a storage buffer holding VkDrawIndirectCommands) and item.indirect_count.
    virtual std::shared_ptr<IIr77Return const> MakeProceduralDrawItem(std::size_t const& window, Ir77PVPipelineHandle const& pipeline,
                                                                      std::uint32_t const& material_id, std::uint32_t const& vertex_count,
                                                                      std::uint32_t const& instance_count, Ir77PVDrawItem& item) = 0;

    virtual ~IIr77PVGeometry() = default;
}* pIIr77PVGeometry;
}  // namespace NSIr77PeregrineV