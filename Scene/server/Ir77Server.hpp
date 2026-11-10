#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>

#include <simdjson.h>

#include "../../Ir77RT/runtime/Ir77Json.hpp"
#include "../../Ir77RT/runtime/Ir77Return.hpp"

#include "../dictionary/IDIIr77Scene.hpp"

#include "../interface/IIr77Graph.hpp"
#include "../interface/IIr77Model.hpp"
#include "../interface/IIr77View.hpp"
#include "../interface/IIr77World.hpp"

#include "../runtime/Ir77Graph.hpp"
#include "../runtime/Ir77World.hpp"

#include "../services/Ir77Flatten.hpp"

#include "../types/Ir77Types.hpp"

#include "../../PeregrineV/server/Ir77Server.hpp"
#include "../../Fields/server/Ir77Server.hpp"

using namespace NSIr77RT;

namespace NSIr77Scene {

// =============================================================================================================================================
// Ir77Server -- the scene module's switchboard. Owns Ir77World (the work) and Ir77Graph (the scene graph, which drives World) --
// created here, held through their interfaces, handed to no one. World owns the entities (a Model each), View and Lighting; this server
// holds View through its interface and reaches an entity's Model through World (GetModel) per op. The module's one door:
//
//   pipe:        Sink(json) in, the post function out (SetPost). Every command arrives here as JSON and every reply leaves the same way.
//   direct link: only what can't travel as JSON -- the renderer's and the fields module's servers (SetRenderer / SetField) and the
//                per-frame calls (Update / CreateFrame / stats). Everything else is an op, setup included.
//
// Message in:  {"id":7, "from":"ui", "to":"scene", "op":"camera.move", "args":{"right":1, "up":0, "forward":0}}
// Reply out:   {"id":7, "from":"scene", "to":"ui", "op":"camera.move", "ok":true, "error":""}   (only when the message names a "from")
//
// ops:
//   scene.create    path, use_mesh (false), lights, cameras -- loads a glTF as entity 0 ("" = empty scene; a file that won't load leaves an
//                                                    empty scene); lights / cameras: import the file's (default SCENE_IMPORT_LIGHTS / _CAMERAS)
//   scene.view_size windows, width, height        -- the windows CreateFrame fills and their size (the cameras' aspect)
//   scene.project   path                          -- the project folder -- Models/<name>.glb lives here
//   scene.clear                                   -- empties the scene for another app; the stored scene it was (scene.document) becomes
//                                                    the startup scene again, so the next Aurum page opens it
//   scene.startup   name                          -- the stored scene the UI opens at start (iridium.json startup.scene); the dump carries it
//                                                    until the UI's first scene.create
//   scene.document  id, name                      -- which stored scene this is (the UI's, after a load or a save) -- kept here so a page
//                                                    reload knows; the dump carries it
//   scene.settings  settings {...}                -- the scene's settings: every key given replaces that key whole; the rest stay. Any keys
//                                                    at all (the UI owns them; they're stored with the scene) -- the ones the renderer acts on
//                                                    are applied as they arrive: background [r, g, b] (sRGB 0..1), grid (bool). Cleared by
//                                                    scene.create. -- data { settings } (no settings given -> just reads them)
//   scene.dump                                    -- data { project, startup, document {id, name}, settings, active, interaction, edit {mode,
//                                                    selected, gizmo}, entities: [{id, name, kind, parent, order, visible, t, r, s, + model
//                                                    {model, clips: [{clip, name, duration}], playing: [{clip, name, time, speed, loop}], nodes:
//                                                    [as node.list]} | light {light: {...}} | camera {camera: {...}}}] } -- the whole scene
//                                                    graph, parents first, in one reply
//   camera.move     right / up / forward in -1..1 -- held keys, applied every frame until set back to 0
//   camera.look     dx / dy in pixels             -- turns the fly camera / swings the orbit camera
//   camera.zoom     delta in wheel steps          -- orbit distance, 10% per step
//   camera.speed    value                         -- multiplies move speed
//   camera.mode     value "orbit" | "fly" | "ortho" | "file"
//   camera.view     value "top" | "front" | "side"  -- turns the orbit camera; ortho top looks straight down
//   camera.select   index                         -- == camera count -> the controlled camera
//   camera.next                                   -- next file camera, then the controlled camera
//   camera.reset                                  -- orbit framed on the scene
//   camera.layout   views [{x, y, w, h}] 1 .. 4   -- where each view draws, fractions of the window; view 0 the main camera, 1 / 2 / 3 start
//                                                    ortho top / front / side
//   camera.focus    view                          -- which view the camera.* ops above drive (the one under the mouse)
//   view.reset                                    -- the views back to how they start: one view filling the window, focused, every
//                                                    camera reset -- the UI sends it on every app switch, so no app inherits another's
//   entity.add      kind "collection" | "model" | "light" | "camera", parent ('' = the scene root), name, t, r, s, visible (true), order
//                   (0 = last), id (16 hex -- a load keeps its ids; none -> made), and the kind's payload: model {model (the project's
//                   Models/<model>.glb) | path (a .glb from anywhere -- flattened into Models/ first, under a unique name)}, light {light:
//                   {type, color, intensity, range, inner, outer}}, camera {camera: {fov, ortho, near, far}} -- data { id, model, report } (report =
//                   what an import's flatten left out, one line each)
//   entity.remove   id                            -- it and everything under it
//   entity.move     id, parent, order             -- re-parent / re-order; never under itself
//   entity.set      id, then any of name, t, r, s, visible, order, light {...}, camera {...} -- what's left out keeps its value
//   entity.get      id                            -- data { entity } (as in the dump)
//   entity.list                                   -- data { entities: [...] } (as in the dump)
//   entity.select   id ('' = none)                -- the active entity (the dump's "active"). Selecting goes back to object mode first --
//                                                    data { id, interaction, edit, points, edges, mode, selected, gizmo }
//   entity.mode     mode "object" | "edit" | "sculpt" | "vertex_paint" | "weight_paint" | "texture_paint" -- the active entity's
//                                                    interaction mode. Edit needs an active model: it stops playing and rests in its rest pose,
//                                                    its vertices and edges drawn over it (animation.* on it fail until it leaves; leaving
//                                                    plays its clips again). Sculpt and the paints aren't built yet -- refused. data as
//                                                    entity.select
//   entity.pick     x, y                          -- the model under a point of the window (fractions, from the top left) through the view
//                                                    there -- data { id } ('' = nothing). Doesn't select; entity.select does
//   edit.mode       mode "vertex" | "edge" | "face" -- what a click in edit mode selects -- data { mode, selected }
//   edit.pick       x, y, extend (false)          -- a click in edit mode: the nearest visible element under it; extend toggles it in the
//                                                    selection, otherwise it replaces it (a miss clears it) -- data { hit, mode, selected }
//   edit.select_all value (true)                  -- every vertex, or none -- data { mode, selected }
//   edit.box        x0, y0, x1, y1, extend (false) -- a box drag (fractions of the window, through the view it started in): every visible
//                                                    element inside it, by the mode; extend adds them -- data { taken, mode, selected }
//   edit.undo / edit.redo                         -- the last confirmed edit back / again -- data { done, modified, undo, redo }
//   edit.commit                                   -- every edited model's .glb rewritten with its edits (Save runs it first) -- data {
//                                                    written: [model], errors: [line] }
//   gizmo.mode      mode "translate" | "rotate" | "scale" -- what the gizmo does, in edit and object mode (a drag under way goes back first)
//                                                    -- data { gizmo }
//   gizmo.hover     x, y                          -- the handle under the cursor lights up -- data { handle } (-1 = none)
//   gizmo.press     x, y                          -- on a handle: a drag starts -- edit mode: of the selection; object mode: of the active
//                                                    entity's whole transform (world axes; a scale along its own) -- data { handle } (-1 = no
//                                                    handle there: the press is the viewport's)
//   gizmo.move      x, y                          -- the drag follows the cursor -- data { changed }
//   gizmo.end       confirm (true)                -- the drag's change stays (edit mode: onto the undo history), or everything goes back
//                                                    -- data { modified, undo, redo } (edit) / { modified, id } (object)
//   node.list       id                            -- a model's own nodes: data { nodes: [{node, name, parent (-1 = root), t, r, s}] }
//   node.transform  id, node, t, r, s             -- one node's rest TRS; a clip that drives it still wins on what it animates
//   animation.play  id, name, loop (true), speed (1)
//   animation.stop  id, name
//   animation.stop_all id
//
// Transforms are the parent's space; the graph composes them down the tree, visibility too. Light intensity in glTF units (lux / candela;
// 683 = 1 engine unit), cones in radians; a light shines down its local -Z.
//
// Entity 0 is the scene file (scene.create starts the ids over).
//
// A handler that returns something leaves it in m_data -- the reply's "data" (an Ir77Json object; {} otherwise). Every reply is built as an
// Ir77Json (Ir77RT/runtime/Ir77Json.hpp), never glued together as text.
// Ops live in a table (op -> handler), filled once by RegisterPipeOperations -- one lookup per message however many there are. A new op is one
// line in the group it belongs to; a new group is one more Define*Operations.
// =============================================================================================================================================
class Ir77Server : public Ir77Enlisted, public std::enable_shared_from_this<Ir77Server> {
   public:
    Ir77Server() {
        m_world = std::static_pointer_cast<IIr77World>(std::make_shared<Ir77World>());

        // World's peers exist from its construction -- the ops go straight to them; an entity's Model comes per op (GetModel)
        m_world->GetView(m_view);

        m_graph = std::static_pointer_cast<IIr77Graph>(std::make_shared<Ir77Graph>());
        m_graph->SetWorld(m_world);

        if (m_empty_parser.parse(std::string_view{"{}"}).get(m_empty_args) != simdjson::SUCCESS)  // what a handler sees when a message has no "args"
            throw std::runtime_error{"Ir77Server: empty args"};

        RegisterPipeOperations();
    }

    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Scene>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Scene>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77Scene)
            obj = std::shared_ptr<Ir77Server>(shared_from_this(), static_cast<Ir77Server*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // pipe
    // -------------------------------------------------------------------------------------------------------------------------------------
   public:
    // Where this module's replies and events go -- the router's queue
    void SetPost(std::function<void(std::string)> post) { m_post = std::move(post); }

    // One message from the router. Runs on the main thread, between frames.
    void Sink(std::string json) {
        simdjson::dom::element root{};
        if (m_parser.parse(json).get(root) != simdjson::SUCCESS) {
            Ir77RETURN<Ir77OperationFailed>(this, "[WARN] scene: not JSON: " + json + "\n");
            return;
        }

        // a reply to something this module sent -- never answered, or two modules would answer each other forever
        if (root["ok"].error() == simdjson::SUCCESS) {
            bool reply_ok{true};
            if (root["ok"].get(reply_ok) == simdjson::SUCCESS && !reply_ok) Ir77RETURN<Ir77OperationFailed>(this, "Ir77Server: reply failed: " + json);
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

        auto const handler = m_operations.find(op);
        bool const known = handler != m_operations.end();

        m_error.clear();
        m_data = Ir77Json::Object();

        bool const ok = known && handler->second(args)->ID() == GUIDIr77OperationSucceeded;

        // a handler that has more to say (compile errors) leaves it in m_error
        std::string const error = ok ? "" : !known ? "unknown op" : m_error.empty() ? "operation failed" : m_error;

        if (!ok) Ir77RETURN<Ir77OperationFailed>(this, "[WARN] scene: " + error + " : " + json);

        if (from.empty() || !m_post) return;  // nobody to answer

        Ir77Json reply = Ir77Json::Object();
        reply["id"] = id;
        reply["from"] = "scene";
        reply["to"] = from;
        reply["op"] = op;
        reply["ok"] = ok;
        reply["error"] = error;
        reply["data"] = ok ? m_data : Ir77Json::Object();

        m_post(reply.Write());
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // direct link -- setup (Scene -> PeregrineV) and the frame loop
    // -------------------------------------------------------------------------------------------------------------------------------------
    // PeregrineV's server -- the scene's only way into the renderer. A pointer, so direct; before scene.create.
    std::shared_ptr<IIr77Return const> SetRenderer(std::shared_ptr<NSIr77PeregrineV::Ir77Server> const& renderer) { return m_world->SetRenderer(renderer); }

    // The fields module's server -- the frame draws its field. Optional; any time, the next frame uses it.
    std::shared_ptr<IIr77Return const> SetField(std::shared_ptr<NSIr77Fields::Ir77Server> const& fields) { return m_world->SetField(fields); }

    std::shared_ptr<IIr77Return const> Update(float const& dt) { return m_world->Update(dt); }

    std::shared_ptr<IIr77Return const> CreateFrame(float const& seconds) { return m_world->CreateFrame(seconds); }

    std::shared_ptr<IIr77Return const> WriteFrameStats(Ir77FrameStats& stats) const { return m_world->WriteFrameStats(stats); }

    std::shared_ptr<IIr77Return const> GetSceneStats(Ir77SceneStats& stats) const { return m_world->GetSceneStats(stats); }

   private:
    // Ir77PVLight::type -- as Ir77Light writes them from the glTF
    static constexpr int LIGHT_DIRECTIONAL{0};
    static constexpr int LIGHT_POINT{1};
    static constexpr int LIGHT_SPOT{2};

    using Ir77SceneArgs = simdjson::dom::object;

    using Ir77SceneOperation = std::function<std::shared_ptr<IIr77Return const>(Ir77SceneArgs const&)>;

    // heterogeneous lookup -- find by the string_view out of the parser, no std::string per message
    struct Ir77SceneOpHash {
        using is_transparent = void;
        std::size_t operator()(std::string_view const& op) const { return std::hash<std::string_view>{}(op); }
    };

    void RegisterPipeOperations() {
        DefineSceneOperations();
        DefineCameraOperations();
        DefineEntityOperations();
        DefineEditOperations();
        DefineAnimationOperations();
    }

    // setup -- iridium sends these at startup; the UI can send scene.create to open another file
    void DefineSceneOperations() {
        m_operations["scene.project"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            m_project = std::string{Text(a, "path")};

            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        // another app is taking the view -- empty, but remember which stored scene to bring back
        m_operations["scene.clear"] = [this](Ir77SceneArgs const&) -> std::shared_ptr<IIr77Return const> {
            ForgetScene();

            if (!m_document_name.empty()) m_startup_scene = m_document_name;

            return m_world->CreateScene("", false, false, false);
        };

        m_operations["scene.create"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::string const model{Text(a, "model")};
            std::string const path = model.empty() ? std::string{Text(a, "path")} : ModelPath(model);

            ForgetScene();
            m_startup_scene.clear();  // the UI has opened something -- a page reload mustn't open the startup scene over it

            std::shared_ptr<IIr77Return const> result{};
            if (Failed(m_world->CreateScene(path, Flag(a, "use_mesh", false), Flag(a, "lights", SCENE_IMPORT_LIGHTS), Flag(a, "cameras", SCENE_IMPORT_CAMERAS)),
                       result))
                return result;

            return result;
        };

        m_operations["scene.view_size"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            float const windows = Number(a, "windows", 1.0f);
            float const width = Number(a, "width", 0.0f);
            float const height = Number(a, "height", 0.0f);

            if (windows < 1.0f || width < 1.0f || height < 1.0f)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: scene.view_size needs windows, width and height of at least 1.");

            return m_world->SetViewSize(static_cast<std::size_t>(windows), static_cast<int>(width), static_cast<int>(height));
        };

        // the scene's settings -- see the op list above. Keys merge in whole; the ones the renderer acts on are applied as they arrive.
        m_operations["scene.settings"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::shared_ptr<IIr77Return const> result{};

            simdjson::dom::object settings{};
            if (a["settings"].get(settings) == simdjson::SUCCESS) {
                for (auto const field : settings) {
                    std::string_view const key = field.key;
                    m_settings[key] = FromElement(field.value);

                    if (Failed(ApplySetting(key, field.value), result)) return result;
                }
            }

            m_data["settings"] = m_settings;

            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        m_operations["scene.startup"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            m_startup_scene = std::string{Text(a, "name")};

            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        m_operations["scene.document"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            m_document_id = std::string{Text(a, "id")};
            m_document_name = std::string{Text(a, "name")};

            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        // the whole scene graph in one reply -- see the op list above
        m_operations["scene.dump"] = [this](Ir77SceneArgs const&) -> std::shared_ptr<IIr77Return const> {
            m_data = Dump();

            return Ir77RETURN<Ir77OperationSucceeded>();
        };
    }

    // What a scene leaves behind, gone -- before scene.create opens another or scene.clear empties it: the ids start over, World's entities
    // and lights go with the graph, edit mode with them (World leaves it in CreateScene), no active entity, no drag, no gizmo, and the
    // settings (a new scene's come with it -- scene.settings after the load)
    void ForgetScene() {
        m_graph->Clear();
        m_settings = Ir77Json::Object();
        m_active.clear();
        m_interaction = Ir77InteractionMode::Object;
        m_object_drag = Ir77ObjectDrag{};
        m_world->SetObjectGizmo(false, glm::mat4(1.0f));
    }

    void DefineCameraOperations() {
        m_operations["camera.move"] = [this](Ir77SceneArgs const& a) {
            return m_view->SetCameraMove(glm::vec3(Number(a, "right", 0.0f), Number(a, "up", 0.0f), Number(a, "forward", 0.0f)));
        };
        m_operations["camera.look"] = [this](Ir77SceneArgs const& a) { return m_view->SetCameraLook(Number(a, "dx", 0.0f), Number(a, "dy", 0.0f)); };
        m_operations["camera.zoom"] = [this](Ir77SceneArgs const& a) { return m_view->SetCameraZoom(Number(a, "delta", 0.0f)); };
        m_operations["camera.speed"] = [this](Ir77SceneArgs const& a) { return m_view->SetCameraSpeed(Number(a, "value", 0.0f)); };
        m_operations["camera.mode"] = [this](Ir77SceneArgs const& a) { return m_view->SetCameraMode(Text(a, "value")); };
        m_operations["camera.view"] = [this](Ir77SceneArgs const& a) { return m_view->SetCameraView(Text(a, "value")); };
        m_operations["camera.select"] = [this](Ir77SceneArgs const& a) { return m_view->SelectCamera(static_cast<std::uint32_t>(Number(a, "index", 0.0f))); };
        m_operations["camera.next"] = [this](Ir77SceneArgs const&) { return m_view->NextCamera(); };
        m_operations["camera.reset"] = [this](Ir77SceneArgs const&) { return m_view->ResetCamera(); };
        m_operations["camera.layout"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            simdjson::dom::array views{};
            if (a["views"].get(views) != simdjson::SUCCESS) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: camera.layout needs views.");

            std::vector<glm::vec4> areas{};

            for (auto const element : views) {
                simdjson::dom::object view{};
                if (element.get(view) != simdjson::SUCCESS) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: camera.layout views are {x, y, w, h}.");

                areas.push_back(glm::vec4(Number(view, "x", 0.0f), Number(view, "y", 0.0f), Number(view, "w", 0.0f), Number(view, "h", 0.0f)));
            }

            return m_view->SetViewLayout(areas);
        };
        m_operations["camera.focus"] = [this](Ir77SceneArgs const& a) { return m_view->FocusView(static_cast<std::uint32_t>(Number(a, "view", 0.0f))); };
        m_operations["view.reset"] = [this](Ir77SceneArgs const&) { return m_view->ResetViews(); };
    }

    // The scene graph (types/Ir77Types.hpp) -- every entity by its id: collections, models, lights, cameras; parented, ordered, shown or hidden
    void DefineEntityOperations() {
        // a new entity -- the core (kind, parent, name, t, r, s, visible, order, id) and its kind's payload: model {model | path}, light {light:
        // {type, color, ...}}, camera {camera: {fov, ortho, near, far}}. id given (a load) -> kept; none -> made. -> { id, model }
        m_operations["entity.add"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            Ir77GraphEntity entity{};
            if (!EntityCore(a, entity, true)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: " + m_error);

            std::string path{};

            if (entity.kind == Ir77GraphKind::Model) {
                std::string model{Text(a, "model")};
                std::string const source{Text(a, "path")};

                // a file from anywhere -> into the project's Models/ first, so it's a project model the graph can save
                if (model.empty() && !source.empty()) {
                    if (m_project.empty()) return Fail("open a project first -- a model goes into its Models/");
                    if (!AdoptModel(source, model)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: " + m_error);
                }

                if (model.empty()) return Fail("a model entity needs model or path");

                entity.model = model;
                if (entity.name.empty()) entity.name = model;
                path = ModelPath(model);
            }

            if (entity.kind == Ir77GraphKind::Light) {
                entity.light.type = LIGHT_POINT;
                entity.light.color = glm::vec3(1.0f);
                entity.light.intensity = 683.0f;
                entity.light.outerConeAngle = glm::quarter_pi<float>();

                std::shared_ptr<IIr77Return const> result{};
                simdjson::dom::object light{};
                if (a["light"].get(light) == simdjson::SUCCESS && Failed(LightFields(light, entity.light), result)) return result;
            }

            if (entity.kind == Ir77GraphKind::Camera) {
                simdjson::dom::object camera{};
                if (a["camera"].get(camera) == simdjson::SUCCESS) CameraFields(camera, entity.camera);
            }

            std::string id{};
            std::string error{};
            if (m_graph->Add(entity, path, Flag(a, "use_mesh", false), id, error)->ID() != GUIDIr77OperationSucceeded) return Fail(error);

            m_data["id"] = id;
            m_data["model"] = entity.model;

            // an import says what its flatten left out
            Ir77Json& report = m_data["report"] = Ir77Json::Array();
            for (std::string const& line : m_import_report) report.Push(line);
            m_import_report.clear();

            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        // it and everything under it
        m_operations["entity.remove"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::string error{};
            if (m_graph->Remove(std::string{Text(a, "id")}, error)->ID() != GUIDIr77OperationSucceeded) return Fail(error);

            // the active entity went with it (it, or something under it) -- nothing active; World already left edit mode on a model
            Ir77GraphEntity active{};
            if (!m_active.empty() && m_graph->Get(m_active, active)->ID() != GUIDIr77OperationSucceeded) {
                Select("");
                m_data = Ir77Json::Object();  // entity.remove answers nothing
            }

            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        // parent ('' = the scene root), order (0 = after its new siblings)
        m_operations["entity.move"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::string error{};
            if (m_graph->Move(std::string{Text(a, "id")}, std::string{Text(a, "parent")}, static_cast<std::uint32_t>(std::max(Number(a, "order", 0.0f), 0.0f)),
                              error)
                    ->ID() != GUIDIr77OperationSucceeded)
                return Fail(error);

            ObjectGizmo();  // a new parent moves it
            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        // any of: name, t, r, s, visible, order, light {...}, camera {...} -- what's left out keeps its value
        m_operations["entity.set"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            Ir77GraphEntity entity{};
            if (m_graph->Get(std::string{Text(a, "id")}, entity)->ID() != GUIDIr77OperationSucceeded) return Fail("no entity " + std::string{Text(a, "id")});

            if (!EntityCore(a, entity, false)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: " + m_error);

            std::shared_ptr<IIr77Return const> result{};
            simdjson::dom::object light{};
            if (a["light"].get(light) == simdjson::SUCCESS && Failed(LightFields(light, entity.light), result)) return result;

            simdjson::dom::object camera{};
            if (a["camera"].get(camera) == simdjson::SUCCESS) CameraFields(camera, entity.camera);

            std::string error{};
            if (m_graph->Set(entity, error)->ID() != GUIDIr77OperationSucceeded) return Fail(error);

            ObjectGizmo();  // the active entity, or a parent of it, may have moved
            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        // -> { entity } -- one, as in the dump
        m_operations["entity.get"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            Ir77GraphEntity entity{};
            if (m_graph->Get(std::string{Text(a, "id")}, entity)->ID() != GUIDIr77OperationSucceeded) return Fail("no entity " + std::string{Text(a, "id")});

            m_data["entity"] = EntityJson(entity);

            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        // -> { entities: [...] } -- the whole graph, parents first
        m_operations["entity.list"] = [this](Ir77SceneArgs const&) -> std::shared_ptr<IIr77Return const> {
            m_data["entities"] = EntitiesJson();

            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        // the active entity ('' = none) -> as ModeData
        m_operations["entity.select"] = [this](Ir77SceneArgs const& a) { return Select(std::string{Text(a, "id")}); };

        m_operations["entity.mode"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            Ir77InteractionMode mode{Ir77InteractionMode::Object};
            if (!InteractionModeFromName(Text(a, "mode"), mode))
                return Fail("entity.mode mode is object, edit, sculpt, vertex_paint, weight_paint or texture_paint");

            return Interact(mode);
        };

        // the model under x, y (fractions of the window, from the top left) -> { id } ('' = nothing there)
        m_operations["entity.pick"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            float x{0.0f};
            float y{0.0f};
            if (!Point(a, x, y)) return Fail("entity.pick needs x and y");

            std::shared_ptr<IIr77Return const> result{};

            std::uint32_t runtime{EDIT_NO_ENTITY};
            if (Failed(m_world->PickEntity(x, y, runtime), result)) return result;

            std::string id{};
            if (runtime != EDIT_NO_ENTITY) m_graph->FindModel(runtime, id);

            m_data["id"] = id;

            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        // a model's own nodes -- its file's structure (bones included), not graph entities -> { nodes: [{node, name, parent, t, r, s}] }
        m_operations["node.list"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::shared_ptr<IIr77Model> model{};
            if (!LoadedModel(a, model)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: " + m_error);

            m_data["nodes"] = NodesJson(model);

            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        // one node's rest TRS inside a model -- a clip that drives it still wins on what it animates
        m_operations["node.transform"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::shared_ptr<IIr77Return const> result{};
            std::shared_ptr<IIr77Model> model{};
            if (!LoadedModel(a, model)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: " + m_error);

            double node_number{-1.0};
            if (a["node"].get(node_number) != simdjson::SUCCESS || node_number < 0.0) return Fail("node.transform needs node");

            std::uint32_t const node = static_cast<std::uint32_t>(node_number);

            std::string name{};
            std::uint32_t parent{UINT32_MAX};
            glm::vec3 translation{0.0f};
            glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
            glm::vec3 scale{1.0f};
            if (Failed(model->GetNode(node, name, parent, translation, rotation, scale), result)) return result;

            Placement(a, translation, rotation, scale);

            return model->SetNodeTransform(node, translation, rotation, scale);
        };
    }

    // id, parent, name, kind, order, visible, t, r, s -> entity. adding: kind required, id / parent taken; setting: only what's there changes
    // (id, kind and parent stay -- entity.move re-parents)
    bool EntityCore(Ir77SceneArgs const& a, Ir77GraphEntity& entity, bool const adding) {
        if (adding) {
            if (!GraphKindFromName(Text(a, "kind"), entity.kind)) {
                m_error = "kind must be collection, model, light or camera";
                return false;
            }

            entity.id = std::string{Text(a, "id")};
            entity.parent = std::string{Text(a, "parent")};
        }

        std::string_view name{};
        if (a["name"].get(name) == simdjson::SUCCESS) entity.name = std::string{name};

        entity.visible = Flag(a, "visible", entity.visible);
        entity.order = static_cast<std::uint32_t>(std::max(Number(a, "order", static_cast<float>(entity.order)), 0.0f));

        Placement(a, entity.t, entity.r, entity.s);

        return true;
    }

    // fov, ortho, near, far over what the camera had
    void CameraFields(Ir77SceneArgs const& args, Ir77GraphCamera& camera) {
        camera.fov = std::clamp(Number(args, "fov", camera.fov), 1.0f, 179.0f);
        camera.ortho = Flag(args, "ortho", camera.ortho);
        camera.near = std::max(Number(args, "near", camera.near), 1e-4f);
        camera.far = std::max(Number(args, "far", camera.far), camera.near * 2.0f);
    }

    // a model entity's Model, by the op's id
    bool LoadedModel(Ir77SceneArgs const& a, std::shared_ptr<IIr77Model>& model) {
        Ir77GraphEntity entity{};
        if (m_graph->Get(std::string{Text(a, "id")}, entity)->ID() != GUIDIr77OperationSucceeded) {
            m_error = "no entity " + std::string{Text(a, "id")};
            return false;
        }

        if (entity.kind != Ir77GraphKind::Model || entity.runtime == GRAPH_NO_RUNTIME ||
            m_world->GetModel(entity.runtime, model)->ID() != GUIDIr77OperationSucceeded) {
            m_error = "entity " + entity.id + " isn't a loaded model";
            return false;
        }

        return true;
    }

    std::shared_ptr<IIr77Return const> Fail(std::string const& message) {
        m_error = message;
        return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: " + message);
    }

    // edit mode on the active model -- selecting its vertices, edges and faces
    void DefineEditOperations() {
        m_operations["edit.mode"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            Ir77EditMode mode{Ir77EditMode::Vertex};
            if (!EditModeFromName(Text(a, "mode"), mode)) return Fail("edit.mode is vertex, edge or face");

            std::shared_ptr<IIr77Return const> result{};
            if (Failed(m_world->SetEditMode(mode), result)) return Fail("nothing is in edit mode");

            return SelectionData();
        };

        m_operations["edit.pick"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            float x{0.0f};
            float y{0.0f};
            if (!Point(a, x, y)) return Fail("edit.pick needs x and y");

            std::shared_ptr<IIr77Return const> result{};

            bool hit{false};
            if (Failed(m_world->EditPick(x, y, Flag(a, "extend", false), hit), result)) return Fail("nothing is in edit mode");

            m_data["hit"] = hit;
            return SelectionData();
        };

        m_operations["edit.box"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            double x0{0.0};
            double y0{0.0};
            double x1{0.0};
            double y1{0.0};
            if (a["x0"].get(x0) != simdjson::SUCCESS || a["y0"].get(y0) != simdjson::SUCCESS || a["x1"].get(x1) != simdjson::SUCCESS ||
                a["y1"].get(y1) != simdjson::SUCCESS)
                return Fail("edit.box needs x0, y0, x1, y1");

            std::shared_ptr<IIr77Return const> result{};

            std::uint32_t taken{0};
            if (Failed(m_world->EditBoxSelect(static_cast<float>(x0), static_cast<float>(y0), static_cast<float>(x1), static_cast<float>(y1),
                                              Flag(a, "extend", false), taken),
                       result))
                return Fail("nothing is in edit mode");

            m_data["taken"] = taken;
            return SelectionData();
        };

        m_operations["edit.select_all"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::shared_ptr<IIr77Return const> result{};
            if (Failed(m_world->EditSelectAll(Flag(a, "value", true)), result)) return Fail("nothing is in edit mode");

            return SelectionData();
        };

        m_operations["edit.undo"] = [this](Ir77SceneArgs const&) -> std::shared_ptr<IIr77Return const> {
            std::shared_ptr<IIr77Return const> result{};

            bool done{false};
            if (Failed(m_world->EditUndo(done), result)) return Fail("nothing is in edit mode");

            m_data["done"] = done;
            return HistoryData();
        };

        m_operations["edit.redo"] = [this](Ir77SceneArgs const&) -> std::shared_ptr<IIr77Return const> {
            std::shared_ptr<IIr77Return const> result{};

            bool done{false};
            if (Failed(m_world->EditRedo(done), result)) return Fail("nothing is in edit mode");

            m_data["done"] = done;
            return HistoryData();
        };

        m_operations["edit.commit"] = [this](Ir77SceneArgs const&) -> std::shared_ptr<IIr77Return const> {
            std::vector<std::string> written{};
            std::vector<std::string> errors{};
            m_world->CommitEdits(written, errors);  // a model that wasn't written is in errors -- the reply carries both

            Ir77Json& files = m_data["written"] = Ir77Json::Array();
            for (std::string const& path : written) files.Push(std::filesystem::path{path}.stem().string());

            Ir77Json& lines = m_data["errors"] = Ir77Json::Array();
            for (std::string const& line : errors) lines.Push(line);

            return HistoryData();
        };

        m_operations["gizmo.mode"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            Ir77GizmoMode mode{Ir77GizmoMode::Translate};
            if (!GizmoModeFromName(Text(a, "mode"), mode)) return Fail("gizmo.mode is translate, rotate or scale");

            EndObjectDrag(false);  // an object drag under way was the old mode's -- back where it started
            m_world->SetGizmoMode(mode);  // and an edit one (Edit puts it back)
            m_data["gizmo"] = GizmoModeName(mode);

            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        m_operations["gizmo.hover"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            float x{0.0f};
            float y{0.0f};
            if (!Point(a, x, y)) return Fail("gizmo.hover needs x and y");

            std::shared_ptr<IIr77Return const> result{};

            std::uint32_t handle{GIZMO_NONE};
            if (m_interaction == Ir77InteractionMode::Edit) {
                if (Failed(m_world->EditGizmoHover(x, y, handle), result)) return Fail("nothing is in edit mode");
            } else if (Failed(m_world->ObjectGizmoHover(x, y, handle), result)) {
                return result;
            }

            m_data["handle"] = handle == GIZMO_NONE ? -1 : static_cast<int>(handle);
            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        m_operations["gizmo.press"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            float x{0.0f};
            float y{0.0f};
            if (!Point(a, x, y)) return Fail("gizmo.press needs x and y");

            std::shared_ptr<IIr77Return const> result{};

            std::uint32_t handle{GIZMO_NONE};
            if (m_interaction == Ir77InteractionMode::Edit) {
                if (Failed(m_world->EditGizmoPress(x, y, handle), result)) return Fail("nothing is in edit mode");
            } else if (Failed(BeginObjectDrag(x, y, handle), result)) {
                return result;
            }

            m_data["handle"] = handle == GIZMO_NONE ? -1 : static_cast<int>(handle);
            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        m_operations["gizmo.move"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            float x{0.0f};
            float y{0.0f};
            if (!Point(a, x, y)) return Fail("gizmo.move needs x and y");

            std::shared_ptr<IIr77Return const> result{};

            if (m_object_drag.active) {
                if (Failed(MoveObjectDrag(x, y), result)) return result;

                m_data["changed"] = m_object_drag.changed;
                return Ir77RETURN<Ir77OperationSucceeded>();
            }

            glm::vec3 delta{0.0f};
            if (Failed(m_world->EditGrabMove(x, y, Ir77EditAxis::Free, delta), result)) return Fail("no drag under way");

            m_data["changed"] = glm::dot(delta, delta) > 0.0f;
            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        m_operations["gizmo.end"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            bool const confirm = Flag(a, "confirm", true);

            if (m_object_drag.active) {
                std::string const id = m_object_drag.id;
                bool const modified = EndObjectDrag(confirm);

                m_data["modified"] = modified;
                m_data["id"] = id;
                return Ir77RETURN<Ir77OperationSucceeded>();
            }

            std::shared_ptr<IIr77Return const> result{};
            if (Failed(m_world->EditGrabEnd(confirm), result)) return result;

            return HistoryData();
        };

    }

    // the reply's modified flag and how far undo / redo can go
    std::shared_ptr<IIr77Return const> HistoryData() {
        bool grabbing{false};
        bool modified{false};
        m_world->GetEditGrab(grabbing, modified);

        std::uint32_t undo{0};
        std::uint32_t redo{0};
        m_world->GetEditHistory(undo, redo);

        m_data["modified"] = modified;
        m_data["undo"] = undo;
        m_data["redo"] = redo;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // the reply's mode and selected count
    std::shared_ptr<IIr77Return const> SelectionData() {
        Ir77EditMode mode{Ir77EditMode::Vertex};
        std::uint32_t selected{0};
        m_world->GetEditSelection(mode, selected);

        m_data["mode"] = EditModeName(mode);
        m_data["selected"] = selected;

        Ir77GizmoMode gizmo{Ir77GizmoMode::Translate};
        std::uint32_t hot{GIZMO_NONE};
        m_world->GetGizmo(gizmo, hot);
        m_data["gizmo"] = GizmoModeName(gizmo);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // A model in edit mode -- it plays nothing until it leaves
    bool InEdit(Ir77GraphEntity const& e) const {
        std::uint32_t edit{EDIT_NO_ENTITY};
        std::uint32_t points{0};
        std::uint32_t edges{0};
        m_world->GetEditEntity(edit, points, edges);

        return e.kind == Ir77GraphKind::Model && e.runtime != GRAPH_NO_RUNTIME && e.runtime == edit;
    }

    // The animation ops' model -- one in edit mode plays nothing, so they wait until it leaves (Esc)
    bool AnimatedModel(Ir77SceneArgs const& a, std::shared_ptr<IIr77Model>& model) {
        if (!LoadedModel(a, model)) return false;

        Ir77GraphEntity entity{};
        m_graph->Get(std::string{Text(a, "id")}, entity);

        if (InEdit(entity)) {
            m_error = "entity " + entity.id + " is in edit mode -- its animations are off until it leaves";
            return false;
        }

        return true;
    }

    // The active entity ('' = none) -- in object mode: whatever mode the one before was in, it leaves it first. -> m_data (ModeData)
    std::shared_ptr<IIr77Return const> Select(std::string const& id) {
        EndObjectDrag(false);

        if (!id.empty()) {
            Ir77GraphEntity entity{};
            if (m_graph->Get(id, entity)->ID() != GUIDIr77OperationSucceeded) return Fail("no entity " + id);
        }

        std::shared_ptr<IIr77Return const> result{};
        if (Failed(m_world->SetEditEntity(EDIT_NO_ENTITY), result)) return result;

        m_active = id;
        m_interaction = Ir77InteractionMode::Object;
        Outline();
        ObjectGizmo();

        return ModeData();
    }

    // The active entity into a mode -- object leaves whatever it was in; edit builds the active model's cage. A mode that isn't built yet, or
    // edit with no active model: refused, the mode stays. -> m_data (ModeData)
    std::shared_ptr<IIr77Return const> Interact(Ir77InteractionMode const mode) {
        EndObjectDrag(false);
        if (!InteractionModeBuilt(mode)) return Fail(std::string{InteractionModeName(mode)} + " mode isn't built yet");

        std::uint32_t runtime{EDIT_NO_ENTITY};

        if (mode == Ir77InteractionMode::Edit) {
            Ir77GraphEntity entity{};
            if (m_active.empty() || m_graph->Get(m_active, entity)->ID() != GUIDIr77OperationSucceeded) return Fail("edit mode needs an active model");
            if (entity.kind != Ir77GraphKind::Model || entity.runtime == GRAPH_NO_RUNTIME) return Fail("edit mode needs an active model -- " + m_active + " isn't one");

            runtime = entity.runtime;
        }

        std::shared_ptr<IIr77Return const> result{};
        if (Failed(m_world->SetEditEntity(runtime), result)) return result;

        m_interaction = mode;
        Outline();
        ObjectGizmo();

        return ModeData();
    }

    // The active model's outline -- in object mode only (edit mode draws its cage instead); anything else active, or nothing: none
    void Outline() {
        std::uint32_t runtime{EDIT_NO_ENTITY};

        Ir77GraphEntity entity{};
        if (m_interaction == Ir77InteractionMode::Object && !m_active.empty() && m_graph->Get(m_active, entity)->ID() == GUIDIr77OperationSucceeded &&
            entity.kind == Ir77GraphKind::Model && entity.runtime != GRAPH_NO_RUNTIME)
            runtime = entity.runtime;

        m_world->SetOutlineEntity(runtime);
    }

    // Object mode's gizmo -- at the active entity, any kind, in object mode only; otherwise hidden
    void ObjectGizmo() {
        glm::mat4 world{1.0f};
        bool const shown = m_interaction == Ir77InteractionMode::Object && !m_active.empty() &&
                           m_graph->World(m_active, world)->ID() == GUIDIr77OperationSucceeded;

        m_world->SetObjectGizmo(shown, world);
    }

    // a press on the object gizmo -- a drag of the active entity's whole transform starts (handle = the handle; GIZMO_NONE: none)
    std::shared_ptr<IIr77Return const> BeginObjectDrag(float const x, float const y, std::uint32_t& handle) {
        handle = GIZMO_NONE;
        EndObjectDrag(false);

        Ir77GraphEntity entity{};
        if (m_active.empty() || m_graph->Get(m_active, entity)->ID() != GUIDIr77OperationSucceeded) return Ir77RETURN<Ir77OperationSucceeded>();

        std::shared_ptr<IIr77Return const> result{};
        if (Failed(m_world->ObjectGizmoPress(x, y, handle), result)) return result;
        if (handle == GIZMO_NONE) return Ir77RETURN<Ir77OperationSucceeded>();

        Ir77ObjectDrag drag{};
        drag.active = true;
        drag.id = entity.id;
        drag.t = entity.t;
        drag.r = entity.r;
        drag.s = entity.s;
        m_graph->World(entity.id, drag.start_world);
        if (!entity.parent.empty()) m_graph->World(entity.parent, drag.parent_world);

        m_object_drag = drag;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // the cursor now -> the entity's world = the drag's matrix * where it started, taken back into its parent as its own t / r / s
    std::shared_ptr<IIr77Return const> MoveObjectDrag(float const x, float const y) {
        std::shared_ptr<IIr77Return const> result{};

        glm::mat4 drag{1.0f};
        bool changed{false};
        if (Failed(m_world->ObjectGizmoDrag(x, y, drag, changed), result)) return result;

        Ir77GraphEntity entity{};
        if (m_graph->Get(m_object_drag.id, entity)->ID() != GUIDIr77OperationSucceeded) {
            m_object_drag = Ir77ObjectDrag{};
            m_world->ObjectGizmoEnd();
            return Fail("the dragged entity is gone");
        }

        glm::mat4 const world = drag * m_object_drag.start_world;
        glm::mat4 const local = glm::inverse(m_object_drag.parent_world) * world;

        // a parent's non-uniform scale under its rotation can't be held by t / r / s -- its shear is dropped
        glm::vec3 skew{0.0f};
        glm::vec4 perspective{0.0f};
        glm::vec3 t{0.0f};
        glm::quat r{1.0f, 0.0f, 0.0f, 0.0f};
        glm::vec3 s{1.0f};
        if (!glm::decompose(local, s, r, t, skew, perspective)) return Ir77RETURN<Ir77OperationSucceeded>();  // degenerate (a zero scale) -- hold

        entity.t = t;
        entity.r = glm::normalize(r);
        entity.s = s;

        std::string error{};
        if (m_graph->Set(entity, error)->ID() != GUIDIr77OperationSucceeded) return Fail(error);

        m_object_drag.changed = changed;
        ObjectGizmo();  // the gizmo follows a move

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The object drag is over -- confirm: it stays (true if it changed anything); otherwise the entity goes back to where it started. No drag:
    // false.
    bool EndObjectDrag(bool const confirm) {
        if (!m_object_drag.active) return false;

        Ir77ObjectDrag const drag = m_object_drag;
        m_object_drag = Ir77ObjectDrag{};
        m_world->ObjectGizmoEnd();

        if (confirm) return drag.changed;

        Ir77GraphEntity entity{};
        if (m_graph->Get(drag.id, entity)->ID() == GUIDIr77OperationSucceeded) {
            entity.t = drag.t;
            entity.r = drag.r;
            entity.s = drag.s;

            std::string error{};
            m_graph->Set(entity, error);
        }

        ObjectGizmo();
        return false;
    }

    // { id, interaction, edit, points, edges } + the edit selection
    std::shared_ptr<IIr77Return const> ModeData() {
        std::uint32_t edit{EDIT_NO_ENTITY};
        std::uint32_t points{0};
        std::uint32_t edges{0};
        m_world->GetEditEntity(edit, points, edges);

        m_data["id"] = m_active;
        m_data["interaction"] = InteractionModeName(m_interaction);
        m_data["edit"] = edit != EDIT_NO_ENTITY;
        m_data["points"] = points;
        m_data["edges"] = edges;

        return SelectionData();
    }

    // type / color / intensity / range / inner / outer over what the light had -- what's left out keeps its value
    std::shared_ptr<IIr77Return const> LightFields(Ir77SceneArgs const& args, Ir77PVLight& light) {
        std::string_view const type = Text(args, "type");

        if (type == "directional")
            light.type = LIGHT_DIRECTIONAL;
        else if (type == "point")
            light.type = LIGHT_POINT;
        else if (type == "spot")
            light.type = LIGHT_SPOT;
        else if (!type.empty()) {
            m_error = "light type must be directional, point or spot";
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: " + m_error);
        }

        float color[3]{light.color.r, light.color.g, light.color.b};
        Numbers(args, "color", color, 3);
        light.color = glm::vec3(color[0], color[1], color[2]);

        light.intensity = std::max(Number(args, "intensity", light.intensity), 0.0f);
        light.range = std::max(Number(args, "range", light.range), 0.0f);
        light.innerConeAngle = std::max(Number(args, "inner", light.innerConeAngle), 0.0f);
        light.outerConeAngle = std::max(Number(args, "outer", light.outerConeAngle), light.innerConeAngle);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    static std::string LightTypeName(int const type) { return type == LIGHT_DIRECTIONAL ? "directional" : type == LIGHT_SPOT ? "spot" : "point"; }

    // One model's nodes -- node.list and scene.dump
    Ir77Json NodesJson(std::shared_ptr<IIr77Model> const& model) const {
        Ir77Json nodes = Ir77Json::Array();

        std::uint32_t count{0};
        model->GetNodeCount(count);

        for (std::uint32_t n = 0; n < count; n++) {
            std::string name{};
            std::uint32_t parent{UINT32_MAX};
            glm::vec3 translation{0.0f};
            glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
            glm::vec3 scale{1.0f};
            model->GetNode(n, name, parent, translation, rotation, scale);

            Ir77Json& node = nodes.Push(Ir77Json::Object());
            node["node"] = n;
            node["name"] = name;
            node["parent"] = parent == UINT32_MAX ? std::int64_t{-1} : std::int64_t{parent};
            Placement(node, translation, rotation, scale);
        }

        return nodes;
    }

    // One model's clips (what its file has) and players (what's running). held: the model is in edit mode -- what it plays when it leaves
    // stands in for its (empty) players, time 0, so a save in edit mode stores what the model plays.
    void ClipsJson(std::shared_ptr<IIr77Model> const& model, bool const held, Ir77Json& clips, Ir77Json& playing) const {
        clips = Ir77Json::Array();
        playing = Ir77Json::Array();

        std::shared_ptr<IIr77Animation> animations{};
        std::uint32_t clip_count{0};
        if (model->GetAnimations(animations)->ID() == GUIDIr77OperationSucceeded && animations) animations->GetClipCount(clip_count);

        std::vector<std::string> names(clip_count);
        for (std::uint32_t c = 0; c < clip_count; c++) {
            float duration{0.0f};
            animations->GetClipName(c, names[c]);
            animations->GetClipDuration(c, duration);

            Ir77Json& clip = clips.Push(Ir77Json::Object());
            clip["clip"] = c;
            clip["name"] = names[c];
            clip["duration"] = duration;
        }

        if (held) {
            std::vector<Ir77EditClip> suspended{};
            m_world->GetEditClips(suspended);

            for (Ir77EditClip const& clip : suspended) {
                Ir77Json& player = playing.Push(Ir77Json::Object());
                player["clip"] = clip.clip;
                player["name"] = clip.clip < names.size() ? names[clip.clip] : std::string{};
                player["time"] = 0.0f;
                player["speed"] = clip.speed;
                player["loop"] = clip.loop;
            }

            return;
        }

        std::uint32_t players{0};
        model->GetPlayerCount(players);

        for (std::uint32_t p = 0; p < players; p++) {
            std::uint32_t clip{UINT32_MAX};
            float time{0.0f};
            float duration{0.0f};
            float speed{1.0f};
            bool loop{true};
            if (model->GetPlayer(p, clip, time, duration, speed, loop)->ID() != GUIDIr77OperationSucceeded) continue;

            Ir77Json& player = playing.Push(Ir77Json::Object());
            player["clip"] = clip;
            player["name"] = clip < names.size() ? names[clip] : std::string{};
            player["time"] = time;
            player["speed"] = speed;
            player["loop"] = loop;
        }
    }

    // The whole scene graph -- what the UI builds its outliner from at start, and what a save turns into a scene document
    Ir77Json Dump() const {
        Ir77Json dump = Ir77Json::Object();

        dump["project"] = m_project;
        dump["startup"] = m_startup_scene;
        dump["document"]["id"] = m_document_id;
        dump["document"]["name"] = m_document_name;
        dump["settings"] = m_settings;
        dump["active"] = m_active;
        dump["interaction"] = InteractionModeName(m_interaction);

        // edit mode on the active model -- what a click selects and how many vertices are selected
        Ir77EditMode mode{Ir77EditMode::Vertex};
        std::uint32_t selected{0};
        m_world->GetEditSelection(mode, selected);
        dump["edit"]["mode"] = EditModeName(mode);
        dump["edit"]["selected"] = selected;

        Ir77GizmoMode gizmo{Ir77GizmoMode::Translate};
        std::uint32_t hot{GIZMO_NONE};
        m_world->GetGizmo(gizmo, hot);
        dump["edit"]["gizmo"] = GizmoModeName(gizmo);
        dump["entities"] = EntitiesJson();

        return dump;
    }

    // the graph, parents first
    Ir77Json EntitiesJson() const {
        Ir77Json entities = Ir77Json::Array();
        std::vector<Ir77GraphEntity const*> ordered{};
        m_graph->Ordered(ordered);

        for (Ir77GraphEntity const* entity : ordered) entities.Push(EntityJson(*entity));

        return entities;
    }

    // one entity -- the core, then its kind's payload; a model adds what its file has (clips, nodes) and what's playing
    Ir77Json EntityJson(Ir77GraphEntity const& e) const {
        Ir77Json json = Ir77Json::Object();

        json["id"] = e.id;
        json["name"] = e.name;
        json["kind"] = GraphKindName(e.kind);
        json["parent"] = e.parent;
        json["order"] = e.order;
        json["visible"] = e.visible;
        Placement(json, e.t, e.r, e.s);

        if (e.kind == Ir77GraphKind::Model) {
            Ir77Json clips = Ir77Json::Array();
            Ir77Json playing = Ir77Json::Array();
            Ir77Json nodes = Ir77Json::Array();

            std::shared_ptr<IIr77Model> model{};
            if (e.runtime != GRAPH_NO_RUNTIME && m_world->GetModel(e.runtime, model)->ID() == GUIDIr77OperationSucceeded && model) {
                nodes = NodesJson(model);
                ClipsJson(model, InEdit(e), clips, playing);
            }

            json["model"] = e.model;
            json["clips"] = std::move(clips);
            json["playing"] = std::move(playing);
            json["nodes"] = std::move(nodes);
        }

        if (e.kind == Ir77GraphKind::Light) {
            Ir77Json& light = json["light"];
            light["type"] = LightTypeName(e.light.type);
            light["color"] = Ir77Json::Numbers({e.light.color.r, e.light.color.g, e.light.color.b});
            light["intensity"] = e.light.intensity;
            light["range"] = e.light.range;
            light["inner"] = e.light.innerConeAngle;
            light["outer"] = e.light.outerConeAngle;
        }

        if (e.kind == Ir77GraphKind::Camera) {
            Ir77Json& camera = json["camera"];
            camera["fov"] = e.camera.fov;
            camera["ortho"] = e.camera.ortho;
            camera["near"] = e.camera.near;
            camera["far"] = e.camera.far;
        }

        return json;
    }

    // A setting the renderer acts on -> applied; any other key is only kept
    std::shared_ptr<IIr77Return const> ApplySetting(std::string_view const key, simdjson::dom::element const value) {
        if (key == "background") {
            glm::vec3 color{0.0f};
            m_world->GetBackground(color);

            simdjson::dom::array channels{};
            if (value.get(channels) == simdjson::SUCCESS) {
                std::size_t c{0};
                for (auto const channel : channels) {
                    double number{0.0};
                    if (c < 3 && channel.get(number) == simdjson::SUCCESS) color[static_cast<glm::length_t>(c)] = static_cast<float>(number);
                    c++;
                }
            }

            return m_world->SetBackground(color);
        }

        if (key == "grid") {
            bool show{true};
            if (value.get(show) != simdjson::SUCCESS) return Ir77RETURN<Ir77OperationSucceeded>();

            return m_world->SetGridVisible(show);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // a parsed value -> Ir77Json -- settings are kept as the UI sent them
    static Ir77Json FromElement(simdjson::dom::element const element) {
        switch (element.type()) {
            case simdjson::dom::element_type::ARRAY: {
                Ir77Json array = Ir77Json::Array();
                for (auto const item : element.get_array().value_unsafe()) array.Push(FromElement(item));
                return array;
            }
            case simdjson::dom::element_type::OBJECT: {
                Ir77Json object = Ir77Json::Object();
                for (auto const field : element.get_object().value_unsafe()) object[field.key] = FromElement(field.value);
                return object;
            }
            case simdjson::dom::element_type::STRING:
                return Ir77Json{element.get_string().value_unsafe()};
            case simdjson::dom::element_type::INT64:
                return Ir77Json{element.get_int64().value_unsafe()};
            case simdjson::dom::element_type::UINT64:
                return Ir77Json{element.get_uint64().value_unsafe()};
            case simdjson::dom::element_type::DOUBLE:
                return Ir77Json{element.get_double().value_unsafe()};
            case simdjson::dom::element_type::BOOL:
                return Ir77Json{element.get_bool().value_unsafe()};
            default:
                return Ir77Json{};
        }
    }

    // A .glb from anywhere -> a model of the open project, flattened (services/Ir77Flatten.hpp) into <project>/Models/<model>.glb -- one file,
    // one entity. model = the file's name, made unique (Door, Door_001, Door_002, ...); a flattened result identical to a model already there is
    // that model, so importing the same file twice adds no copy. A file already in Models/ is taken as it is. Nothing in Models/ is ever
    // overwritten. A .gltf is refused: its buffers and textures are separate files. What the flatten left out -> m_import_report.
    bool AdoptModel(std::string const& source_text, std::string& model) {
        m_import_report.clear();

        std::error_code ec{};
        std::filesystem::path const source = std::filesystem::absolute(source_text, ec).lexically_normal();

        if (!std::filesystem::is_regular_file(source, ec)) {
            m_error = "no file " + source.string();
            return false;
        }

        std::string extension = source.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        if (extension != ".glb") {
            m_error = "only .glb files go into a project -- export " + source.filename().string() + " as .glb";
            return false;
        }

        std::filesystem::path const models = std::filesystem::path{m_project} / "Models";
        std::string const base = source.stem().string();

        // already one of the project's models
        if (std::filesystem::equivalent(source.parent_path(), models, ec)) {
            model = base;
            return true;
        }

        Ir77Flatten flatten{};
        std::vector<std::byte> glb{};
        std::string error{};
        if (!flatten.Run(source.string(), base, glb, m_import_report, error)) {
            m_error = error;
            return false;
        }

        std::filesystem::create_directories(models, ec);

        for (int n = 0; n < 1000; n++) {
            char suffix[8]{};
            std::snprintf(suffix, sizeof(suffix), "_%03d", n);

            std::string const name = n == 0 ? base : base + suffix;
            std::filesystem::path const target = models / (name + ".glb");

            if (std::filesystem::exists(target, ec)) {
                if (!SameContents(target, glb)) continue;  // a different model has the name

                model = name;  // this one, imported before
                return true;
            }

            std::ofstream file{target, std::ios::binary};
            file.write(reinterpret_cast<char const*>(glb.data()), static_cast<std::streamsize>(glb.size()));
            if (!file) {
                m_error = "couldn't write " + target.string();
                return false;
            }

            model = name;
            return true;
        }

        m_error = "Models/ already has " + base + " through " + base + "_999";
        return false;
    }

    // a file against bytes -- sizes first
    static bool SameContents(std::filesystem::path const& path, std::vector<std::byte> const& bytes) {
        std::error_code ec{};
        if (std::filesystem::file_size(path, ec) != bytes.size() || ec) return false;

        std::ifstream file{path, std::ios::binary};
        std::vector<char> contents(bytes.size());
        file.read(contents.data(), static_cast<std::streamsize>(contents.size()));

        return file && std::memcmp(contents.data(), bytes.data(), bytes.size()) == 0;
    }

    // A project model by name -- <project>/Models/<model>.glb
    std::string ModelPath(std::string const& model) const { return (std::filesystem::path{m_project} / "Models" / (model + ".glb")).string(); }

    // a model entity's clips, by its id
    void DefineAnimationOperations() {
        m_operations["animation.play"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::shared_ptr<IIr77Model> model{};
            if (!AnimatedModel(a, model)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: " + m_error);

            return model->PlayAnimation(std::string{Text(a, "name")}, Flag(a, "loop", true), Number(a, "speed", 1.0f));
        };
        m_operations["animation.stop"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::shared_ptr<IIr77Model> model{};
            if (!AnimatedModel(a, model)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: " + m_error);

            return model->StopAnimation(std::string{Text(a, "name")});
        };
        m_operations["animation.stop_all"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::shared_ptr<IIr77Model> model{};
            if (!AnimatedModel(a, model)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: " + m_error);

            return model->StopAllAnimations();
        };
    }

    // args readers -- a missing or mistyped key gives the fallback
    // t / r / s over what the thing had -- a part left out keeps its value
    void Placement(Ir77SceneArgs const& args, glm::vec3& translation, glm::quat& rotation, glm::vec3& scale) const {
        float t[3]{translation.x, translation.y, translation.z};
        float r[4]{rotation.x, rotation.y, rotation.z, rotation.w};
        float s[3]{scale.x, scale.y, scale.z};

        Numbers(args, "t", t, 3);
        Numbers(args, "r", r, 4);
        Numbers(args, "s", s, 3);

        glm::quat const turned{r[3], r[0], r[1], r[2]};  // glTF order in the message: x, y, z, w

        translation = glm::vec3(t[0], t[1], t[2]);
        if (glm::dot(turned, turned) > 1e-12f) rotation = turned;  // an all-zero r has no direction -- keeps what it was
        scale = glm::vec3(s[0], s[1], s[2]);
    }

    // t, r, s onto json -- r as x, y, z, w
    static void Placement(Ir77Json& json, glm::vec3 const& translation, glm::quat const& rotation, glm::vec3 const& scale) {
        json["t"] = Ir77Json::Numbers({translation.x, translation.y, translation.z});
        json["r"] = Ir77Json::Numbers({rotation.x, rotation.y, rotation.z, rotation.w});
        json["s"] = Ir77Json::Numbers({scale.x, scale.y, scale.z});
    }

    float Number(Ir77SceneArgs const& args, char const* key, float const fallback) const {
        double value{0.0};
        return args[key].get(value) == simdjson::SUCCESS ? static_cast<float>(value) : fallback;
    }

    bool Flag(Ir77SceneArgs const& args, char const* key, bool const fallback) const {
        bool value{false};
        return args[key].get(value) == simdjson::SUCCESS ? value : fallback;
    }

    std::string_view Text(Ir77SceneArgs const& args, char const* key) const {
        std::string_view value{};
        return args[key].get(value) == simdjson::SUCCESS ? value : std::string_view{};
    }

    // "x" and "y" -- a point of the window (fractions, from the top left); false when either is missing
    bool Point(Ir77SceneArgs const& args, float& x, float& y) const {
        double px{0.0};
        double py{0.0};
        if (args["x"].get(px) != simdjson::SUCCESS || args["y"].get(py) != simdjson::SUCCESS) return false;

        x = static_cast<float>(px);
        y = static_cast<float>(py);
        return true;
    }

    // An array of numbers into out[0 .. count) -- entries past the array (or not numbers) keep what out held
    void Numbers(Ir77SceneArgs const& args, char const* key, float* out, std::size_t const count) const {
        simdjson::dom::array values{};
        if (args[key].get(values) != simdjson::SUCCESS) return;

        std::size_t i{0};
        for (auto const element : values) {
            if (i >= count) break;

            double value{0.0};
            if (element.get(value) == simdjson::SUCCESS) out[i] = static_cast<float>(value);
            i++;
        }
    }

   private:
    std::shared_ptr<IIr77World> m_world{};  // the module's work -- only this server holds it

    std::shared_ptr<IIr77View> m_view{};  // World's -- camera.* ops

    std::function<void(std::string)> m_post{};

    std::string m_error{};  // a handler's detail for the reply's "error" -- cleared before every message

    Ir77Json m_data{Ir77Json::Object()};  // a handler's result for the reply's "data" -- reset to {} before every message

    Ir77Json m_settings{Ir77Json::Object()};  // the scene's settings (scene.settings) -- any keys; the dump carries them

    std::string m_project{};  // scene.project -- the folder models/ live in

    std::shared_ptr<IIr77Graph> m_graph{};  // the scene graph -- every entity by its id; it drives World. Only this server holds it.

    std::vector<std::string> m_import_report{};  // what the last import's flatten left out -- entity.add's reply carries it

    std::string m_startup_scene{};  // scene.startup -- until the UI's first scene.create

    std::string m_document_id{};  // scene.document -- the stored scene this is; '' = not stored

    std::string m_document_name{};

    std::string m_active{};  // entity.select -- the active entity's id ('' = none). The dump carries it.

    Ir77InteractionMode m_interaction{Ir77InteractionMode::Object};  // entity.mode -- what the active entity is worked in

    Ir77ObjectDrag m_object_drag{};  // object mode's gizmo drag on the active entity

    simdjson::dom::parser m_parser{};

    simdjson::dom::parser m_empty_parser{};  // owns m_empty_args

    Ir77SceneArgs m_empty_args{};

    std::unordered_map<std::string, Ir77SceneOperation, Ir77SceneOpHash, std::equal_to<>> m_operations{};
};

}  // namespace NSIr77Scene