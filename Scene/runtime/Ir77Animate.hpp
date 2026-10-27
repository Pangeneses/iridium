#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <limits>
#include <map>
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
// scene's peers -- the entities (one Pose each: a file, its node tree and animation), View (the cameras), Lighting (lights and shadows) and
// Field (the compute visual) -- which it creates and wires; and the one place that talks to all of them at once:
//
//   setup:     SetRenderer / SetViewSize / SetPipeline -> CreateScene(path, use_mesh, lights, cameras) -- every entity and light goes, the
//              file becomes entity 0: Pose parses, the file goes to the renderer as one asset bundle (LoadAsset), its lights become world
//              lights, View takes its cameras and frames it
//   entities:  ImportEntity(path, lights) -> its id / RemoveEntity / SetEntityTransform -- any number of files, each its own asset in the
//              renderer, each placed in the world as a whole
//   lights:    AddLight / SetLight / RemoveLight -- world lights, the scene graph's: placed by the UI, or imported with a file (a copy, placed
//              where the file had it -- it doesn't follow the entity). Lighting gets them all, shadow casters over every entity where it sits,
//              on every change
//   per frame: Update(dt) -> CreateFrame(seconds) -- one Ir77PVFrame: cameras (View), every entity's transforms and bones (its Pose), lights
//              and shadow matrices (Lighting), the field's dispatches and draws (Field) -> SubmitFrame
//   the UI:    the scene server reaches the peers through GetPose(entity) / GetView / GetField for its ops
//
// The renderer makes the draws, the material tables, the skinning and the acceleration structures -- the scene only says what is there.
// =============================================================================================================================================
class Ir77Animate : public Ir77Enlisted, public IIr77Animate, public std::enable_shared_from_this<Ir77Animate> {
   private:
    // one entity -- a file in the world: its Pose, its asset in the renderer, where it sits, and its frame data
    struct Ir77AnimateEntity {
        std::shared_ptr<IIr77Pose> pose{};
        std::uint32_t asset{IR77PV_NO_ASSET};
        std::string path{};
        glm::vec3 translation{0.0f};
        glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
        glm::vec3 scale{1.0f};
        glm::vec3 bounds_min{-1.0f};  // the file's own space
        glm::vec3 bounds_max{1.0f};
        std::vector<glm::mat4> transforms{};  // its instances, in its own slot order
        std::vector<glm::mat4> bones{};       // its joint palettes, back to back
    };

    // one world light -- what it is (type, color, intensity in glTF units: lux / candela, range, cones) and where it sits; it shines down its
    // local -Z. light.position / direction are made from the placement when the lights are built.
    struct Ir77AnimateLight {
        Ir77PVLight light{};
        glm::vec3 translation{0.0f};
        glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
        std::uint32_t entity{UINT32_MAX};  // the entity it was imported with; UINT32_MAX = placed by the UI
    };

   public:
    Ir77Animate() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();

        // the peers -- they never see Animate; the field reports its box to Lighting and reads its shadow count. Entities come with the scene.
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

    // The scene: every entity and light goes, then the glTF becomes entity 0 -- its clips with it, its lights as world lights (lights), its
    // cameras (cameras; every view is framed on it either way). use_mesh -> mesh shading for the static geometry when the device has it.
    // No geometry is fine: an empty path, a file with nothing drawable, or a file that fails to load all leave a running scene with no
    // geometry. A file that fails to load is logged (and listed in the stats warnings), not a failure.
    std::shared_ptr<IIr77Return const> CreateScene(std::string const& path, bool const& use_mesh, bool const& lights, bool const& cameras) {
        if (!m_renderer) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Animate: SetRenderer not called.");

        std::shared_ptr<IIr77Return const> result{};

        // the scene before goes first -- its draws stop with the next frame
        for (auto const& [id, entity] : m_entities) m_renderer->UnloadAsset(entity.asset);

        m_entities.clear();
        m_next_entity = 0;
        m_lights.clear();
        m_next_light = 0;
        m_scene_ready = false;

        // a file that won't load (ParseAsset logged why) -> carry on with an empty scene
        auto pose = std::static_pointer_cast<IIr77Pose>(std::make_shared<Ir77Pose>());

        bool const load_failed = pose->ParseAsset(path)->ID() != GUIDIr77OperationSucceeded;
        if (load_failed) pose->ParseAsset(std::string{});

        Ir77PVAssetReport report{};
        Ir77PVAssetBundle bundle{};
        std::uint32_t entity{0};

        if (Failed(LoadEntity(pose, path, use_mesh, bundle, report, entity), result)) return result;

        Ir77AnimateEntity const& first = m_entities.at(entity);

        if (lights) ImportLights(entity);

        if (Failed(BuildLights(), result)) return result;

        // cameras -- the file's (when imported), then the controlled ones framed on it; view 0 starts on the file's first camera if it has one
        std::shared_ptr<IIr77Camera> scene_cameras{};
        if (cameras) pose->GetCameras(scene_cameras);

        m_view->SetCameras(scene_cameras);
        m_view->SetBounds(first.bounds_min, first.bounds_max);

        // stats -- the scene file's, filled once here, read by the app through GetSceneStats
        std::shared_ptr<IIr77Animation> scene_animations{};
        pose->GetAnimations(scene_animations);

        m_stats = Ir77SceneStats{};
        m_stats.path = path;
        m_stats.gpu = report.device;
        m_stats.render_path = std::string{report.mesh_path ? "mesh shading indirect" : "vertex indirect"} + (report.ray_query ? ", ray queries" : "");

        pose->GetAssetCounts(m_stats.nodes, m_stats.meshes, m_stats.materials, m_stats.textures, m_stats.cameras, m_stats.skins, m_stats.bones,
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
        m_lighting->GetLights(m_stats.lights);  // with the casters' shadow tiles

        if (load_failed) m_stats.warnings.push_back("scene file didn't load -- running with an empty scene");

        PrintSceneStats();  // console copy, for when the app UI isn't up

        m_scene_ready = true;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // entities -- files in the world, each its own asset in the renderer
    // -------------------------------------------------------------------------------------------------------------------------------------

    // One more file in the scene, at the origin, its clips playing -- entity is its id. A file that won't load is a failure here (nothing is
    // added). lights -> its lights become world lights; the cameras stay as they are.
    std::shared_ptr<IIr77Return const> ImportEntity(std::string const& path, bool const& use_mesh, bool const& lights, std::uint32_t& entity) {
        if (!m_renderer) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Animate: SetRenderer not called.");
        if (path.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Animate: an entity needs a file.");

        std::shared_ptr<IIr77Return const> result{};

        auto pose = std::static_pointer_cast<IIr77Pose>(std::make_shared<Ir77Pose>());
        if (Failed(pose->ParseAsset(path), result)) return result;

        Ir77PVAssetReport report{};
        Ir77PVAssetBundle bundle{};

        if (Failed(LoadEntity(pose, path, use_mesh, bundle, report, entity), result)) return result;

        if (lights) ImportLights(entity);

        return BuildLights();
    }

    // The entity's asset leaves the renderer (its draws stop with the next frame). Lights it brought stay -- they're world lights now.
    std::shared_ptr<IIr77Return const> RemoveEntity(std::uint32_t const& entity) {
        auto const found = m_entities.find(entity);
        if (found == m_entities.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Animate: no such entity.");

        m_renderer->UnloadAsset(found->second.asset);
        m_entities.erase(found);

        return BuildLights();
    }

    // Where the whole entity sits in the world -- everything it draws moves with it from the next frame, and the shadow casters' box with it
    std::shared_ptr<IIr77Return const> SetEntityTransform(std::uint32_t const& entity, glm::vec3 const& translation, glm::quat const& rotation,
                                                          glm::vec3 const& scale) {
        auto const found = m_entities.find(entity);
        if (found == m_entities.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Animate: no such entity.");

        Ir77AnimateEntity& placed = found->second;
        placed.translation = translation;
        placed.rotation = glm::normalize(rotation);
        placed.scale = scale;

        std::shared_ptr<IIr77Return const> result{};
        if (Failed(placed.pose->SetRootTransform(EntityMatrix(placed)), result)) return result;

        return BuildLights();
    }

    // The entities in id order -- each one's id, file and placement
    std::shared_ptr<IIr77Return const> GetEntities(std::vector<std::uint32_t>& ids, std::vector<std::string>& paths, std::vector<glm::vec3>& translations,
                                                   std::vector<glm::quat>& rotations, std::vector<glm::vec3>& scales) {
        ids.clear();
        paths.clear();
        translations.clear();
        rotations.clear();
        scales.clear();

        for (auto const& [id, entity] : m_entities) {
            ids.push_back(id);
            paths.push_back(entity.path);
            translations.push_back(entity.translation);
            rotations.push_back(entity.rotation);
            scales.push_back(entity.scale);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // world lights -- the scene graph's. light: type, color, intensity (glTF units -- lux for the sun, candela for point / spot), range (0 =
    // none), cones; its position, direction and shadow are ignored -- the placement says where it is, Lighting picks the casters.
    // -------------------------------------------------------------------------------------------------------------------------------------

    std::shared_ptr<IIr77Return const> AddLight(Ir77PVLight const& light, glm::vec3 const& translation, glm::quat const& rotation, std::uint32_t& id) {
        Ir77AnimateLight placed{};
        placed.light = light;
        placed.translation = translation;
        placed.rotation = glm::normalize(rotation);

        id = m_next_light++;
        m_lights.emplace(id, placed);

        return BuildLights();
    }

    std::shared_ptr<IIr77Return const> SetLight(std::uint32_t const& id, Ir77PVLight const& light, glm::vec3 const& translation, glm::quat const& rotation) {
        auto const found = m_lights.find(id);
        if (found == m_lights.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Animate: no such light.");

        found->second.light = light;
        found->second.translation = translation;
        found->second.rotation = glm::normalize(rotation);

        return BuildLights();
    }

    std::shared_ptr<IIr77Return const> RemoveLight(std::uint32_t const& id) {
        if (m_lights.erase(id) == 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Animate: no such light.");

        return BuildLights();
    }

    // One world light -- entity is the one it was imported with (UINT32_MAX = placed by the UI)
    std::shared_ptr<IIr77Return const> GetLight(std::uint32_t const& id, Ir77PVLight& light, glm::vec3& translation, glm::quat& rotation,
                                                std::uint32_t& entity) {
        auto const found = m_lights.find(id);
        if (found == m_lights.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Animate: no such light.");

        light = found->second.light;
        translation = found->second.translation;
        rotation = found->second.rotation;
        entity = found->second.entity;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The world lights' ids, in order
    std::shared_ptr<IIr77Return const> GetLightIds(std::vector<std::uint32_t>& ids) {
        ids.clear();
        for (auto const& [id, light] : m_lights) ids.push_back(id);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Once per frame, before CreateFrame: every entity's animation, world matrices and joint palettes (its Pose), then the held camera keys
    // (View)
    std::shared_ptr<IIr77Return const> Update(float const& dt) {
        std::shared_ptr<IIr77Return const> result{};

        for (auto const& [id, entity] : m_entities)
            if (Failed(entity.pose->Update(dt), result)) return result;

        return m_view->Update(dt);
    }

    // Every frame, after Update: the frame -- every window's cameras, every entity's transforms and bones, the lights, the field's dispatches
    // and draws -- to the renderer in one SubmitFrame.
    std::shared_ptr<IIr77Return const> CreateFrame(float const& seconds) {
        if (!m_scene_ready) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: CreateScene not called.");

        std::shared_ptr<IIr77Return const> result{};

        float const dt = seconds - m_last_seconds;
        float const aspect = m_view_height > 0 ? static_cast<float>(m_view_width) / static_cast<float>(m_view_height) : 1.78f;

        // the entities -- their instances and joint palettes as their Poses left them
        m_frame.assets.resize(m_entities.size());

        std::size_t a{0};
        for (auto& [id, entity] : m_entities) {
            entity.pose->WriteInstanceTransforms(entity.transforms);
            entity.pose->WriteBoneMatrices(entity.bones);

            Ir77PVFrameAsset& asset = m_frame.assets[a++];
            asset.asset = entity.asset;
            asset.transforms = entity.transforms;
            asset.bones = entity.bones;
        }

        // compute -- the field's (the renderer runs the entities' skinning ahead of it)
        m_frame.dispatches.clear();
        m_field->AppendDispatches(m_frame.dispatches, dt);

        m_lighting->WriteFrame(m_frame);

        // every window -- one camera per view (each a file camera rebuilt with this aspect, or that view's controlled camera), and the field's draws
        m_frame.windows.assign(m_view_windows, Ir77PVFrameWindow{});

        for (std::size_t w = 0; w < m_view_windows; w++) {
            Ir77PVFrameWindow& window = m_frame.windows[w];

            m_view->WriteCameras(aspect, window.cameras, window.areas);
            for (Ir77PVCamera& camera : window.cameras) camera.time = glm::vec4(seconds, dt, static_cast<float>(m_frame_number), 0.0f);

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

    // An entity's Pose -- its clips and nodes
    std::shared_ptr<IIr77Return const> GetPose(std::uint32_t const& entity, std::shared_ptr<IIr77Pose>& pose) {
        auto const found = m_entities.find(entity);
        if (found == m_entities.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Animate: no such entity.");

        pose = found->second.pose;

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

    // The scene's part of the frame stats -- active camera, draw counts, the scene file's (entity 0's) playing clips. Timings stay with
    // whoever runs the loop.
    std::shared_ptr<IIr77Return const> WriteFrameStats(Ir77FrameStats& stats) const {
        m_view->GetCameraState(stats.camera_index, stats.orbit_camera);
        stats.opaque_items = m_opaque_items;
        stats.transparent_items = m_transparent_items;

        stats.playing.clear();

        auto const scene = m_entities.find(0);
        if (scene == m_entities.end()) return Ir77RETURN<Ir77OperationSucceeded>();

        std::shared_ptr<IIr77Pose> const& pose = scene->second.pose;

        std::uint32_t players{0};
        pose->GetPlayerCount(players);

        for (std::uint32_t p = 0; p < players; p++) {
            Ir77ClipStat clip_stat{};
            std::uint32_t clip{UINT32_MAX};

            if (pose->GetPlayer(p, clip, clip_stat.time, clip_stat.duration, clip_stat.speed, clip_stat.loop)->ID() != GUIDIr77OperationSucceeded) continue;
            if (clip < m_stats.clip_names.size()) clip_stat.name = m_stats.clip_names[clip];

            stats.playing.push_back(clip_stat);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    // One parsed file -> the renderer as one asset bundle (geometry per channel with each primitive's material, the decoded textures, the
    // materials, the instance slots and joint palettes) -> a new entity at the origin, its clips playing. entity is its id.
    std::shared_ptr<IIr77Return const> LoadEntity(std::shared_ptr<IIr77Pose> const& pose, std::string const& path, bool const use_mesh,
                                                    Ir77PVAssetBundle& bundle, Ir77PVAssetReport& report, std::uint32_t& entity) {
        std::shared_ptr<IIr77Return const> result{};

        // the file's components -- Pose owns them
        std::shared_ptr<IIr77Mesh> meshes{};
        std::shared_ptr<IIr77Material> materials{};
        std::shared_ptr<IIr77Texture> textures{};

        pose->GetMeshes(meshes);
        pose->GetMaterials(materials);
        pose->GetTextures(textures);

        Ir77AnimateEntity placed{};
        placed.pose = pose;
        placed.path = path;

        bundle = Ir77PVAssetBundle{};
        bundle.name = path.empty() ? std::string{"(empty scene)"} : path;
        bundle.use_mesh = use_mesh;
        bundle.pipeline = m_pipeline;

        // geometry -- each channel stacked, one primitive per range with its glTF material
        auto const primitives = [](std::vector<Ir77MeshRange> const& ranges, std::vector<std::uint32_t> const& range_materials,
                                   std::vector<Ir77PVAssetPrimitive>& out) {
            for (std::size_t r = 0; r < ranges.size(); r++) out.push_back({ranges[r], r < range_materials.size() ? range_materials[r] : UINT32_MAX});
        };

        std::vector<Ir77MeshRange> ranges{};
        std::vector<std::uint32_t> range_materials{};

        meshes->GetVertexBuffer(bundle.vertices);
        meshes->GetIndexBuffer(bundle.indices);
        meshes->GetMeshRanges(ranges);
        meshes->GetRangeMaterials(range_materials);
        primitives(ranges, range_materials, bundle.primitives);

        ranges.clear();
        range_materials.clear();

        meshes->GetSkinnedVertexBuffer(bundle.vertices_skinned);
        meshes->GetSkinnedIndexBuffer(bundle.indices_skinned);
        meshes->GetSkinnedMeshRanges(ranges);
        meshes->GetSkinnedRangeMaterials(range_materials);
        primitives(ranges, range_materials, bundle.primitives_skinned);

        // instances and joint palettes -- the rest pose now, Pose's every frame after
        meshes->GetInstanceBuffer(placed.transforms);
        meshes->GetBounds(placed.bounds_min, placed.bounds_max);

        bundle.instance_count = static_cast<std::uint32_t>(placed.transforms.size());
        pose->WriteInstanceBoneBases(bundle.bone_bases, placed.transforms.size());

        pose->WriteBoneMatrices(placed.bones);
        bundle.bone_count = static_cast<std::uint32_t>(placed.bones.size());

        // textures -- decoded; one that won't decode goes up empty and its maps fall back to the placeholder
        std::uint32_t texture_count{0};
        textures->GetTextureCount(texture_count);

        bundle.textures.assign(texture_count, Ir77PVAssetTexture{});

        for (std::uint32_t t = 0; t < texture_count; t++) {
            Ir77PVAssetTexture& texture = bundle.textures[t];
            if (textures->GetTexture(t, texture.width, texture.height, texture.srgb, texture.rgba)->ID() != GUIDIr77OperationSucceeded) texture.rgba.clear();
        }

        // materials -- texture fields are glTF texture indices; BLEND and MASK as the file says
        std::uint32_t material_count{0};
        materials->GetMaterialCount(material_count);

        bundle.materials.assign(material_count, Ir77PVAssetMaterial{});

        for (std::uint32_t m = 0; m < material_count; m++) {
            Ir77PVAssetMaterial& material = bundle.materials[m];

            std::array<std::uint32_t, 5> slots{};
            bool double_sided{false};
            materials->GetMaterial(m, material.constants, slots, double_sided);
            materials->GetMaterialBlend(m, material.blend);

            material.textures = {slots[0], slots[1], slots[2], slots[3], slots[4]};
            material.mask = material.constants.alpha_cutoff > 0.0f;
        }

        // nothing drawable -- bounds a default box so the camera, the sun's shadow box and the field have something to frame
        if (bundle.primitives.empty() && bundle.primitives_skinned.empty()) {
            placed.bounds_min = glm::vec3(-EMPTY_SCENE_HALF_SIZE);
            placed.bounds_max = glm::vec3(EMPTY_SCENE_HALF_SIZE);
        }

        if (Failed(m_renderer->LoadAsset(bundle, placed.asset, report), result)) return result;

        // every clip in the file plays by default, looping -- the app takes over with animation.play / animation.stop
        pose->PlayAllAnimations(true);

        entity = m_next_entity++;
        m_entities.emplace(entity, std::move(placed));

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // An entity's file lights -> world lights, each placed where the file had it (the entity is at the origin when it's imported). Its
    // direction becomes the rotation that turns -Z onto it.
    void ImportLights(std::uint32_t const entity) {
        std::shared_ptr<IIr77Light> file_lights{};
        m_entities.at(entity).pose->GetLights(file_lights);

        std::vector<Ir77PVLight> lights{};
        file_lights->GetLightsBuffer(lights);

        for (Ir77PVLight const& light : lights) {
            Ir77AnimateLight placed{};
            placed.light = light;
            placed.translation = light.position;
            placed.rotation = glm::rotation(glm::vec3(0.0f, 0.0f, -1.0f), glm::normalize(light.direction));
            placed.entity = entity;

            m_lights.emplace(m_next_light++, placed);
        }
    }

    // The world lights to Lighting (unit conversion, a default sun when there are none), shadow casters over every entity's bounds where it
    // sits now
    std::shared_ptr<IIr77Return const> BuildLights() {
        std::vector<Ir77PVLight> lights{};

        for (auto const& [id, placed] : m_lights) {
            Ir77PVLight light = placed.light;
            light.position = placed.translation;
            light.direction = glm::normalize(placed.rotation * glm::vec3(0.0f, 0.0f, -1.0f));
            light.shadow = -1;

            lights.push_back(light);
        }

        glm::vec3 low{std::numeric_limits<float>::max()};
        glm::vec3 high{std::numeric_limits<float>::lowest()};

        for (auto const& [id, entity] : m_entities) {
            // the bounds' corners, placed
            glm::mat4 const place = EntityMatrix(entity);

            for (int corner = 0; corner < 8; corner++) {
                glm::vec3 const local{corner & 1 ? entity.bounds_max.x : entity.bounds_min.x, corner & 2 ? entity.bounds_max.y : entity.bounds_min.y,
                                      corner & 4 ? entity.bounds_max.z : entity.bounds_min.z};
                glm::vec3 const world = glm::vec3(place * glm::vec4(local, 1.0f));

                low = glm::min(low, world);
                high = glm::max(high, world);
            }
        }

        if (m_entities.empty()) {
            low = glm::vec3(-EMPTY_SCENE_HALF_SIZE);
            high = glm::vec3(EMPTY_SCENE_HALF_SIZE);
        }

        m_lighting->SetBounds(low, high);

        return m_lighting->CreateLights(lights);
    }

    glm::mat4 EntityMatrix(Ir77AnimateEntity const& entity) const {
        return glm::translate(glm::mat4(1.0f), entity.translation) * glm::mat4_cast(entity.rotation) * glm::scale(glm::mat4(1.0f), entity.scale);
    }

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

    std::map<std::uint32_t, Ir77AnimateEntity> m_entities{};  // [id] -- 0 is the scene file

    std::uint32_t m_next_entity{0};  // ids are never reused within a scene; CreateScene starts over at 0

    std::map<std::uint32_t, Ir77AnimateLight> m_lights{};  // [id] -- the world lights

    std::uint32_t m_next_light{0};  // like entity ids -- never reused within a scene

    Ir77SceneStats m_stats{};  // filled by CreateScene

    // per frame -- what CreateFrame hands the renderer
    Ir77PVFrame m_frame{};

    float m_last_seconds{0.0f};  // camera.time.y

    std::uint32_t m_frame_number{0};  // camera.time.z

    std::uint32_t m_opaque_items{0};

    std::uint32_t m_transparent_items{0};

    static constexpr float EMPTY_SCENE_HALF_SIZE{4.0f};  // the box a scene with no geometry frames
};
}  // namespace NSIr77Scene
