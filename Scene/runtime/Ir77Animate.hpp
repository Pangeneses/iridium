#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <memory>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77Scene.hpp"
#include "../dictionary/IDIr77Scene.hpp"

#include "../../Ir77RT/types/Ir77Types.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"
#include "../../Ir77RT/runtime/Ir77Return.hpp"

#include "../interface/IIr77Camera.hpp"
#include "../interface/IIr77Light.hpp"
#include "../interface/IIr77Mesh.hpp"
#include "../interface/IIr77Material.hpp"
#include "../interface/IIr77Texture.hpp"
#include "../interface/IIr77Animation.hpp"
#include "../interface/IIr77Pose.hpp"
#include "../interface/IIr77View.hpp"
#include "../interface/IIr77Lighting.hpp"
#include "../interface/IIr77Field.hpp"
#include "../interface/IIr77Animate.hpp"

#include "./Ir77Pose.hpp"
#include "./Ir77View.hpp"
#include "./Ir77Lighting.hpp"
#include "./Ir77Field.hpp"

#include "../types/Ir77Stats.hpp"
#include "../types/Ir77Field.hpp"

// the direct link -- Scene -> PeregrineV's server, one way: Scene calls it, PeregrineV never sees Scene
#include "../../PeregrineV/server/Ir77Server.hpp"

using namespace NSIr77RT;
using namespace NSIr77PeregrineV;

namespace NSIr77Scene {

// =============================================================================================================================================
// Ir77Animate -- the scene module's work, owned by its server (Ir77Server) and reached only through it. The lifetime controller for the
// scene's peers -- Pose (the file, its node tree and animation), View (the cameras), Lighting (lights and shadows) and Field (the compute
// visual) -- which it creates and wires; and the one place that talks to all of them at once:
//
//   setup:     SetRenderer / SetViewSize / SetPipeline -> CreateScene(path, use_mesh) -- Pose parses, the geometry / textures / materials go up
//              through the renderer, Lighting packs the lights, View frames the scene
//   per frame: Update(dt) -> CreateFrame(seconds) -- camera (View), transforms and bones (Pose), lights (Lighting), dispatches and draws (Field)
//   the UI:    the scene server reaches the peers through GetPose / GetView / GetField for its ops
// =============================================================================================================================================
class Ir77Animate : public Ir77Enlisted, public IIr77Animate, public std::enable_shared_from_this<Ir77Animate> {
   private:
    // one material run -- contiguous indirect commands sharing a glTF material
    struct Ir77SceneRun {
        std::uint32_t gltf_material{UINT32_MAX};
        std::uint32_t first_command{0};
        std::uint32_t command_count{0};
    };

    // one transparent object -- a single indirect command, sorted by camera distance every frame
    struct Ir77SceneBlendDraw {
        std::uint32_t gltf_material{UINT32_MAX};
        std::uint32_t command{0};         // in the static blend scene, or in the skinned scene when skinned
        std::uint32_t first_instance{0};  // models[] slot -- its translation is the sort position
        bool skinned{false};
    };

   public:
    Ir77Animate() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();

        // the peers -- they never see Animate; the field reports its box to Lighting and reads its shadow count
        m_pose = std::static_pointer_cast<IIr77Pose>(std::make_shared<Ir77Pose>());
        m_view = std::static_pointer_cast<IIr77View>(std::make_shared<Ir77View>());
        m_lighting = std::static_pointer_cast<IIr77Lighting>(std::make_shared<Ir77Lighting>());
        m_field = std::static_pointer_cast<IIr77Field>(std::make_shared<Ir77Field>());

        m_field->SetLighting(m_lighting);
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77Animate>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77Scene>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77Animate)
            obj = std::shared_ptr<IIr77Animate>(shared_from_this(), static_cast<IIr77Animate*>(this));

        else if (iid == GUIDIr77Animate)
            obj = std::shared_ptr<Ir77Animate>(shared_from_this(), static_cast<Ir77Animate*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

   public:
    // The renderer's server -- where the scene's textures, materials and geometry go (CreateScene), which pipelines its draw items use, and
    // where each frame goes (CreateFrame). Field and Lighting get it too. Its forwarding methods are the only way into PeregrineV.
    std::shared_ptr<IIr77Return const> SetRenderer(std::shared_ptr<NSIr77PeregrineV::Ir77Server> const& renderer) {
        m_renderer = renderer;

        m_field->SetRenderer(renderer);
        m_lighting->SetRenderer(renderer);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The windows CreateFrame fills, and their size -- the cameras' aspect. Any time; the next frame uses it.
    std::shared_ptr<IIr77Return const> SetViewSize(std::size_t const& windows, int const& width, int const& height) {
        m_view_windows = windows;
        m_view_width = width;
        m_view_height = height;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // What the static (non-skinned, opaque) runs draw with -- a manifest pipeline or role. Unset -> the "static" role.
    std::shared_ptr<IIr77Return const> SetPipeline(Ir77PVPipelineHandle const& pipeline) {
        m_pipeline = pipeline;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }


    // Once, after CreatePipelines. Parses the glTF, stacks every primitive into one upload, sorts the commands into material runs, uploads
    // textures / materials / geometry through the Asset and packs the lights. use_mesh -> mesh shading for the static stream when the device has it.
    // No geometry is fine: an empty path, a file with nothing drawable, or a file that fails to load all leave a running scene with no
    // geometry. A file that fails to load is logged (and listed in the stats warnings), not a failure.
    std::shared_ptr<IIr77Return const> CreateScene(std::string const& path, bool const& use_mesh) {
        if (!m_renderer) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Animate: SetRenderer not called.");

        // a file that won't load (ParseAsset logged why) -> carry on with an empty scene
        auto result = m_pose->ParseAsset(path);

        bool const load_failed = result->ID() != GUIDIr77OperationSucceeded;
        if (load_failed) m_pose->ParseAsset(std::string{});

        // the file's components -- Pose owns them
        std::shared_ptr<IIr77Mesh> scene_meshes{};
        std::shared_ptr<IIr77Camera> scene_cameras{};
        std::shared_ptr<IIr77Light> scene_lights{};
        std::shared_ptr<IIr77Material> scene_materials{};
        std::shared_ptr<IIr77Texture> scene_textures{};
        std::shared_ptr<IIr77Animation> scene_animations{};

        m_pose->GetMeshes(scene_meshes);
        m_pose->GetCameras(scene_cameras);
        m_pose->GetLights(scene_lights);
        m_pose->GetMaterials(scene_materials);
        m_pose->GetTextures(scene_textures);
        m_pose->GetAnimations(scene_animations);

        if (m_pipeline == IR77PV_NO_PIPELINE) {
            result = m_renderer->FindPipeline(Ir77PVPipelineKind::Static, m_pipeline);
            if (result->ID() != GUIDIr77OperationSucceeded) return result;
        }

        // commands ordered by material; command_materials gets each command's glTF material in the same order -- the stream's per-draw
        // material table. runs are still built for the mesh path, which takes its material per call.
        auto const build_runs = [](std::vector<Ir77MeshRange> const& ranges, std::vector<std::uint32_t> const& materials,
                                   std::vector<VkDrawIndexedIndirectCommand>& commands, std::vector<std::uint32_t>& command_materials,
                                   std::vector<Ir77SceneRun>& runs) {
            std::vector<std::size_t> order(ranges.size());
            std::iota(order.begin(), order.end(), 0);
            std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) { return materials[a] < materials[b]; });

            runs.clear();
            for (std::size_t k = 0; k < order.size(); k++) {
                Ir77MeshRange const& r = ranges[order[k]];
                commands.push_back(VkDrawIndexedIndirectCommand{r.index_count, r.instance_count, r.index_offset, r.vertex_offset, r.first_instance});

                std::uint32_t const material = materials[order[k]];
                command_materials.push_back(material);

                if (runs.empty() || runs.back().gltf_material != material) runs.push_back({material, static_cast<std::uint32_t>(k), 0});
                runs.back().command_count++;
            }
        };

        // glTF material per indirect command, one list per uploaded scene -- become the per-draw material tables
        std::vector<std::uint32_t> static_command_materials{};
        std::vector<std::uint32_t> blend_command_materials{};
        std::vector<std::uint32_t> skinned_command_materials{};

        // static stream -- vertex path or mesh shading
        Ir77PVSceneUpload upload{};
        std::vector<Ir77MeshRange> ranges{};
        std::vector<std::uint32_t> materials{};

        scene_meshes->GetVertexBuffer(upload.vertices);
        scene_meshes->GetIndexBuffer(upload.indices);
        scene_meshes->GetMeshRanges(ranges);
        scene_meshes->GetRangeMaterials(materials);
        scene_meshes->GetInstanceBuffer(m_transforms);
        scene_meshes->GetBounds(m_bounds_min, m_bounds_max);

        // BLEND materials -> Transparent pass (sorted per object each frame); everything else stays in the batched opaque runs
        auto const split_blend = [&](std::vector<Ir77MeshRange>& in_ranges, std::vector<std::uint32_t>& in_materials, std::vector<Ir77MeshRange>& blend_ranges,
                                     std::vector<std::uint32_t>& blend_materials) {
            std::vector<Ir77MeshRange> keep_ranges{};
            std::vector<std::uint32_t> keep_materials{};

            for (std::size_t r = 0; r < in_ranges.size(); r++) {
                bool blend{false};
                scene_materials->GetMaterialBlend(in_materials[r], blend);  // no material / out of range -> stays opaque

                if (blend) {
                    blend_ranges.push_back(in_ranges[r]);
                    blend_materials.push_back(in_materials[r]);
                } else {
                    keep_ranges.push_back(in_ranges[r]);
                    keep_materials.push_back(in_materials[r]);
                }
            }

            in_ranges = std::move(keep_ranges);
            in_materials = std::move(keep_materials);
        };

        m_blend_draws.clear();

        std::vector<Ir77MeshRange> blend_ranges{};
        std::vector<std::uint32_t> blend_materials{};
        split_blend(ranges, materials, blend_ranges, blend_materials);

        build_runs(ranges, materials, upload.indirect_commands, static_command_materials, m_runs);
        m_static_command_count = static_cast<std::uint32_t>(upload.indirect_commands.size());

        // static blend ranges -- compacted into their own small vertex-path scene (the mesh path has no Transparent pipeline)
        Ir77PVSceneUpload blend_upload{};

        for (std::size_t b = 0; b < blend_ranges.size(); b++) {
            Ir77MeshRange const& r = blend_ranges[b];
            if (r.index_count == 0) continue;

            std::uint32_t highest{0};  // indices are relative to vertex_offset -- the highest one bounds the vertex span
            for (std::uint32_t i = 0; i < r.index_count; i++) highest = std::max(highest, upload.indices[r.index_offset + i]);

            m_blend_draws.push_back({blend_materials[b], static_cast<std::uint32_t>(blend_upload.indirect_commands.size()), r.first_instance, false});

            blend_command_materials.push_back(blend_materials[b]);

            blend_upload.indirect_commands.push_back(VkDrawIndexedIndirectCommand{r.index_count, r.instance_count,
                                                                                  static_cast<std::uint32_t>(blend_upload.indices.size()),
                                                                                  static_cast<std::int32_t>(blend_upload.vertices.size()), r.first_instance});

            blend_upload.indices.insert(blend_upload.indices.end(), upload.indices.begin() + r.index_offset,
                                        upload.indices.begin() + r.index_offset + r.index_count);
            blend_upload.vertices.insert(blend_upload.vertices.end(), upload.vertices.begin() + r.vertex_offset,
                                         upload.vertices.begin() + r.vertex_offset + highest + 1);
        }

        // skinned stream -- vertex path only, Skinned pipeline
        Ir77PVSceneUpload skinned_upload{};
        std::vector<Ir77MeshRange> skinned_ranges{};
        std::vector<std::uint32_t> skinned_materials{};

        scene_meshes->GetSkinnedVertexBuffer(skinned_upload.vertices_skinned);
        scene_meshes->GetSkinnedIndexBuffer(skinned_upload.indices_skinned);
        scene_meshes->GetSkinnedMeshRanges(skinned_ranges);
        scene_meshes->GetSkinnedRangeMaterials(skinned_materials);

        std::vector<Ir77MeshRange> skinned_blend_ranges{};
        std::vector<std::uint32_t> skinned_blend_materials{};
        split_blend(skinned_ranges, skinned_materials, skinned_blend_ranges, skinned_blend_materials);

        build_runs(skinned_ranges, skinned_materials, skinned_upload.indirect_commands, skinned_command_materials, m_skinned_runs);
        m_skinned_opaque_count = static_cast<std::uint32_t>(skinned_upload.indirect_commands.size());  // blends are appended after these

        // skinned blend ranges -- appended after the opaque runs in the skinned scene (same buffers, SkinnedTransparent pipeline)
        for (std::size_t b = 0; b < skinned_blend_ranges.size(); b++) {
            Ir77MeshRange const& r = skinned_blend_ranges[b];

            m_blend_draws.push_back({skinned_blend_materials[b], static_cast<std::uint32_t>(skinned_upload.indirect_commands.size()), r.first_instance, true});

            skinned_command_materials.push_back(skinned_blend_materials[b]);

            skinned_upload.indirect_commands.push_back(
                VkDrawIndexedIndirectCommand{r.index_count, r.instance_count, r.index_offset, r.vertex_offset, r.first_instance});
        }

        m_has_skinned = !skinned_upload.indirect_commands.empty();

        // nothing drawable -- bounds a default box so the camera, the sun's shadow box and the field have something to frame
        if (upload.indirect_commands.empty() && skinned_upload.indirect_commands.empty() && blend_upload.indirect_commands.empty()) {
            m_bounds_min = glm::vec3(-EMPTY_SCENE_HALF_SIZE);
            m_bounds_max = glm::vec3(EMPTY_SCENE_HALF_SIZE);
        }

        // grey fallback -- primitives with no glTF material
        Ir77PVMaterialConstants constants{};
        constants.base_color = {0.8f, 0.8f, 0.8f, 1.0f};

        result = m_renderer->CreateMaterial(constants, m_default_material);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        // textures -- glTF texture index -> Asset texture id; IR77PV_NO_TEXTURE where decode / upload failed
        std::uint32_t texture_count{0};
        scene_textures->GetTextureCount(texture_count);

        std::vector<std::uint32_t> texture_ids(texture_count, IR77PV_NO_TEXTURE);

        for (std::uint32_t t = 0; t < texture_count; t++) {
            std::uint32_t width{0}, height{0};
            bool srgb{false};
            std::vector<std::uint8_t> rgba{};

            if (scene_textures->GetTexture(t, width, height, srgb, rgba)->ID() != GUIDIr77OperationSucceeded) continue;
            if (m_renderer->CreateTexture(rgba.data(), width, height, srgb, texture_ids[t])->ID() != GUIDIr77OperationSucceeded)
                texture_ids[t] = IR77PV_NO_TEXTURE;
        }

        // materials -- glTF material index -> Asset material id, uploaded in one batch
        std::uint32_t material_count{0};
        scene_materials->GetMaterialCount(material_count);

        std::vector<Ir77PVMaterialConstants> material_constants(material_count);
        std::vector<Ir77PVMaterialTextures> material_textures(material_count);

        auto const texture_id = [&](std::uint32_t const& gltf_texture) {
            return gltf_texture < texture_ids.size() ? texture_ids[gltf_texture] : IR77PV_NO_TEXTURE;
        };

        for (std::uint32_t m = 0; m < material_count; m++) {
            std::array<std::uint32_t, 5> slots{};
            bool double_sided{false};
            scene_materials->GetMaterial(m, material_constants[m], slots, double_sided);

            material_textures[m].albedo = texture_id(slots[0]);
            material_textures[m].normal = texture_id(slots[1]);
            material_textures[m].orm = texture_id(slots[2]);
            material_textures[m].emissive = texture_id(slots[3]);
            material_textures[m].ao = texture_id(slots[4]);
        }

        m_material_ids.clear();
        if (material_count > 0) {
            result = m_renderer->UploadMaterials(material_constants, material_textures, m_material_ids);
            if (result->ID() != GUIDIr77OperationSucceeded) return result;
        }

        // per-draw material tables -- one Asset material row per indirect command; the vertex shader reads [first + gl_DrawID]
        if (Failed(DefineMaterialTable(static_command_materials, m_static_material_buffer, m_static_material_table), result)) return result;
        if (Failed(DefineMaterialTable(blend_command_materials, m_blend_material_buffer, m_blend_material_table), result)) return result;
        if (Failed(DefineMaterialTable(skinned_command_materials, m_skinned_material_buffer, m_skinned_material_table), result)) return result;

        // static: mesh path when the device has it, vertex path otherwise -- same upload either way
        m_mesh_path = false;
        if (!ranges.empty()) {
            std::uint32_t records{0};
            m_mesh_path = use_mesh && m_renderer->UploadMeshlets(upload, m_static_id, records)->ID() == GUIDIr77OperationSucceeded;

            if (!m_mesh_path) {
                result = m_renderer->UploadScene(upload, m_static_id);
                if (result->ID() != GUIDIr77OperationSucceeded) return result;
            }
        }

        // skinned: its own scene upload (skinned vertex / index channels + its own indirect commands)
        if (m_has_skinned) {
            result = m_renderer->UploadScene(skinned_upload, m_skinned_id);
            if (result->ID() != GUIDIr77OperationSucceeded) return result;
        }

        // static transparent: its own compacted scene
        if (!blend_upload.indirect_commands.empty()) {
            result = m_renderer->UploadScene(blend_upload, m_blend_id);
            if (result->ID() != GUIDIr77OperationSucceeded) return result;
        }

        // lights -- unit conversion, a default sun when the file has none, shadow casters over these bounds
        std::vector<Ir77PVLight> light_list{};
        scene_lights->GetLightsBuffer(light_list);

        m_lighting->SetBounds(m_bounds_min, m_bounds_max);
        if (Failed(m_lighting->CreateLights(light_list), result)) return result;

        m_lighting->GetLights(light_list);  // with the casters' shadow tiles, for the stats

        // cameras -- the file's, then the controlled one framed on the scene; starts on the file's first camera if it has one
        m_view->SetCameras(scene_cameras);
        m_view->SetBounds(m_bounds_min, m_bounds_max);

        // the field's draw items carry the grey default
        m_field->SetDefaultMaterial(m_default_material);

        // every clip in the file plays by default, looping -- the app takes over with PlayAnimation / StopAnimation
        m_pose->PlayAllAnimations(true);

        // stats -- filled once here, read by the app through GetSceneStats
        m_stats = Ir77SceneStats{};
        m_stats.path = path;
        m_stats.render_path = m_mesh_path ? "mesh shading indirect" : "vertex indirect";

        m_renderer->GetDeviceName(m_stats.gpu);

        m_pose->GetAssetCounts(m_stats.nodes, m_stats.meshes, m_stats.materials, m_stats.textures, m_stats.cameras, m_stats.skins, m_stats.bones, m_stats.clips);

        m_stats.clip_names.assign(m_stats.clips, std::string{});
        for (std::uint32_t c = 0; c < m_stats.clips; c++) scene_animations->GetClipName(c, m_stats.clip_names[c]);

        m_stats.static_vertices = upload.vertices.size();
        m_stats.static_indices = upload.indices.size();
        m_stats.skinned_vertices = skinned_upload.vertices_skinned.size();
        m_stats.skinned_indices = skinned_upload.indices_skinned.size();
        m_stats.static_primitives = static_cast<std::uint32_t>(ranges.size() + blend_ranges.size());
        m_stats.skinned_primitives = static_cast<std::uint32_t>(skinned_ranges.size() + skinned_blend_ranges.size());
        m_stats.instances = static_cast<std::uint32_t>(m_transforms.size());
        m_stats.opaque_runs = static_cast<std::uint32_t>(m_runs.size());
        m_stats.skinned_runs = static_cast<std::uint32_t>(m_skinned_runs.size());
        m_stats.transparent_objects = static_cast<std::uint32_t>(m_blend_draws.size());
        m_stats.lights = light_list;

        if (load_failed) m_stats.warnings.push_back("scene file didn't load -- running with an empty scene");
        if (m_transforms.size() > 8192) m_stats.warnings.push_back("more than 8192 instances -- raise the Instances capacity");

        PrintSceneStats();  // console copy, for when the app UI isn't up

        m_scene_ready = true;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Once per frame, before CreateFrame: animation, world matrices and joint palettes (Pose), then the held camera keys (View)
    std::shared_ptr<IIr77Return const> Update(float const& dt) {
        std::shared_ptr<IIr77Return const> result{};

        if (Failed(m_pose->Update(dt), result)) return result;

        return m_view->Update(dt);
    }

    // Every frame, after Update: hands the frame's results to Paint -- camera, instance transforms, lights, bones, one draw item per material
    // run and the sorted transparent draws, for every window.
    std::shared_ptr<IIr77Return const> CreateFrame(float const& seconds) {
        if (!m_scene_ready) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: CreateScene not called.");

        m_pose->WriteInstanceTransforms(m_transforms);

        // bones -- uint bone_base[8192] then every joint palette, the layout skinned.vert reads at set 0 binding 4
        bool const skinned = m_has_skinned;
        if (skinned) m_pose->WritePackedBones(m_transforms.size(), m_bones_packed);

        float const aspect = m_view_height > 0 ? static_cast<float>(m_view_width) / static_cast<float>(m_view_height) : 1.78f;

        m_field->WriteDispatches(m_view_windows, seconds - m_last_seconds);

        std::uint32_t shadow_count{0};
        m_lighting->GetShadowCount(shadow_count);

        for (std::size_t w = 0; w < m_view_windows; w++) {
            // file camera (rebuilt with this window's aspect), or the controlled camera (orbit / fly, driven from the UI)
            Ir77PVCamera camera{};
            m_view->WriteCamera(aspect, camera);

            camera.time = glm::vec4(seconds, seconds - m_last_seconds, static_cast<float>(m_frame_number), 0.0f);

            m_renderer->SetFrameData(w, Ir77PVBufferSlot::Camera, &camera, sizeof(camera));
            m_renderer->SetFrameData(w, Ir77PVBufferSlot::Instances, m_transforms.data(), m_transforms.size() * sizeof(glm::mat4));
            m_lighting->WriteFrameData(w);
            if (skinned) m_renderer->SetFrameData(w, Ir77PVBufferSlot::Bones, m_bones_packed.data(), m_bones_packed.size());

            // opaque -- one draw per stream. Each indirect command finds its material in the stream's table: pc.user[4] = table slot,
            // pc.user[5] = first command of the draw, gl_DrawID = the command within it.
            std::shared_ptr<IIr77Return const> result{};
            std::vector<Ir77PVDrawItem> items{};

            if (m_mesh_path) {
                // mesh shading takes its material per call -- one draw per material run
                for (Ir77SceneRun const& run : m_runs) {
                    Ir77PVDrawItem item{};
                    if (Failed(m_renderer->MakeMeshletSceneDrawItem(w, m_static_id, MaterialId(run.gltf_material), run.first_command, run.command_count, item),
                               result))
                        return result;

                    items.push_back(item);
                }
            } else if (m_static_command_count > 0) {
                Ir77PVDrawItem item{};
                if (Failed(m_renderer->MakeIndirectDrawItem(w, m_pipeline, m_static_id, m_default_material, 0, m_static_command_count, false, item), result))
                    return result;

                item.user[0] = m_static_material_table;
                item.user[1] = 0;
                items.push_back(item);
            }

            if (m_skinned_opaque_count > 0) {
                Ir77PVDrawItem item{};
                if (Failed(m_renderer->MakeIndirectDrawItem(w, Ir77PVPipelineKind::Skinned, m_skinned_id, m_default_material, 0, m_skinned_opaque_count, true,
                                                            item),
                           result))
                    return result;

                item.user[0] = m_skinned_material_table;
                item.user[1] = 0;
                items.push_back(item);
            }

            m_field->AppendDraws(w, Ir77PVPass::Opaque, items);

            m_renderer->SetDraws(w, Ir77PVPass::Opaque, items);

            // shadows -- per casting light, each stream's opaque commands in one draw, depth only, into that light's atlas tile
            // (light_index -> pc.user[0] and the tile's viewport). The mesh path's static stream has no vertex / index buffers, so on that path
            // only skinned geometry casts.
            std::vector<Ir77PVDrawItem> shadow_items{};
            shadow_items.reserve(shadow_count * 2);

            for (std::uint32_t tile = 0; tile < shadow_count; tile++) {
                if (!m_mesh_path && m_static_command_count > 0) {
                    Ir77PVDrawItem item{};
                    if (Failed(m_renderer->MakeIndirectDrawItem(w, Ir77PVPipelineKind::Shadow, m_static_id, m_default_material, 0, m_static_command_count,
                                                                false, item),
                               result))
                        return result;

                    item.light_index = tile;
                    shadow_items.push_back(item);
                }

                if (m_skinned_opaque_count > 0) {
                    Ir77PVDrawItem item{};
                    if (Failed(m_renderer->MakeIndirectDrawItem(w, Ir77PVPipelineKind::ShadowSkinned, m_skinned_id, m_default_material, 0,
                                                                m_skinned_opaque_count, true, item),
                               result))
                        return result;

                    item.light_index = tile;
                    shadow_items.push_back(item);
                }
            }

            m_field->AppendDraws(w, Ir77PVPass::Shadow, shadow_items);

            m_renderer->SetDraws(w, Ir77PVPass::Shadow, shadow_items);
            m_opaque_items = static_cast<std::uint32_t>(items.size());

            // transparent -- one draw per object, back to front from this window's eye (instances within one object keep their order)
            glm::vec3 const eye = glm::vec3(glm::inverse(camera.view)[3]);

            std::vector<float> distance(m_blend_draws.size());
            for (std::size_t d = 0; d < m_blend_draws.size(); d++) {
                glm::vec3 const position = glm::vec3(m_transforms[m_blend_draws[d].first_instance][3]);
                distance[d] = glm::dot(position - eye, position - eye);
            }

            std::vector<std::size_t> order(m_blend_draws.size());
            std::iota(order.begin(), order.end(), 0);
            std::sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) { return distance[a] > distance[b]; });

            std::vector<Ir77PVDrawItem> blend_items{};
            blend_items.reserve(order.size() * 2);

            for (std::size_t const d : order) {
                Ir77SceneBlendDraw const& draw = m_blend_draws[d];
                std::uint32_t const scene = draw.skinned ? m_skinned_id : m_blend_id;

                // far side first (front faces culled), then near side -- one pipeline, the cull flips per draw
                Ir77PVPipelineKind const kind{draw.skinned ? Ir77PVPipelineKind::SkinnedTransparent : Ir77PVPipelineKind::Transparent};

                for (bool const back_faces : {true, false}) {
                    Ir77PVDrawItem item{};
                    auto const made = m_renderer->MakeIndirectDrawItem(w, kind, scene, MaterialId(draw.gltf_material), draw.command, 1, draw.skinned, item);
                    if (made->ID() != GUIDIr77OperationSucceeded) return made;

                    item.back_faces = back_faces;

                    item.user[0] = draw.skinned ? m_skinned_material_table : m_blend_material_table;
                    item.user[1] = draw.command;  // a single-command draw: gl_DrawID = 0, so the row is at the command itself

                    blend_items.push_back(item);
                }
            }

            m_field->AppendDraws(w, Ir77PVPass::Transparent, blend_items);

            m_renderer->SetDraws(w, Ir77PVPass::Transparent, blend_items);
            m_transparent_items = static_cast<std::uint32_t>(blend_items.size());
        }

        m_last_seconds = seconds;
        m_frame_number++;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // ---- the peers -- the scene server holds these for its ops ----------------------------------------------------------------------------

    std::shared_ptr<IIr77Return const> GetPose(std::shared_ptr<IIr77Pose>& pose) {
        pose = m_pose;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetView(std::shared_ptr<IIr77View>& view) {
        view = m_view;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetField(std::shared_ptr<IIr77Field>& field) {
        field = m_field;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetLighting(std::shared_ptr<IIr77Lighting>& lighting) {
        lighting = m_lighting;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // stats -- copies, safe to keep
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> GetSceneStats(Ir77SceneStats& stats) const {
        stats = m_stats;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The scene's part of the frame stats -- active camera, draw counts, playing clips. Timings stay with whoever runs the loop.
    std::shared_ptr<IIr77Return const> WriteFrameStats(Ir77FrameStats& stats) const {
        m_view->GetCameraState(stats.camera_index, stats.orbit_camera);
        stats.opaque_items = m_opaque_items;
        stats.transparent_items = m_transparent_items;

        stats.playing.clear();

        std::uint32_t players{0};
        m_pose->GetPlayerCount(players);

        for (std::uint32_t p = 0; p < players; p++) {
            Ir77ClipStat clip_stat{};
            std::uint32_t clip{UINT32_MAX};

            if (m_pose->GetPlayer(p, clip, clip_stat.time, clip_stat.duration, clip_stat.speed, clip_stat.loop)->ID() != GUIDIr77OperationSucceeded) continue;
            if (clip < m_stats.clip_names.size()) clip_stat.name = m_stats.clip_names[clip];

            stats.playing.push_back(clip_stat);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    // glTF material -> Asset material id; primitives without one use the grey default
    std::uint32_t MaterialId(std::uint32_t const& gltf_material) const {
        return gltf_material < m_material_ids.size() ? m_material_ids[gltf_material] : m_default_material;
    }

    // glTF material per indirect command -> Asset material rows, uploaded as a storage buffer the vertex shaders index. No commands ->
    // no buffer, slot BINDLESS_NO_SLOT.
    std::shared_ptr<IIr77Return const> DefineMaterialTable(std::vector<std::uint32_t> const& gltf_materials, std::uint32_t& buffer_id, std::uint32_t& slot) {
        std::shared_ptr<IIr77Return const> result{};

        buffer_id = 0;
        slot = BINDLESS_NO_SLOT;
        if (gltf_materials.empty()) return Ir77RETURN<Ir77OperationSucceeded>();

        std::vector<std::uint32_t> rows(gltf_materials.size(), 0);

        for (std::size_t c = 0; c < gltf_materials.size(); c++) {
            Ir77PVMaterialData material{};
            if (Failed(m_renderer->GetMaterial(MaterialId(gltf_materials[c]), material), result)) return result;

            rows[c] = material.row;
        }

        return m_renderer->CreateStorageBuffer(static_cast<VkDeviceSize>(rows.size() * sizeof(std::uint32_t)), rows.data(), buffer_id, slot);
    }

    // The scene stats into the return log -- until the app displays them
    void PrintSceneStats() const {
        Ir77SceneStats const& s = m_stats;

        Ir77RETURN<Ir77OperationSucceeded>(this, "[STATS] " + s.path);
        Ir77RETURN<Ir77OperationSucceeded>(this, "[STATS] gpu " + s.gpu + ", " + s.render_path);
        Ir77RETURN<Ir77OperationSucceeded>(this, "[STATS] nodes " + std::to_string(s.nodes) + ", meshes " + std::to_string(s.meshes) + ", materials " +
                                                     std::to_string(s.materials) + ", textures " + std::to_string(s.textures) + ", cameras " +
                                                     std::to_string(s.cameras) + ", skins " + std::to_string(s.skins) + ", bones " +
                                                     std::to_string(s.bones) + ", clips " + std::to_string(s.clips));
        Ir77RETURN<Ir77OperationSucceeded>(this, "[STATS] static " + std::to_string(s.static_vertices) + " verts / " +
                                                     std::to_string(s.static_primitives) + " prims, skinned " + std::to_string(s.skinned_vertices) +
                                                     " verts / " + std::to_string(s.skinned_primitives) + " prims, " + std::to_string(s.instances) +
                                                     " instances");
        Ir77RETURN<Ir77OperationSucceeded>(this, "[STATS] runs: " + std::to_string(s.opaque_runs) + " opaque, " + std::to_string(s.skinned_runs) +
                                                     " skinned; " + std::to_string(s.transparent_objects) + " transparent objects");

        for (std::size_t l = 0; l < s.lights.size(); l++) {
            std::ostringstream line{};
            line << std::fixed << std::setprecision(2) << "[STATS] light " << l << ": type " << s.lights[l].type << ", intensity " << s.lights[l].intensity
                 << ", pos (" << s.lights[l].position.x << ", " << s.lights[l].position.y << ", " << s.lights[l].position.z << ")";

            Ir77RETURN<Ir77OperationSucceeded>(this, line.str());
        }

        for (std::size_t c = 0; c < s.clip_names.size(); c++)
            Ir77RETURN<Ir77OperationSucceeded>(this, "[STATS] clip " + std::to_string(c) + ": '" + s.clip_names[c] + "'");

        for (std::string const& warning : s.warnings) Ir77RETURN<Ir77OutOfRange>(this, "[WARN] " + warning);
    }

   private:
    // the peers -- created here, wired here, never handed past the scene server
    std::shared_ptr<IIr77Pose> m_pose{};

    std::shared_ptr<IIr77View> m_view{};

    std::shared_ptr<IIr77Lighting> m_lighting{};

    std::shared_ptr<IIr77Field> m_field{};

    // direct link -- Scene -> PeregrineV
    std::shared_ptr<NSIr77PeregrineV::Ir77Server> m_renderer{};  // PeregrineV's server -- the module's one way into the renderer

    std::size_t m_view_windows{1};

    int m_view_width{1280};

    int m_view_height{720};

    Ir77PVPipelineHandle m_pipeline{IR77PV_NO_PIPELINE};  // what the static runs draw with

    // what CreateScene uploaded
    bool m_scene_ready{false};

    bool m_mesh_path{false};  // static stream on the mesh shading path

    std::uint32_t m_static_id{0};  // UploadScene id, or UploadMeshlets id when m_mesh_path

    std::uint32_t m_skinned_id{0};  // UploadScene id of the skinned stream

    std::uint32_t m_blend_id{0};  // UploadScene id of the compacted static transparent scene

    bool m_has_skinned{false};  // skinned scene uploaded (opaque or transparent)

    std::vector<Ir77SceneRun> m_runs{};

    std::vector<Ir77SceneRun> m_skinned_runs{};  // commands index the skinned scene's buffers

    std::vector<Ir77SceneBlendDraw> m_blend_draws{};

    // per-draw material tables -- Asset material row per indirect command (storage buffer id + bindless slot), per uploaded scene
    std::uint32_t m_static_material_buffer{0};

    std::uint32_t m_static_material_table{BINDLESS_NO_SLOT};

    std::uint32_t m_blend_material_buffer{0};

    std::uint32_t m_blend_material_table{BINDLESS_NO_SLOT};

    std::uint32_t m_skinned_material_buffer{0};

    std::uint32_t m_skinned_material_table{BINDLESS_NO_SLOT};

    std::uint32_t m_static_command_count{0};  // opaque commands in the static scene -- one draw covers them all

    std::uint32_t m_skinned_opaque_count{0};  // opaque commands in the skinned scene (its blends follow them)

    std::uint32_t m_default_material{0};

    std::vector<std::uint32_t> m_material_ids{};  // [glTF material] -> Asset material id

    glm::vec3 m_bounds_min{-1.0f};

    glm::vec3 m_bounds_max{1.0f};

    Ir77SceneStats m_stats{};  // filled by CreateScene

    // per frame -- what CreateFrame hands to Paint
    std::vector<glm::mat4> m_transforms{};

    std::vector<std::uint8_t> m_bones_packed{};  // bone_base[8192] + palettes, as set 0 binding 4 expects

    float m_last_seconds{0.0f};  // camera.time.y

    std::uint32_t m_frame_number{0};  // camera.time.z

    std::uint32_t m_opaque_items{0};

    std::uint32_t m_transparent_items{0};

    static constexpr float EMPTY_SCENE_HALF_SIZE{4.0f};  // the box a scene with no geometry frames
};
}  // namespace NSIr77Scene
