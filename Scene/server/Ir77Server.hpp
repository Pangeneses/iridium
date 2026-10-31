#pragma once

#include <algorithm>
#include <cctype>
#include <cstring>
#include <array>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <stdexcept>
#include <system_error>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>

#include <simdjson.h>

#include "../../Ir77RT/runtime/Ir77Json.hpp"
#include "../../Ir77RT/runtime/Ir77Return.hpp"

#include "../dictionary/IDIIr77Scene.hpp"

#include "../interface/IIr77Pose.hpp"
#include "../interface/IIr77View.hpp"
#include "../interface/IIr77Field.hpp"
#include "../interface/IIr77Animate.hpp"

#include "../runtime/Ir77Animate.hpp"
#include "../runtime/Ir77Flatten.hpp"
#include "../runtime/Ir77Graph.hpp"

#include "../types/Ir77Types.hpp"
#include "../types/Ir77Field.hpp"

#include "../../PeregrineV/server/Ir77Server.hpp"

using namespace NSIr77RT;

namespace NSIr77Scene {

// =============================================================================================================================================
// Ir77Server -- the scene module's switchboard. Owns Ir77Animate (the work) -- created here, held through IIr77Animate, handed to no one.
// Animate owns the entities (a Pose each), View, Lighting and Field; this server holds View and Field through their interfaces and reaches an
// entity's Pose through Animate (GetPose) per op. The module's one door:
//
//   pipe:        Sink(json) in, the post function out (SetPost). Every command arrives here as JSON and every reply leaves the same way.
//   direct link: only what can't travel as JSON -- the renderer's server (SetRenderer) and the per-frame calls (Update / CreateFrame / stats).
//                Everything else is an op, setup included.
//
// Message in:  {"id":7, "from":"ui", "to":"scene", "op":"camera.move", "args":{"right":1, "up":0, "forward":0}}
// Reply out:   {"id":7, "from":"scene", "to":"ui", "op":"camera.move", "ok":true, "error":""}   (only when the message names a "from")
//
// ops:
//   scene.create    path, use_mesh (false), lights, cameras -- loads a glTF as entity 0 ("" = empty scene; a file that won't load leaves an
//                                                    empty scene); lights / cameras: import the file's (default SCENE_IMPORT_LIGHTS / _CAMERAS)
//   scene.pipeline  name                          -- what the static runs draw with: a manifest pipeline; "" -> the "static" role
//   scene.view_size windows, width, height        -- the windows CreateFrame fills and their size (the cameras' aspect)
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
//   field.compile   source, topology "surface" | "particles", dims [x, y, z], x / y / z [min, max], value [min, max],
//                   shading "lit" | "colormap" | "contours", colormap "viridis" | "turbo" | "grey", contour {spacing, width}, params [8],
//                   canvas (size), pen {target "canvas" | "points", trace "time" | "sweep", speed, sweep, decay, wrap "fade" | "clear", steps,
//                   pens [{color [r,g,b,a], width}]} -- width in pixels on the canvas, in grid cells as point ink
//                   -- a failed compile answers with the compiler's messages in "error" (lines counted from the program's first line)
//   field.params    params [8]                     -- live values, no recompile
//   field.clear                                    -- blank the canvas
//   field.reset                                    -- the program's first frame again (f.first), blank canvas, pens at t = 0
//   field.show      value true | false
//   scene.project   path                          -- the project folder -- fields.json and Models/<name>.glb live here
//   scene.startup   name                          -- the stored scene the UI opens at start (iridium.json startup.scene); the dump carries it
//                                                    until the UI's first scene.create
//   scene.document  id, name                      -- which stored scene this is (the UI's, after a load or a save) -- kept here so a page
//                                                    reload knows; the dump carries it
//   scene.settings  settings {...}                -- the scene's settings: every key given replaces that key whole; the rest stay. Any keys
//                                                    at all (the UI owns them; they're stored with the scene) -- the ones the renderer acts on
//                                                    are applied as they arrive: background [r, g, b] (sRGB 0..1), grid (bool). Cleared by
//                                                    scene.create. -- data { settings } (no settings given -> just reads them)
//   scene.dump                                    -- data { project, startup, document {id, name}, settings, entities: [{id, name,
//                                                    kind, parent, order, visible, t, r, s, + model {model, clips: [{clip, name, duration}],
//                                                    playing: [{clip, name, time, speed, loop}], nodes: [as node.list]} | light {light: {...}}
//                                                    | camera {camera: {...}}}] } -- the whole scene graph, parents first, in one reply
//   field.list                                    -- data { names: [...] } from <project>/fields.json (none yet -> [])
//   field.load      name                          -- data { name, field } -- the panel's form, as saved
//   field.save      name, field                   -- adds or replaces that entry; the file is rewritten (temp file, then rename)
//   field.delete    name
//
// A handler that returns something leaves it in m_data -- the reply's "data" (an Ir77Json object; {} otherwise). Every reply is built as an
// Ir77Json (Ir77RT/runtime/Ir77Json.hpp), never glued together as text.
// Ops live in a table (op -> handler), filled once by RegisterPipeOperations -- one lookup per message however many there are. A new op is one
// line in the group it belongs to; a new group is one more Define*Operations.
// =============================================================================================================================================
class Ir77Server : public Ir77Enlisted, public std::enable_shared_from_this<Ir77Server> {
   public:
    Ir77Server() {
        m_animate = std::static_pointer_cast<IIr77Animate>(std::make_shared<Ir77Animate>());

        // Animate's peers exist from its construction -- the ops go straight to them; an entity's Pose comes per op (GetPose)
        m_animate->GetView(m_view);
        m_animate->GetField(m_field);

        m_graph.SetAnimate(m_animate);

        if (m_empty_parser.parse(std::string_view{"{}"}).get(m_empty_args) != simdjson::SUCCESS)  // what a handler sees when a message has no "args"
            throw std::runtime_error{"Ir77Server: empty args"};

        RegisterPipeOperations();
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
        seat_shared_uuid<&GUIDIIr77Scene>(uid);

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
    // PeregrineV's server -- the scene's only way into the renderer. A pointer, so direct; before scene.pipeline / scene.create.
    std::shared_ptr<IIr77Return const> SetRenderer(std::shared_ptr<NSIr77PeregrineV::Ir77Server> const& renderer) {
        return m_animate->SetRenderer(renderer);
    }

    std::shared_ptr<IIr77Return const> Update(float const& dt) { return m_animate->Update(dt); }

    std::shared_ptr<IIr77Return const> CreateFrame(float const& seconds) { return m_animate->CreateFrame(seconds); }

    std::shared_ptr<IIr77Return const> WriteFrameStats(Ir77FrameStats& stats) const { return m_animate->WriteFrameStats(stats); }

    std::shared_ptr<IIr77Return const> GetSceneStats(Ir77SceneStats& stats) const { return m_animate->GetSceneStats(stats); }

   private:
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
        DefineAnimationOperations();
        DefineFieldOperations();
        DefineFieldStoreOperations();
    }

    // setup -- iridium sends these at startup; the UI can send scene.create to open another file
    void DefineSceneOperations() {
        m_operations["scene.create"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::string const model{Text(a, "model")};
            std::string const path = model.empty() ? std::string{Text(a, "path")} : ModelPath(model);

            m_entity_models.clear();  // the ids start over
            m_graph.Clear();          // Animate's entities and lights go with the scene
            m_startup_scene.clear();  // the UI has opened something -- a page reload mustn't open the startup scene over it
            m_settings = Ir77Json::Object();  // a new scene's settings come with it (scene.settings after the load)

            std::shared_ptr<IIr77Return const> result{};
            if (Failed(m_animate->CreateScene(path, Flag(a, "use_mesh", false), Flag(a, "lights", SCENE_IMPORT_LIGHTS), Flag(a, "cameras", SCENE_IMPORT_CAMERAS)),
                       result))
                return result;

            if (!model.empty()) m_entity_models[0] = model;

            return result;
        };

        m_operations["scene.view_size"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            float const windows = Number(a, "windows", 1.0f);
            float const width = Number(a, "width", 0.0f);
            float const height = Number(a, "height", 0.0f);

            if (windows < 1.0f || width < 1.0f || height < 1.0f)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: scene.view_size needs windows, width and height of at least 1.");

            return m_animate->SetViewSize(static_cast<std::size_t>(windows), static_cast<int>(width), static_cast<int>(height));
        };

        // the background -- color [r, g, b] sRGB 0..1 sets it; no color just reads it. Replies {"color":[r, g, b]} either way.
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
    }

    // The scene graph (types/Ir77Graph.hpp) -- every entity by its id: collections, models, lights, cameras; parented, ordered, shown or hidden
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
            if (!m_graph.Add(entity, path, Flag(a, "use_mesh", false), id, error)) return Fail(error);

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
            if (!m_graph.Remove(std::string{Text(a, "id")}, error)) return Fail(error);

            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        // parent ('' = the scene root), order (0 = after its new siblings)
        m_operations["entity.move"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::string error{};
            if (!m_graph.Move(std::string{Text(a, "id")}, std::string{Text(a, "parent")}, static_cast<std::uint32_t>(std::max(Number(a, "order", 0.0f), 0.0f)),
                              error))
                return Fail(error);

            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        // any of: name, t, r, s, visible, order, light {...}, camera {...} -- what's left out keeps its value
        m_operations["entity.set"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            Ir77GraphEntity entity{};
            if (!m_graph.Get(std::string{Text(a, "id")}, entity)) return Fail("no entity " + std::string{Text(a, "id")});

            if (!EntityCore(a, entity, false)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: " + m_error);

            std::shared_ptr<IIr77Return const> result{};
            simdjson::dom::object light{};
            if (a["light"].get(light) == simdjson::SUCCESS && Failed(LightFields(light, entity.light), result)) return result;

            simdjson::dom::object camera{};
            if (a["camera"].get(camera) == simdjson::SUCCESS) CameraFields(camera, entity.camera);

            std::string error{};
            if (!m_graph.Set(entity, error)) return Fail(error);

            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        // -> { entity } -- one, as in the dump
        m_operations["entity.get"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            Ir77GraphEntity entity{};
            if (!m_graph.Get(std::string{Text(a, "id")}, entity)) return Fail("no entity " + std::string{Text(a, "id")});

            m_data["entity"] = EntityJson(entity);

            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        // -> { entities: [...] } -- the whole graph, parents first
        m_operations["entity.list"] = [this](Ir77SceneArgs const&) -> std::shared_ptr<IIr77Return const> {
            m_data["entities"] = EntitiesJson();

            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        // a model's own nodes -- its file's structure (bones included), not graph entities -> { nodes: [{node, name, parent, t, r, s}] }
        m_operations["node.list"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::shared_ptr<IIr77Pose> pose{};
            if (!ModelPose(a, pose)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: " + m_error);

            m_data["nodes"] = NodesJson(pose);

            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        // one node's rest TRS inside a model -- a clip that drives it still wins on what it animates
        m_operations["node.transform"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::shared_ptr<IIr77Return const> result{};
            std::shared_ptr<IIr77Pose> pose{};
            if (!ModelPose(a, pose)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: " + m_error);

            double node_number{-1.0};
            if (a["node"].get(node_number) != simdjson::SUCCESS || node_number < 0.0) return Fail("node.transform needs node");

            std::uint32_t const node = static_cast<std::uint32_t>(node_number);

            std::string name{};
            std::uint32_t parent{UINT32_MAX};
            glm::vec3 translation{0.0f};
            glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
            glm::vec3 scale{1.0f};
            if (Failed(pose->GetNode(node, name, parent, translation, rotation, scale), result)) return result;

            Placement(a, translation, rotation, scale);

            return pose->SetNodeTransform(node, translation, rotation, scale);
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

    // a model entity's Pose, by the op's id
    bool ModelPose(Ir77SceneArgs const& a, std::shared_ptr<IIr77Pose>& pose) {
        Ir77GraphEntity entity{};
        if (!m_graph.Get(std::string{Text(a, "id")}, entity)) {
            m_error = "no entity " + std::string{Text(a, "id")};
            return false;
        }

        if (entity.kind != Ir77GraphKind::Model || entity.runtime == GRAPH_NO_RUNTIME ||
            m_animate->GetPose(entity.runtime, pose)->ID() != GUIDIr77OperationSucceeded) {
            m_error = "entity " + entity.id + " isn't a loaded model";
            return false;
        }

        return true;
    }

    std::shared_ptr<IIr77Return const> Fail(std::string const& message) {
        m_error = message;
        return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: " + message);
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

    std::string LightTypeName(int const type) const { return type == LIGHT_DIRECTIONAL ? "directional" : type == LIGHT_SPOT ? "spot" : "point"; }

    // One model's nodes -- node.list and scene.dump
    Ir77Json NodesJson(std::shared_ptr<IIr77Pose> const& pose) const {
        Ir77Json nodes = Ir77Json::Array();

        std::uint32_t count{0};
        pose->GetNodeCount(count);

        for (std::uint32_t n = 0; n < count; n++) {
            std::string name{};
            std::uint32_t parent{UINT32_MAX};
            glm::vec3 translation{0.0f};
            glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
            glm::vec3 scale{1.0f};
            pose->GetNode(n, name, parent, translation, rotation, scale);

            Ir77Json& node = nodes.Push(Ir77Json::Object());
            node["node"] = n;
            node["name"] = name;
            node["parent"] = parent == UINT32_MAX ? std::int64_t{-1} : std::int64_t{parent};
            Placement(node, translation, rotation, scale);
        }

        return nodes;
    }

    // One model's clips (what its file has) and players (what's running)
    void ClipsJson(std::shared_ptr<IIr77Pose> const& pose, Ir77Json& clips, Ir77Json& playing) const {
        clips = Ir77Json::Array();
        playing = Ir77Json::Array();

        std::shared_ptr<IIr77Animation> animations{};
        std::uint32_t clip_count{0};
        if (pose->GetAnimations(animations)->ID() == GUIDIr77OperationSucceeded && animations) animations->GetClipCount(clip_count);

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

        std::uint32_t players{0};
        pose->GetPlayerCount(players);

        for (std::uint32_t p = 0; p < players; p++) {
            std::uint32_t clip{UINT32_MAX};
            float time{0.0f};
            float duration{0.0f};
            float speed{1.0f};
            bool loop{true};
            if (pose->GetPlayer(p, clip, time, duration, speed, loop)->ID() != GUIDIr77OperationSucceeded) continue;

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
        dump["entities"] = EntitiesJson();

        return dump;
    }

    // the graph, parents first
    Ir77Json EntitiesJson() const {
        Ir77Json entities = Ir77Json::Array();
        for (Ir77GraphEntity const* entity : m_graph.Ordered()) entities.Push(EntityJson(*entity));

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

            std::shared_ptr<IIr77Pose> pose{};
            if (e.runtime != GRAPH_NO_RUNTIME && m_animate->GetPose(e.runtime, pose)->ID() == GUIDIr77OperationSucceeded && pose) {
                nodes = NodesJson(pose);
                ClipsJson(pose, clips, playing);
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
            m_animate->GetBackground(color);

            simdjson::dom::array channels{};
            if (value.get(channels) == simdjson::SUCCESS) {
                std::size_t c{0};
                for (auto const channel : channels) {
                    double number{0.0};
                    if (c < 3 && channel.get(number) == simdjson::SUCCESS) color[static_cast<glm::length_t>(c)] = static_cast<float>(number);
                    c++;
                }
            }

            return m_animate->SetBackground(color);
        }

        if (key == "grid") {
            bool show{true};
            if (value.get(show) != simdjson::SUCCESS) return Ir77RETURN<Ir77OperationSucceeded>();

            return m_animate->SetGridVisible(show);
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

    // A .glb from anywhere -> a model of the open project, flattened (runtime/Ir77Flatten.hpp) into <project>/Models/<model>.glb -- one file,
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
            std::shared_ptr<IIr77Pose> pose{};
            if (!ModelPose(a, pose)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: " + m_error);

            return pose->PlayAnimation(std::string{Text(a, "name")}, Flag(a, "loop", true), Number(a, "speed", 1.0f));
        };
        m_operations["animation.stop"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::shared_ptr<IIr77Pose> pose{};
            if (!ModelPose(a, pose)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: " + m_error);

            return pose->StopAnimation(std::string{Text(a, "name")});
        };
        m_operations["animation.stop_all"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::shared_ptr<IIr77Pose> pose{};
            if (!ModelPose(a, pose)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: " + m_error);

            return pose->StopAllAnimations();
        };
    }

    void DefineFieldOperations() {
        m_operations["field.compile"] = [this](Ir77SceneArgs const& a) {
            Ir77FieldSettings settings{};

            settings.source = std::string{Text(a, "source")};
            settings.topology = Text(a, "topology") == "particles" ? 1u : 0u;

            float dims[3]{128.0f, 128.0f, 1.0f};
            Numbers(a, "dims", dims, 3);
            settings.dims = glm::uvec3(static_cast<std::uint32_t>(std::max(dims[0], 1.0f)), static_cast<std::uint32_t>(std::max(dims[1], 1.0f)),
                                       static_cast<std::uint32_t>(std::max(dims[2], 1.0f)));

            Numbers(a, "x", &settings.x.x, 2);
            Numbers(a, "y", &settings.y.x, 2);
            Numbers(a, "z", &settings.z.x, 2);
            Numbers(a, "value", &settings.value.x, 2);

            std::string_view const shading = Text(a, "shading");
            settings.shading = shading == "contours" ? 2u : shading == "colormap" ? 1u : 0u;

            std::string_view const colormap = Text(a, "colormap");
            settings.colormap = colormap == "turbo" ? 1u : colormap == "grey" ? 2u : 0u;

            simdjson::dom::object contour{};
            if (a["contour"].get(contour) == simdjson::SUCCESS) {
                settings.contour_spacing = Number(contour, "spacing", settings.contour_spacing);
                settings.contour_width = Number(contour, "width", settings.contour_width);
            }

            Numbers(a, "params", settings.params.data(), settings.params.size());

            settings.canvas_size = static_cast<std::uint32_t>(std::max(Number(a, "canvas", 2048.0f), 16.0f));

            simdjson::dom::object pen{};
            if (a["pen"].get(pen) == simdjson::SUCCESS) {
                bool const sweep = Text(pen, "trace") == "sweep";

                settings.pen_target = Text(pen, "target") == "points" ? 1u : 0u;

                settings.pen_speed = Number(pen, "speed", settings.pen_speed);
                settings.sweep = sweep ? std::max(Number(pen, "sweep", 1.0f), 1e-4f) : 0.0f;
                settings.decay = std::max(Number(pen, "decay", 0.0f), 0.0f);
                settings.wrap_clear = Text(pen, "wrap") == "clear";
                settings.steps = static_cast<std::uint32_t>(std::clamp(Number(pen, "steps", 256.0f), 1.0f, 65536.0f));

                simdjson::dom::array pens{};
                if (pen["pens"].get(pens) == simdjson::SUCCESS) {
                    std::uint32_t count{0};

                    for (auto const element : pens) {
                        if (count >= settings.pen_color.size()) break;

                        simdjson::dom::object entry{};
                        if (element.get(entry) != simdjson::SUCCESS) continue;

                        float color[4]{1.0f, 1.0f, 1.0f, 1.0f};
                        Numbers(entry, "color", color, 4);

                        settings.pen_color[count] = glm::vec4(color[0], color[1], color[2], color[3]);
                        settings.pen_width[count] = std::max(Number(entry, "width", 2.0f), 0.5f);
                        count++;
                    }

                    settings.pens = std::max(count, 1u);
                }
            }

            std::string errors{};
            auto const result = m_field->CompileField(settings, errors);

            m_error = errors;

            return result;
        };

        m_operations["field.params"] = [this](Ir77SceneArgs const& a) {
            std::array<float, 8> params{};
            Numbers(a, "params", params.data(), params.size());

            return m_field->SetFieldParams(params);
        };

        m_operations["field.clear"] = [this](Ir77SceneArgs const&) { return m_field->ClearFieldCanvas(); };
        m_operations["field.reset"] = [this](Ir77SceneArgs const&) { return m_field->ResetField(); };
        m_operations["field.show"] = [this](Ir77SceneArgs const& a) { return m_field->ShowField(Flag(a, "value", true)); };
    }

    // A project's saved fields -- <project>/fields.json, one object keyed by name. Each value is the panel's form as it sent it; this
    // server never looks inside one. Read fresh on every op, so a hand edit to the file shows up.
    void DefineFieldStoreOperations() {
        m_operations["scene.project"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            m_project = std::string{Text(a, "path")};

            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        m_operations["field.list"] = [this](Ir77SceneArgs const&) -> std::shared_ptr<IIr77Return const> {
            std::shared_ptr<IIr77Return const> result{};

            std::map<std::string, std::string> entries{};
            if (Failed(DefineFieldsRead(entries), result)) return result;

            Ir77Json& names = m_data["names"] = Ir77Json::Array();
            for (auto const& [name, entry] : entries) names.Push(name);

            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        m_operations["field.load"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::shared_ptr<IIr77Return const> result{};

            std::string const name{Text(a, "name")};

            std::map<std::string, std::string> entries{};
            if (Failed(DefineFieldsRead(entries), result)) return result;

            auto const found = entries.find(name);
            if (found == entries.end()) {
                m_error = "no field '" + name + "'";
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: " + m_error);
            }

            m_data["name"] = name;
            m_data["field"] = Ir77Json::Raw(found->second);  // stored as the panel sent it

            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        m_operations["field.save"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::shared_ptr<IIr77Return const> result{};

            std::string const name{Text(a, "name")};
            if (name.empty()) {
                m_error = "a field needs a name";
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: " + m_error);
            }

            simdjson::dom::element field{};
            if (a["field"].get(field) != simdjson::SUCCESS || !field.is_object()) {
                m_error = "field.save needs \"field\" as an object";
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: " + m_error);
            }

            std::string const entry = simdjson::minify(field);  // copied before the file is parsed

            std::map<std::string, std::string> entries{};
            if (Failed(DefineFieldsRead(entries), result)) return result;

            entries[name] = entry;

            return DefineFieldsWrite(entries);
        };

        m_operations["field.delete"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::shared_ptr<IIr77Return const> result{};

            std::string const name{Text(a, "name")};

            std::map<std::string, std::string> entries{};
            if (Failed(DefineFieldsRead(entries), result)) return result;

            if (entries.erase(name) == 0) {
                m_error = "no field '" + name + "'";
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: " + m_error);
            }

            return DefineFieldsWrite(entries);
        };
    }

    // <project>/fields.json -> name -> entry (minified JSON). No file yet is an empty list, not an error.
    std::shared_ptr<IIr77Return const> DefineFieldsRead(std::map<std::string, std::string>& entries) {
        entries.clear();

        if (m_project.empty()) {
            m_error = "no project open";
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: " + m_error);
        }

        std::filesystem::path const file = std::filesystem::path{m_project} / "fields.json";

        std::error_code error{};
        if (!std::filesystem::exists(file, error)) return Ir77RETURN<Ir77OperationSucceeded>();

        // its own parser -- m_parser still holds the message being handled, and its args point into it
        simdjson::dom::element root{};
        if (m_fields_parser.load(file.string()).get(root) != simdjson::SUCCESS) {
            m_error = file.string() + " is not valid JSON";
            return Ir77RETURN<Ir77OperationFailed>(this, "Ir77Server: " + m_error);
        }

        simdjson::dom::object fields{};
        if (root.get(fields) != simdjson::SUCCESS) {
            m_error = file.string() + " must be one object keyed by field name";
            return Ir77RETURN<Ir77OperationFailed>(this, "Ir77Server: " + m_error);
        }

        for (auto const field : fields) entries[std::string{field.key}] = simdjson::minify(field.value);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // name -> entry back to <project>/fields.json, one entry per line. Written to fields.json.tmp, then renamed over the old file -- a
    // crash mid-write leaves the old file whole.
    std::shared_ptr<IIr77Return const> DefineFieldsWrite(std::map<std::string, std::string> const& entries) {
        std::filesystem::path const file = std::filesystem::path{m_project} / "fields.json";
        std::filesystem::path temp = file;
        temp += ".tmp";

        std::error_code error{};
        std::filesystem::create_directories(m_project, error);

        {
            std::ofstream out{temp, std::ios::trunc};
            if (!out) {
                m_error = "cannot write " + temp.string();
                return Ir77RETURN<Ir77OperationFailed>(this, "Ir77Server: " + m_error);
            }

            out << "{\n";

            std::size_t written{0};
            for (auto const& [name, entry] : entries) {
                written++;
                out << "  " << Ir77Json::Quote(name) << ": " << entry << (written < entries.size() ? ",\n" : "\n");
            }

            out << "}\n";
            out.flush();

            if (!out) {
                m_error = "cannot write " + temp.string();
                return Ir77RETURN<Ir77OperationFailed>(this, "Ir77Server: " + m_error);
            }
        }

        std::filesystem::rename(temp, file, error);
        if (error) {
            m_error = "cannot replace " + file.string() + ": " + error.message();
            return Ir77RETURN<Ir77OperationFailed>(this, "Ir77Server: " + m_error);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // args readers -- a missing or mistyped key gives the fallback
    // "entity" -- 0 (the scene file) when left out
    std::uint32_t Entity(Ir77SceneArgs const& args) const { return static_cast<std::uint32_t>(std::max(Number(args, "entity", 0.0f), 0.0f)); }

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
    std::shared_ptr<IIr77Animate> m_animate{};  // the module's work -- only this server holds it

    std::shared_ptr<IIr77View> m_view{};  // Animate's -- camera.* ops

    std::shared_ptr<IIr77Field> m_field{};  // Animate's -- field.* ops

    // Ir77PVLight::type -- as Ir77Light writes them from the glTF
    static constexpr int LIGHT_DIRECTIONAL{0};

    static constexpr int LIGHT_POINT{1};

    static constexpr int LIGHT_SPOT{2};

    std::function<void(std::string)> m_post{};

    std::string m_error{};  // a handler's detail for the reply's "error" -- cleared before every message

    Ir77Json m_data{Ir77Json::Object()};  // a handler's result for the reply's "data" -- reset to {} before every message

    Ir77Json m_settings{Ir77Json::Object()};  // the scene's settings (scene.settings) -- any keys; the dump carries them

    std::string m_project{};  // scene.project -- the folder fields.json and models/ live in

    Ir77Graph m_graph{};  // the scene graph -- every entity by its id; it drives Animate

    std::vector<std::string> m_import_report{};  // what the last import's flatten left out -- entity.add's reply carries it

    std::map<std::uint32_t, std::string> m_entity_models{};  // entity -> the project model it was loaded as (scene.create / entity.import model)

    std::string m_startup_scene{};  // scene.startup -- until the UI's first scene.create

    std::string m_document_id{};  // scene.document -- the stored scene this is; '' = not stored

    std::string m_document_name{};

    simdjson::dom::parser m_fields_parser{};  // fields.json -- separate from m_parser, which holds the message being handled

    simdjson::dom::parser m_parser{};

    simdjson::dom::parser m_empty_parser{};  // owns m_empty_args

    Ir77SceneArgs m_empty_args{};

    std::unordered_map<std::string, Ir77SceneOperation, Ir77SceneOpHash, std::equal_to<>> m_operations{};
};

}  // namespace NSIr77Scene