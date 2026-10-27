#pragma once

#include <glm/glm.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <numeric>
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
//   startup:   Ir77PVInit wires it -- SetTransfer / SetMaterials / SetGeometry / SetPipelineCache / SetPaint / [SetAcceleration] -> CreateWorld
//   load:      LoadAsset(bundle) -- textures and materials (Materials), BLEND primitives split off into their own compacted scene, the opaque
//              ones sorted into material runs, one material-row table per stream (Transfer), the static stream on the mesh or vertex path,
//              the skinned stream and its compute-skinned copy (the posed stream), BLASes with MASK alpha rows (Acceleration)
//   per frame: SubmitFrame(frame) -- models[], bone bases + palettes, lights, shadow matrices and each window's camera (Paint frame data),
//              skinning then the frame's dispatches, every window's opaque / shadow / sorted transparent draws, the TLAS transforms and which
//              assets are in the rays
//   unload:    UnloadAsset -- BLASes, scenes and tables freed, its ranges back
//
// Every asset gets its own range of models[] slots and joint matrices (first fit), so a scene is any number of files. LoadAsset moves the
// bundle's instance slots there; SubmitFrame writes each asset's transforms and bones into its ranges.
// =============================================================================================================================================
class Ir77PVWorld : public Ir77Enlisted, public IIr77PVWorld, public std::enable_shared_from_this<Ir77PVWorld> {
   private:
    // one transparent object -- a single indirect command, sorted by camera distance every frame
    struct Ir77PVWorldBlend {
        std::uint32_t material_id{0};
        std::uint32_t command{0};  // in the asset's blend scene, or in its skinned scene when skinned
        std::uint32_t slot{0};     // models[] slot -- its translation is the sort position
        bool skinned{false};
    };

    // one loaded asset -- what it put on the device and where it sits in models[] and the joint matrices
    struct Ir77PVWorldAsset {
        std::string name{};

        std::uint32_t instance_base{0};
        std::uint32_t instance_count{0};
        std::uint32_t bone_base{0};
        std::uint32_t bone_count{0};
        std::vector<std::uint32_t> bone_bases{};  // per instance, into the asset's own bones

        Ir77PVPipelineHandle pipeline{IR77PV_NO_PIPELINE};  // static opaque

        std::vector<std::uint32_t> texture_ids{};  // [bundle texture] -> Materials id; IR77PV_NO_TEXTURE where the upload failed

        std::vector<std::uint32_t> material_ids{};  // [bundle material] -> Materials id

        // static opaque -- vertex path (UploadScene) or mesh path (UploadMeshlets)
        bool has_static{false};
        bool mesh_path{false};
        std::uint32_t static_id{0};
        std::uint32_t static_count{0};
        std::uint32_t static_table_buffer{0};
        std::uint32_t static_table{BINDLESS_NO_SLOT};

        // static transparent -- its own compacted vertex-path scene
        bool has_blend{false};
        std::uint32_t blend_id{0};
        std::uint32_t blend_table_buffer{0};
        std::uint32_t blend_table{BINDLESS_NO_SLOT};

        // skinned -- opaque commands, then the transparent ones
        bool has_skinned{false};
        std::uint32_t skinned_id{0};
        std::uint32_t skinned_opaque_count{0};
        std::uint32_t skinned_table_buffer{0};
        std::uint32_t skinned_table{BINDLESS_NO_SLOT};

        // compute skinning -- the skinned opaque geometry posed by skin.comp every frame, drawn as static
        bool has_posed{false};
        std::uint32_t posed_id{0};
        std::uint32_t posed_count{0};
        std::uint32_t posed_vertices{0};
        std::uint32_t posed_table_buffer{0};
        std::uint32_t posed_table{BINDLESS_NO_SLOT};
        std::uint32_t jobs_buffer{0};
        std::uint32_t jobs_slot{BINDLESS_NO_SLOT};  // uvec2 per posed vertex: source vertex, models[] slot
        std::uint32_t skin_source{BINDLESS_NO_SLOT};
        std::uint32_t skin_destination{BINDLESS_NO_SLOT};
        Ir77PVPipelineHandle skin_compute{IR77PV_NO_PIPELINE};

        std::vector<Ir77PVWorldBlend> blends{};

        // rays -- the BLASes of each stream, and whether they are in the TLAS now
        std::vector<glm::uvec2> objects{};
        bool in_rays{true};
    };

    // one transparent object's place in a window's back-to-front order
    struct Ir77PVWorldSort {
        float distance{0.0f};
        std::uint32_t asset{0};
        std::size_t blend{0};
    };

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

    // The grey default material -- primitives without one, and every procedural draw. device = the GPU's name, for the asset reports.
    std::shared_ptr<IIr77Return const> CreateWorld(std::string const& device) {
        if (!m_materials) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVWorld: SetMaterials not called.");

        m_device = device;

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
            return Ir77RETURN<Ir77OutOfRange>(this, "Ir77PVWorld: " + bundle.name + " -- no room for " + std::to_string(asset.bone_count) +
                                                        " more joint matrices (" + std::to_string(WORLD_BONE_CAPACITY) + " in all).");
        }

        if (Failed(UploadAsset(bundle, asset, report), result)) {
            ReleaseAsset(asset);
            return result;
        }

        asset_id = m_next_asset++;
        m_assets.emplace(asset_id, std::move(asset));

        ResizeFrameData();

        report.device = m_device;
        report.ray_query = static_cast<bool>(m_acceleration);
        report.instance_base = m_assets.at(asset_id).instance_base;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Ir77PVWorld: asset " + std::to_string(asset_id) + " (" + bundle.name + ") -- models[" +
                                                            std::to_string(report.instance_base) + ".." +
                                                            std::to_string(report.instance_base + bundle.instance_count) + ").");
    }

    // Its BLASes, scenes and tables freed, its models[] and joint ranges back. The draw lists are cleared now -- they name its buffers --
    // and the next SubmitFrame refills them.
    std::shared_ptr<IIr77Return const> UnloadAsset(std::uint32_t const& asset_id) {
        auto const found = m_assets.find(asset_id);
        if (found == m_assets.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVWorld: unknown asset.");

        if (m_paint) {
            for (std::size_t w = 0; w < m_windows; w++) m_paint->ClearDraws(w);
            m_paint->SetFrameDispatches({});
        }

        ReleaseAsset(found->second);

        m_assets.erase(found);

        ResizeFrameData();

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Ir77PVWorld: asset " + std::to_string(asset_id) + " unloaded.");
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // per frame
    // -------------------------------------------------------------------------------------------------------------------------------------

    // The frame -> Paint and Acceleration:
    //   1. models[] and the bones from each asset in the frame; assets left out drop out of the rays
    //   2. skinning, then the frame's own dispatches -- once a frame
    //   3. per window: camera, models[], lights, shadow matrices, bones; opaque and shadow draws (the same for every window), then the
    //      transparent objects back to front from that window's eye; the frame's own draws after the assets' in each list
    std::shared_ptr<IIr77Return const> SubmitFrame(Ir77PVFrame const& frame, Ir77PVFrameReport& report) {
        report = Ir77PVFrameReport{};

        if (!m_paint || !m_geometry) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVWorld: not wired.");

        std::shared_ptr<IIr77Return const> result{};

        m_windows = frame.windows.size();

        // 1.
        m_present.clear();

        for (Ir77PVFrameAsset const& entry : frame.assets) {
            auto const found = m_assets.find(entry.asset);
            if (found == m_assets.end()) continue;

            Ir77PVWorldAsset const& asset = found->second;
            m_present.push_back(entry.asset);

            std::size_t const transforms = std::min<std::size_t>(entry.transforms.size(), asset.instance_count);
            std::copy_n(entry.transforms.begin(), transforms, m_models.begin() + asset.instance_base);

            std::size_t const bones = std::min<std::size_t>(entry.bones.size(), asset.bone_count);
            if (bones > 0)
                std::memcpy(m_bones_packed.data() + BONE_BASE_BYTES + static_cast<std::size_t>(asset.bone_base) * sizeof(glm::mat4), entry.bones.data(),
                            bones * sizeof(glm::mat4));
        }

        std::sort(m_present.begin(), m_present.end());
        m_present.erase(std::unique(m_present.begin(), m_present.end()), m_present.end());

        if (m_acceleration) {
            for (auto& [id, asset] : m_assets) {
                bool const present = std::binary_search(m_present.begin(), m_present.end(), id);
                if (present == asset.in_rays) continue;

                for (glm::uvec2 const& objects : asset.objects) m_acceleration->SetObjectsVisible(objects, present);
                asset.in_rays = present;
            }

            m_acceleration->SetInstanceTransforms(m_models);
        }

        // 2.
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

        m_paint->SetFrameDispatches(dispatches);

        // 3. -- lights: header + Ir77PVLight[], as set 0 binding 1 expects
        Ir77PVLightsHeader header{};
        header.count = static_cast<std::uint32_t>(frame.lights.size());

        m_lights_packed.resize(sizeof(header) + frame.lights.size() * sizeof(Ir77PVLight));
        std::memcpy(m_lights_packed.data(), &header, sizeof(header));
        if (!frame.lights.empty()) std::memcpy(m_lights_packed.data() + sizeof(header), frame.lights.data(), frame.lights.size() * sizeof(Ir77PVLight));

        std::uint32_t const shadow_count = static_cast<std::uint32_t>(std::min<std::size_t>(frame.shadow_matrices.size(), MAX_SHADOW_LIGHTS));

        std::vector<Ir77PVDrawItem> opaque{};
        std::vector<Ir77PVDrawItem> shadow{};
        if (Failed(MakeOpaqueDraws(shadow_count, opaque, shadow), result)) return result;

        for (std::size_t w = 0; w < frame.windows.size(); w++) {
            Ir77PVFrameWindow const& window = frame.windows[w];

            m_paint->SetFrameData(w, Ir77PVBufferSlot::Camera, &window.camera, sizeof(Ir77PVCamera));
            m_paint->SetFrameData(w, Ir77PVBufferSlot::Lights, m_lights_packed.data(), m_lights_packed.size());

            if (!m_models.empty()) m_paint->SetFrameData(w, Ir77PVBufferSlot::Instances, m_models.data(), m_models.size() * sizeof(glm::mat4));
            if (shadow_count > 0)
                m_paint->SetFrameData(w, Ir77PVBufferSlot::ShadowMatrices, frame.shadow_matrices.data(), shadow_count * sizeof(glm::mat4));
            if (m_bone_high > 0) m_paint->SetFrameData(w, Ir77PVBufferSlot::Bones, m_bones_packed.data(), m_bones_packed.size());

            std::vector<Ir77PVDrawItem> window_opaque = opaque;
            window_opaque.insert(window_opaque.end(), window.opaque.begin(), window.opaque.end());

            std::vector<Ir77PVDrawItem> window_shadow = shadow;
            window_shadow.insert(window_shadow.end(), window.shadow.begin(), window.shadow.end());

            std::vector<Ir77PVDrawItem> window_transparent{};
            if (Failed(MakeTransparentDraws(window.camera, window_transparent), result)) return result;
            window_transparent.insert(window_transparent.end(), window.transparent.begin(), window.transparent.end());

            m_paint->SetDraws(w, Ir77PVPass::Opaque, window_opaque);
            m_paint->SetDraws(w, Ir77PVPass::Shadow, window_shadow);
            m_paint->SetDraws(w, Ir77PVPass::Transparent, window_transparent);

            report.opaque_items = static_cast<std::uint32_t>(window_opaque.size());
            report.shadow_items = static_cast<std::uint32_t>(window_shadow.size());
            report.transparent_items = static_cast<std::uint32_t>(window_transparent.size());
        }

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

    // Textures, materials, the streams and their material tables, the BLASes -- in that order. Fills asset as it goes, so a failure leaves
    // exactly what ReleaseAsset has to free.
    std::shared_ptr<IIr77Return const> UploadAsset(Ir77PVAssetBundle const& bundle, Ir77PVWorldAsset& asset, Ir77PVAssetReport& report) {
        std::shared_ptr<IIr77Return const> result{};

        asset.pipeline = bundle.pipeline;
        if (asset.pipeline == IR77PV_NO_PIPELINE && Failed(m_pipeline_cache->FindPipeline(Ir77PVPipelineKind::Static, asset.pipeline), result)) return result;

        // textures -- bundle index -> Materials id; IR77PV_NO_TEXTURE where the upload failed (the placeholder stands in)
        std::vector<std::uint32_t>& texture_ids = asset.texture_ids;
        texture_ids.assign(bundle.textures.size(), IR77PV_NO_TEXTURE);

        for (std::size_t t = 0; t < bundle.textures.size(); t++) {
            Ir77PVAssetTexture const& texture = bundle.textures[t];
            if (texture.rgba.empty()) continue;

            if (m_materials->CreateTexture(texture.rgba.data(), texture.width, texture.height, texture.srgb, texture_ids[t])->ID() != GUIDIr77OperationSucceeded)
                texture_ids[t] = IR77PV_NO_TEXTURE;
        }

        // materials -- one batch, bundle index -> Materials id
        auto const texture_id = [&](std::uint32_t const& index) { return index < texture_ids.size() ? texture_ids[index] : IR77PV_NO_TEXTURE; };

        if (!bundle.materials.empty()) {
            std::vector<Ir77PVMaterialConstants> constants(bundle.materials.size());
            std::vector<Ir77PVMaterialTextures> textures(bundle.materials.size());

            for (std::size_t m = 0; m < bundle.materials.size(); m++) {
                constants[m] = bundle.materials[m].constants;

                Ir77PVMaterialTextures const& in = bundle.materials[m].textures;
                textures[m] = {texture_id(in.albedo), texture_id(in.normal), texture_id(in.orm), texture_id(in.emissive), texture_id(in.ao)};
            }

            if (Failed(m_materials->UploadMaterials(constants, textures, asset.material_ids), result)) return result;
        }

        auto const material_id = [&](std::uint32_t const& index) { return MaterialId(asset, index); };
        auto const blend = [&](std::uint32_t const& index) { return index < bundle.materials.size() && bundle.materials[index].blend; };
        auto const mask = [&](std::uint32_t const& index) { return index < bundle.materials.size() && bundle.materials[index].mask; };

        // primitives -> commands sorted by material (opaque) or one each (BLEND), first_instance moved into the asset's models[] range.
        // commands get each command's bundle material in the same order -- the stream's material table.
        auto const command = [&](Ir77MeshRange const& r) {
            return VkDrawIndexedIndirectCommand{r.index_count, r.instance_count, r.index_offset, r.vertex_offset, r.first_instance + asset.instance_base};
        };

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

        // ---- static opaque -- vertex path or mesh shading ----
        Ir77PVSceneUpload upload{};
        upload.vertices = bundle.vertices;
        upload.indices = bundle.indices;

        std::vector<std::uint32_t> static_materials{};
        report.opaque_runs = opaque_runs(bundle.primitives, upload.indirect_commands, static_materials);

        // ---- static transparent -- compacted into its own small vertex-path scene (the mesh path has no Transparent pipeline) ----
        Ir77PVSceneUpload blend_upload{};
        std::vector<std::uint32_t> blend_materials{};

        for (Ir77PVAssetPrimitive const& primitive : bundle.primitives) {
            Ir77MeshRange const& r = primitive.range;
            if (!blend(primitive.material) || r.index_count == 0) continue;

            std::uint32_t highest{0};  // indices are relative to vertex_offset -- the highest one bounds the vertex span
            for (std::uint32_t i = 0; i < r.index_count; i++) highest = std::max(highest, bundle.indices[r.index_offset + i]);

            std::uint32_t const index = static_cast<std::uint32_t>(blend_upload.indirect_commands.size());
            asset.blends.push_back({material_id(primitive.material), index, r.first_instance + asset.instance_base, false});
            blend_materials.push_back(primitive.material);

            blend_upload.indirect_commands.push_back(VkDrawIndexedIndirectCommand{r.index_count, r.instance_count,
                                                                                  static_cast<std::uint32_t>(blend_upload.indices.size()),
                                                                                  static_cast<std::int32_t>(blend_upload.vertices.size()),
                                                                                  r.first_instance + asset.instance_base});

            blend_upload.indices.insert(blend_upload.indices.end(), bundle.indices.begin() + r.index_offset,
                                        bundle.indices.begin() + r.index_offset + r.index_count);
            blend_upload.vertices.insert(blend_upload.vertices.end(), bundle.vertices.begin() + r.vertex_offset,
                                         bundle.vertices.begin() + r.vertex_offset + highest + 1);
        }

        // ---- skinned -- vertex path only: opaque runs, then one command per transparent object (SkinnedTransparent) ----
        Ir77PVSceneUpload skinned_upload{};
        std::vector<std::uint32_t> posed_materials{};  // bundle material per posed command -- its BLASes' alpha rows

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

        // ---- material tables -- one Materials row per indirect command; the vertex shaders read [first + gl_DrawID] ----
        if (Failed(UploadMaterialTable(asset, static_materials, asset.static_table_buffer, asset.static_table), result)) return result;
        if (Failed(UploadMaterialTable(asset, blend_materials, asset.blend_table_buffer, asset.blend_table), result)) return result;
        if (Failed(UploadMaterialTable(asset, skinned_materials, asset.skinned_table_buffer, asset.skinned_table), result)) return result;

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

                std::uint32_t records{0};
                asset.mesh_path = m_geometry->UploadMeshlets(upload, asset.static_id, records, true)->ID() == GUIDIr77OperationSucceeded;
            }

            if (!asset.mesh_path && Failed(m_geometry->UploadScene(upload, asset.static_id, true), result)) return result;

            asset.has_static = true;
        }

        if (!blend_upload.indirect_commands.empty()) {
            if (Failed(m_geometry->UploadScene(blend_upload, asset.blend_id, true), result)) return result;

            asset.has_blend = true;
        }

        if (!skinned_upload.indirect_commands.empty()) {
            if (Failed(m_geometry->UploadScene(skinned_upload, asset.skinned_id, true), result)) return result;

            asset.has_skinned = true;

            if (Failed(UploadPosedStream(skinned_upload, skinned_materials, asset, posed_materials), result)) return result;
        }

        report.mesh_path = asset.mesh_path;

        // ---- rays -- one BLAS per object of the static stream (either path) and of the posed stream (refit every frame). MASK materials go
        // in alpha-tested: the material row per command, BINDLESS_NO_SLOT for everything opaque. Transparent objects stay out. ----
        if (!m_acceleration) return Ir77RETURN<Ir77OperationSucceeded>();

        auto const alpha_rows = [&](std::vector<std::uint32_t> const& materials, std::vector<std::uint32_t>& rows) {
            rows.assign(materials.size(), BINDLESS_NO_SLOT);

            for (std::size_t c = 0; c < materials.size(); c++) {
                if (!mask(materials[c])) continue;

                Ir77PVMaterialData material{};
                if (m_materials->GetMaterial(material_id(materials[c]), material)->ID() == GUIDIr77OperationSucceeded) rows[c] = material.row;
            }
        };

        std::vector<std::uint32_t> rows{};
        glm::uvec2 objects{0u};

        if (asset.has_static) {
            alpha_rows(static_materials, rows);

            if (Failed(asset.mesh_path ? m_acceleration->BuildMeshlets(asset.static_id, rows, objects)
                                       : m_acceleration->BuildScene(asset.static_id, false, rows, objects),
                       result))
                return result;

            asset.objects.push_back(objects);
        }

        if (asset.has_posed) {
            alpha_rows(posed_materials, rows);

            if (Failed(m_acceleration->BuildScene(asset.posed_id, true, rows, objects), result)) return result;

            asset.objects.push_back(objects);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Compute skinning -- the skinned opaque commands become the posed stream: one region of plain vertices per (primitive, instance), filled
    // with the bind pose here and posed by skin.comp every frame. It draws through the static and shadow pipelines like any static geometry.
    // No skin.compute pipeline -> the skinned geometry stays on the Skinned pipelines.
    // posed_materials = the bundle material of every posed command.
    std::shared_ptr<IIr77Return const> UploadPosedStream(Ir77PVSceneUpload const& skinned_upload, std::vector<std::uint32_t> const& skinned_materials,
                                                         Ir77PVWorldAsset& asset, std::vector<std::uint32_t>& posed_materials) {
        std::shared_ptr<IIr77Return const> result{};

        posed_materials.clear();
        if (asset.skinned_opaque_count == 0) return Ir77RETURN<Ir77OperationSucceeded>();

        if (m_pipeline_cache->FindPipeline(std::string{"skin.compute"}, asset.skin_compute)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77OperationSucceeded>(this, "Ir77PVWorld: no skin.compute pipeline -- skinned geometry stays on the Skinned pipeline.");

        Ir77PVSceneUpload posed{};
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

        if (Failed(m_geometry->UploadScene(posed, asset.posed_id, true), result)) return result;

        asset.has_posed = true;  // from here on ReleaseAsset unloads it

        if (Failed(m_geometry->GetSceneVertexSlot(asset.posed_id, false, asset.skin_destination), result)) return result;
        if (Failed(m_geometry->GetSceneVertexSlot(asset.skinned_id, true, asset.skin_source), result)) return result;
        if (Failed(m_transfer->CreateStorageBuffer(static_cast<VkDeviceSize>(jobs.size() * sizeof(glm::uvec2)), jobs.data(), asset.jobs_buffer, asset.jobs_slot),
                   result))
            return result;
        if (Failed(UploadMaterialTable(asset, posed_materials, asset.posed_table_buffer, asset.posed_table), result)) return result;

        asset.posed_count = static_cast<std::uint32_t>(posed.indirect_commands.size());
        asset.posed_vertices = static_cast<std::uint32_t>(posed.vertices.size());

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Bundle material per indirect command -> Materials rows, as a storage buffer the vertex shaders index. No commands -> no buffer, slot
    // stays BINDLESS_NO_SLOT (and ReleaseAsset leaves it alone).
    std::shared_ptr<IIr77Return const> UploadMaterialTable(Ir77PVWorldAsset const& asset, std::vector<std::uint32_t> const& materials, std::uint32_t& buffer_id,
                                                           std::uint32_t& slot) {
        std::shared_ptr<IIr77Return const> result{};

        if (materials.empty()) return Ir77RETURN<Ir77OperationSucceeded>();

        std::vector<std::uint32_t> rows(materials.size(), 0);

        for (std::size_t c = 0; c < materials.size(); c++) {
            Ir77PVMaterialData material{};
            if (Failed(m_materials->GetMaterial(MaterialId(asset, materials[c]), material), result)) return result;

            rows[c] = material.row;
        }

        return m_transfer->CreateStorageBuffer(static_cast<VkDeviceSize>(rows.size() * sizeof(std::uint32_t)), rows.data(), buffer_id, slot);
    }

    // Bundle material -> Materials id; out of range (no material) -> the grey default
    std::uint32_t MaterialId(Ir77PVWorldAsset const& asset, std::uint32_t const& index) const {
        return index < asset.material_ids.size() ? asset.material_ids[index] : m_default_material;
    }

    // Everything an asset put on the device, BLASes first (they read the scenes' buffers), then its materials and textures, and its ranges.
    // Safe on a half-loaded asset. All of it goes through Transfer's graveyard -- freed once no frame in flight can read it.
    void ReleaseAsset(Ir77PVWorldAsset& asset) {
        if (m_acceleration)
            for (glm::uvec2 const& objects : asset.objects) m_acceleration->ReleaseObjects(objects);

        if (asset.has_static) asset.mesh_path ? m_geometry->UnloadMeshlets(asset.static_id) : m_geometry->UnloadScene(asset.static_id);
        if (asset.has_blend) m_geometry->UnloadScene(asset.blend_id);
        if (asset.has_skinned) m_geometry->UnloadScene(asset.skinned_id);
        if (asset.has_posed) m_geometry->UnloadScene(asset.posed_id);

        for (auto const& [buffer, slot] : {std::pair{asset.static_table_buffer, asset.static_table}, std::pair{asset.blend_table_buffer, asset.blend_table},
                                           std::pair{asset.skinned_table_buffer, asset.skinned_table}, std::pair{asset.posed_table_buffer, asset.posed_table},
                                           std::pair{asset.jobs_buffer, asset.jobs_slot}})
            if (slot != BINDLESS_NO_SLOT) m_transfer->DestroyStorageBuffer(buffer);

        if (!asset.material_ids.empty()) m_materials->DestroyMaterials(asset.material_ids);

        for (std::uint32_t const texture : asset.texture_ids)
            if (texture != IR77PV_NO_TEXTURE) m_materials->DestroyTexture(texture);

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
        m_bones_packed.assign(m_bone_high > 0 ? BONE_BASE_BYTES + static_cast<std::size_t>(m_bone_high) * sizeof(glm::mat4) : 0, 0);
        if (m_bone_high == 0) return;

        std::uint32_t* const bases = reinterpret_cast<std::uint32_t*>(m_bones_packed.data());

        for (auto const& [id, asset] : m_assets)
            for (std::uint32_t i = 0; i < asset.instance_count; i++) bases[asset.instance_base + i] = asset.bone_base + asset.bone_bases[i];
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // frame steps
    // -------------------------------------------------------------------------------------------------------------------------------------

    // Every asset in the frame: one opaque draw per stream (each command finds its material in the stream's table: pc.user[4] = table,
    // pc.user[5] = first command, gl_DrawID = the command within the draw), and per shadow tile the same streams depth-only. The mesh
    // path's static stream has no vertex / index buffers, so on that path only skinned geometry casts. The same lists serve every window.
    std::shared_ptr<IIr77Return const> MakeOpaqueDraws(std::uint32_t const& shadow_count, std::vector<Ir77PVDrawItem>& opaque, std::vector<Ir77PVDrawItem>& shadow) {
        std::shared_ptr<IIr77Return const> result{};

        for (std::uint32_t const id : m_present) {
            Ir77PVWorldAsset const& asset = m_assets.at(id);

            Ir77PVDrawItem item{};

            if (asset.has_static && asset.mesh_path) {
                // mesh shading -- one draw for every record; each meshlet carries its own material row
                if (Failed(m_geometry->MakeMeshletSceneDrawItem(0, asset.static_id, m_default_material, 0, asset.static_count, item), result)) return result;

                opaque.push_back(item);
            } else if (asset.has_static) {
                if (Failed(m_geometry->MakeIndirectDrawItem(0, asset.pipeline, asset.static_id, m_default_material, 0, asset.static_count, false, item), result))
                    return result;

                item.user = {asset.static_table, 0u, 0u, 0u};
                opaque.push_back(item);
            }

            if (asset.has_posed) {
                // compute-skinned -- the posed stream through the static pipeline, one command per (primitive, instance)
                if (Failed(m_geometry->MakeIndirectDrawItem(0, Ir77PVPipelineKind::Static, asset.posed_id, m_default_material, 0, asset.posed_count, false,
                                                            item),
                           result))
                    return result;

                item.user = {asset.posed_table, 0u, 0u, 0u};
                opaque.push_back(item);
            } else if (asset.skinned_opaque_count > 0) {
                if (Failed(m_geometry->MakeIndirectDrawItem(0, Ir77PVPipelineKind::Skinned, asset.skinned_id, m_default_material, 0, asset.skinned_opaque_count,
                                                            true, item),
                           result))
                    return result;

                item.user = {asset.skinned_table, 0u, 0u, 0u};
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

            std::uint32_t const scene = draw.skinned ? asset.skinned_id : asset.blend_id;
            Ir77PVPipelineKind const kind{draw.skinned ? Ir77PVPipelineKind::SkinnedTransparent : Ir77PVPipelineKind::Transparent};

            for (bool const back_faces : {true, false}) {
                Ir77PVDrawItem item{};
                if (Failed(m_geometry->MakeIndirectDrawItem(0, kind, scene, draw.material_id, draw.command, 1, draw.skinned, item), result)) return result;

                item.back_faces = back_faces;

                // a single-command draw: gl_DrawID = 0, so the row is at the command itself
                item.user = {draw.skinned ? asset.skinned_table : asset.blend_table, draw.command, 0u, 0u};

                items.push_back(item);
            }
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
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

    std::string m_device{};

    std::uint32_t m_default_material{0};

    // assets
    std::map<std::uint32_t, Ir77PVWorldAsset> m_assets{};

    std::uint32_t m_next_asset{0};

    std::map<std::uint32_t, std::uint32_t> m_instance_ranges{};  // models[] first -> count, one per asset

    std::map<std::uint32_t, std::uint32_t> m_bone_ranges{};  // joint matrix first -> count, one per asset

    // per frame
    std::size_t m_windows{0};  // the last frame's -- UnloadAsset clears their draws

    std::vector<std::uint32_t> m_present{};  // the assets in this frame, sorted

    std::vector<glm::mat4> m_models{};  // models[] -- up to the highest range in use

    std::uint32_t m_bone_high{0};  // joint matrices up to the highest range in use

    std::vector<std::uint8_t> m_bones_packed{};  // bone_base[WORLD_INSTANCE_CAPACITY] + palettes, as set 0 binding 4 expects

    std::vector<std::uint8_t> m_lights_packed{};  // Ir77PVLightsHeader + Ir77PVLight[], as set 0 binding 1 expects

    static constexpr std::size_t BONE_BASE_BYTES{WORLD_INSTANCE_CAPACITY * sizeof(std::uint32_t)};
};
}  // namespace NSIr77PeregrineV