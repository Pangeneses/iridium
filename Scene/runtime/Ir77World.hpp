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
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77Scene.hpp"
#include "../dictionary/IDIr77Scene.hpp"

#include "../../Ir77RT/types/Ir77Types.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"
#include "../../Ir77RT/runtime/Ir77Return.hpp"

#include "../interface/IIr77Animation.hpp"
#include "../interface/IIr77Camera.hpp"
#include "../interface/IIr77Edit.hpp"
#include "../interface/IIr77Gizmo.hpp"
#include "../interface/IIr77Light.hpp"
#include "../interface/IIr77Lighting.hpp"
#include "../interface/IIr77Material.hpp"
#include "../interface/IIr77Mesh.hpp"
#include "../interface/IIr77Model.hpp"
#include "../interface/IIr77Texture.hpp"
#include "../interface/IIr77View.hpp"
#include "../interface/IIr77World.hpp"

#include "./Ir77Edit.hpp"
#include "./Ir77Gizmo.hpp"
#include "./Ir77Lighting.hpp"
#include "./Ir77Model.hpp"
#include "./Ir77Skinning.hpp"
#include "./Ir77View.hpp"

#include "../services/Ir77Flatten.hpp"

#include "../types/Ir77Stats.hpp"
#include "../types/Ir77Types.hpp"

// the direct link -- Scene -> PeregrineV's server, one way: Scene calls it, PeregrineV never sees Scene
#include "../../PeregrineV/server/Ir77Server.hpp"

// the fields module -- Scene -> Fields, one way: the frame asks it for its dispatches, draws and box
#include "../../Fields/server/Ir77Server.hpp"

using namespace NSIr77RT;
using namespace NSIr77PeregrineV;

namespace NSIr77Scene {

// =============================================================================================================================================
// Ir77World -- the scene module's work, owned by its server (Ir77Server) and reached only through it. The lifetime controller for the
// scene's peers -- the entities (one Model each: a file, its node tree and animation), View (the cameras), Lighting (lights and shadows),
// Edit (the edit cage) and Gizmo (the transform handles) -- which it creates and wires; and the one place that talks to all of them at once
// (plus the fields module, when set):
//
//   setup:     SetRenderer / SetField / SetViewSize / [SetBackground / SetGridVisible] -> CreateScene(path, use_mesh, lights, cameras) --
//              every entity and light goes, the file becomes entity 0: Model parses, the file goes to the renderer as one asset bundle
//              (LoadAsset), its lights become world lights, View takes its cameras and frames it
//   entities:  ImportEntity(path, use_mesh, lights) -> its id / RemoveEntity / SetEntityVisible / SetEntityTransform -- any number of files,
//              each its own asset in the renderer, each placed in the world as a whole; PickEntity finds the one under a point
//   lights:    AddLight / SetLight / RemoveLight / SetLightVisible -- world lights, the scene graph's: placed by the UI, or imported with a
//              file (a copy, placed where the file had it -- it doesn't follow the entity). Lighting gets them all, shadow casters over every
//              entity where it sits, on every change
//   edit mode: SetEditEntity -> Edit* (pick, box, select all, grab, gizmo, undo / redo) -> CommitEdits writes the edited models' .glb files
//   object:    SetOutlineEntity / SetObjectGizmo -> ObjectGizmo* -- the active entity's outline and its whole-transform gizmo
//   per frame: Update(dt) -> CreateFrame(seconds) -- one Ir77PVFrame: cameras (View), every entity's transforms and bones (its Model), lights
//              and shadow matrices (Lighting), the field's dispatches and draws (the fields server), the grid, the cage and the gizmo ->
//              SubmitFrame
//   the UI:    the scene server reaches the peers through GetModel(entity) / GetView for its ops
//
// The renderer makes the draws, the material tables, the skinning and the acceleration structures -- the scene only says what is there.
// =============================================================================================================================================
class Ir77World : public Ir77Enlisted, public IIr77World, public std::enable_shared_from_this<Ir77World> {
   private:
    // one entity -- a file in the world: its Model, its asset in the renderer, where it sits, and its frame data
    struct Ir77WorldModel {
        std::shared_ptr<IIr77Model> model{};
        std::uint32_t asset{IR77PV_NO_ASSET};
        std::string path{};
        glm::vec3 translation{0.0f};
        glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
        glm::vec3 scale{1.0f};
        glm::vec3 bounds_min{-1.0f};  // the file's own space
        glm::vec3 bounds_max{1.0f};
        std::vector<glm::mat4> transforms{};  // its instances, in its own slot order
        std::vector<glm::mat4> bones{};       // its joint palettes, back to back
        bool visible{true};                   // false -- left out of the frame (SetEntityVisible)
        std::uint32_t instance_base{0};       // its first models[] slot -- where LoadAsset put it (the edit cage is placed through it)
        bool geometry_dirty{false};           // an edit its file doesn't hold yet -- CommitEdits writes it
        Ir77SceneStats stats{};               // its file's counts and its load's report -- LoadEntity; GetSceneStats adds them up
    };

    // one world light -- what it is (type, color, intensity in glTF units: lux / candela, range, cones) and where it sits; it shines down its
    // local -Z. light.position / direction are made from the placement when the lights are built.
    struct Ir77WorldLight {
        Ir77PVLight light{};
        glm::vec3 translation{0.0f};
        glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
        std::uint32_t entity{UINT32_MAX};  // the entity it was imported with; UINT32_MAX = placed by the UI
        bool visible{true};                // false -- off (SetLightVisible)
    };

   public:
    Ir77World() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();

        // the peers -- they never see World. Entities come with the scene.
        m_view = std::static_pointer_cast<IIr77View>(std::make_shared<Ir77View>());
        m_lighting = std::static_pointer_cast<IIr77Lighting>(std::make_shared<Ir77Lighting>());
        m_edit = std::static_pointer_cast<IIr77Edit>(std::make_shared<Ir77Edit>());
        m_gizmo = std::static_pointer_cast<IIr77Gizmo>(std::make_shared<Ir77Gizmo>());
        m_edit->SetGizmo(m_gizmo);  // edit mode and object mode share one gizmo -- one mode, one draw
    }

    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77World>(uid);

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

        else if (iid == GUIDIIr77World)
            obj = std::shared_ptr<IIr77World>(shared_from_this(), static_cast<IIr77World*>(this));

        else if (iid == GUIDIr77World)
            obj = std::shared_ptr<Ir77World>(shared_from_this(), static_cast<Ir77World*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

    // The renderer's server -- where the scene's asset goes (CreateScene) and each frame goes (CreateFrame). Its forwarding methods are the
    // only way into PeregrineV.
    std::shared_ptr<IIr77Return const> SetRenderer(std::shared_ptr<NSIr77PeregrineV::Ir77Server> const& renderer) {
        m_renderer = renderer;

        m_gizmo->SetRenderer(renderer);
        return m_edit->SetRenderer(renderer);
    }

    // The fields module's server -- CreateFrame asks it for its dispatches, draws and box. Optional: without it the frame has no field.
    std::shared_ptr<IIr77Return const> SetField(std::shared_ptr<NSIr77Fields::Ir77Server> const& fields) {
        m_fields = fields;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The windows CreateFrame fills, and their size -- the cameras' aspect. Any time; the next frame uses it.
    std::shared_ptr<IIr77Return const> SetViewSize(std::size_t const& windows, int const& width, int const& height) {
        m_view_windows = windows;
        m_view_width = width;
        m_view_height = height;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The background every window clears to -- sRGB 0..1; CreateFrame hands the renderer its linear value
    std::shared_ptr<IIr77Return const> SetBackground(glm::vec3 const& color) {
        m_background = glm::clamp(color, glm::vec3(0.0f), glm::vec3(1.0f));

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetBackground(glm::vec3& color) const {
        color = m_background;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The editor grid -- one procedural draw per view at the end of the opaque list (grid.vert / grid.frag); the next frame uses it
    std::shared_ptr<IIr77Return const> SetGridVisible(bool const& visible) {
        m_grid_visible = visible;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetGridVisible(bool& visible) const {
        visible = m_grid_visible;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The scene: every entity and light goes, then the glTF becomes entity 0 -- its clips with it, its lights as world lights (lights), its
    // cameras (cameras; every view is framed on it either way). use_mesh -> mesh shading for the static geometry when the device has it.
    // No geometry is fine: an empty path, a file with nothing drawable, or a file that fails to load all leave a running scene with no
    // geometry. A file that fails to load is logged (and listed in the stats warnings), not a failure.
    std::shared_ptr<IIr77Return const> CreateScene(std::string const& path, bool const& use_mesh, bool const& lights, bool const& cameras) {
        if (!m_renderer) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77World: SetRenderer not called.");

        std::shared_ptr<IIr77Return const> result{};

        // the scene before goes first -- its draws stop with the next frame; the ids start over, so edit mode and the outline go with it
        SetEditEntity(EDIT_NO_ENTITY);
        m_outline_entity = EDIT_NO_ENTITY;
        for (auto const& [id, entity] : m_entities) m_renderer->UnloadAsset(entity.asset);

        m_entities.clear();
        m_next_entity = 0;
        m_lights.clear();
        m_next_light = 0;
        m_scene_ready = false;

        // a file that won't load (ParseAsset logged why) -> carry on with an empty scene
        auto model = std::static_pointer_cast<IIr77Model>(std::make_shared<Ir77Model>());

        bool const load_failed = model->ParseAsset(path)->ID() != GUIDIr77OperationSucceeded;
        if (load_failed) model->ParseAsset(std::string{});

        Ir77PVAssetReport report{};
        Ir77PVAssetBundle bundle{};
        std::uint32_t entity{0};

        if (Failed(LoadEntity(model, path, use_mesh, bundle, report, entity), result)) return result;

        Ir77WorldModel const& first = m_entities.at(entity);

        if (lights) ImportLights(entity);

        if (Failed(BuildLights(), result)) return result;

        // cameras -- the file's (when imported), then the controlled ones framed on it; view 0 starts on the file's first camera if it has one
        std::shared_ptr<IIr77Camera> scene_cameras{};
        if (cameras) model->GetCameras(scene_cameras);

        m_view->SetCameras(scene_cameras);
        m_view->SetBounds(first.bounds_min, first.bounds_max);

        // this scene's warnings start over -- the grid's are the module's (GetSceneStats adds them)
        m_scene_warnings.clear();
        if (load_failed) m_scene_warnings.push_back("scene file didn't load -- running with an empty scene");

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
        if (!m_renderer) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77World: SetRenderer not called.");
        if (path.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77World: an entity needs a file.");

        std::shared_ptr<IIr77Return const> result{};

        auto model = std::static_pointer_cast<IIr77Model>(std::make_shared<Ir77Model>());
        if (Failed(model->ParseAsset(path), result)) return result;

        Ir77PVAssetReport report{};
        Ir77PVAssetBundle bundle{};

        if (Failed(LoadEntity(model, path, use_mesh, bundle, report, entity), result)) return result;

        if (lights) ImportLights(entity);

        if (Failed(BuildLights(), result)) return result;

        PrintSceneStats();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The entity's asset leaves the renderer (its draws stop with the next frame). Lights it brought stay -- they're world lights now.
    std::shared_ptr<IIr77Return const> RemoveEntity(std::uint32_t const& entity) {
        auto const found = m_entities.find(entity);
        if (found == m_entities.end()) return NoEntity();

        if (entity == m_edit_entity) SetEditEntity(EDIT_NO_ENTITY);
        if (entity == m_outline_entity) m_outline_entity = EDIT_NO_ENTITY;

        m_renderer->UnloadAsset(found->second.asset);
        m_entities.erase(found);

        return BuildLights();
    }

    std::shared_ptr<IIr77Return const> SetEntityVisible(std::uint32_t const& entity, bool const& visible) {
        auto const found = m_entities.find(entity);
        if (found == m_entities.end()) return NoEntity();

        found->second.visible = visible;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Where the whole entity sits in the world -- everything it draws moves with it from the next frame, and the shadow casters' box with it
    std::shared_ptr<IIr77Return const> SetEntityTransform(std::uint32_t const& entity, glm::vec3 const& translation, glm::quat const& rotation,
                                                          glm::vec3 const& scale) {
        auto const found = m_entities.find(entity);
        if (found == m_entities.end()) return NoEntity();

        Ir77WorldModel& placed = found->second;
        placed.translation = translation;
        placed.rotation = glm::normalize(rotation);
        placed.scale = scale;

        std::shared_ptr<IIr77Return const> result{};
        if (Failed(placed.model->SetRootTransform(EntityMatrix(placed)), result)) return result;

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
        Ir77WorldLight placed{};
        placed.light = light;
        placed.translation = translation;
        placed.rotation = glm::normalize(rotation);

        id = m_next_light++;
        m_lights.emplace(id, placed);

        return BuildLights();
    }

    std::shared_ptr<IIr77Return const> SetLight(std::uint32_t const& id, Ir77PVLight const& light, glm::vec3 const& translation, glm::quat const& rotation) {
        auto const found = m_lights.find(id);
        if (found == m_lights.end()) return NoLight();

        found->second.light = light;
        found->second.translation = translation;
        found->second.rotation = glm::normalize(rotation);

        return BuildLights();
    }

    std::shared_ptr<IIr77Return const> RemoveLight(std::uint32_t const& id) {
        if (m_lights.erase(id) == 0) return NoLight();

        return BuildLights();
    }

    std::shared_ptr<IIr77Return const> SetLightVisible(std::uint32_t const& id, bool const& visible) {
        auto const found = m_lights.find(id);
        if (found == m_lights.end()) return NoLight();

        found->second.visible = visible;

        return BuildLights();
    }

    // One world light -- entity is the one it was imported with (UINT32_MAX = placed by the UI)
    std::shared_ptr<IIr77Return const> GetLight(std::uint32_t const& id, Ir77PVLight& light, glm::vec3& translation, glm::quat& rotation,
                                                std::uint32_t& entity) {
        auto const found = m_lights.find(id);
        if (found == m_lights.end()) return NoLight();

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

    // Once per frame, before CreateFrame: every entity's animation, world matrices and joint palettes (its Model), then the held camera keys
    // (View)
    std::shared_ptr<IIr77Return const> Update(float const& dt) {
        std::shared_ptr<IIr77Return const> result{};

        for (auto const& [id, entity] : m_entities)
            if (Failed(entity.model->Update(dt), result)) return result;

        return m_view->Update(dt);
    }

    // Every frame, after Update: the frame -- every window's cameras, every entity's transforms and bones, the lights, the field's dispatches
    // and draws -- to the renderer in one SubmitFrame.
    std::shared_ptr<IIr77Return const> CreateFrame(float const& seconds) {
        if (!m_scene_ready) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: CreateScene not called.");

        std::shared_ptr<IIr77Return const> result{};

        float const dt = seconds - m_last_seconds;
        float const aspect = m_view_height > 0 ? static_cast<float>(m_view_width) / static_cast<float>(m_view_height) : 1.78f;

        // the entities -- their instances and joint palettes as their Models left them
        std::size_t const shown = static_cast<std::size_t>(std::count_if(m_entities.begin(), m_entities.end(), [](auto const& e) { return e.second.visible; }));
        m_frame.assets.resize(shown);

        std::size_t a{0};
        for (auto& [id, entity] : m_entities) {
            if (!entity.visible) continue;  // hidden -- out of the frame, so out of the draws and the rays

            entity.model->WriteInstanceTransforms(entity.transforms);
            entity.model->WriteBoneMatrices(entity.bones);

            Ir77PVFrameAsset& asset = m_frame.assets[a++];
            asset.asset = entity.asset;
            asset.transforms = entity.transforms;
            asset.bones = entity.bones;
            asset.outline = id == m_outline_entity ? SELECT_OUTLINE_COLOR : glm::vec4(0.0f);
        }

        // compute -- the field's (the renderer runs the entities' skinning ahead of it)
        m_frame.dispatches.clear();
        if (m_fields) m_fields->AppendDispatches(m_frame.dispatches, dt);

        // the field's box -- to Lighting only when it changed (SetFieldBounds rebuilds the shadow casters)
        UpdateFieldBounds();

        m_lighting->WriteFrame(m_frame);

        // the field's shadow draws -- one per shadow tile
        std::uint32_t shadows{0};
        m_lighting->GetShadowCount(shadows);

        // every window -- one camera per view (each a file camera rebuilt with this aspect, or that view's controlled camera), and the field's draws
        m_frame.windows.assign(m_view_windows, Ir77PVFrameWindow{});

        // the background, sRGB -> linear (the swapchain is sRGB and converts it back)
        glm::vec4 clear_color{1.0f};
        for (int c = 0; c < 3; c++) {
            float const s = m_background[c];
            clear_color[c] = s <= 0.04045f ? s / 12.92f : std::pow((s + 0.055f) / 1.055f, 2.4f);
        }

        for (std::size_t w = 0; w < m_view_windows; w++) {
            Ir77PVFrameWindow& window = m_frame.windows[w];

            window.clear_color = clear_color;

            m_view->WriteCameras(aspect, window.cameras, window.areas);
            for (Ir77PVCamera& camera : window.cameras) camera.time = glm::vec4(seconds, dt, static_cast<float>(m_frame_number), 0.0f);

            if (m_fields) {
                m_fields->AppendDraws(w, Ir77PVPass::Opaque, shadows, window.opaque);
                m_fields->AppendDraws(w, Ir77PVPass::Shadow, shadows, window.shadow);
                m_fields->AppendDraws(w, Ir77PVPass::Transparent, shadows, window.transparent);
            }

            AppendGrid(w, window.opaque);

            // the edit cage -- in the transparent list, after the model's own transparent draws: tested against the opaque depth (the model's
            // solid faces, the grid), never blended under an alpha surface
            if (EditShown()) {
                m_edit->AppendDraws(w, window.transparent);
                m_edit->AppendGizmo(w, m_entities.at(m_edit_entity).transforms, window.transparent);  // last -- on top of everything
            } else if (m_object_gizmo_shown) {
                m_gizmo->Append(w, ObjectPlacement(), window.transparent);  // the active entity's, object mode -- last, on top
            }
        }

        Ir77PVFrameReport report{};
        if (Failed(m_renderer->SubmitFrame(m_frame, report), result)) return result;

        m_opaque_items = report.opaque_items;
        m_transparent_items = report.transparent_items;

        m_last_seconds = seconds;
        m_frame_number++;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The model drawn with the selection's outline from the next frame -- EDIT_NO_ENTITY: none. An id that isn't a model here: none.
    std::shared_ptr<IIr77Return const> SetOutlineEntity(std::uint32_t const& entity) {
        m_outline_entity = m_entities.count(entity) > 0 ? entity : EDIT_NO_ENTITY;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // ---- edit mode -------------------------------------------------------------------------------------------------------------------------

    // The model in edit mode -- it stops playing and rests in its rest pose (its clips are held, ResumeEditClips plays them again when it
    // leaves), then its cage (vertices and edges) is built in that pose and drawn over it from the next frame. EDIT_NO_ENTITY leaves edit mode.
    // A model with no triangles goes into edit mode with an empty cage. The model already in edit mode -> nothing changes.
    std::shared_ptr<IIr77Return const> SetEditEntity(std::uint32_t const& entity) {
        if (entity == m_edit_entity && entity != EDIT_NO_ENTITY) return Ir77RETURN<Ir77OperationSucceeded>();

        NoteEditModified();
        ResumeEditClips();
        m_edit_entity = EDIT_NO_ENTITY;

        if (entity == EDIT_NO_ENTITY) return m_edit->ReleaseCage();

        auto const found = m_entities.find(entity);
        if (found == m_entities.end()) {
            m_edit->ReleaseCage();
            return NoEntity();
        }

        Ir77WorldModel& placed = found->second;
        std::shared_ptr<IIr77Return const> result{};

        // what it plays, held -- then nothing, and the rest pose now rather than on the next Update
        HoldClips(placed.model, m_edit_clips);
        placed.model->StopAllAnimations();
        if (Failed(placed.model->Update(0.0f), result)) return result;

        placed.model->WriteInstanceTransforms(placed.transforms);
        placed.model->WriteBoneMatrices(placed.bones);

        std::vector<std::uint32_t> bone_bases{};
        placed.model->WriteInstanceBoneBases(bone_bases, placed.transforms.size());

        std::shared_ptr<IIr77Mesh> meshes{};
        placed.model->GetMeshes(meshes);

        m_edit_entity = entity;

        std::shared_ptr<IIr77Material> materials{};
        placed.model->GetMaterials(materials);

        if (Failed(m_edit->CreateCage(meshes, materials, placed.asset, placed.instance_base, placed.bones, bone_bases), result))
            SetEditEntity(EDIT_NO_ENTITY);  // its clips back

        return result;
    }

    std::shared_ptr<IIr77Return const> GetEditEntity(std::uint32_t& entity, std::uint32_t& points, std::uint32_t& edges) const {
        entity = m_edit_entity;

        return m_edit->GetCageCounts(points, edges);
    }

    // The clips the model in edit mode was playing -- what it plays again when it leaves; empty when nothing is in edit mode
    std::shared_ptr<IIr77Return const> GetEditClips(std::vector<Ir77EditClip>& clips) const {
        clips = m_edit_entity == EDIT_NO_ENTITY ? std::vector<Ir77EditClip>{} : m_edit_clips;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The model under a point of window 0 -- x, y fractions of the window, from its top left. The view whose area holds the point (the last
    // one, it draws on top) casts a ray from its camera as the last frame drew it; every shown model's triangles (static and skinned) where they
    // sat in that frame; the nearest hit. Nothing hit -> EDIT_NO_ENTITY.
    std::shared_ptr<IIr77Return const> PickEntity(float const& x, float const& y, std::uint32_t& entity) {
        entity = EDIT_NO_ENTITY;

        Ir77EditView view{};
        if (!ViewAt(x, y, view)) return Ir77RETURN<Ir77OperationSucceeded>();

        float nearest = std::numeric_limits<float>::max();

        for (auto const& [id, placed] : m_entities) {
            if (!placed.visible) continue;

            float distance{0.0f};
            if (PickTriangles(placed, view.origin, view.ray, distance) && distance < nearest) {
                nearest = distance;
                entity = id;
            }
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // What a click in edit mode selects -- vertex / edge / face
    std::shared_ptr<IIr77Return const> SetEditMode(Ir77EditMode const& mode) {
        if (m_edit_entity == EDIT_NO_ENTITY) return NotEditing();

        return m_edit->SetMode(mode);
    }

    // A click in edit mode -- x, y fractions of window 0 from its top left. The nearest visible vertex / edge / face of the model in edit mode
    // under it, through the view there, as the last frame drew it. extend: toggle it in the selection; otherwise it replaces the selection, and
    // a miss clears it.
    std::shared_ptr<IIr77Return const> EditPick(float const& x, float const& y, bool const& extend, bool& hit) {
        hit = false;

        auto const found = m_entities.find(m_edit_entity);
        if (m_edit_entity == EDIT_NO_ENTITY || found == m_entities.end()) return NotEditing();

        Ir77EditView view{};
        if (!ViewAt(x, y, view)) return Ir77RETURN<Ir77OperationSucceeded>();  // outside every view -- nothing changes

        return m_edit->Pick(view, found->second.transforms, extend, hit);
    }

    // A box drag in edit mode -- from (x0, y0) to (x1, y1), fractions of window 0; the view the drag started in. Every visible element inside
    // it, by the mode; extend adds them, otherwise they replace the selection. selected = the elements taken.
    std::shared_ptr<IIr77Return const> EditBoxSelect(float const& x0, float const& y0, float const& x1, float const& y1, bool const& extend,
                                                     std::uint32_t& selected) {
        selected = 0;

        auto const found = m_entities.find(m_edit_entity);
        if (m_edit_entity == EDIT_NO_ENTITY || found == m_entities.end()) return NotEditing();

        std::size_t view_index{0};
        Ir77EditView start{};
        Ir77EditView end{};
        if (!ViewIndexAt(x0, y0, view_index) || !ViewRay(view_index, x0, y0, start) || !ViewRay(view_index, x1, y1, end))
            return Ir77RETURN<Ir77OperationSucceeded>();

        return m_edit->BoxSelect(start, start.cursor, end.cursor, found->second.transforms, extend, selected);
    }

    // Every vertex of the model in edit mode (true) or none
    std::shared_ptr<IIr77Return const> EditSelectAll(bool const& select) {
        if (m_edit_entity == EDIT_NO_ENTITY) return NotEditing();

        return m_edit->SelectAll(select);
    }

    // The edit mode's mode and how many vertices are selected
    std::shared_ptr<IIr77Return const> GetEditSelection(Ir77EditMode& mode, std::uint32_t& selected) const { return m_edit->GetSelection(mode, selected); }

    // The cursor at x, y now -> the selection moved (axis: free on the plane facing the camera, or along one world axis). delta = the move,
    // world space.
    std::shared_ptr<IIr77Return const> EditGrabMove(float const& x, float const& y, Ir77EditAxis const& axis, glm::vec3& delta) {
        delta = glm::vec3(0.0f);

        Ir77EditView view{};
        if (!ViewRay(m_grab_view, x, y, view)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77World: the grab's view is gone.");

        return m_edit->MoveGrab(view, axis, delta);
    }

    // confirm: the move stays (a save writes it into the model's file); otherwise it goes back
    std::shared_ptr<IIr77Return const> EditGrabEnd(bool const& confirm) { return m_edit->EndGrab(confirm); }

    // What the gizmo does -- translate / rotate / scale
    std::shared_ptr<IIr77Return const> SetGizmoMode(Ir77GizmoMode const& mode) { return m_edit->SetGizmoMode(mode); }

    std::shared_ptr<IIr77Return const> GetGizmo(Ir77GizmoMode& mode, std::uint32_t& hot) const { return m_gizmo->GetMode(mode, hot); }

    // ---- object mode's gizmo -- the active entity's whole transform -----------------------------------------------------------------------

    // shown at world (the active entity's world matrix: its pivot, and its own axes for a scale) from the next frame; hidden: none
    std::shared_ptr<IIr77Return const> SetObjectGizmo(bool const& shown, glm::mat4 const& world) {
        m_object_gizmo_shown = shown;
        m_object_world = world;

        if (!shown) m_gizmo->ClearHot();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // the cursor (x, y fractions of window 0) -- the handle under it lights up; handle = it (GIZMO_NONE: none)
    std::shared_ptr<IIr77Return const> ObjectGizmoHover(float const& x, float const& y, std::uint32_t& handle) {
        handle = GIZMO_NONE;
        if (!m_object_gizmo_shown) return m_gizmo->ClearHot();

        Ir77EditView view{};
        if (!ViewAt(x, y, view)) return m_gizmo->ClearHot();

        return m_gizmo->Hover(view, ObjectPlacement(), handle);
    }

    // a press on a handle starts a drag in the view it's in -- handle = the one pressed (GIZMO_NONE: none, the press is a pick)
    std::shared_ptr<IIr77Return const> ObjectGizmoPress(float const& x, float const& y, std::uint32_t& handle) {
        handle = GIZMO_NONE;
        if (!m_object_gizmo_shown) return Ir77RETURN<Ir77OperationSucceeded>();

        Ir77EditView view{};
        if (!ViewIndexAt(x, y, m_grab_view) || !ViewRay(m_grab_view, x, y, view)) return Ir77RETURN<Ir77OperationSucceeded>();

        return m_gizmo->Press(view, ObjectPlacement(), handle);
    }

    // the cursor now -> the drag as one world matrix about the pivot pressed at, since the press; changed = not the identity
    std::shared_ptr<IIr77Return const> ObjectGizmoDrag(float const& x, float const& y, glm::mat4& world, bool& changed) {
        world = glm::mat4(1.0f);
        changed = false;

        Ir77EditView view{};
        if (!ViewRay(m_grab_view, x, y, view)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77World: the drag's view is gone.");

        return m_gizmo->Drag(view, world, changed);
    }

    std::shared_ptr<IIr77Return const> ObjectGizmoEnd() { return m_gizmo->End(); }

    // The cursor over the viewport (x, y fractions of window 0) -- the gizmo handle under it lights up. handle = it (GIZMO_NONE: none).
    std::shared_ptr<IIr77Return const> EditGizmoHover(float const& x, float const& y, std::uint32_t& handle) {
        handle = GIZMO_NONE;

        auto const found = m_entities.find(m_edit_entity);
        if (m_edit_entity == EDIT_NO_ENTITY || found == m_entities.end()) return NotEditing();

        Ir77EditView view{};
        if (!ViewAt(x, y, view)) return m_edit->ClearGizmoHot();

        return m_edit->HoverGizmo(view, found->second.transforms, handle);
    }

    // A press (x, y fractions of window 0): on a gizmo handle it starts a transform of the selection -- a grab the moves and EditGrabEnd drive,
    // in the view it started in. handle = the one pressed (GIZMO_NONE: none -- the press is the viewport's, a pick).
    std::shared_ptr<IIr77Return const> EditGizmoPress(float const& x, float const& y, std::uint32_t& handle) {
        handle = GIZMO_NONE;

        auto const found = m_entities.find(m_edit_entity);
        if (m_edit_entity == EDIT_NO_ENTITY || found == m_entities.end()) return NotEditing();

        Ir77EditView view{};
        if (!ViewIndexAt(x, y, m_grab_view) || !ViewRay(m_grab_view, x, y, view)) return Ir77RETURN<Ir77OperationSucceeded>();

        return m_edit->PressGizmo(view, found->second.transforms, handle);
    }

    // grabbing = a grab is under way; modified = the model in edit mode has a confirmed edit
    std::shared_ptr<IIr77Return const> GetEditGrab(bool& grabbing, bool& modified) const { return m_edit->GetGrab(grabbing, modified); }

    // the last confirmed edit of the model in edit mode back / again; done = false: nothing there
    std::shared_ptr<IIr77Return const> EditUndo(bool& done) {
        done = false;
        if (m_edit_entity == EDIT_NO_ENTITY) return NotEditing();

        return m_edit->Undo(done);
    }

    std::shared_ptr<IIr77Return const> EditRedo(bool& done) {
        done = false;
        if (m_edit_entity == EDIT_NO_ENTITY) return NotEditing();

        return m_edit->Redo(done);
    }

    std::shared_ptr<IIr77Return const> GetEditHistory(std::uint32_t& undo, std::uint32_t& redo) const { return m_edit->GetHistory(undo, redo); }

    // Every model with an edit its file doesn't hold yet -> its .glb rewritten with the edited positions and normals (temp file, then rename
    // -- a failed write leaves the old file whole). Other entities on the same file take the edit too. written = the files written; errors =
    // one line each for the ones that weren't.
    std::shared_ptr<IIr77Return const> CommitEdits(std::vector<std::string>& written, std::vector<std::string>& errors) {
        written.clear();
        errors.clear();

        NoteEditModified();

        for (auto& [id, placed] : m_entities) {
            if (!placed.geometry_dirty) continue;

            std::string error{};
            if (!WriteModel(placed, error)) {
                errors.push_back(placed.path + ": " + error);
                continue;
            }

            placed.geometry_dirty = false;
            written.push_back(placed.path);

            ShareGeometry(id);
        }

        auto const edited = m_entities.find(m_edit_entity);
        if (edited != m_entities.end() && !edited->second.geometry_dirty) m_edit->MarkSaved();

        if (!errors.empty()) return Ir77RETURN<Ir77OperationFailed>(this, "[WARN] Ir77World: " + std::to_string(errors.size()) + " model(s) not written.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // ---- the peers -- the scene server holds these for its ops ----------------------------------------------------------------------------

    // An entity's Model -- its clips and nodes
    std::shared_ptr<IIr77Return const> GetModel(std::uint32_t const& entity, std::shared_ptr<IIr77Model>& model) {
        auto const found = m_entities.find(entity);
        if (found == m_entities.end()) return NoEntity();

        model = found->second.model;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetView(std::shared_ptr<IIr77View>& view) {
        view = m_view;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetLighting(std::shared_ptr<IIr77Lighting>& lighting) {
        lighting = m_lighting;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // stats -- copies, safe to keep
    // -------------------------------------------------------------------------------------------------------------------------------------

    // The scene as it is now -- every entity's counts added up (path: the scene file's, entity 0), every model's clips in entity order, the
    // world lights as Lighting last built them (with the casters' shadow tiles), the GPU and render path of the last load, this scene's
    // warnings and the module's
    std::shared_ptr<IIr77Return const> GetSceneStats(Ir77SceneStats& stats) const {
        stats = Ir77SceneStats{};
        stats.gpu = m_gpu;
        stats.render_path = m_render_path;

        auto const scene = m_entities.find(0);
        if (scene != m_entities.end()) stats.path = scene->second.path;

        for (auto const& [id, entity] : m_entities) {
            Ir77SceneStats const& e = entity.stats;

            stats.nodes += e.nodes;
            stats.meshes += e.meshes;
            stats.materials += e.materials;
            stats.textures += e.textures;
            stats.cameras += e.cameras;
            stats.skins += e.skins;
            stats.bones += e.bones;
            stats.clips += e.clips;
            stats.static_vertices += e.static_vertices;
            stats.static_indices += e.static_indices;
            stats.skinned_vertices += e.skinned_vertices;
            stats.skinned_indices += e.skinned_indices;
            stats.static_primitives += e.static_primitives;
            stats.skinned_primitives += e.skinned_primitives;
            stats.instances += e.instances;
            stats.opaque_runs += e.opaque_runs;
            stats.skinned_runs += e.skinned_runs;
            stats.transparent_objects += e.transparent_objects;
            stats.clip_names.insert(stats.clip_names.end(), e.clip_names.begin(), e.clip_names.end());
        }

        m_lighting->GetLights(stats.lights);

        stats.warnings = m_scene_warnings;
        if (m_grid_missing) stats.warnings.push_back("no \"grid\" pipeline in the manifest -- the editor grid is off");

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

        Ir77WorldModel const& placed = scene->second;
        std::shared_ptr<IIr77Model> const& model = placed.model;

        std::uint32_t players{0};
        model->GetPlayerCount(players);

        for (std::uint32_t p = 0; p < players; p++) {
            Ir77ClipStat clip_stat{};
            std::uint32_t clip{UINT32_MAX};

            if (model->GetPlayer(p, clip, clip_stat.time, clip_stat.duration, clip_stat.speed, clip_stat.loop)->ID() != GUIDIr77OperationSucceeded) continue;
            if (clip < placed.stats.clip_names.size()) clip_stat.name = placed.stats.clip_names[clip];

            stats.playing.push_back(clip_stat);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    // the failures more than one method gives
    std::shared_ptr<IIr77Return const> NoEntity() const { return Ir77RETURN<Ir77NotConfigured>(this, "Ir77World: no such entity."); }

    std::shared_ptr<IIr77Return const> NoLight() const { return Ir77RETURN<Ir77NotConfigured>(this, "Ir77World: no such light."); }

    std::shared_ptr<IIr77Return const> NotEditing() const { return Ir77RETURN<Ir77NotConfigured>(this, "Ir77World: nothing is in edit mode."); }

    // The field's box -> Lighting, only when it moved or the field came up / went down -- SetFieldBounds rebuilds the shadow casters. No
    // fields server: nothing to cover.
    void UpdateFieldBounds() {
        bool active{false};
        glm::vec3 low{0.0f};
        glm::vec3 high{0.0f};

        if (m_fields) m_fields->GetBounds(active, low, high);

        if (active == m_field_active && (!active || (low == m_field_low && high == m_field_high))) return;

        m_field_active = active;
        m_field_low = low;
        m_field_high = high;

        m_lighting->SetFieldBounds(active, low, high);
    }

    // the model in edit mode has a confirmed edit -> its file owes it
    void NoteEditModified() {
        auto const found = m_entities.find(m_edit_entity);
        if (found == m_entities.end()) return;

        bool grabbing{false};
        bool modified{false};
        m_edit->GetGrab(grabbing, modified);

        if (modified) found->second.geometry_dirty = true;
    }

    // One model's mesh -> its file: per file primitive, every stream vertex written back to the file vertex it came from
    bool WriteModel(Ir77WorldModel const& placed, std::string& error) const {
        std::shared_ptr<IIr77Mesh> meshes{};
        placed.model->GetMeshes(meshes);
        if (!meshes) {
            error = "no meshes";
            return false;
        }

        std::map<std::pair<std::size_t, std::size_t>, Ir77FlattenOverride> overrides{};

        // one stream's ranges into the overrides -- position(v) / normal(v) of its v-th vertex
        auto const stream = [&](bool const skinned, std::vector<Ir77MeshRange> const& ranges, std::vector<std::uint32_t> const& counts,
                                auto const& position, auto const& normal) {
            std::vector<Ir77MeshOrigin> origins{};
            std::vector<std::uint32_t> files{};
            meshes->GetVertexOrigins(skinned, origins, files);

            for (std::size_t r = 0; r < ranges.size() && r < origins.size() && r < counts.size(); r++) {
                Ir77MeshOrigin const& origin = origins[r];
                Ir77FlattenOverride& edit = overrides[{origin.mesh, origin.primitive}];

                if (edit.positions.empty()) {
                    edit.positions.assign(origin.vertex_count, glm::vec3(0.0f));
                    edit.normals.assign(origin.vertex_count, glm::vec3(0.0f, 1.0f, 0.0f));
                    edit.set.assign(origin.vertex_count, 0);
                }

                std::size_t const first = static_cast<std::size_t>(std::max(ranges[r].vertex_offset, 0));
                for (std::size_t v = first; v < first + counts[r] && v < files.size(); v++) {
                    std::uint32_t const file = files[v];
                    if (file >= origin.vertex_count) continue;

                    edit.positions[file] = position(v);
                    edit.normals[file] = normal(v);
                    edit.set[file] = 1;
                }
            }
        };

        std::vector<Ir77PVVertex> vertices{};
        std::vector<Ir77MeshRange> ranges{};
        meshes->GetVertexBuffer(vertices);
        meshes->GetMeshRanges(ranges);

        std::vector<Ir77PVVertexSkinned> vertices_skinned{};
        std::vector<Ir77MeshRange> ranges_skinned{};
        meshes->GetSkinnedVertexBuffer(vertices_skinned);
        meshes->GetSkinnedMeshRanges(ranges_skinned);

        // a range's vertex count -- up to the next range's start, or the stream's end
        auto const counts = [](std::vector<Ir77MeshRange> const& rs, std::size_t const total) {
            std::vector<std::uint32_t> out(rs.size(), 0);
            for (std::size_t r = 0; r < rs.size(); r++) {
                std::size_t const start = static_cast<std::size_t>(std::max(rs[r].vertex_offset, 0));
                std::size_t const end = r + 1 < rs.size() ? static_cast<std::size_t>(std::max(rs[r + 1].vertex_offset, 0)) : total;
                out[r] = static_cast<std::uint32_t>(end > start ? end - start : 0);
            }
            return out;
        };

        stream(false, ranges, counts(ranges, vertices.size()), [&](std::size_t const v) { return vertices[v].position; },
               [&](std::size_t const v) { return vertices[v].normal; });
        stream(true, ranges_skinned, counts(ranges_skinned, vertices_skinned.size()), [&](std::size_t const v) { return vertices_skinned[v].position; },
               [&](std::size_t const v) { return vertices_skinned[v].normal; });

        // the file again, with the edits in
        std::filesystem::path const file{placed.path};

        Ir77Flatten flatten{};
        std::vector<std::byte> glb{};
        if (!flatten.Rewrite(file.string(), file.stem().string(), overrides, glb, error)) return false;

        std::filesystem::path temp = file;
        temp += ".tmp";

        {
            std::ofstream out{temp, std::ios::binary | std::ios::trunc};
            out.write(reinterpret_cast<char const*>(glb.data()), static_cast<std::streamsize>(glb.size()));
            out.flush();

            if (!out) {
                error = "couldn't write " + temp.string();
                return false;
            }
        }

        std::error_code ec{};
        std::filesystem::rename(temp, file, ec);
        if (ec) {
            error = "couldn't replace " + file.string() + ": " + ec.message();
            return false;
        }

        return true;
    }

    // the entities on the same file as entity -- its edited vertices, in their meshes and the renderer
    void ShareGeometry(std::uint32_t const entity) {
        Ir77WorldModel const& source = m_entities.at(entity);

        std::shared_ptr<IIr77Mesh> meshes{};
        source.model->GetMeshes(meshes);
        if (!meshes) return;

        std::vector<Ir77PVVertex> vertices{};
        std::vector<Ir77PVVertexSkinned> vertices_skinned{};
        meshes->GetVertexBuffer(vertices);
        meshes->GetSkinnedVertexBuffer(vertices_skinned);

        for (auto& [id, placed] : m_entities) {
            if (id == entity || placed.path != source.path) continue;

            std::shared_ptr<IIr77Mesh> other{};
            placed.model->GetMeshes(other);
            if (!other) continue;

            if (other->UpdateVertices(vertices)->ID() != GUIDIr77OperationSucceeded) continue;  // a different count -- not the same load
            if (!vertices_skinned.empty()) other->UpdateSkinnedVertices(vertices_skinned);

            m_renderer->UpdateAssetVertices(placed.asset, vertices, vertices_skinned, glm::uvec2(0u, static_cast<std::uint32_t>(vertices.size())),
                                            glm::uvec2(0u, static_cast<std::uint32_t>(vertices_skinned.size())));
        }
    }

    // A model's players -> clips (each clip once, the speed and loop it plays with)
    static void HoldClips(std::shared_ptr<IIr77Model> const& model, std::vector<Ir77EditClip>& clips) {
        clips.clear();

        std::uint32_t players{0};
        model->GetPlayerCount(players);

        for (std::uint32_t p = 0; p < players; p++) {
            Ir77EditClip held{};
            float time{0.0f};
            float duration{0.0f};
            if (model->GetPlayer(p, held.clip, time, duration, held.speed, held.loop)->ID() == GUIDIr77OperationSucceeded) clips.push_back(held);
        }
    }

    // The model leaving edit mode plays what it played before, from the start -- one that's gone (removed, a new scene) has nothing to play
    void ResumeEditClips() {
        auto const found = m_entities.find(m_edit_entity);

        if (m_edit_entity != EDIT_NO_ENTITY && found != m_entities.end())
            for (Ir77EditClip const& held : m_edit_clips) found->second.model->PlayAnimation(held.clip, held.loop, held.speed);

        m_edit_clips.clear();
    }

    // The view of window 0 under x, y (fractions from its top left -- the last view whose area holds it, it draws on top), as the last frame
    // drew it: its camera, the cursor and the view's size in pixels, the ray under the cursor. false: outside every view, or no frame yet.
    bool ViewAt(float const x, float const y, Ir77EditView& out) const {
        std::size_t view{0};
        return ViewIndexAt(x, y, view) && ViewRay(view, x, y, out);
    }

    // which view of window 0 holds x, y -- the last one, it draws on top
    bool ViewIndexAt(float const x, float const y, std::size_t& view) const {
        if (m_frame.windows.empty()) return false;

        Ir77PVFrameWindow const& window = m_frame.windows[0];

        view = window.cameras.size();
        for (std::size_t v = 0; v < window.cameras.size() && v < window.areas.size(); v++) {
            glm::vec4 const& area = window.areas[v];
            if (x >= area.x && x < area.x + area.z && y >= area.y && y < area.y + area.w) view = v;
        }

        return view < window.cameras.size();
    }

    // one view's ray under x, y -- the point may lie outside the view (a grab dragged past its edge keeps its view)
    bool ViewRay(std::size_t const view, float const x, float const y, Ir77EditView& out) const {
        if (m_frame.windows.empty()) return false;

        Ir77PVFrameWindow const& window = m_frame.windows[0];
        if (view >= window.cameras.size() || view >= window.areas.size()) return false;

        glm::vec4 const& area = window.areas[view];
        glm::vec2 const local{(x - area.x) / area.z, (y - area.y) / area.w};  // 0..1 across the view, from its top left
        glm::vec2 const ndc = local * 2.0f - 1.0f;                            // Vulkan clip space is Y-down too

        // reversed-Z: the near plane is 1; 0.5 is still in front of the camera when the far plane is at infinity
        glm::mat4 const& view_proj = window.cameras[view].view_proj;
        glm::mat4 const inverse = glm::inverse(view_proj);
        glm::vec4 const near = inverse * glm::vec4(ndc, 1.0f, 1.0f);
        glm::vec4 const deep = inverse * glm::vec4(ndc, 0.5f, 1.0f);

        if (std::abs(near.w) < 1e-12f || std::abs(deep.w) < 1e-12f) return false;

        out.view = window.cameras[view].view;
        out.proj = window.cameras[view].proj;
        out.view_proj = view_proj;
        out.size = glm::vec2(area.z * static_cast<float>(std::max(m_view_width, 1)), area.w * static_cast<float>(std::max(m_view_height, 1)));
        out.cursor = local * out.size;
        out.origin = glm::vec3(near) / near.w;
        out.ray = glm::normalize(glm::vec3(deep) / deep.w - out.origin);

        return true;
    }

    // object mode's gizmo placement -- at the entity's origin; world axes for a move or a turn, its own for a scale
    glm::mat4 ObjectPlacement() const {
        Ir77GizmoMode mode{Ir77GizmoMode::Translate};
        std::uint32_t hot{GIZMO_NONE};
        m_gizmo->GetMode(mode, hot);

        glm::mat4 placement = mode == Ir77GizmoMode::Scale ? m_object_world : glm::mat4(1.0f);
        placement[3] = glm::vec4(glm::vec3(m_object_world[3]), 1.0f);
        return placement;
    }

    // in edit mode on a model that is in the frame
    bool EditShown() const {
        if (m_edit_entity == EDIT_NO_ENTITY) return false;

        auto const found = m_entities.find(m_edit_entity);
        return found != m_entities.end() && found->second.visible;
    }

    // One entity's triangles against a ray, world space, where its instances sat this frame -- skinned ones posed by the palette it drew with.
    // The nearest hit's distance along the ray. Both faces count (Moller-Trumbore, no culling): a click on a back face still picks the model.
    bool PickTriangles(Ir77WorldModel const& placed, glm::vec3 const& origin, glm::vec3 const& ray, float& distance) const {
        std::shared_ptr<IIr77Mesh> meshes{};
        placed.model->GetMeshes(meshes);
        if (!meshes) return false;

        std::vector<Ir77PVVertex> vertices{};
        std::vector<std::uint32_t> indices{};
        std::vector<Ir77MeshRange> ranges{};

        meshes->GetVertexBuffer(vertices);
        meshes->GetIndexBuffer(indices);
        meshes->GetMeshRanges(ranges);

        std::vector<Ir77PVVertexSkinned> vertices_skinned{};
        std::vector<std::uint32_t> indices_skinned{};
        std::vector<Ir77MeshRange> ranges_skinned{};

        meshes->GetSkinnedVertexBuffer(vertices_skinned);
        meshes->GetSkinnedIndexBuffer(indices_skinned);
        meshes->GetSkinnedMeshRanges(ranges_skinned);

        std::vector<std::uint32_t> bone_bases{};
        placed.model->WriteInstanceBoneBases(bone_bases, placed.transforms.size());

        bool hit{false};
        distance = std::numeric_limits<float>::max();

        PickStream(placed, indices, vertices.size(), ranges, origin, ray, distance, hit,
                   [&](std::size_t const v, std::uint32_t) { return vertices[v].position; });

        PickStream(placed, indices_skinned, vertices_skinned.size(), ranges_skinned, origin, ray, distance, hit, [&](std::size_t const v, std::uint32_t const local) {
            return Ir77SkinPosition(vertices_skinned[v], placed.bones, local < bone_bases.size() ? bone_bases[local] : 0);
        });

        return hit;
    }

    // One stream's ranges against the ray -- position(vertex, local instance) is a corner in its instance's space
    template <typename Position>
    static void PickStream(Ir77WorldModel const& placed, std::vector<std::uint32_t> const& indices, std::size_t const vertex_count,
                           std::vector<Ir77MeshRange> const& ranges, glm::vec3 const& origin, glm::vec3 const& ray, float& distance, bool& hit,
                           Position const& position) {
        for (Ir77MeshRange const& range : ranges) {
            if (range.vertex_offset < 0) continue;

            std::size_t const base = static_cast<std::size_t>(range.vertex_offset);

            for (std::uint32_t instance = 0; instance < range.instance_count; instance++) {
                std::uint32_t const local = range.first_instance + instance;
                if (local >= placed.transforms.size()) continue;

                glm::mat4 const& model = placed.transforms[local];

                for (std::uint32_t i = 0; i + 2 < range.index_count; i += 3) {
                    std::size_t const at = range.index_offset + i;
                    if (at + 2 >= indices.size()) break;

                    std::size_t const a = base + indices[at];
                    std::size_t const b = base + indices[at + 1];
                    std::size_t const c = base + indices[at + 2];
                    if (a >= vertex_count || b >= vertex_count || c >= vertex_count) continue;

                    glm::vec3 const p0 = glm::vec3(model * glm::vec4(position(a, local), 1.0f));
                    glm::vec3 const p1 = glm::vec3(model * glm::vec4(position(b, local), 1.0f));
                    glm::vec3 const p2 = glm::vec3(model * glm::vec4(position(c, local), 1.0f));

                    glm::vec3 const e1 = p1 - p0;
                    glm::vec3 const e2 = p2 - p0;
                    glm::vec3 const h = glm::cross(ray, e2);
                    float const det = glm::dot(e1, h);
                    if (std::abs(det) < 1e-12f) continue;  // the ray runs along the triangle

                    float const inv = 1.0f / det;
                    glm::vec3 const s = origin - p0;
                    float const u = glm::dot(s, h) * inv;
                    if (u < 0.0f || u > 1.0f) continue;

                    glm::vec3 const q = glm::cross(s, e1);
                    float const v = glm::dot(ray, q) * inv;
                    if (v < 0.0f || u + v > 1.0f) continue;

                    float const t = glm::dot(e2, q) * inv;
                    if (t > 0.0f && t < distance) {
                        distance = t;
                        hit = true;
                    }
                }
            }
        }
    }

    // The grid's draw for one window -- after the opaque geometry it has to sit behind, before anything transparent. The pipeline is looked
    // up once; a manifest without "grid" leaves the grid out, and the stats warnings say so from then on.
    void AppendGrid(std::size_t const& window, std::vector<Ir77PVDrawItem>& opaque) {
        if (!m_grid_visible || m_grid_missing || !m_renderer) return;

        if (m_grid_pipeline == IR77PV_NO_PIPELINE && m_renderer->FindPipeline(std::string{"grid"}, m_grid_pipeline)->ID() != GUIDIr77OperationSucceeded) {
            m_grid_pipeline = IR77PV_NO_PIPELINE;
            m_grid_missing = true;  // GetSceneStats says so
            return;
        }

        Ir77PVDrawItem item{};
        if (m_renderer->MakeProceduralDrawItem(window, m_grid_pipeline, 3, 1, item)->ID() != GUIDIr77OperationSucceeded) return;

        opaque.push_back(item);
    }

    // One parsed file -> the renderer as one asset bundle (geometry per channel with each primitive's material, the decoded textures, the
    // materials, the instance slots and joint palettes) -> a new entity at the origin, its clips playing. entity is its id.
    std::shared_ptr<IIr77Return const> LoadEntity(std::shared_ptr<IIr77Model> const& model, std::string const& path, bool const use_mesh,
                                                  Ir77PVAssetBundle& bundle, Ir77PVAssetReport& report, std::uint32_t& entity) {
        std::shared_ptr<IIr77Return const> result{};

        // the file's components -- Model owns them
        std::shared_ptr<IIr77Mesh> meshes{};
        std::shared_ptr<IIr77Material> materials{};
        std::shared_ptr<IIr77Texture> textures{};

        model->GetMeshes(meshes);
        model->GetMaterials(materials);
        model->GetTextures(textures);

        Ir77WorldModel placed{};
        placed.model = model;
        placed.path = path;

        bundle = Ir77PVAssetBundle{};
        bundle.name = path.empty() ? std::string{"(empty scene)"} : path;
        bundle.use_mesh = use_mesh;
        bundle.pipeline = IR77PV_NO_PIPELINE;  // the manifest's "static" role

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

        // instances and joint palettes -- the rest pose now, Model's every frame after
        meshes->GetInstanceBuffer(placed.transforms);
        meshes->GetBounds(placed.bounds_min, placed.bounds_max);

        bundle.instance_count = static_cast<std::uint32_t>(placed.transforms.size());
        model->WriteInstanceBoneBases(bundle.bone_bases, placed.transforms.size());

        model->WriteBoneMatrices(placed.bones);
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

        placed.instance_base = report.instance_base;

        // its counts -- the bundle and the report are gone after this; the device's are the scene's (every load goes to the same GPU)
        Ir77SceneStats& stats = placed.stats;
        stats.path = path;
        model->GetAssetCounts(stats.nodes, stats.meshes, stats.materials, stats.textures, stats.cameras, stats.skins, stats.bones, stats.clips);

        std::shared_ptr<IIr77Animation> animations{};
        model->GetAnimations(animations);
        stats.clip_names.assign(stats.clips, std::string{});
        if (animations)
            for (std::uint32_t c = 0; c < stats.clips; c++) animations->GetClipName(c, stats.clip_names[c]);

        stats.static_vertices = bundle.vertices.size();
        stats.static_indices = bundle.indices.size();
        stats.skinned_vertices = bundle.vertices_skinned.size();
        stats.skinned_indices = bundle.indices_skinned.size();
        stats.static_primitives = static_cast<std::uint32_t>(bundle.primitives.size());
        stats.skinned_primitives = static_cast<std::uint32_t>(bundle.primitives_skinned.size());
        stats.instances = bundle.instance_count;
        stats.opaque_runs = report.opaque_runs;
        stats.skinned_runs = report.skinned_runs;
        stats.transparent_objects = report.transparent_objects;

        m_gpu = report.device_name;
        m_render_path = std::string{report.mesh_path ? "mesh shading indirect" : "vertex indirect"} + (report.ray_query ? ", ray queries" : "");

        // every clip in the file plays by default, looping -- the app takes over with animation.play / animation.stop
        model->PlayAllAnimations(true);

        entity = m_next_entity++;
        m_entities.emplace(entity, std::move(placed));

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // An entity's file lights -> world lights, each placed where the file had it (the entity is at the origin when it's imported). Its
    // direction becomes the rotation that turns -Z onto it.
    void ImportLights(std::uint32_t const entity) {
        std::shared_ptr<IIr77Light> file_lights{};
        m_entities.at(entity).model->GetLights(file_lights);

        std::vector<Ir77PVLight> lights{};
        file_lights->GetLightsBuffer(lights);

        for (Ir77PVLight const& light : lights) {
            Ir77WorldLight placed{};
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
            if (!placed.visible) continue;

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

    glm::mat4 EntityMatrix(Ir77WorldModel const& entity) const {
        return glm::translate(glm::mat4(1.0f), entity.translation) * glm::mat4_cast(entity.rotation) * glm::scale(glm::mat4(1.0f), entity.scale);
    }

    // The scene stats into the return log -- until the app displays them
    void PrintSceneStats() const {
        Ir77SceneStats s{};
        GetSceneStats(s);

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
    std::shared_ptr<IIr77Edit> m_edit{};
    std::shared_ptr<IIr77Gizmo> m_gizmo{};  // the transform gizmo -- edit mode's at the selection, object mode's at the active entity

    // object mode
    bool m_object_gizmo_shown{false};                // object mode with something active -- SetObjectGizmo
    glm::mat4 m_object_world{1.0f};                  // the active entity's world matrix -- where the object gizmo sits
    std::uint32_t m_outline_entity{EDIT_NO_ENTITY};  // the model drawn with the selection's outline (the active one, in object mode)

    // edit mode
    std::uint32_t m_edit_entity{EDIT_NO_ENTITY};  // the model in edit mode -- its cage is m_edit's
    std::vector<Ir77EditClip> m_edit_clips{};     // what it was playing -- played again when it leaves edit mode
    std::size_t m_grab_view{0};                   // the view a grab started in -- its rays until it ends

    // direct link -- Scene -> PeregrineV
    std::shared_ptr<NSIr77PeregrineV::Ir77Server> m_renderer{};  // PeregrineV's server -- the module's one way into the renderer

    // direct link -- Scene -> Fields
    std::shared_ptr<NSIr77Fields::Ir77Server> m_fields{};  // the fields module's server -- empty: no field in the frame

    // the field's box as Lighting last got it -- compared each frame
    bool m_field_active{false};
    glm::vec3 m_field_low{0.0f};
    glm::vec3 m_field_high{0.0f};

    // the windows -- SetViewSize
    std::size_t m_view_windows{1};
    int m_view_width{1280};
    int m_view_height{720};

    glm::vec3 m_background{0.0f};  // sRGB 0..1 -- what every window clears to

    // the editor grid
    bool m_grid_visible{true};
    bool m_grid_missing{false};  // the manifest has no "grid" -- stop looking
    Ir77PVPipelineHandle m_grid_pipeline{IR77PV_NO_PIPELINE};

    // what CreateScene loaded
    bool m_scene_ready{false};
    std::map<std::uint32_t, Ir77WorldModel> m_entities{};  // [id] -- 0 is the scene file
    std::uint32_t m_next_entity{0};                        // ids are never reused within a scene; CreateScene starts over at 0
    std::map<std::uint32_t, Ir77WorldLight> m_lights{};    // [id] -- the world lights
    std::uint32_t m_next_light{0};                         // like entity ids -- never reused within a scene
    std::vector<std::string> m_scene_warnings{};           // this scene's -- CreateScene starts them over
    std::string m_gpu{};                                   // the last load's report -- GetSceneStats
    std::string m_render_path{};

    // per frame -- what CreateFrame hands the renderer
    Ir77PVFrame m_frame{};
    float m_last_seconds{0.0f};       // camera.time.y
    std::uint32_t m_frame_number{0};  // camera.time.z
    std::uint32_t m_opaque_items{0};  // the renderer's last report -- WriteFrameStats
    std::uint32_t m_transparent_items{0};

    static constexpr float EMPTY_SCENE_HALF_SIZE{4.0f};  // the box a scene with no geometry frames
};
}  // namespace NSIr77Scene
