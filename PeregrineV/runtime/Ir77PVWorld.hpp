#pragma once

#include <glm/glm.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
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

#include "../interface/IIr77PVPipelineCache.hpp"
#include "../interface/IIr77PVTransfer.hpp"
#include "../interface/IIr77PVMaterial.hpp"
#include "../interface/IIr77PVGeometry.hpp"
#include "../interface/IIr77PVAcceleration.hpp"
#include "../interface/IIr77PVPaint.hpp"
#include "../interface/IIr77PVWorld.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// =============================================================================================================================================
// Ir77PVWorld -- what gets drawn. Takes assets whole and frames whole, and does everything between them and the peers that draw.
//
//   startup:   Ir77PVInit wires it -- SetTransfer / SetMaterials / SetGeometry / SetPipelineCache / SetPaint / [SetAcceleration] /
//              SetDeviceName -> CreateDefaultMaterial
//   load:      LoadAsset(bundle) -> UploadAsset: UploadAssetMaterials (textures and materials -- Materials) -> UploadAssetStreams (BLEND
//              primitives split off into their own compacted block, the opaque ones sorted into material runs, each command's material row
//              going up with it -- Geometry; the static stream on the mesh or vertex path, the skinned stream and its compute-skinned copy, the
//              posed stream) -> BuildAssetRays (BLASes with MASK alpha rows -- Acceleration)
//   per frame: SubmitFrame(frame) -> WriteFrameAssets (models[], bones, which assets are in the rays) -> MakeFrameDispatches (skinning, then
//              the frame's own) -> PackLights -> the draws (culled or not) -> SubmitWindow per window (frame data, pass list when its view
//              areas change, every view's opaque / shadow / sorted transparent draws)
//   spawn:     SetInstanceCount(asset, first_instance, count) -- a mesh's live instances, inside the slots it was loaded with
//   unload:    UnloadAsset -- BLASes, blocks and tables freed, its ranges back
//
// GPU culling: with a "cull.compute" pipeline in the manifest, the opaque and shadow geometry on the vertex path is grouped into cull bins --
// one per pipeline and vertex stream, opaque and shadow apart -- remade whenever the assets in the frame change. Every view draws each opaque
// bin's culled list with one indirect-count draw, the shadow pass each shadow bin's (view 0's); static geometry is frustum-tested, animated
// (posed / skinned) geometry passes untested. The mesh path keeps its own task-shader culling; transparent objects stay sorted on the CPU.
// Without the pipeline, every asset draws unculled as before.
//
// Every asset gets its own range of models[] slots and joint matrices (first fit), so a scene is any number of files. LoadAsset moves the
// bundle's instance slots there; SubmitFrame writes each asset's transforms and bones into its ranges.
// =============================================================================================================================================
class Ir77PVWorld : public Ir77Enlisted, public IIr77PVWorld, public std::enable_shared_from_this<Ir77PVWorld> {
   public:
    Ir77PVWorld() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVWorld>(uid);

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

        else if (iid == GUIDIIr77PVWorld)
            obj = std::shared_ptr<IIr77PVWorld>(shared_from_this(), static_cast<IIr77PVWorld*>(this));

        else if (iid == GUIDIr77PVWorld)
            obj = std::shared_ptr<Ir77PVWorld>(shared_from_this(), static_cast<Ir77PVWorld*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // wiring -- Ir77PVInit
    // -------------------------------------------------------------------------------------------------------------------------------------
   public:
    std::shared_ptr<IIr77Return const> SetTransfer(std::shared_ptr<IIr77PVTransfer> transfer) {
        m_transfer = transfer;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetMaterials(std::shared_ptr<IIr77PVMaterial> materials) {
        m_materials = materials;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetGeometry(std::shared_ptr<IIr77PVGeometry> geometry) {
        m_geometry = geometry;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetPipelineCache(std::shared_ptr<IIr77PVPipelineCache> pipeline_cache) {
        m_pipeline_cache = pipeline_cache;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Where each frame goes -- used from the first SubmitFrame, after CreateCommandBuffers
    std::shared_ptr<IIr77Return const> SetPaint(std::shared_ptr<IIr77PVPaint> paint) {
        m_paint = paint;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Ray-query devices only -- assets get BLASes, the TLAS follows models[]
    std::shared_ptr<IIr77Return const> SetAcceleration(std::shared_ptr<IIr77PVAcceleration> acceleration) {
        m_acceleration = acceleration;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The GPU's name -- for the asset reports (LoadAsset's report.device)
    std::shared_ptr<IIr77Return const> SetDeviceName(std::string const& device_name) {
        m_device_name = device_name;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The grey default material -- primitives without one, and the placeholder material of procedural and multi-command draw items. After
    // SetMaterials.
    std::shared_ptr<IIr77Return const> CreateDefaultMaterial() {
        if (!m_materials) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVWorld: SetMaterials not called.");

        Ir77PVMaterialConstants constants{};
        constants.base_color = {0.8f, 0.8f, 0.8f, 1.0f};

        return m_materials->CreateMaterial(constants, m_default_material);
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // assets
    // -------------------------------------------------------------------------------------------------------------------------------------

    // Everything the bundle holds onto the device, blocking. A failure part way frees what was made. asset_id for SubmitFrame / UnloadAsset.
    std::shared_ptr<IIr77Return const> LoadAsset(Ir77PVAssetBundle const& bundle, std::uint32_t& asset_id, Ir77PVAssetReport& report) {
        asset_id = IR77PV_NO_ASSET;
        report = Ir77PVAssetReport{};

        if (!m_transfer || !m_materials || !m_geometry || !m_pipeline_cache) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVWorld: not wired.");

        std::shared_ptr<IIr77Return const> result{};

        Ir77PVWorldAsset asset{};
        asset.name = bundle.name;
        asset.instance_count = bundle.instance_count;
        asset.bone_count = bundle.bone_count;
        asset.bone_bases = bundle.bone_bases;
        asset.bone_bases.resize(bundle.instance_count, 0);

        if (!AllocateRange(m_instance_ranges, asset.instance_count, WORLD_INSTANCE_CAPACITY, asset.instance_base))
            return Ir77RETURN<Ir77OutOfRange>(this, "Ir77PVWorld: " + bundle.name + " -- no room for " + std::to_string(asset.instance_count) +
                                                        " more instances (" + std::to_string(WORLD_INSTANCE_CAPACITY) + " in all).");

        if (!AllocateRange(m_bone_ranges, asset.bone_count, WORLD_BONE_CAPACITY, asset.bone_base)) {
            asset.bone_count = 0;  // nothing taken -- only the instance range goes back
            ReleaseAsset(asset);
            return Ir77RETURN<Ir77OutOfRange>(this, "Ir77PVWorld: " + bundle.name + " -- no room for " + std::to_string(bundle.bone_count) +
                                                        " more joint matrices (" + std::to_string(WORLD_BONE_CAPACITY) + " in all).");
        }

        if (Failed(UploadAsset(bundle, asset, report), result)) {
            ReleaseAsset(asset);
            return result;
        }

        asset_id = m_next_asset++;
        m_assets.emplace(asset_id, std::move(asset));

        m_cull_dirty = true;

        ResizeFrameData();

        report.device_name = m_device_name;
        report.ray_query = static_cast<bool>(m_acceleration);
        report.instance_base = m_assets.at(asset_id).instance_base;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Ir77PVWorld: asset " + std::to_string(asset_id) + " (" + bundle.name + ") -- models[" +
                                                            std::to_string(report.instance_base) + ".." +
                                                            std::to_string(report.instance_base + bundle.instance_count) + ").");
    }

    // Its BLASes, blocks and tables freed, its models[] and joint ranges back. The draw lists are cleared now -- they name its buffers --
    // and the next SubmitFrame refills them.
    std::shared_ptr<IIr77Return const> UnloadAsset(std::uint32_t const& asset_id) {
        auto const found = m_assets.find(asset_id);
        if (found == m_assets.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVWorld: unknown asset.");

        if (m_paint) {
            for (std::size_t w = 0; w < m_windows; w++) m_paint->ClearDraws(w);
            m_paint->SetFrameDispatches({});
            m_paint->SetViewDispatches({});  // its commands leave the cull jobs with the next SubmitFrame
        }

        m_cull_dirty = true;

        ReleaseAsset(found->second);

        m_assets.erase(found);

        ResizeFrameData();

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Ir77PVWorld: asset " + std::to_string(asset_id) + " unloaded.");
    }

    // A mesh's live instances -- the one whose instance range starts at first_instance (asset-local, the bundle's first_instance). The asset was
    // loaded with that range's instance_count as its capacity: models[] slots, cull places and TLAS entries for all of them. count of them draw
    // (the first count -- [first_instance, first_instance + count)) and are in the rays; the rest stay reserved, hidden. Every stream of the asset
    // follows: static (either path), blend, skinned, posed. count past the capacity fails. Takes effect over the next frames in flight (Tick).
    std::shared_ptr<IIr77Return const> SetInstanceCount(std::uint32_t const& asset_id, std::uint32_t const& first_instance, std::uint32_t const& count) {
        auto const found = m_assets.find(asset_id);
        if (found == m_assets.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVWorld: unknown asset.");

        Ir77PVWorldAsset const& asset = found->second;

        auto const range = asset.instance_ranges.find(first_instance);
        if (range == asset.instance_ranges.end())
            return Ir77RETURN<Ir77NotConfigured>(this,
                                                 "Ir77PVWorld: " + asset.name + " has no instance range starting at " + std::to_string(first_instance) + ".");

        std::uint32_t const reserved = range->second;
        if (count > reserved)
            return Ir77RETURN<Ir77OutOfRange>(
                this, "Ir77PVWorld: " + asset.name + " -- " + std::to_string(count) + " instances, loaded with room for " + std::to_string(reserved) + ".");

        std::shared_ptr<IIr77Return const> result{};

        std::uint32_t const first_slot = asset.instance_base + first_instance;

        if (asset.has_static) {
            if (Failed(asset.mesh_path ? m_geometry->SetMeshletInstanceRange(asset.static_id, first_slot, reserved, count)
                                       : m_geometry->SetInstanceRange(asset.static_id, first_slot, reserved, count),
                       result))
                return result;
        }

        if (asset.has_blend && Failed(m_geometry->SetInstanceRange(asset.blend_id, first_slot, reserved, count), result)) return result;
        if (asset.has_skinned && Failed(m_geometry->SetInstanceRange(asset.skinned_id, first_slot, reserved, count), result)) return result;
        if (asset.has_posed && Failed(m_geometry->SetInstanceRange(asset.posed_id, first_slot, reserved, count), result)) return result;

        if (m_acceleration) {
            m_acceleration->SetSlotsLive(first_slot, count, true);
            m_acceleration->SetSlotsLive(first_slot + count, reserved - count, false);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // An edit -- the asset's vertices rewritten from the bundle's streams as they are now (the same order and counts LoadAsset had):
    // [static_range.x, + static_range.y) of vertices, [skinned_range.x, + skinned_range.y) of vertices_skinned. Every stream they went to
    // follows: static (either path), its BLEND copies, skinned (posed again by skin.comp every frame). The first edit unbounds the static
    // commands for the cull pass and rebuilds the static BLASes to refit every frame. Not waited on -- in order ahead of the next frames.
    std::shared_ptr<IIr77Return const> UpdateAssetVertices(std::uint32_t const& asset_id, std::vector<Ir77PVVertex> const& vertices,
                                                           std::vector<Ir77PVVertexSkinned> const& vertices_skinned, glm::uvec2 const& static_range,
                                                           glm::uvec2 const& skinned_range) {
        auto const found = m_assets.find(asset_id);
        if (found == m_assets.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVWorld: unknown asset.");

        Ir77PVWorldAsset& asset = found->second;

        if (static_cast<std::size_t>(static_range.x) + static_range.y > vertices.size() ||
            static_cast<std::size_t>(skinned_range.x) + skinned_range.y > vertices_skinned.size())
            return Ir77RETURN<Ir77OutOfRange>(this, "Ir77PVWorld: " + asset.name + " -- edit range past the streams given.");

        std::shared_ptr<IIr77Return const> result{};

        if (!asset.edited && Failed(BeginAssetEdit(asset), result)) return result;

        if (static_range.y > 0) {
            if (asset.has_static) {
                Ir77PVVertex const* const data = vertices.data() + static_range.x;

                if (Failed(asset.mesh_path ? m_geometry->UpdateMeshletVertices(asset.static_id, static_range.x, static_range.y, data)
                                           : m_geometry->UpdateBlockVertices(asset.static_id, false, static_range.x, static_range.y, data),
                           result))
                    return result;
            }

            // the BLEND primitives' compacted copies -- each span the edit overlaps
            for (Ir77PVWorldSpan const& span : asset.blend_spans) {
                std::uint32_t const low = std::max(span.source, static_range.x);
                std::uint32_t const high = std::min(span.source + span.count, static_range.x + static_range.y);
                if (!asset.has_blend || low >= high) continue;

                if (Failed(m_geometry->UpdateBlockVertices(asset.blend_id, false, span.destination + (low - span.source), high - low, vertices.data() + low), result))
                    return result;
            }
        }

        if (skinned_range.y > 0 && asset.has_skinned &&
            Failed(m_geometry->UpdateBlockVertices(asset.skinned_id, true, skinned_range.x, skinned_range.y, vertices_skinned.data() + skinned_range.x), result))
            return result;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // per frame
    // -------------------------------------------------------------------------------------------------------------------------------------

    // The frame -> Paint and Acceleration: WriteFrameAssets (models[], bones, the rays), MakeFrameDispatches (skinning, then the frame's own --
    // once a frame), PackLights, the assets' opaque and shadow draws (culled per view, or one list), then SubmitWindow for each window.
    // report = the last window's counts.
    std::shared_ptr<IIr77Return const> SubmitFrame(Ir77PVFrame const& frame, Ir77PVFrameReport& report) {
        report = Ir77PVFrameReport{};

        if (!m_paint || !m_geometry) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVWorld: not wired.");

        for (Ir77PVFrameWindow const& window : frame.windows) {
            if (window.cameras.empty() || window.cameras.size() > MAX_VIEWS)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVWorld: every window needs 1 to MAX_VIEWS cameras.");

            if (!window.areas.empty() && window.areas.size() != window.cameras.size())
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVWorld: a window's areas must be one per camera, or none.");
        }

        std::shared_ptr<IIr77Return const> result{};

        m_windows = frame.windows.size();
        m_window_areas.resize(m_windows);

        WriteFrameAssets(frame);

        m_paint->SetFrameDispatches(MakeFrameDispatches(frame));

        PackLights(frame);

        std::uint32_t const shadow_count = static_cast<std::uint32_t>(std::min<std::size_t>(frame.shadow_matrices.size(), MAX_SHADOW_LIGHTS));

        // GPU culling -- the bins again when the assets in the frame changed
        if (m_cull_dirty || m_present != m_cull_present) {
            if (Failed(BuildCullBins(), result)) return result;

            m_cull_present = m_present;
            m_cull_dirty = false;
        }

        // the assets' draws -- the same for every window: per view when culled (each view its own culled lists), one list otherwise
        std::uint32_t views{1};
        for (Ir77PVFrameWindow const& window : frame.windows) views = std::max(views, static_cast<std::uint32_t>(window.cameras.size()));

        std::vector<std::vector<Ir77PVDrawItem>> opaque{};  // [view]
        std::vector<Ir77PVDrawItem> shadow{};

        if (m_culled) {
            if (Failed(MakeCulledDraws(shadow_count, views, opaque, shadow), result)) return result;
        } else {
            opaque.assign(1, {});
            if (Failed(MakeOpaqueDraws(shadow_count, opaque[0], shadow), result)) return result;
        }

        // the selection's outlines -- never culled (an outlined asset is what you're looking at), the same in every view
        MakeOutlineDraws(m_outline_draws, m_outline_blend_draws);

        for (std::size_t w = 0; w < frame.windows.size(); w++)
            if (Failed(SubmitWindow(w, frame, opaque, shadow, shadow_count, report), result)) return result;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The grey default's material id -- what procedural draw items carry
    std::shared_ptr<IIr77Return const> GetDefaultMaterial(std::uint32_t& material_id) {
        material_id = m_default_material;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    // -------------------------------------------------------------------------------------------------------------------------------------
    // load steps
    // -------------------------------------------------------------------------------------------------------------------------------------

    // Textures and materials, the streams and their material tables, the BLASes -- in that order. Fills asset as it goes, so a failure leaves
    // exactly what ReleaseAsset has to free.
    std::shared_ptr<IIr77Return const> UploadAsset(Ir77PVAssetBundle const& bundle, Ir77PVWorldAsset& asset, Ir77PVAssetReport& report) {
        std::shared_ptr<IIr77Return const> result{};

        asset.pipeline = bundle.pipeline;
        if (asset.pipeline == IR77PV_NO_PIPELINE && Failed(m_pipeline_cache->FindPipeline(Ir77PVPipelineKind::Static, asset.pipeline), result)) return result;

        // every mesh's instance range -- what SetInstanceCount addresses (both streams of a mesh share it)
        for (auto const* primitives : {&bundle.primitives, &bundle.primitives_skinned})
            for (Ir77PVAssetPrimitive const& primitive : *primitives)
                if (primitive.range.instance_count > 0) asset.instance_ranges[primitive.range.first_instance] = primitive.range.instance_count;

        if (Failed(UploadAssetMaterials(bundle, asset), result)) return result;

        std::vector<std::uint32_t> static_materials{};  // bundle material per static command
        std::vector<std::uint32_t> posed_materials{};   // bundle material per posed command
        if (Failed(UploadAssetStreams(bundle, asset, report, static_materials, posed_materials), result)) return result;

        if (m_acceleration && Failed(BuildAssetRays(bundle, asset, static_materials, posed_materials), result)) return result;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The first edit of an asset -- its static commands never culled (their spheres no longer hold), its static BLASes (vertex path) rebuilt to
    // refit every frame, so the rays follow the edit. The mesh path's BLASes stay as built.
    std::shared_ptr<IIr77Return const> BeginAssetEdit(Ir77PVWorldAsset& asset) {
        std::shared_ptr<IIr77Return const> result{};

        if (asset.has_static && !asset.mesh_path && Failed(m_geometry->UnboundBlock(asset.static_id), result)) return result;

        if (m_acceleration && asset.has_static && !asset.mesh_path && asset.static_objects < asset.objects.size()) {
            glm::uvec2& objects = asset.objects[asset.static_objects];

            if (Failed(m_acceleration->ReleaseObjects(objects), result)) return result;
            objects = glm::uvec2(0u);

            if (Failed(m_acceleration->BuildBlockObjects(asset.static_id, true, asset.static_alpha, objects), result)) return result;
            if (!asset.in_rays) m_acceleration->SetObjectsVisible(objects, false);
        }

        asset.edited = true;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Ir77PVWorld: " + asset.name + " -- editing (unculled, rays refit every frame).");
    }

    // Textures -- bundle index -> Materials id in asset.texture_ids, IR77PV_NO_TEXTURE where the upload failed (the placeholder stands in).
    // Materials -- one batch, bundle index -> Materials id in asset.material_ids.
    std::shared_ptr<IIr77Return const> UploadAssetMaterials(Ir77PVAssetBundle const& bundle, Ir77PVWorldAsset& asset) {
        std::shared_ptr<IIr77Return const> result{};

        std::vector<std::uint32_t>& texture_ids = asset.texture_ids;
        texture_ids.assign(bundle.textures.size(), IR77PV_NO_TEXTURE);

        for (std::size_t t = 0; t < bundle.textures.size(); t++) {
            Ir77PVAssetTexture const& texture = bundle.textures[t];
            if (texture.rgba.empty()) continue;

            if (m_materials->CreateTexture(texture.rgba.data(), texture.width, texture.height, texture.srgb, texture_ids[t])->ID() !=
                GUIDIr77OperationSucceeded)
                texture_ids[t] = IR77PV_NO_TEXTURE;
        }

        if (bundle.materials.empty()) return Ir77RETURN<Ir77OperationSucceeded>();

        auto const texture_id = [&](std::uint32_t const& index) { return index < texture_ids.size() ? texture_ids[index] : IR77PV_NO_TEXTURE; };

        std::vector<Ir77PVMaterialConstants> constants(bundle.materials.size());
        std::vector<Ir77PVMaterialTextures> textures(bundle.materials.size());

        for (std::size_t m = 0; m < bundle.materials.size(); m++) {
            constants[m] = bundle.materials[m].constants;

            Ir77PVMaterialTextures const& in = bundle.materials[m].textures;
            textures[m] = {texture_id(in.albedo), texture_id(in.normal), texture_id(in.orm), texture_id(in.emissive), texture_id(in.ao)};
        }

        if (Failed(m_materials->UploadMaterials(constants, textures, asset.material_ids), result)) return result;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The streams -> Geometry, each command with its material row:
    //   static opaque     -- commands sorted into material runs; mesh path when bundle.use_mesh and the meshlets go up, vertex path otherwise
    //   static BLEND      -- compacted into its own small vertex-path block (the mesh path has no Transparent pipeline), one command each
    //   skinned           -- vertex path only: opaque runs, then one command per transparent object (SkinnedTransparent)
    //   posed             -- the skinned opaque commands' compute-skinned copy (UploadPosedStream)
    // static_materials / posed_materials = the bundle material of every static / posed command -- the BLASes' alpha rows.
    std::shared_ptr<IIr77Return const> UploadAssetStreams(Ir77PVAssetBundle const& bundle, Ir77PVWorldAsset& asset, Ir77PVAssetReport& report,
                                                          std::vector<std::uint32_t>& static_materials, std::vector<std::uint32_t>& posed_materials) {
        std::shared_ptr<IIr77Return const> result{};

        auto const material_id = [&](std::uint32_t const& index) { return MaterialId(asset, index); };
        auto const blend = [&](std::uint32_t const& index) { return index < bundle.materials.size() && bundle.materials[index].blend; };

        // a primitive's command, first_instance moved into the asset's models[] range
        auto const command = [&](Ir77MeshRange const& r) {
            return VkDrawIndexedIndirectCommand{r.index_count, r.instance_count, r.index_offset, r.vertex_offset, r.first_instance + asset.instance_base};
        };

        // the opaque primitives as commands sorted by material, each command's bundle material beside it (the stream's material table);
        // returns the number of material runs
        auto const opaque_runs = [&](std::vector<Ir77PVAssetPrimitive> const& primitives, std::vector<VkDrawIndexedIndirectCommand>& commands,
                                     std::vector<std::uint32_t>& materials) {
            std::vector<std::size_t> order{};
            for (std::size_t p = 0; p < primitives.size(); p++)
                if (!blend(primitives[p].material)) order.push_back(p);

            std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) { return primitives[a].material < primitives[b].material; });

            std::uint32_t runs{0};
            for (std::size_t k = 0; k < order.size(); k++) {
                Ir77PVAssetPrimitive const& primitive = primitives[order[k]];

                commands.push_back(command(primitive.range));
                materials.push_back(primitive.material);

                if (k == 0 || primitives[order[k - 1]].material != primitive.material) runs++;
            }

            return runs;
        };

        // ---- static opaque ----
        Ir77PVBlockUpload upload{};
        upload.vertices = bundle.vertices;
        upload.indices = bundle.indices;

        report.opaque_runs = opaque_runs(bundle.primitives, upload.indirect_commands, static_materials);

        // ---- static BLEND ----
        Ir77PVBlockUpload blend_upload{};
        std::vector<std::uint32_t> blend_materials{};

        for (Ir77PVAssetPrimitive const& primitive : bundle.primitives) {
            Ir77MeshRange const& r = primitive.range;
            if (!blend(primitive.material) || r.index_count == 0) continue;

            std::uint32_t highest{0};  // indices are relative to vertex_offset -- the highest one bounds the vertex span
            for (std::uint32_t i = 0; i < r.index_count; i++) highest = std::max(highest, bundle.indices[r.index_offset + i]);

            std::uint32_t const index = static_cast<std::uint32_t>(blend_upload.indirect_commands.size());
            asset.blends.push_back({material_id(primitive.material), index, r.first_instance + asset.instance_base, false});
            blend_materials.push_back(primitive.material);

            blend_upload.indirect_commands.push_back(
                VkDrawIndexedIndirectCommand{r.index_count, r.instance_count, static_cast<std::uint32_t>(blend_upload.indices.size()),
                                             static_cast<std::int32_t>(blend_upload.vertices.size()), r.first_instance + asset.instance_base});

            asset.blend_spans.push_back({static_cast<std::uint32_t>(r.vertex_offset), highest + 1, static_cast<std::uint32_t>(blend_upload.vertices.size())});

            blend_upload.indices.insert(blend_upload.indices.end(), bundle.indices.begin() + r.index_offset,
                                        bundle.indices.begin() + r.index_offset + r.index_count);
            blend_upload.vertices.insert(blend_upload.vertices.end(), bundle.vertices.begin() + r.vertex_offset,
                                         bundle.vertices.begin() + r.vertex_offset + highest + 1);
        }

        // ---- skinned ----
        Ir77PVBlockUpload skinned_upload{};
        skinned_upload.vertices_skinned = bundle.vertices_skinned;
        skinned_upload.indices_skinned = bundle.indices_skinned;

        std::vector<std::uint32_t> skinned_materials{};
        report.skinned_runs = opaque_runs(bundle.primitives_skinned, skinned_upload.indirect_commands, skinned_materials);
        asset.skinned_opaque_count = static_cast<std::uint32_t>(skinned_upload.indirect_commands.size());

        for (Ir77PVAssetPrimitive const& primitive : bundle.primitives_skinned) {
            if (!blend(primitive.material)) continue;

            std::uint32_t const index = static_cast<std::uint32_t>(skinned_upload.indirect_commands.size());
            asset.blends.push_back({material_id(primitive.material), index, primitive.range.first_instance + asset.instance_base, true});

            skinned_upload.indirect_commands.push_back(command(primitive.range));
            skinned_materials.push_back(primitive.material);
        }

        report.transparent_objects = static_cast<std::uint32_t>(asset.blends.size());

        // ---- material rows -- one per indirect command; Geometry keeps them beside the commands, where the vertex shaders read them ----
        if (Failed(MaterialRows(asset, static_materials, upload.command_materials), result)) return result;
        if (Failed(MaterialRows(asset, blend_materials, blend_upload.command_materials), result)) return result;
        if (Failed(MaterialRows(asset, skinned_materials, skinned_upload.command_materials), result)) return result;

        // ---- uploads ----
        asset.static_count = static_cast<std::uint32_t>(upload.indirect_commands.size());

        if (asset.static_count > 0) {
            // mesh path -- the material row of every triangle, at its place in indices. Meshlets are cut per row.
            if (bundle.use_mesh) {
                upload.triangle_materials.assign(upload.indices.size() / 3, 0);

                for (std::size_t c = 0; c < upload.indirect_commands.size(); c++) {
                    Ir77PVMaterialData material{};
                    if (Failed(m_materials->GetMaterial(material_id(static_materials[c]), material), result)) return result;

                    VkDrawIndexedIndirectCommand const& cmd = upload.indirect_commands[c];
                    std::fill_n(upload.triangle_materials.begin() + cmd.firstIndex / 3, cmd.indexCount / 3, material.row);
                }

                std::uint32_t instances{0};
                asset.mesh_path = m_geometry->UploadMeshlets(upload, asset.static_id, instances, true)->ID() == GUIDIr77OperationSucceeded;
            }

            if (!asset.mesh_path && Failed(m_geometry->UploadBlock(upload, asset.static_id, true), result)) return result;

            asset.has_static = true;
        }

        report.mesh_path = asset.mesh_path;

        if (!blend_upload.indirect_commands.empty()) {
            if (Failed(m_geometry->UploadBlock(blend_upload, asset.blend_id, true), result)) return result;

            asset.has_blend = true;
        }

        if (!skinned_upload.indirect_commands.empty()) {
            if (Failed(m_geometry->UploadBlock(skinned_upload, asset.skinned_id, true), result)) return result;

            asset.has_skinned = true;

            if (Failed(UploadPosedStream(skinned_upload, skinned_materials, asset, posed_materials), result)) return result;
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Rays -- one BLAS per object of the static stream (either path) and of the posed stream (refit every frame). MASK materials go in
    // alpha-tested: the material row per command, BINDLESS_NO_SLOT for everything opaque. Transparent objects stay out.
    std::shared_ptr<IIr77Return const> BuildAssetRays(Ir77PVAssetBundle const& bundle, Ir77PVWorldAsset& asset,
                                                      std::vector<std::uint32_t> const& static_materials, std::vector<std::uint32_t> const& posed_materials) {
        std::shared_ptr<IIr77Return const> result{};

        auto const mask = [&](std::uint32_t const& index) { return index < bundle.materials.size() && bundle.materials[index].mask; };

        auto const alpha_rows = [&](std::vector<std::uint32_t> const& materials, std::vector<std::uint32_t>& rows) {
            rows.assign(materials.size(), BINDLESS_NO_SLOT);

            for (std::size_t c = 0; c < materials.size(); c++) {
                if (!mask(materials[c])) continue;

                Ir77PVMaterialData material{};
                if (m_materials->GetMaterial(MaterialId(asset, materials[c]), material)->ID() == GUIDIr77OperationSucceeded) rows[c] = material.row;
            }
        };

        std::vector<std::uint32_t> rows{};
        glm::uvec2 objects{0u};

        if (asset.has_static) {
            alpha_rows(static_materials, rows);

            if (Failed(asset.mesh_path ? m_acceleration->BuildMeshletObjects(asset.static_id, rows, objects)
                                       : m_acceleration->BuildBlockObjects(asset.static_id, false, rows, objects),
                       result))
                return result;

            asset.static_objects = asset.objects.size();
            asset.static_alpha = rows;
            asset.objects.push_back(objects);
        }

        if (asset.has_posed) {
            alpha_rows(posed_materials, rows);

            if (Failed(m_acceleration->BuildBlockObjects(asset.posed_id, true, rows, objects), result)) return result;

            asset.objects.push_back(objects);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Compute skinning -- the skinned opaque commands become the posed stream: one region of plain vertices per (primitive, instance), filled
    // with the bind pose here and posed by skin.comp every frame. It draws through the static and shadow pipelines like any static geometry.
    // No skin.compute pipeline -> the skinned geometry stays on the Skinned pipelines.
    // posed_materials = the bundle material of every posed command.
    std::shared_ptr<IIr77Return const> UploadPosedStream(Ir77PVBlockUpload const& skinned_upload, std::vector<std::uint32_t> const& skinned_materials,
                                                         Ir77PVWorldAsset& asset, std::vector<std::uint32_t>& posed_materials) {
        std::shared_ptr<IIr77Return const> result{};

        posed_materials.clear();
        if (asset.skinned_opaque_count == 0) return Ir77RETURN<Ir77OperationSucceeded>();

        // a handle is issued for any name -- only resolving it says whether skin.compute exists
        std::shared_ptr<IIr77PVPipelineCPT> skin{};
        if (m_pipeline_cache->FindPipeline(std::string{"skin.compute"}, asset.skin_compute)->ID() != GUIDIr77OperationSucceeded ||
            m_pipeline_cache->GetComputePipeline(asset.skin_compute, skin)->ID() != GUIDIr77OperationSucceeded) {
            asset.skin_compute = IR77PV_NO_PIPELINE;
            return Ir77RETURN<Ir77OperationSucceeded>(this, "Ir77PVWorld: no skin.compute pipeline -- skinned geometry stays on the Skinned pipeline.");
        }

        Ir77PVBlockUpload posed{};
        std::vector<glm::uvec2> jobs{};  // per posed vertex: source vertex, models[] slot

        posed.indices = skinned_upload.indices_skinned;  // shared by every instance -- vertexOffset picks the region

        for (std::uint32_t c = 0; c < asset.skinned_opaque_count; c++) {
            VkDrawIndexedIndirectCommand const& cmd = skinned_upload.indirect_commands[c];
            if (cmd.indexCount == 0) continue;

            // vertices this primitive uses -- indices are local, so highest + 1
            std::uint32_t highest{0};
            for (std::uint32_t i = 0; i < cmd.indexCount; i++) highest = std::max(highest, skinned_upload.indices_skinned[cmd.firstIndex + i]);

            for (std::uint32_t inst = 0; inst < cmd.instanceCount; inst++) {
                std::uint32_t const region = static_cast<std::uint32_t>(posed.vertices.size());
                std::uint32_t const slot = cmd.firstInstance + inst;

                for (std::uint32_t v = 0; v <= highest; v++) {
                    std::uint32_t const source = static_cast<std::uint32_t>(cmd.vertexOffset) + v;
                    Ir77PVVertexSkinned const& bind = skinned_upload.vertices_skinned[source];

                    posed.vertices.push_back({bind.position, bind.normal, bind.uv, bind.tangent});
                    jobs.push_back(glm::uvec2(source, slot));
                }

                posed.indirect_commands.push_back(VkDrawIndexedIndirectCommand{cmd.indexCount, 1, cmd.firstIndex, static_cast<std::int32_t>(region), slot});
                posed_materials.push_back(skinned_materials[c]);
            }
        }

        if (posed.indirect_commands.empty()) return Ir77RETURN<Ir77OperationSucceeded>();

        if (Failed(MaterialRows(asset, posed_materials, posed.command_materials), result)) return result;

        if (Failed(m_geometry->UploadBlock(posed, asset.posed_id, true), result)) return result;

        asset.has_posed = true;  // from here on ReleaseAsset unloads it

        if (Failed(m_geometry->GetBlockVertexSlot(asset.posed_id, false, asset.skin_destination), result)) return result;
        if (Failed(m_geometry->GetBlockVertexSlot(asset.skinned_id, true, asset.skin_source), result)) return result;
        if (Failed(
                m_transfer->CreateStorageBuffer(static_cast<VkDeviceSize>(jobs.size() * sizeof(glm::uvec2)), jobs.data(), asset.jobs_buffer, asset.jobs_slot),
                result))
            return result;

        asset.posed_count = static_cast<std::uint32_t>(posed.indirect_commands.size());
        asset.posed_vertices = static_cast<std::uint32_t>(posed.vertices.size());

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Bundle material per indirect command -> Materials rows, for the upload's command_materials
    std::shared_ptr<IIr77Return const> MaterialRows(Ir77PVWorldAsset const& asset, std::vector<std::uint32_t> const& materials,
                                                    std::vector<std::uint32_t>& rows) {
        std::shared_ptr<IIr77Return const> result{};

        rows.assign(materials.size(), 0);

        for (std::size_t c = 0; c < materials.size(); c++) {
            Ir77PVMaterialData material{};
            if (Failed(m_materials->GetMaterial(MaterialId(asset, materials[c]), material), result)) return result;

            rows[c] = material.row;
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Bundle material -> Materials id; out of range (no material) -> the grey default
    std::uint32_t MaterialId(Ir77PVWorldAsset const& asset, std::uint32_t const& index) const {
        return index < asset.material_ids.size() ? asset.material_ids[index] : m_default_material;
    }

    // Everything an asset put on the device, BLASes first (they read the blocks' buffers), then its materials and textures, and its ranges.
    // Safe on a half-loaded asset. All of it goes through Transfer's graveyard -- freed once no frame in flight can read it.
    void ReleaseAsset(Ir77PVWorldAsset& asset) {
        if (m_acceleration)
            for (glm::uvec2 const& objects : asset.objects) m_acceleration->ReleaseObjects(objects);

        if (asset.has_static) asset.mesh_path ? m_geometry->UnloadMeshlets(asset.static_id) : m_geometry->UnloadBlock(asset.static_id);
        if (asset.has_blend) m_geometry->UnloadBlock(asset.blend_id);
        if (asset.has_skinned) m_geometry->UnloadBlock(asset.skinned_id);
        if (asset.has_posed) m_geometry->UnloadBlock(asset.posed_id);

        if (asset.jobs_slot != BINDLESS_NO_SLOT) m_transfer->DestroyStorageBuffer(asset.jobs_buffer);

        if (!asset.material_ids.empty()) m_materials->DestroyMaterials(asset.material_ids);

        for (std::uint32_t const texture : asset.texture_ids)
            if (texture != IR77PV_NO_TEXTURE) m_materials->DestroyTexture(texture);

        // a later asset in these slots starts with every instance live
        if (m_acceleration && asset.instance_count > 0) m_acceleration->SetSlotsLive(asset.instance_base, asset.instance_count, true);

        if (asset.instance_count > 0) m_instance_ranges.erase(asset.instance_base);
        if (asset.bone_count > 0) m_bone_ranges.erase(asset.bone_base);

        asset = Ir77PVWorldAsset{};
    }

    // First fit: count entries in [0, capacity) that no range in used covers. used maps first -> count. count 0 takes nothing.
    bool AllocateRange(std::map<std::uint32_t, std::uint32_t>& used, std::uint32_t const& count, std::uint32_t const& capacity, std::uint32_t& first) {
        first = 0;
        if (count == 0) return true;

        std::uint32_t cursor{0};
        for (auto const& [start, size] : used) {
            if (start - cursor >= count) break;
            cursor = start + size;
        }

        if (static_cast<std::uint64_t>(cursor) + count > capacity) return false;

        used.emplace(cursor, count);
        first = cursor;

        return true;
    }

    // models[] and the bones buffer sized to the highest range in use -- what SubmitFrame writes and Paint copies each frame
    void ResizeFrameData() {
        std::uint32_t models{0};
        for (auto const& [start, size] : m_instance_ranges) models = std::max(models, start + size);

        m_bone_high = 0;
        for (auto const& [start, size] : m_bone_ranges) m_bone_high = std::max(m_bone_high, start + size);

        m_models.resize(models, glm::mat4(1.0f));

        // bone_base[] then the palettes -- the layout skinned.vert and skin.comp read at set 0 binding 4. The bases only change here.
        m_bones_packed.assign(m_bone_high > 0 ? WORLD_BONE_BASE_BYTES + static_cast<std::size_t>(m_bone_high) * sizeof(glm::mat4) : 0, 0);
        if (m_bone_high == 0) return;

        std::uint32_t* const bases = reinterpret_cast<std::uint32_t*>(m_bones_packed.data());

        for (auto const& [id, asset] : m_assets)
            for (std::uint32_t i = 0; i < asset.instance_count; i++) bases[asset.instance_base + i] = asset.bone_base + asset.bone_bases[i];
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // frame steps
    // -------------------------------------------------------------------------------------------------------------------------------------

    // Each asset in the frame: its transforms into models[], its bones into the palette, its id into m_present (sorted, once each). Assets in
    // or out of the frame go in or out of the rays; the TLAS gets models[].
    void WriteFrameAssets(Ir77PVFrame const& frame) {
        m_present.clear();
        m_outlines.clear();

        for (Ir77PVFrameAsset const& entry : frame.assets) {
            auto const found = m_assets.find(entry.asset);
            if (found == m_assets.end()) continue;

            Ir77PVWorldAsset const& asset = found->second;
            m_present.push_back(entry.asset);
            if (entry.outline.a > 0.0f) m_outlines.emplace_back(entry.asset, entry.outline);

            std::size_t const transforms = std::min<std::size_t>(entry.transforms.size(), asset.instance_count);
            std::copy_n(entry.transforms.begin(), transforms, m_models.begin() + asset.instance_base);

            std::size_t const bones = std::min<std::size_t>(entry.bones.size(), asset.bone_count);
            if (bones > 0)
                std::memcpy(m_bones_packed.data() + WORLD_BONE_BASE_BYTES + static_cast<std::size_t>(asset.bone_base) * sizeof(glm::mat4), entry.bones.data(),
                            bones * sizeof(glm::mat4));
        }

        std::sort(m_present.begin(), m_present.end());
        m_present.erase(std::unique(m_present.begin(), m_present.end()), m_present.end());

        if (!m_acceleration) return;

        for (auto& [id, asset] : m_assets) {
            bool const present = std::binary_search(m_present.begin(), m_present.end(), id);
            if (present == asset.in_rays) continue;

            for (glm::uvec2 const& objects : asset.objects) m_acceleration->SetObjectsVisible(objects, present);
            asset.in_rays = present;
        }

        m_acceleration->SetInstanceTransforms(m_models);
    }

    // The frame's compute, once a frame: each present asset's skinning (skin.comp over its posed vertices), then the frame's own dispatches
    std::vector<Ir77PVDispatch> MakeFrameDispatches(Ir77PVFrame const& frame) const {
        std::vector<Ir77PVDispatch> dispatches{};

        for (std::uint32_t const id : m_present) {
            Ir77PVWorldAsset const& asset = m_assets.at(id);
            if (!asset.has_posed) continue;

            Ir77PVDispatch skin{};
            skin.pipeline = asset.skin_compute;
            skin.push.user = {asset.jobs_slot, asset.skin_source, asset.skin_destination, asset.posed_vertices, 0u, 0u, 0u, 0u};
            skin.groups = SkinGroups(asset.posed_vertices);
            dispatches.push_back(skin);
        }

        dispatches.insert(dispatches.end(), frame.dispatches.begin(), frame.dispatches.end());

        return dispatches;
    }

    // The lights as set 0 binding 1 reads them -- the header (count), then Ir77PVLight[]
    void PackLights(Ir77PVFrame const& frame) {
        Ir77PVLightsHeader header{};
        header.count = static_cast<std::uint32_t>(frame.lights.size());

        m_lights_packed.resize(sizeof(header) + frame.lights.size() * sizeof(Ir77PVLight));
        std::memcpy(m_lights_packed.data(), &header, sizeof(header));
        if (!frame.lights.empty()) std::memcpy(m_lights_packed.data() + sizeof(header), frame.lights.data(), frame.lights.size() * sizeof(Ir77PVLight));
    }

    // One window -> Paint: its pass list when its view areas changed, the clear color, its frame data (cameras VIEW_STRIDE apart, lights,
    // models[], shadow matrices, bones), the shadow list (the assets' then the window's own), and per view the opaque list (the assets' --
    // opaque[view] when culled, opaque[0] otherwise -- then the window's own) and the transparent objects back to front from that view's eye.
    // report = this window's counts (the last window's win).
    std::shared_ptr<IIr77Return const> SubmitWindow(std::size_t const w, Ir77PVFrame const& frame, std::vector<std::vector<Ir77PVDrawItem>> const& opaque,
                                                    std::vector<Ir77PVDrawItem> const& shadow, std::uint32_t const shadow_count, Ir77PVFrameReport& report) {
        std::shared_ptr<IIr77Return const> result{};

        Ir77PVFrameWindow const& window = frame.windows[w];
        std::uint32_t const views = static_cast<std::uint32_t>(window.cameras.size());
        std::vector<glm::vec4> const areas = window.areas.empty() ? GridAreas(views) : window.areas;

        if (areas != m_window_areas[w]) {
            if (Failed(m_paint->SetPassList(w, MakePassList(areas)), result)) return result;

            m_paint->ClearDraws(w);  // the views it no longer has
            m_window_areas[w] = areas;
        }

        VkClearColorValue clear{};
        std::memcpy(clear.float32, &window.clear_color, sizeof(clear.float32));
        if (Failed(m_paint->SetClearColor(w, clear), result)) return result;

        // one camera per view, VIEW_STRIDE apart -- where each view's frame table points
        m_cameras_packed.assign(static_cast<std::size_t>(VIEW_STRIDE) * views, 0);
        for (std::uint32_t v = 0; v < views; v++)
            std::memcpy(m_cameras_packed.data() + static_cast<std::size_t>(VIEW_STRIDE) * v, &window.cameras[v], sizeof(Ir77PVCamera));

        m_paint->SetFrameData(w, Ir77PVBufferSlot::Camera, m_cameras_packed.data(), m_cameras_packed.size());
        m_paint->SetFrameData(w, Ir77PVBufferSlot::Lights, m_lights_packed.data(), m_lights_packed.size());

        if (!m_models.empty()) m_paint->SetFrameData(w, Ir77PVBufferSlot::Instances, m_models.data(), m_models.size() * sizeof(glm::mat4));
        if (shadow_count > 0) m_paint->SetFrameData(w, Ir77PVBufferSlot::ShadowMatrices, frame.shadow_matrices.data(), shadow_count * sizeof(glm::mat4));
        if (m_bone_high > 0) m_paint->SetFrameData(w, Ir77PVBufferSlot::Bones, m_bones_packed.data(), m_bones_packed.size());

        std::vector<Ir77PVDrawItem> window_shadow = shadow;
        window_shadow.insert(window_shadow.end(), window.shadow.begin(), window.shadow.end());

        m_paint->SetDraws(w, 0, Ir77PVPass::Shadow, window_shadow);

        report = Ir77PVFrameReport{};
        report.shadow_items = static_cast<std::uint32_t>(window_shadow.size());

        for (std::uint32_t v = 0; v < views; v++) {
            std::vector<Ir77PVDrawItem> view_opaque = opaque[opaque.size() > 1 ? v : 0];
            view_opaque.insert(view_opaque.end(), m_outline_draws.begin(), m_outline_draws.end());  // after the models they outline
            view_opaque.insert(view_opaque.end(), window.opaque.begin(), window.opaque.end());

            std::vector<Ir77PVDrawItem> view_transparent{};
            if (Failed(MakeTransparentDraws(window.cameras[v], view_transparent), result)) return result;
            view_transparent.insert(view_transparent.end(), m_outline_blend_draws.begin(), m_outline_blend_draws.end());  // after every surface
            view_transparent.insert(view_transparent.end(), window.transparent.begin(), window.transparent.end());

            m_paint->SetDraws(w, v, Ir77PVPass::Opaque, view_opaque);
            m_paint->SetDraws(w, v, Ir77PVPass::Transparent, view_transparent);

            report.opaque_items += static_cast<std::uint32_t>(view_opaque.size());
            report.transparent_items += static_cast<std::uint32_t>(view_transparent.size());
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Every asset in the frame: one opaque draw per stream (each command finds its material in Geometry's command material table -- the item
    // carries it: pc.user[4] = table, pc.user[5] = first command in the pool, gl_DrawID = the command within the draw), and per shadow tile the same streams
    // depth-only. The mesh path's static stream has no vertex / index buffers, so on that path only skinned geometry casts. The same lists serve every window.
    std::shared_ptr<IIr77Return const> MakeOpaqueDraws(std::uint32_t const& shadow_count, std::vector<Ir77PVDrawItem>& opaque,
                                                       std::vector<Ir77PVDrawItem>& shadow) {
        std::shared_ptr<IIr77Return const> result{};

        for (std::uint32_t const id : m_present) {
            Ir77PVWorldAsset const& asset = m_assets.at(id);

            Ir77PVDrawItem item{};

            if (asset.has_static && asset.mesh_path) {
                // mesh shading -- one draw for every instance; each meshlet carries its own material row
                if (Failed(m_geometry->MakeMeshletDrawItem(0, asset.static_id, m_default_material, 0, asset.static_count, item), result)) return result;

                opaque.push_back(item);
            } else if (asset.has_static) {
                if (Failed(m_geometry->MakeIndirectDrawItem(0, asset.pipeline, asset.static_id, m_default_material, 0, asset.static_count, false, item),
                           result))
                    return result;

                opaque.push_back(item);
            }

            if (asset.has_posed) {
                // compute-skinned -- the posed stream through the static pipeline, one command per (primitive, instance)
                if (Failed(
                        m_geometry->MakeIndirectDrawItem(0, Ir77PVPipelineKind::Static, asset.posed_id, m_default_material, 0, asset.posed_count, false, item),
                        result))
                    return result;

                opaque.push_back(item);
            } else if (asset.skinned_opaque_count > 0) {
                if (Failed(m_geometry->MakeIndirectDrawItem(0, Ir77PVPipelineKind::Skinned, asset.skinned_id, m_default_material, 0, asset.skinned_opaque_count,
                                                            true, item),
                           result))
                    return result;

                opaque.push_back(item);
            }

            for (std::uint32_t tile = 0; tile < shadow_count; tile++) {
                if (asset.has_static && !asset.mesh_path) {
                    if (Failed(m_geometry->MakeIndirectDrawItem(0, Ir77PVPipelineKind::Shadow, asset.static_id, m_default_material, 0, asset.static_count,
                                                                false, item),
                               result))
                        return result;

                    item.light_index = tile;
                    shadow.push_back(item);
                }

                if (asset.has_posed) {
                    if (Failed(m_geometry->MakeIndirectDrawItem(0, Ir77PVPipelineKind::Shadow, asset.posed_id, m_default_material, 0, asset.posed_count, false,
                                                                item),
                               result))
                        return result;

                    item.light_index = tile;
                    shadow.push_back(item);
                } else if (asset.skinned_opaque_count > 0) {
                    if (Failed(m_geometry->MakeIndirectDrawItem(0, Ir77PVPipelineKind::ShadowSkinned, asset.skinned_id, m_default_material, 0,
                                                                asset.skinned_opaque_count, true, item),
                               result))
                        return result;

                    item.light_index = tile;
                    shadow.push_back(item);
                }
            }
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Each outlined asset's streams again through the outline pipelines. items (the opaque list, after the assets'): static / posed through
    // outline.static, vertex-skinned through outline.skinned, the mesh path through outline.mesh (keeping its back-facing meshlets).
    // blend_items (the transparent list, after every surface): each BLEND surface's mask (outline.mask.*), then their shells. pc.user[5] = the
    // colour, RGBA8. A kind with no pipeline in the manifest, or a stream a draw can't be made for, is left out -- an outline is never worth
    // failing a frame over.
    void MakeOutlineDraws(std::vector<Ir77PVDrawItem>& items, std::vector<Ir77PVDrawItem>& blend_items) {
        items.clear();
        blend_items.clear();
        if (m_outlines.empty()) return;

        Ir77PVPipelineHandle const static_pipeline = OutlinePipeline(OUTLINE_STATIC_PIPELINE, m_outline_static);
        Ir77PVPipelineHandle const skinned_pipeline = OutlinePipeline(OUTLINE_SKINNED_PIPELINE, m_outline_skinned);
        Ir77PVPipelineHandle const mesh_pipeline = OutlinePipeline(OUTLINE_MESH_PIPELINE, m_outline_mesh);
        Ir77PVPipelineHandle const mask_static_pipeline = OutlinePipeline(OUTLINE_MASK_STATIC_PIPELINE, m_outline_mask_static);
        Ir77PVPipelineHandle const mask_skinned_pipeline = OutlinePipeline(OUTLINE_MASK_SKINNED_PIPELINE, m_outline_mask_skinned);

        std::vector<Ir77PVDrawItem> blend_shells{};

        for (auto const& [id, colour] : m_outlines) {
            auto const found = m_assets.find(id);
            if (found == m_assets.end()) continue;

            Ir77PVWorldAsset const& asset = found->second;

            glm::uvec4 const c{glm::clamp(colour, 0.0f, 1.0f) * 255.0f + 0.5f};
            std::uint32_t const packed = c.r | (c.g << 8) | (c.b << 16) | (c.a << 24);  // packUnorm4x8's order -- r in the low byte

            // both sides, whatever the material says
            auto const add = [&](std::vector<Ir77PVDrawItem>& list, Ir77PVDrawItem& item, std::uint32_t const flags) {
                item.double_sided = false;
                item.back_faces = false;
                item.user = {flags, packed, 0u, 0u};
                list.push_back(item);
            };

            Ir77PVDrawItem item{};

            if (asset.has_static && asset.mesh_path) {
                if (mesh_pipeline != IR77PV_NO_PIPELINE &&
                    m_geometry->MakeMeshletDrawItem(0, mesh_pipeline, asset.static_id, m_default_material, 0, asset.static_count, item)->ID() ==
                        GUIDIr77OperationSucceeded)
                    add(items, item, OUTLINE_KEEP_BACK_MESHLETS);
            } else if (asset.has_static && static_pipeline != IR77PV_NO_PIPELINE) {
                if (m_geometry->MakeIndirectDrawItem(0, static_pipeline, asset.static_id, m_default_material, 0, asset.static_count, false, item)->ID() ==
                    GUIDIr77OperationSucceeded)
                    add(items, item, 0u);
            }

            if (asset.has_posed) {
                if (static_pipeline != IR77PV_NO_PIPELINE &&
                    m_geometry->MakeIndirectDrawItem(0, static_pipeline, asset.posed_id, m_default_material, 0, asset.posed_count, false, item)->ID() ==
                        GUIDIr77OperationSucceeded)
                    add(items, item, 0u);
            } else if (asset.skinned_opaque_count > 0 && skinned_pipeline != IR77PV_NO_PIPELINE) {
                if (m_geometry->MakeIndirectDrawItem(0, skinned_pipeline, asset.skinned_id, m_default_material, 0, asset.skinned_opaque_count, true, item)
                        ->ID() == GUIDIr77OperationSucceeded)
                    add(items, item, 0u);
            }

            // BLEND surfaces -- static ones in the blend block, skinned ones in the skinned block past its opaque commands
            for (Ir77PVWorldBlend const& draw : asset.blends) {
                std::uint32_t const block = draw.skinned ? asset.skinned_id : asset.blend_id;
                Ir77PVPipelineHandle const mask = draw.skinned ? mask_skinned_pipeline : mask_static_pipeline;
                Ir77PVPipelineHandle const shell = draw.skinned ? skinned_pipeline : static_pipeline;
                if (mask == IR77PV_NO_PIPELINE || shell == IR77PV_NO_PIPELINE) continue;

                if (m_geometry->MakeIndirectDrawItem(0, mask, block, draw.material_id, draw.command, 1, draw.skinned, item)->ID() == GUIDIr77OperationSucceeded)
                    add(blend_items, item, OUTLINE_MASK);

                if (m_geometry->MakeIndirectDrawItem(0, shell, block, draw.material_id, draw.command, 1, draw.skinned, item)->ID() == GUIDIr77OperationSucceeded)
                    add(blend_shells, item, 0u);
            }
        }

        // every mask before any shell -- one surface's shell mustn't show through another's
        blend_items.insert(blend_items.end(), blend_shells.begin(), blend_shells.end());
    }

    // An outline pipeline's handle, looked up once -- NO_PIPELINE when the manifest hasn't got it (FindPipeline issues a handle for any
    // name, so the manifest entry is what says it's there)
    Ir77PVPipelineHandle OutlinePipeline(char const* const name, Ir77PVOutlinePipeline& cached) {
        if (cached.looked) return cached.handle;
        cached.looked = true;

        Ir77PVPipelineDesc desc{};
        if (m_pipeline_cache->DescribeNamed(name, desc)->ID() != GUIDIr77OperationSucceeded ||
            m_pipeline_cache->FindPipeline(std::string{name}, cached.handle)->ID() != GUIDIr77OperationSucceeded)
            cached.handle = IR77PV_NO_PIPELINE;

        return cached.handle;
    }

    // The cull bins for the assets in the frame -- one per (pipeline, vertex stream), opaque and shadow apart. Static geometry is frustum-tested;
    // the posed / skinned streams aren't (their bind-pose spheres don't follow the animation). The mesh path isn't binned (its task shader
    // culls). Then the cull dispatches for them; with no "cull.compute" pipeline, m_culled stays false and the frame draws unculled.
    std::shared_ptr<IIr77Return const> BuildCullBins() {
        std::shared_ptr<IIr77Return const> result{};

        m_cull_keys.clear();
        m_cull_dispatches.clear();
        m_culled = false;

        std::vector<Ir77PVCullBin> bins{};

        auto const bin = [&](Ir77PVPipelineHandle const pipeline, bool const skinned, bool const shadow) -> Ir77PVCullBin& {
            for (std::size_t b = 0; b < m_cull_keys.size(); b++)
                if (m_cull_keys[b].pipeline == pipeline && m_cull_keys[b].skinned == skinned && m_cull_keys[b].shadow == shadow) return bins[b];

            m_cull_keys.push_back({pipeline, skinned, shadow});
            bins.push_back(Ir77PVCullBin{{}, shadow});

            return bins.back();
        };

        Ir77PVPipelineHandle static_pipeline{IR77PV_NO_PIPELINE};
        Ir77PVPipelineHandle skinned_pipeline{IR77PV_NO_PIPELINE};
        Ir77PVPipelineHandle shadow_pipeline{IR77PV_NO_PIPELINE};
        Ir77PVPipelineHandle shadow_skinned_pipeline{IR77PV_NO_PIPELINE};

        m_pipeline_cache->FindPipeline(Ir77PVPipelineKind::Static, static_pipeline);
        m_pipeline_cache->FindPipeline(Ir77PVPipelineKind::Skinned, skinned_pipeline);
        m_pipeline_cache->FindPipeline(Ir77PVPipelineKind::Shadow, shadow_pipeline);
        m_pipeline_cache->FindPipeline(Ir77PVPipelineKind::ShadowSkinned, shadow_skinned_pipeline);

        for (std::uint32_t const id : m_present) {
            Ir77PVWorldAsset const& asset = m_assets.at(id);

            if (asset.has_static && !asset.mesh_path) {
                bin(asset.pipeline, false, false).runs.push_back({asset.static_id, 0, asset.static_count, true});
                bin(shadow_pipeline, false, true).runs.push_back({asset.static_id, 0, asset.static_count, false});
            }

            if (asset.has_posed) {
                bin(static_pipeline, false, false).runs.push_back({asset.posed_id, 0, asset.posed_count, false});
                bin(shadow_pipeline, false, true).runs.push_back({asset.posed_id, 0, asset.posed_count, false});
            } else if (asset.skinned_opaque_count > 0) {
                bin(skinned_pipeline, true, false).runs.push_back({asset.skinned_id, 0, asset.skinned_opaque_count, false});
                bin(shadow_skinned_pipeline, true, true).runs.push_back({asset.skinned_id, 0, asset.skinned_opaque_count, false});
            }
        }

        if (Failed(m_geometry->SetCullBins(bins), result)) return result;

        // no cull.compute -- drawing unculled (Geometry said why)
        m_culled = m_geometry->GetCullDispatches(m_cull_dispatches)->ID() == GUIDIr77OperationSucceeded;

        m_paint->SetViewDispatches(m_culled ? m_cull_dispatches : std::vector<Ir77PVDispatch>{});

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // GPU culling: per view, each opaque bin's culled list (one indirect-count draw) and the mesh-path assets' meshlet draws (their task shader
    // culls); per shadow tile, each shadow bin's list -- view 0's.
    std::shared_ptr<IIr77Return const> MakeCulledDraws(std::uint32_t const& shadow_count, std::uint32_t const& views,
                                                       std::vector<std::vector<Ir77PVDrawItem>>& opaque, std::vector<Ir77PVDrawItem>& shadow) {
        std::shared_ptr<IIr77Return const> result{};

        opaque.assign(views, {});

        std::vector<Ir77PVDrawItem> meshlets{};

        for (std::uint32_t const id : m_present) {
            Ir77PVWorldAsset const& asset = m_assets.at(id);
            if (!asset.has_static || !asset.mesh_path) continue;

            Ir77PVDrawItem item{};
            if (Failed(m_geometry->MakeMeshletDrawItem(0, asset.static_id, m_default_material, 0, asset.static_count, item), result)) return result;

            meshlets.push_back(item);
        }

        for (std::uint32_t b = 0; b < m_cull_keys.size(); b++) {
            Ir77PVWorldBin const& key = m_cull_keys[b];

            Ir77PVDrawItem item{};

            if (key.shadow) {
                for (std::uint32_t tile = 0; tile < shadow_count; tile++) {
                    if (Failed(m_geometry->MakeCulledDrawItem(key.pipeline, b, 0, key.skinned, item), result)) return result;

                    item.light_index = tile;
                    shadow.push_back(item);
                }

                continue;
            }

            for (std::uint32_t v = 0; v < views; v++) {
                if (Failed(m_geometry->MakeCulledDrawItem(key.pipeline, b, v, key.skinned, item), result)) return result;

                opaque[v].push_back(item);
            }
        }

        for (std::vector<Ir77PVDrawItem>& list : opaque) list.insert(list.end(), meshlets.begin(), meshlets.end());

        return Ir77RETURN<Ir77OperationSucceeded>();  // not result -- an empty scene calls nothing above, and it would still be null
    }

    // Every transparent object of every asset in the frame, back to front from this camera's eye -- two draws each: far side first (front
    // faces culled), then near side. One pipeline; the cull flips per draw.
    std::shared_ptr<IIr77Return const> MakeTransparentDraws(Ir77PVCamera const& camera, std::vector<Ir77PVDrawItem>& items) {
        std::shared_ptr<IIr77Return const> result{};

        glm::vec3 const eye = glm::vec3(glm::inverse(camera.view)[3]);

        // every candidate, then sorted far to near
        std::vector<Ir77PVWorldSort> order{};

        for (std::uint32_t const id : m_present) {
            Ir77PVWorldAsset const& asset = m_assets.at(id);

            for (std::size_t b = 0; b < asset.blends.size(); b++) {
                std::uint32_t const slot = asset.blends[b].slot;
                glm::vec3 const position = slot < m_models.size() ? glm::vec3(m_models[slot][3]) : glm::vec3(0.0f);

                order.push_back({glm::dot(position - eye, position - eye), id, b});
            }
        }

        std::stable_sort(order.begin(), order.end(), [](Ir77PVWorldSort const& a, Ir77PVWorldSort const& b) { return a.distance > b.distance; });

        items.reserve(order.size() * 2);

        for (Ir77PVWorldSort const& entry : order) {
            Ir77PVWorldAsset const& asset = m_assets.at(entry.asset);
            Ir77PVWorldBlend const& draw = asset.blends[entry.blend];

            std::uint32_t const block = draw.skinned ? asset.skinned_id : asset.blend_id;
            Ir77PVPipelineKind const kind{draw.skinned ? Ir77PVPipelineKind::SkinnedTransparent : Ir77PVPipelineKind::Transparent};

            for (bool const back_faces : {true, false}) {
                Ir77PVDrawItem item{};
                if (Failed(m_geometry->MakeIndirectDrawItem(0, kind, block, draw.material_id, draw.command, 1, draw.skinned, item), result)) return result;

                item.back_faces = back_faces;

                items.push_back(item);
            }
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // A window's areas when the frame gives none: one view fills it; 2 to 4 tile it 2 x 2, left to right then top to bottom
    std::vector<glm::vec4> GridAreas(std::uint32_t const views) const {
        if (views == 1) return {glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)};

        std::vector<glm::vec4> areas{};
        for (std::uint32_t v = 0; v < views; v++) areas.push_back(glm::vec4(0.5f * static_cast<float>(v % 2), 0.5f * static_cast<float>(v / 2), 0.5f, 0.5f));

        return areas;
    }

    // What a window records for these views: Compute, the Shadow pass (view 0's shadow list -- the lights are the same for every view), one
    // swapchain pass per view on its area, then the overlay over the whole window. A single view that fills the window composites the overlay
    // itself; otherwise the overlay gets a whole-window pass of its own after the views.
    std::vector<Ir77PVPassDesc> MakePassList(std::vector<glm::vec4> const& areas) const {
        Ir77PVPassDesc compute{};
        compute.type = Ir77PVPassType::Compute;

        Ir77PVPassDesc shadow{};
        shadow.target = Ir77PVPassTarget::Shadow;
        shadow.formats = Ir77PVRenderPassKind::Shadow;
        shadow.lists = {Ir77PVPass::Shadow};

        std::vector<Ir77PVPassDesc> passes{compute, shadow};

        bool const whole{areas.size() == 1 && areas[0] == glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)};

        for (std::uint32_t v = 0; v < areas.size(); v++) {
            Ir77PVPassDesc pass{};
            pass.target = Ir77PVPassTarget::Swapchain;
            pass.formats = Ir77PVRenderPassKind::Main;
            pass.lists = {Ir77PVPass::Opaque, Ir77PVPass::Transparent};
            pass.view = v;
            pass.area = areas[v];
            pass.overlay = whole;

            passes.push_back(pass);
        }

        if (!whole) {
            Ir77PVPassDesc overlay{};
            overlay.target = Ir77PVPassTarget::Swapchain;
            overlay.formats = Ir77PVRenderPassKind::Main;
            overlay.overlay = true;

            passes.push_back(overlay);
        }

        return passes;
    }

    // 64 threads a group, spread over x and y -- skin.comp rebuilds the flat index
    std::array<std::uint32_t, 3> SkinGroups(std::uint64_t const threads) const {
        std::uint64_t const groups = std::max<std::uint64_t>((threads + 63) / 64, 1);
        std::uint64_t const x = std::min<std::uint64_t>(groups, 65535);

        return {static_cast<std::uint32_t>(x), static_cast<std::uint32_t>((groups + x - 1) / x), 1u};
    }

   private:
    // wiring -- Ir77PVInit
    std::shared_ptr<IIr77PVTransfer> m_transfer{};

    std::shared_ptr<IIr77PVMaterial> m_materials{};

    std::shared_ptr<IIr77PVGeometry> m_geometry{};

    std::shared_ptr<IIr77PVPipelineCache> m_pipeline_cache{};

    std::shared_ptr<IIr77PVPaint> m_paint{};

    std::shared_ptr<IIr77PVAcceleration> m_acceleration{};  // ray-query devices only

    std::string m_device_name{};

    std::uint32_t m_default_material{0};

    // assets
    std::map<std::uint32_t, Ir77PVWorldAsset> m_assets{};

    std::uint32_t m_next_asset{0};

    std::map<std::uint32_t, std::uint32_t> m_instance_ranges{};  // models[] first -> count, one per asset

    std::map<std::uint32_t, std::uint32_t> m_bone_ranges{};  // joint matrix first -> count, one per asset

    // per frame
    std::size_t m_windows{0};  // the last frame's -- UnloadAsset clears their draws

    std::vector<std::vector<glm::vec4>> m_window_areas{};  // [window] -- the view areas its pass list was built for (none yet = empty)

    std::vector<std::uint8_t> m_cameras_packed{};  // one window's cameras, VIEW_STRIDE apart

    std::vector<std::uint32_t> m_present{};  // the assets in this frame, sorted

    // the frame's outlined assets and their colours, and their draws (every view's opaque list, after the assets')
    std::vector<std::pair<std::uint32_t, glm::vec4>> m_outlines{};

    std::vector<Ir77PVDrawItem> m_outline_draws{};

    std::vector<Ir77PVDrawItem> m_outline_blend_draws{};  // the BLEND surfaces' masks then shells -- every view's transparent list, after the surfaces

    // the outline pipelines, looked up on first use
    Ir77PVOutlinePipeline m_outline_static{};
    Ir77PVOutlinePipeline m_outline_skinned{};
    Ir77PVOutlinePipeline m_outline_mesh{};
    Ir77PVOutlinePipeline m_outline_mask_static{};
    Ir77PVOutlinePipeline m_outline_mask_skinned{};

    // GPU culling
    bool m_culled{false};  // the bins are live and the cull pass runs -- false: no cull.compute, draw unculled

    bool m_cull_dirty{true};  // an asset loaded / unloaded -- the bins again

    std::vector<std::uint32_t> m_cull_present{};  // the assets the bins were made for

    std::vector<Ir77PVWorldBin> m_cull_keys{};  // [bin] what draws it

    std::vector<Ir77PVDispatch> m_cull_dispatches{};  // the cull pass -- every window, once per view

    std::vector<glm::mat4> m_models{};  // models[] -- up to the highest range in use

    std::uint32_t m_bone_high{0};  // joint matrices up to the highest range in use

    std::vector<std::uint8_t> m_bones_packed{};  // bone_base[WORLD_INSTANCE_CAPACITY] + palettes, as set 0 binding 4 expects

    std::vector<std::uint8_t> m_lights_packed{};  // Ir77PVLightsHeader + Ir77PVLight[], as set 0 binding 1 expects
};
}  // namespace NSIr77PeregrineV