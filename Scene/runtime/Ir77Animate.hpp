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
//   setup:     SetRenderer / SetViewSize / SetPipeline -> CreateScene(path, use_mesh) -- Pose parses, the file goes to the renderer as one asset
//              bundle (LoadAsset), Lighting converts the lights, View frames the scene
//   per frame: Update(dt) -> CreateFrame(seconds) -- one Ir77PVFrame: cameras (View), transforms and bones (Pose), lights and shadow
//              matrices (Lighting), the field's dispatches and draws (Field) -> SubmitFrame
//   the UI:    the scene server reaches the peers through GetPose / GetView / GetField for its ops
//
// The renderer makes the draws, the material tables, the skinning and the acceleration structures -- the scene only says what is there.
// =============================================================================================================================================
class Ir77Animate : public Ir77Enlisted, public IIr77Animate, public std::enable_shared_from_this<Ir77Animate> {
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
    // The renderer's server -- where the scene's asset goes (CreateScene) and each frame goes (CreateFrame). Field gets it too (its
    // program, buffers and draw items). Its forwarding methods are the only way into PeregrineV.
    std::shared_ptr<IIr77Return const> SetRenderer(std::shared_ptr<NSIr77PeregrineV::Ir77Server> const& renderer) {
        m_renderer = renderer;

        m_field->SetRenderer(renderer);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The windows CreateFrame fills, and their size -- the cameras' aspect. Any time; the next frame uses it.
    std::shared_ptr<IIr77Return const> SetViewSize(std::size_t const& windows, int const& width, int const& height) {
        m_view_windows = windows;
        m_view_width = width;
        m_view_height = height;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // What the static (non-skinned, opaque) geometry draws with -- a manifest pipeline or role. Unset -> the "static" role. Taken by the next
    // CreateScene.
    std::shared_ptr<IIr77Return const> SetPipeline(Ir77PVPipelineHandle const& pipeline) {
        m_pipeline = pipeline;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Parses the glTF and hands it to the renderer as one asset bundle -- geometry per channel with each primitive's material, the decoded
    // textures, the materials, the instance slots and joint palettes -- replacing the scene loaded before. Then the lights, the cameras and
    // the clips. use_mesh -> mesh shading for the static geometry when the device has it.
    // No geometry is fine: an empty path, a file with nothing drawable, or a file that fails to load all leave a running scene with no
    // geometry. A file that fails to load is logged (and listed in the stats warnings), not a failure.
    std::shared_ptr<IIr77Return const> CreateScene(std::string const& path, bool const& use_mesh) {
        if (!m_renderer) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Animate: SetRenderer not called.");

        // the scene before goes first -- its draws stop with the next frame
        if (m_asset != IR77PV_NO_ASSET) {
            m_renderer->UnloadAsset(m_asset);
            m_asset = IR77PV_NO_ASSET;
        }

        m_scene_ready = false;

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

        // ---- the bundle ----
        Ir77PVAssetBundle bundle{};
        bundle.name = path.empty() ? std::string{"(empty scene)"} : path;
        bundle.use_mesh = use_mesh;
        bundle.pipeline = m_pipeline;

        // geometry -- each channel stacked, one primitive per range with its glTF material
        auto const primitives = [](std::vector<Ir77MeshRange> const& ranges, std::vector<std::uint32_t> const& materials,
                                   std::vector<Ir77PVAssetPrimitive>& out) {
            for (std::size_t r = 0; r < ranges.size(); r++) out.push_back({ranges[r], r < materials.size() ? materials[r] : UINT32_MAX});
        };

        std::vector<Ir77MeshRange> ranges{};
        std::vector<std::uint32_t> materials{};

        scene_meshes->GetVertexBuffer(bundle.vertices);
        scene_meshes->GetIndexBuffer(bundle.indices);
        scene_meshes->GetMeshRanges(ranges);
        scene_meshes->GetRangeMaterials(materials);
        primitives(ranges, materials, bundle.primitives);

        ranges.clear();
        materials.clear();

        scene_meshes->GetSkinnedVertexBuffer(bundle.vertices_skinned);
        scene_meshes->GetSkinnedIndexBuffer(bundle.indices_skinned);
        scene_meshes->GetSkinnedMeshRanges(ranges);
        scene_meshes->GetSkinnedRangeMaterials(materials);
        primitives(ranges, materials, bundle.primitives_skinned);

        // instances and joint palettes -- the rest pose now, Pose's every frame after
        scene_meshes->GetInstanceBuffer(m_transforms);
        scene_meshes->GetBounds(m_bounds_min, m_bounds_max);

        bundle.instance_count = static_cast<std::uint32_t>(m_transforms.size());
        m_pose->WriteInstanceBoneBases(bundle.bone_bases, m_transforms.size());

        m_pose->WriteBoneMatrices(m_bones);
        bundle.bone_count = static_cast<std::uint32_t>(m_bones.size());

        // textures -- decoded; one that won't decode goes up empty and its maps fall back to the placeholder
        std::uint32_t texture_count{0};
        scene_textures->GetTextureCount(texture_count);

        bundle.textures.assign(texture_count, Ir77PVAssetTexture{});

        for (std::uint32_t t = 0; t < texture_count; t++) {
            Ir77PVAssetTexture& texture = bundle.textures[t];
            if (scene_textures->GetTexture(t, texture.width, texture.height, texture.srgb, texture.rgba)->ID() != GUIDIr77OperationSucceeded) texture.rgba.clear();
        }

        // materials -- texture fields are glTF texture indices; BLEND and MASK as the file says
        std::uint32_t material_count{0};
        scene_materials->GetMaterialCount(material_count);

        bundle.materials.assign(material_count, Ir77PVAssetMaterial{});

        for (std::uint32_t m = 0; m < material_count; m++) {
            Ir77PVAssetMaterial& material = bundle.materials[m];

            std::array<std::uint32_t, 5> slots{};
            bool double_sided{false};
            scene_materials->GetMaterial(m, material.constants, slots, double_sided);
            scene_materials->GetMaterialBlend(m, material.blend);

            material.textures = {slots[0], slots[1], slots[2], slots[3], slots[4]};
            material.mask = material.constants.alpha_cutoff > 0.0f;
        }

        // nothing drawable -- bounds a default box so the camera, the sun's shadow box and the field have something to frame
        if (bundle.primitives.empty() && bundle.primitives_skinned.empty()) {
            m_bounds_min = glm::vec3(-EMPTY_SCENE_HALF_SIZE);
            m_bounds_max = glm::vec3(EMPTY_SCENE_HALF_SIZE);
        }

        Ir77PVAssetReport report{};
        if (Failed(m_renderer->LoadAsset(bundle, m_asset, report), result)) return result;

        // lights -- unit conversion, a default sun when the file has none, shadow casters over these bounds
        std::vector<Ir77PVLight> light_list{};
        scene_lights->GetLightsBuffer(light_list);

        m_lighting->SetBounds(m_bounds_min, m_bounds_max);
        if (Failed(m_lighting->CreateLights(light_list), result)) return result;

        m_lighting->GetLights(light_list);  // with the casters' shadow tiles, for the stats

        // cameras -- the file's, then the controlled one framed on the scene; starts on the file's first camera if it has one
        m_view->SetCameras(scene_cameras);
        m_view->SetBounds(m_bounds_min, m_bounds_max);

        // every clip in the file plays by default, looping -- the app takes over with PlayAnimation / StopAnimation
        m_pose->PlayAllAnimations(true);

        // stats -- filled once here, read by the app through GetSceneStats
        m_stats = Ir77SceneStats{};
        m_stats.path = path;
        m_stats.gpu = report.device;
        m_stats.render_path = std::string{report.mesh_path ? "mesh shading indirect" : "vertex indirect"} + (report.ray_query ? ", ray queries" : "");

        m_pose->GetAssetCounts(m_stats.nodes, m_stats.meshes, m_stats.materials, m_stats.textures, m_stats.cameras, m_stats.skins, m_stats.bones,
                               m_stats.clips);

        m_stats.clip_names.assign(m_stats.clips, std::string{});
        for (std::uint32_t c = 0; c < m_stats.clips; c++) scene_animations->GetClipName(c, m_stats.clip_names[c]);

        m_stats.static_vertices = bundle.vertices.size();
        m_stats.static_indices = bundle.indices.size();
        m_stats.skinned_vertices = bundle.vertices_skinned.size();
        m_stats.skinned_indices = bundle.indices_skinned.size();
        m_stats.static_primitives = static_cast<std::uint32_t>(bundle.primitives.size());
        m_stats.skinned_primitives = static_cast<std::uint32_t>(bundle.primitives_skinned.size());
        m_stats.instances = bundle.instance_count;
        m_stats.opaque_runs = report.opaque_runs;
        m_stats.skinned_runs = report.skinned_runs;
        m_stats.transparent_objects = report.transparent_objects;
        m_stats.lights = light_list;

        if (load_failed) m_stats.warnings.push_back("scene file didn't load -- running with an empty scene");

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

    // Every frame, after Update: the frame -- every window's camera, the asset's transforms and bones, the lights, the field's dispatches and
    // draws -- to the renderer in one SubmitFrame.
    std::shared_ptr<IIr77Return const> CreateFrame(float const& seconds) {
        if (!m_scene_ready) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: CreateScene not called.");

        std::shared_ptr<IIr77Return const> result{};

        float const dt = seconds - m_last_seconds;
        float const aspect = m_view_height > 0 ? static_cast<float>(m_view_width) / static_cast<float>(m_view_height) : 1.78f;

        // the asset -- its instances and joint palettes as Pose left them
        m_frame.assets.resize(1);

        Ir77PVFrameAsset& asset = m_frame.assets[0];
        asset.asset = m_asset;

        m_pose->WriteInstanceTransforms(m_transforms);
        m_pose->WriteBoneMatrices(m_bones);

        asset.transforms = m_transforms;
        asset.bones = m_bones;

        // compute -- the field's (the renderer runs the asset's skinning ahead of it)
        m_frame.dispatches.clear();
        m_field->AppendDispatches(m_frame.dispatches, dt);

        m_lighting->WriteFrame(m_frame);

        // every window -- file camera (rebuilt with this aspect) or the controlled camera, and the field's draws
        m_frame.windows.assign(m_view_windows, Ir77PVFrameWindow{});

        for (std::size_t w = 0; w < m_view_windows; w++) {
            Ir77PVFrameWindow& window = m_frame.windows[w];

            m_view->WriteCamera(aspect, window.camera);
            window.camera.time = glm::vec4(seconds, dt, static_cast<float>(m_frame_number), 0.0f);

            m_field->AppendDraws(w, Ir77PVPass::Opaque, window.opaque);
            m_field->AppendDraws(w, Ir77PVPass::Shadow, window.shadow);
            m_field->AppendDraws(w, Ir77PVPass::Transparent, window.transparent);
        }

        Ir77PVFrameReport report{};
        if (Failed(m_renderer->SubmitFrame(m_frame, report), result)) return result;

        m_opaque_items = report.opaque_items;
        m_transparent_items = report.transparent_items;

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
    // The scene stats into the return log -- until the app displays them
    void PrintSceneStats() const {
        Ir77SceneStats const& s = m_stats;

        Ir77RETURN<Ir77OperationSucceeded>(this, "[STATS] " + s.path);
        Ir77RETURN<Ir77OperationSucceeded>(this, "[STATS] gpu " + s.gpu + ", " + s.render_path);
        Ir77RETURN<Ir77OperationSucceeded>(this, "[STATS] nodes " + std::to_string(s.nodes) + ", meshes " + std::to_string(s.meshes) + ", materials " +
                                                     std::to_string(s.materials) + ", textures " + std::to_string(s.textures) + ", cameras " +
                                                     std::to_string(s.cameras) + ", skins " + std::to_string(s.skins) + ", bones " + std::to_string(s.bones) +
                                                     ", clips " + std::to_string(s.clips));
        Ir77RETURN<Ir77OperationSucceeded>(this, "[STATS] static " + std::to_string(s.static_vertices) + " verts / " + std::to_string(s.static_primitives) +
                                                     " prims, skinned " + std::to_string(s.skinned_vertices) + " verts / " +
                                                     std::to_string(s.skinned_primitives) + " prims, " + std::to_string(s.instances) + " instances");
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

    Ir77PVPipelineHandle m_pipeline{IR77PV_NO_PIPELINE};  // what the static geometry draws with

    // what CreateScene loaded
    bool m_scene_ready{false};

    std::uint32_t m_asset{IR77PV_NO_ASSET};  // the file, as the renderer holds it

    glm::vec3 m_bounds_min{-1.0f};

    glm::vec3 m_bounds_max{1.0f};

    Ir77SceneStats m_stats{};  // filled by CreateScene

    // per frame -- what CreateFrame hands the renderer
    Ir77PVFrame m_frame{};

    std::vector<glm::mat4> m_transforms{};  // the asset's instances, in its own slot order

    std::vector<glm::mat4> m_bones{};  // every joint palette, back to back

    float m_last_seconds{0.0f};  // camera.time.y

    std::uint32_t m_frame_number{0};  // camera.time.z

    std::uint32_t m_opaque_items{0};

    std::uint32_t m_transparent_items{0};

    static constexpr float EMPTY_SCENE_HALF_SIZE{4.0f};  // the box a scene with no geometry frames
};
}  // namespace NSIr77Scene
