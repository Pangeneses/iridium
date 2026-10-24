#pragma once

#include <cstdint>
#include <cstdio>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <SDL3/SDL_video.h>

#include <simdjson.h>

#include "../../Ir77RT/types//Ir77Types.hpp"

#include "../../Ir77RT/runtime/Ir77Return.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"

#include "../interface/IIr77PVInit.hpp"
#include "../interface/IIr77PVCEF.hpp"
#include "../interface/IIr77PVPaint.hpp"
#include "../interface/IIr77PVShader.hpp"
#include "../interface/IIr77PVTransfer.hpp"
#include "../interface/IIr77PVMaterial.hpp"
#include "../interface/IIr77PVGeometry.hpp"
#include "../interface/IIr77PVOverlay.hpp"

#include "../runtime/Ir77PVInit.hpp"

#include "../types/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// =============================================================================================================================================
// Ir77Server -- the renderer module's switchboard. Owns Ir77PVInit, the lifetime controller; Init owns and wires CEF, Paint, Transfer,
// Materials and Geometry, and holds the shader registry and pipeline cache.
// Nothing below holds this. Everything past Init is reached through its interface. The module's one door:
//
//   pipe:        Sink(json) in, the post function out (SetPost) -- setup and teardown are ops
//   direct link: the windows (SetWindows), the frame loop (Tick / Draw) and the forwarding methods other modules call -- uploads, draw items,
//                frame data, CEF paints. The peers never leave this module.
//
//   setup:     SetWindows -> renderer.create -> [other modules hold this server, load content]
//              -> renderer.command_buffers ... renderer.destroy (before SDL destroys the windows)
//   per frame: Tick -> [scene hands its frame to Paint] -> Draw
//
// Message in:  {"id":3, "from":"ui", "to":"peregrinev", "op":"device.info"}
// Reply out:   {"id":3, "from":"peregrinev", "to":"ui", "op":"device.info", "ok":true, "error":"", "data":{"gpu":"..."}}
//
// ops:
//   renderer.create           { shader_root } -> data { device_id }
//   renderer.command_buffers
//   renderer.destroy
//   device.info               -> data { gpu }
// =============================================================================================================================================
class Ir77Server : public Ir77Enlisted, public std::enable_shared_from_this<Ir77Server> {
   public:
    Ir77Server() {
        m_init = std::static_pointer_cast<IIr77PVInit>(std::make_shared<Ir77PVInit>());

        if (m_empty_parser.parse(std::string_view{"{}"}).get(m_empty_args) != simdjson::SUCCESS)  // what a handler sees when a message has no "args"
            throw std::runtime_error{"Ir77Server: empty args"};

        DefineOperations();
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Server>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77PeregrineV>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77Server)
            obj = std::shared_ptr<Ir77Server>(shared_from_this(), static_cast<Ir77Server*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // direct links -- setup
    // -------------------------------------------------------------------------------------------------------------------------------------

    // The windows the display device presents to -- before renderer.create
   public:
    std::shared_ptr<IIr77Return const> SetWindows(std::vector<SDL_Window*> const& windows) { return m_init->SetWindows(windows); }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // forwarding -- the only way into the renderer for other modules. The peers stay inside; each call finds its peer through
    // Init, so a peer that isn't created yet (or was destroyed) answers NotConfigured instead of crashing.
    // -------------------------------------------------------------------------------------------------------------------------------------

    // ---- overlay -- the windows module's CEF paints ----------------------------------------------------------------------------------------

    // One CEF paint (BGRA, tightly packed) into window `window`'s overlay. Skipped by the overlay while it is resizing.
    std::shared_ptr<IIr77Return const> UploadOverlay(std::uint32_t const& window, void const* buffer, int const& width, int const& height) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVOverlay> overlay{};
        if (Failed(DefineOverlay(window, overlay), result)) return result;

        return overlay->UploadFrame(buffer, width, height);
    }

    // Window `window`'s overlay is mid-rebuild -- a paint now would be lost
    std::shared_ptr<IIr77Return const> IsOverlayResizing(std::uint32_t const& window, bool& resizing) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVOverlay> overlay{};
        if (Failed(DefineOverlay(window, overlay), result)) return result;

        return overlay->IsResizing(resizing);
    }

    // ---- shaders, pipelines, device -- Init and the shader registry ---------------------------------------------------------------------------------

    // GLSL text -> SPIR-V, through the shader compiler (glslangValidator, on PATH). The text is written to <shader root>/generated/<name> and
    // compiled with the shader root on the include path, so it can #include "include/bindless.glsl". name is the registry name to be, so it
    // ends in its stage ("field.comp"). errors = the compiler's ERROR / WARNING lines, with the file path cut down to the name.
    // Blocking (a compiler process) -- for hot compiles between frames, not every frame.
    std::shared_ptr<IIr77Return const> CompileShader(std::string const& name, std::string const& text, std::vector<std::uint32_t>& spirv, std::string& errors) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVShader> shader{};
        if (Failed(m_init->GetShader(shader), result)) return result;

        return shader->CompileShader(name, text, spirv, errors);
    }

    // Puts SPIR-V into the registry under name -- replacing the shader of that name (same handle) or adding it -- then rebuilds every pipeline
    // that used it and reloads the manifest, so pipelines naming a shader that wasn't registered before resolve now. Waits for the device to
    // go idle first: the old pipelines may still be recording. A shader that breaks the contract fails and changes nothing.
    std::shared_ptr<IIr77Return const> ReloadShader(std::string const& name, std::vector<std::uint32_t> const& spirv) {
        return m_init->ReloadShader(name, spirv);
    }

    // The handle a draw item carries for a manifest pipeline ("water") -- look it up once, reuse it for every item. Needs CreatePipelines.
    std::shared_ptr<IIr77Return const> FindPipeline(std::string const& name, Ir77PVPipelineHandle& handle) { return m_init->FindPipeline(name, handle); }

    // The handle for an engine role -- follows the manifest's "roles"
    std::shared_ptr<IIr77Return const> FindPipeline(Ir77PVPipelineKind const& kind, Ir77PVPipelineHandle& handle) { return m_init->FindPipeline(kind, handle); }

    // Name of the GPU this context renders on -- for stats / logs
    std::shared_ptr<IIr77Return const> GetDeviceName(std::string& name) { return m_init->GetDeviceName(name); }

    // ---- transfer -- storage buffers ----------------------------------------------------------------------------------------------------------------

    // size bytes, filled from data (zeros when data is null). Blocking upload. buffer_id for Get / Upload / Destroy; slot for the shaders.
    std::shared_ptr<IIr77Return const> CreateStorageBuffer(VkDeviceSize const& size, void const* data, std::uint32_t& buffer_id, std::uint32_t& slot) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVTransfer> transfer{};
        if (Failed(m_init->GetTransfer(transfer), result)) return result;

        return transfer->CreateStorageBuffer(size, data, buffer_id, slot);
    }

    // The buffer itself -- for item.indirect / dispatch.indirect
    std::shared_ptr<IIr77Return const> GetStorageBuffer(std::uint32_t const& buffer_id, std::shared_ptr<IIr77PVBuffer>& buffer) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVTransfer> transfer{};
        if (Failed(m_init->GetTransfer(transfer), result)) return result;

        return transfer->GetStorageBuffer(buffer_id, buffer);
    }

    // CPU rewrite of [offset, offset + size) -- resets, spawn parameters. The copy runs in submission order ahead of later frames.
    std::shared_ptr<IIr77Return const> UploadStorageBuffer(std::uint32_t const& buffer_id, void const* data, VkDeviceSize const& size,
                                                           VkDeviceSize const& offset, bool const& wait) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVTransfer> transfer{};
        if (Failed(m_init->GetTransfer(transfer), result)) return result;

        return transfer->UploadStorageBuffer(buffer_id, data, size, offset, wait);
    }

    // Releases the slot and frees the buffer once no frame in flight can read it. Remove the draws / dispatches using it first.
    std::shared_ptr<IIr77Return const> DestroyStorageBuffer(std::uint32_t const& buffer_id) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVTransfer> transfer{};
        if (Failed(m_init->GetTransfer(transfer), result)) return result;

        return transfer->DestroyStorageBuffer(buffer_id);
    }

    // ---- geometry -- single mesh, scene, meshlets ---------------------------------------------------------------------------------------------------

    // single-mesh path -- one Static vertex + index buffer pair per mesh. procedural / debug geometry, one-off loads.
    // One code-built mesh (gizmo, debug shape, procedural geometry) -> its own GPU vertex + index buffers. single_mesh_id names it for
    // GetSingleMesh / MakeDrawItem / UnloadSingleMesh. wait = true blocks until the copy is done; false lets it run ahead of later frames.
    std::shared_ptr<IIr77Return const> UploadSingleMesh(Ir77PVSingleMesh const& single_mesh, std::uint32_t& single_mesh_id, bool const& wait = true) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVGeometry> geometry{};
        if (Failed(m_init->GetGeometry(geometry), result)) return result;

        return geometry->UploadSingleMesh(single_mesh, single_mesh_id, wait);
    }

    std::shared_ptr<IIr77Return const> GetSingleMeshBuffer(std::uint32_t const& mesh_id, Ir77PVSingleMeshBuffer& mesh) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVGeometry> geometry{};
        if (Failed(m_init->GetGeometry(geometry), result)) return result;

        return geometry->GetSingleMeshBuffer(mesh_id, mesh);
    }

    // The id stays reserved; GetMesh fails for it afterwards. Remove its draw items from Paint first.
    std::shared_ptr<IIr77Return const> UnloadSingleMeshBuffer(std::uint32_t const& mesh_id) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVGeometry> geometry{};
        if (Failed(m_init->GetGeometry(geometry), result)) return result;

        return geometry->UnloadSingleMeshBuffer(mesh_id);
    }

    // scene path (vertex) -- the client stacks every object into one Ir77PVSceneUpload and prebuilds one indirect command per object:
    //   firstIndex / indexCount   -> its slice of indices (indices stay local, 0-based)
    //   vertexOffset              -> where its vertices start
    //   instanceCount / firstInstance -> how many copies, and their slots in models[] (set 0 binding 2)
    // Sort objects by material so each material is one contiguous run of commands; one draw item per run (MakeIndirectDrawItem).
    // The indirect buffer is Dynamic, so objects can be hidden / shown / resized per frame without re-uploading geometry.
    std::shared_ptr<IIr77Return const> UploadScene(Ir77PVSceneUpload const& data, std::uint32_t& scene_id, bool const& wait = true) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVGeometry> geometry{};
        if (Failed(m_init->GetGeometry(geometry), result)) return result;

        return geometry->UploadScene(data, scene_id, wait);
    }

    // Draw item for commands [first_command, first_command + command_count) of a scene -- one material run -- through an engine role
    std::shared_ptr<IIr77Return const> MakeIndirectDrawItem(std::size_t const& window, Ir77PVPipelineKind const& kind, std::uint32_t const& scene_id,
                                                            std::uint32_t const& material_id, std::uint32_t const& first_command,
                                                            std::uint32_t const& command_count, bool const& skinned, Ir77PVDrawItem& item) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVGeometry> geometry{};
        if (Failed(m_init->GetGeometry(geometry), result)) return result;

        return geometry->MakeIndirectDrawItem(window, kind, scene_id, material_id, first_command, command_count, skinned, item);
    }

    // The same through any pipeline handle (FindPipeline) -- its vertex shader must read the channel `skinned` selects
    std::shared_ptr<IIr77Return const> MakeIndirectDrawItem(std::size_t const& window, Ir77PVPipelineHandle const& pipeline, std::uint32_t const& scene_id,
                                                            std::uint32_t const& material_id, std::uint32_t const& first_command,
                                                            std::uint32_t const& command_count, bool const& skinned, Ir77PVDrawItem& item) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVGeometry> geometry{};
        if (Failed(m_init->GetGeometry(geometry), result)) return result;

        return geometry->MakeIndirectDrawItem(window, pipeline, scene_id, material_id, first_command, command_count, skinned, item);
    }

    // Hide / show one object (every instance of it). Applied to each frame copy by Tick.
    std::shared_ptr<IIr77Return const> SetSceneObjectVisible(std::uint32_t const& scene_id, std::uint32_t const& command, bool const& visible) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVGeometry> geometry{};
        if (Failed(m_init->GetGeometry(geometry), result)) return result;

        return geometry->SetSceneObjectVisible(scene_id, command, visible);
    }

    // Change how many copies of an object draw -- spawn / despawn inside the models[] slots the client reserved for it
    // ([firstInstance, firstInstance + count) must not run into the next object's slots).
    std::shared_ptr<IIr77Return const> SetSceneObjectInstances(std::uint32_t const& scene_id, std::uint32_t const& command, std::uint32_t const& count) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVGeometry> geometry{};
        if (Failed(m_init->GetGeometry(geometry), result)) return result;

        return geometry->SetSceneObjectInstances(scene_id, command, count);
    }

    // Frees the scene's buffers once no frame in flight can read them. Remove its draw items from Paint first.
    std::shared_ptr<IIr77Return const> UnloadScene(std::uint32_t const& scene_id) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVGeometry> geometry{};
        if (Failed(m_init->GetGeometry(geometry), result)) return result;

        return geometry->UnloadScene(scene_id);
    }

    // meshlet path -- Mesh pipeline kind only (VK_EXT_mesh_shader). Takes the same stacked Ir77PVSceneUpload as UploadScene.
    // Each indirect command is one object's range; meshlets are cut from that range only, so none ever spans two objects.
    // One draw record + one mesh command per instance. Static (non-skinned) vertices only.
    // record_count = records written. Fails when the device has no mesh shading.
    std::shared_ptr<IIr77Return const> UploadMeshlets(Ir77PVSceneUpload const& scene, std::uint32_t& meshlet_id, std::uint32_t& record_count,
                                                      bool const& wait = true) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVGeometry> geometry{};
        if (Failed(m_init->GetGeometry(geometry), result)) return result;

        return geometry->UploadMeshlets(scene, meshlet_id, record_count, wait);
    }

    std::shared_ptr<IIr77Return const> GetMeshlets(std::uint32_t const& meshlet_id, Ir77PVMeshletMesh& mesh) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVGeometry> geometry{};
        if (Failed(m_init->GetGeometry(geometry), result)) return result;

        return geometry->GetMeshlets(meshlet_id, mesh);
    }

    // Direct (non-indirect) draw of meshlets [0, meshlet_count) -- single-object meshlet meshes only -- through the mesh role
    std::shared_ptr<IIr77Return const> MakeMeshletDrawItem(std::size_t const& window, std::uint32_t const& meshlet_id, std::uint32_t const& material_id,
                                                           Ir77PVDrawItem& item) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVGeometry> geometry{};
        if (Failed(m_init->GetGeometry(geometry), result)) return result;

        return geometry->MakeMeshletDrawItem(window, meshlet_id, material_id, item);
    }

    // The same through any task / mesh pipeline handle
    std::shared_ptr<IIr77Return const> MakeMeshletDrawItem(std::size_t const& window, Ir77PVPipelineHandle const& pipeline, std::uint32_t const& meshlet_id,
                                                           std::uint32_t const& material_id, Ir77PVDrawItem& item) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVGeometry> geometry{};
        if (Failed(m_init->GetGeometry(geometry), result)) return result;

        return geometry->MakeMeshletDrawItem(window, pipeline, meshlet_id, material_id, item);
    }

    // One indirect draw for records [first_record, first_record + record_count).
    // indirect_offset and draw_base both start at first_record, so gl_DrawID 0..record_count-1 lines up with the records.
    std::shared_ptr<IIr77Return const> MakeMeshletIndirectDrawItem(std::size_t const& window, std::uint32_t const& meshlet_id, std::uint32_t const& material_id,
                                                                   std::uint32_t const& first_record, std::uint32_t const& record_count, Ir77PVDrawItem& item) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVGeometry> geometry{};
        if (Failed(m_init->GetGeometry(geometry), result)) return result;

        return geometry->MakeMeshletIndirectDrawItem(window, meshlet_id, material_id, first_record, record_count, item);
    }

    std::shared_ptr<IIr77Return const> MakeMeshletIndirectDrawItem(std::size_t const& window, Ir77PVPipelineHandle const& pipeline,
                                                                   std::uint32_t const& meshlet_id, std::uint32_t const& material_id,
                                                                   std::uint32_t const& first_record, std::uint32_t const& record_count, Ir77PVDrawItem& item) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVGeometry> geometry{};
        if (Failed(m_init->GetGeometry(geometry), result)) return result;

        return geometry->MakeMeshletIndirectDrawItem(window, pipeline, meshlet_id, material_id, first_record, record_count, item);
    }

    // Same as MakeIndirectDrawItem but on the mesh path: commands [first_command, first_command + command_count) of the uploaded scene,
    // mapped to their records. This is how a material run becomes one mesh draw.
    std::shared_ptr<IIr77Return const> MakeMeshletSceneDrawItem(std::size_t const& window, std::uint32_t const& meshlet_id, std::uint32_t const& material_id,
                                                                std::uint32_t const& first_command, std::uint32_t const& command_count, Ir77PVDrawItem& item) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVGeometry> geometry{};
        if (Failed(m_init->GetGeometry(geometry), result)) return result;

        return geometry->MakeMeshletSceneDrawItem(window, meshlet_id, material_id, first_command, command_count, item);
    }

    std::shared_ptr<IIr77Return const> MakeMeshletSceneDrawItem(std::size_t const& window, Ir77PVPipelineHandle const& pipeline,
                                                                std::uint32_t const& meshlet_id, std::uint32_t const& material_id,
                                                                std::uint32_t const& first_command, std::uint32_t const& command_count, Ir77PVDrawItem& item) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVGeometry> geometry{};
        if (Failed(m_init->GetGeometry(geometry), result)) return result;

        return geometry->MakeMeshletSceneDrawItem(window, pipeline, meshlet_id, material_id, first_command, command_count, item);
    }

    // Hide / show every instance of one scene object on the mesh path. Applied to each frame copy by Tick.
    std::shared_ptr<IIr77Return const> SetMeshletObjectVisible(std::uint32_t const& meshlet_id, std::uint32_t const& command, bool const& visible) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVGeometry> geometry{};
        if (Failed(m_init->GetGeometry(geometry), result)) return result;

        return geometry->SetMeshletObjectVisible(meshlet_id, command, visible);
    }

    // Hide / show one instance (record). The record count is fixed at build time -- spawn capacity is reserved by uploading
    // extra instances and starting them hidden.
    std::shared_ptr<IIr77Return const> SetMeshletRecordVisible(std::uint32_t const& meshlet_id, std::uint32_t const& record, bool const& visible) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVGeometry> geometry{};
        if (Failed(m_init->GetGeometry(geometry), result)) return result;

        return geometry->SetMeshletRecordVisible(meshlet_id, record, visible);
    }

    // First record of a scene command -- record = first_record + instance offset, for SetMeshletRecordVisible.
    std::shared_ptr<IIr77Return const> GetMeshletFirstRecord(std::uint32_t const& meshlet_id, std::uint32_t const& command, std::uint32_t& first_record) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVGeometry> geometry{};
        if (Failed(m_init->GetGeometry(geometry), result)) return result;

        return geometry->GetMeshletFirstRecord(meshlet_id, command, first_record);
    }

    // Releases the meshlet buffer slots and frees the buffers once no frame in flight can read them. Remove its draw items from Paint first.
    std::shared_ptr<IIr77Return const> UnloadMeshlets(std::uint32_t const& meshlet_id) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVGeometry> geometry{};
        if (Failed(m_init->GetGeometry(geometry), result)) return result;

        return geometry->UnloadMeshlets(meshlet_id);
    }

    // ---- materials -- textures, materials -----------------------------------------------------------------------------------------------------------

    // textures -- built on Ir77PVTexture (the same class as the placeholders), so they get its kinds, mip chain and sampler handling.
    // srgb = true for color data (albedo, emissive) -> Color kind (sRGB); false for data maps (normal, ORM, AO) -> Data kind (UNORM).
    // Blocking: Ir77PVTexture uploads and waits on its own. Registered in the bindless table -- GetTextureSlot gives its slot.
    std::shared_ptr<IIr77Return const> CreateTexture(std::uint8_t const* rgba, std::uint32_t const& width, std::uint32_t const& height, bool const& srgb,
                                                     std::uint32_t& texture_id) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVMaterial> materials{};
        if (Failed(m_init->GetMaterials(materials), result)) return result;

        return materials->CreateTexture(rgba, width, height, srgb, texture_id);
    }

    // A canvas -- size x size RGBA8, cleared to transparent, that compute writes (storage image slot) and fragment shaders sample (texture
    // slot). Stays in VK_IMAGE_LAYOUT_GENERAL for both; the Compute pass's barriers order the writes against the reads.
    std::shared_ptr<IIr77Return const> CreateCanvas(std::uint32_t const& size, std::uint32_t& texture_slot, std::uint32_t& image_slot) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVMaterial> materials{};
        if (Failed(m_init->GetMaterials(materials), result)) return result;

        return materials->CreateCanvas(size, texture_slot, image_slot);
    }

    // Bindless texture slot of a texture -- what material rows and shaders use
    std::shared_ptr<IIr77Return const> GetTextureSlot(std::uint32_t const& texture_id, std::uint32_t& slot) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVMaterial> materials{};
        if (Failed(m_init->GetMaterials(materials), result)) return result;

        return materials->GetTextureSlot(texture_id, slot);
    }

    // materials -- rows in the device's material table: constants + 5 texture slots (placeholders where no texture is given)
    std::shared_ptr<IIr77Return const> CreateMaterial(Ir77PVMaterialConstants const& constants, std::uint32_t& material_id) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVMaterial> materials{};
        if (Failed(m_init->GetMaterials(materials), result)) return result;

        return materials->CreateMaterial(constants, material_id);
    }

    std::shared_ptr<IIr77Return const> CreateMaterial(Ir77PVMaterialConstants const& constants, Ir77PVMaterialTextures const& textures,
                                                      std::uint32_t& material_id) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVMaterial> materials{};
        if (Failed(m_init->GetMaterials(materials), result)) return result;

        return materials->CreateMaterial(constants, textures, material_id);
    }

    std::shared_ptr<IIr77Return const> UploadMaterials(std::vector<Ir77PVMaterialConstants> const& constants, std::vector<std::uint32_t>& material_ids) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVMaterial> materials{};
        if (Failed(m_init->GetMaterials(materials), result)) return result;

        return materials->UploadMaterials(constants, material_ids);
    }

    // One staging copy for every new row. Rows are appended -- existing rows (which frames in flight may be reading) are never rewritten.
    std::shared_ptr<IIr77Return const> UploadMaterials(std::vector<Ir77PVMaterialConstants> const& constants,
                                                       std::vector<Ir77PVMaterialTextures> const& textures, std::vector<std::uint32_t>& material_ids) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVMaterial> materials{};
        if (Failed(m_init->GetMaterials(materials), result)) return result;

        return materials->UploadMaterials(constants, textures, material_ids);
    }

    std::shared_ptr<IIr77Return const> GetMaterial(std::uint32_t const& material_id, Ir77PVMaterialData& material) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVMaterial> materials{};
        if (Failed(m_init->GetMaterials(materials), result)) return result;

        return materials->GetMaterial(material_id, material);
    }

    // ---- geometry -- procedural and single-mesh draw items ------------------------------------------------------------------------------------------

    // Draw item with no vertex buffer: vertex_count vertices (times instance_count) that the vertex shader builds from gl_VertexIndex /
    // gl_InstanceIndex and storage buffers. The pipeline's vertex shader must read no inputs. Caller fills model / user; for a GPU-driven
    // count set item.indirect (a storage buffer holding VkDrawIndirectCommands) and item.indirect_count.
    std::shared_ptr<IIr77Return const> MakeProceduralDrawItem(std::size_t const& window, Ir77PVPipelineHandle const& pipeline, std::uint32_t const& material_id,
                                                              std::uint32_t const& vertex_count, std::uint32_t const& instance_count, Ir77PVDrawItem& item) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVGeometry> geometry{};
        if (Failed(m_init->GetGeometry(geometry), result)) return result;

        return geometry->MakeProceduralDrawItem(window, pipeline, material_id, vertex_count, instance_count, item);
    }

    // Draw item for mesh + material through an engine role on a window. Caller fills model / instances. Needs CreatePipelines first.
    std::shared_ptr<IIr77Return const> MakeSingleMeshDrawItem(std::size_t const& window, Ir77PVPipelineKind const& kind, std::uint32_t const& single_mesh_id,
                                                              std::uint32_t const& material_id, Ir77PVDrawItem& item) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVGeometry> geometry{};
        if (Failed(m_init->GetGeometry(geometry), result)) return result;

        return geometry->MakeSingleMeshDrawItem(window, kind, single_mesh_id, material_id, item);
    }

    // A single mesh (UploadSingleMesh) through any vertex-input pipeline handle (FindPipeline). One draw, one instance; the caller fills
    // item.model. Task / mesh pipelines take meshlets instead (MakeMeshletDrawItem).
    std::shared_ptr<IIr77Return const> MakeSingleMeshDrawItem(std::size_t const& window, Ir77PVPipelineHandle const& pipeline,
                                                              std::uint32_t const& single_mesh_id, std::uint32_t const& material_id, Ir77PVDrawItem& item) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVGeometry> geometry{};
        if (Failed(m_init->GetGeometry(geometry), result)) return result;

        return geometry->MakeSingleMeshDrawItem(window, pipeline, single_mesh_id, material_id, item);
    }

    // ---- paint -- frame data, draw lists, dispatches, pass lists ---------------------------------------------------------------------------

    // Latest contents of a frame-level buffer (camera, lights, instances ...) -- re-applied every frame to every frame-in-flight copy
    std::shared_ptr<IIr77Return const> SetFrameData(std::size_t const& window, Ir77PVBufferSlot const& slot, void const* data, VkDeviceSize const& size,
                                                    VkDeviceSize const& offset = 0) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVPaint> paint{};
        if (Failed(m_init->GetPaint(paint), result)) return result;

        return paint->SetFrameData(window, slot, data, size, offset);
    }

    // Replaces one pass's draw list for a window. Persists until replaced.
    std::shared_ptr<IIr77Return const> SetDraws(std::size_t const& window, Ir77PVPass const& pass, std::vector<Ir77PVDrawItem> const& draws) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVPaint> paint{};
        if (Failed(m_init->GetPaint(paint), result)) return result;

        return paint->SetDraws(window, pass, draws);
    }

    // Replaces a window's compute list -- recorded in order by its Compute pass every frame. Persists until replaced.
    std::shared_ptr<IIr77Return const> SetDispatches(std::size_t const& window, std::vector<Ir77PVDispatch> const& dispatches) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVPaint> paint{};
        if (Failed(m_init->GetPaint(paint), result)) return result;

        return paint->SetDispatches(window, dispatches);
    }

    // Replaces a window's pass list. Exactly one pass must target the swapchain.
    std::shared_ptr<IIr77Return const> SetPassList(std::size_t const& window, std::vector<Ir77PVPassDesc> const& passes) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVPaint> paint{};
        if (Failed(m_init->GetPaint(paint), result)) return result;

        return paint->SetPassList(window, passes);
    }

    std::shared_ptr<IIr77Return const> ClearDraws(std::size_t const& window) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVPaint> paint{};
        if (Failed(m_init->GetPaint(paint), result)) return result;

        return paint->ClearDraws(window);
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // frame loop -- direct, every frame
    // -------------------------------------------------------------------------------------------------------------------------------------

    // Uploads, indirect show/hide writes, graveyard -- before anything records
    std::shared_ptr<IIr77Return const> Tick() { return m_init->Tick(m_frame); }

    // Records and presents, then moves to the next frame in flight. Assumes Paint advances one frame in flight per Draw.
    std::shared_ptr<IIr77Return const> Draw() {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVPaint> paint{};
        if (Failed(m_init->GetPaint(paint), result)) return result;

        paint->Draw();

        m_frame = (m_frame + 1) % MAX_FRAMES_IN_FLIGHT;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // pipe
    // -------------------------------------------------------------------------------------------------------------------------------------

    // Where this module's replies and events go -- the router's queue
    void SetPost(std::function<void(std::string)> post) { m_post = std::move(post); }

    // One message from the router. Runs on the main thread, between frames.
    void Sink(std::string json) {
        simdjson::dom::element root{};
        if (m_parser.parse(json).get(root) != simdjson::SUCCESS) {
            Ir77RETURN<Ir77OperationFailed>(this, "Ir77Server: not JSON.");
            return;
        }

        // a reply to something this module sent -- never answered, or two modules would answer each other forever
        if (root["ok"].error() == simdjson::SUCCESS) {
            bool reply_ok{true};
            if (root["ok"].get(reply_ok) == simdjson::SUCCESS && !reply_ok) Ir77RETURN<Ir77OperationFailed>(this, "Ir77Server: reply Failed: " + json);
            return;
        }

        std::int64_t id{0};
        if (root["id"].get(id) != simdjson::SUCCESS) id = 0;

        std::string_view from{};
        if (root["from"].get(from) != simdjson::SUCCESS) from = {};

        std::string_view op{};
        if (root["op"].get(op) != simdjson::SUCCESS) op = {};

        simdjson::dom::object args{};
        if (root["args"].get(args) != simdjson::SUCCESS) args = m_empty_args;

        std::string data{"{}"};

        auto const handler = m_operations.find(op);
        bool const known = handler != m_operations.end();

        bool const ok = known && handler->second(args, data)->ID() == GUIDIr77OperationSucceeded;
        char const* const error = ok ? "" : known ? "operation Failed" : "unknown op";

        if (!ok) Ir77RETURN<Ir77OperationFailed>(this, std::string{"Ir77Server: "} + error + ": " + json);

        if (from.empty() || !m_post) return;  // nobody to answer

        m_post("{\"id\":" + std::to_string(id) + ",\"from\":\"peregrinev\",\"to\":\"" + std::string{from} + "\",\"op\":\"" + std::string{op} +
               "\",\"ok\":" + (ok ? "true" : "false") + ",\"error\":\"" + error + "\",\"data\":" + data + "}");
    }

   private:
    using Ir77PVArgs = simdjson::dom::object;

    // a handler fills `data` (a JSON object) for the reply
    using Ir77PVOperation = std::function<std::shared_ptr<IIr77Return const>(Ir77PVArgs const&, std::string&)>;

    // heterogeneous lookup -- find by the string_view out of the parser, no std::string per message
    struct Ir77PVOpHash {
        using is_transparent = void;
        std::size_t operator()(std::string_view const& op) const { return std::hash<std::string_view>{}(op); }
    };

    // Window `window`'s overlay, through Init -> CEF
    std::shared_ptr<IIr77Return const> DefineOverlay(std::uint32_t const& window, std::shared_ptr<IIr77PVOverlay>& overlay) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVCEF> cef{};
        if (Failed(m_init->GetCEF(cef), result)) return result;

        return cef->GetOverlay(window, overlay);
    }

    void DefineOperations() {
        DefineRendererOperations();
        DefineDeviceOperations();
    }

    // Init's lifetime sequence -- Iridium sends these at startup / shutdown, in order
    void DefineRendererOperations() {
        m_operations["renderer.create"] = [this](Ir77PVArgs const& args, std::string& data) -> std::shared_ptr<IIr77Return const> {
            std::string_view shader_root{};
            if (args["shader_root"].get(shader_root) != simdjson::SUCCESS)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: renderer.create needs \"shader_root\".");

            std::uint64_t device_id{0};
            auto const result = m_init->CreateRenderer(std::string{shader_root}, device_id);

            data = "{\"device_id\":" + std::to_string(device_id) + "}";

            return result;
        };

        m_operations["renderer.command_buffers"] = [this](Ir77PVArgs const&, std::string&) { return m_init->CreateCommandBuffers(); };

        m_operations["renderer.destroy"] = [this](Ir77PVArgs const&, std::string&) { return m_init->DestroyRenderer(); };
    }

    void DefineDeviceOperations() {
        m_operations["device.info"] = [this](Ir77PVArgs const&, std::string& data) {
            std::string gpu{};
            auto const result = m_init->GetDeviceName(gpu);

            data = "{\"gpu\":\"" + Escape(gpu) + "\"}";

            return result;
        };
    }

    // JSON string escaping for text that goes into a reply
    std::string Escape(std::string const& text) const {
        std::string out{};
        out.reserve(text.size());

        for (char const c : text) {
            if (c == '"' || c == '\\') out += '\\';
            if (static_cast<unsigned char>(c) < 0x20) continue;  // control characters dropped
            out += c;
        }

        return out;
    }

   private:
    std::shared_ptr<IIr77PVInit> m_init{};  // the lifetime controller -- owns and wires the peers and everything under them

    std::uint32_t m_frame{0};  // frame in flight, for Init::Tick

    std::function<void(std::string)> m_post{};

    simdjson::dom::parser m_parser{};

    simdjson::dom::parser m_empty_parser{};  // owns m_empty_args

    Ir77PVArgs m_empty_args{};

    std::unordered_map<std::string, Ir77PVOperation, Ir77PVOpHash, std::equal_to<>> m_operations{};
};

}  // namespace NSIr77PeregrineV