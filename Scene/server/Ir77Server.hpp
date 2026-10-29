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

#include "../../Ir77RT/runtime/Ir77Return.hpp"

#include "../dictionary/IDIIr77Scene.hpp"

#include "../interface/IIr77Pose.hpp"
#include "../interface/IIr77View.hpp"
#include "../interface/IIr77Field.hpp"
#include "../interface/IIr77Animate.hpp"

#include "../runtime/Ir77Animate.hpp"

#include "../types/Ir77Field.hpp"

#include "../../PeregrineV/server/Ir77Server.hpp"

#include "../types/Ir77Types.hpp"

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
//   entity.import   path | model, use_mesh (false), lights -- one more model in the scene at the origin; lights: its lights become world
//                                                    lights (default SCENE_IMPORT_LIGHTS) -- data { entity, model }. model: the project's,
//                                                    <project>/Models/<model>.glb. path: a .glb from anywhere, copied into Models/ first (a
//                                                    project open) -- either way the dump names it, so it saves
//   entity.remove   entity
//   entity.transform entity, t [x,y,z], r [x,y,z,w], s [x,y,z] -- the whole entity in the world; a part left out keeps what it was
//   entity.list                                   -- data { entities: [{entity, path, t, r, s}] }
//   node.list       entity (0)                    -- data { nodes: [{node, name, parent (-1 = root), t, r, s}] } -- rest TRS, parent's space
//   node.transform  entity (0), node, t, r, s     -- one node's rest TRS; a clip that drives it still wins on what it animates
//   light.add       type "directional" | "point" | "spot" (point), color [r,g,b] (1), intensity (683), range (0 = none), inner (0),
//                   outer (pi/4), t, r  -- a world light; intensity in glTF units (lux / candela; 683 = 1 engine unit), cones in radians,
//                   it shines down its local -Z -- data { light }
//   light.set       light, then any of light.add's -- what's left out keeps its value
//   light.remove    light
//   light.list                                    -- data { lights: [{light, type, color, intensity, range, inner, outer, t, r, entity (-1 = placed
//                                                    by the UI)}] }
//   animation.play  entity (0), name, loop (true), speed (1)
//   animation.stop  entity (0), name
//   animation.stop_all entity (0)
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
//   scene.dump                                    -- data { project, startup, document {id, name}, background, grid,
//                                                    entities: [{entity, model ("" = imported by path), path,
//                                                    t, r, s, clips: [{clip, name, duration}], playing: [{clip, name, time, speed, loop}],
//                                                    nodes: [as node.list]}], lights: [as light.list] } -- the whole scene graph in one reply
//   field.list                                    -- data { names: [...] } from <project>/fields.json (none yet -> [])
//   field.load      name                          -- data { name, field } -- the panel's form, as saved
//   field.save      name, field                   -- adds or replaces that entry; the file is rewritten (temp file, then rename)
//   field.delete    name
//
// A handler that returns something leaves it in m_data -- the reply's "data" (an object; {} otherwise).
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
        m_data = "{}";

        bool const ok = known && handler->second(args)->ID() == GUIDIr77OperationSucceeded;

        // a handler that has more to say (compile errors) leaves it in m_error
        std::string const error = ok ? "" : !known ? "unknown op" : m_error.empty() ? "operation failed" : m_error;

        if (!ok) Ir77RETURN<Ir77OperationFailed>(this, "[WARN] scene: " + error + " : " + json);

        if (from.empty() || !m_post) return;  // nobody to answer

        m_post("{\"id\":" + std::to_string(id) + ",\"from\":\"scene\",\"to\":\"" + Escape(from) + "\",\"op\":\"" + Escape(op) +
               "\",\"ok\":" + (ok ? "true" : "false") + ",\"error\":\"" + Escape(error) + "\",\"data\":" + (ok ? m_data : std::string{"{}"}) + "}");
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // direct link -- setup (Scene -> PeregrineV) and the frame loop
    // -------------------------------------------------------------------------------------------------------------------------------------
    // PeregrineV's server -- the scene's only way into the renderer. A pointer, so direct; before scene.pipeline / scene.create.
    std::shared_ptr<IIr77Return const> SetRenderer(std::shared_ptr<NSIr77PeregrineV::Ir77Server> const& renderer) {
        m_renderer = renderer;

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
        DefineLightOperations();
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
            m_startup_scene.clear();  // the UI has opened something -- a page reload mustn't open the startup scene over it

            std::shared_ptr<IIr77Return const> result{};
            if (Failed(m_animate->CreateScene(path, Flag(a, "use_mesh", false), Flag(a, "lights", SCENE_IMPORT_LIGHTS), Flag(a, "cameras", SCENE_IMPORT_CAMERAS)),
                       result))
                return result;

            if (!model.empty()) m_entity_models[0] = model;

            return result;
        };

        m_operations["scene.pipeline"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::string_view const name = Text(a, "name");
            if (name.empty()) return m_animate->SetPipeline(IR77PV_NO_PIPELINE);  // the "static" role, resolved by CreateScene

            if (!m_renderer) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: scene.pipeline before SetRenderer.");

            std::shared_ptr<IIr77Return const> result{};
            Ir77PVPipelineHandle pipeline{IR77PV_NO_PIPELINE};

            if (Failed(m_renderer->FindPipeline(std::string{name}, pipeline), result)) {
                m_error = "no pipeline '" + std::string{name} + "' in the manifest";
                return result;
            }

            return m_animate->SetPipeline(pipeline);
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
        m_operations["scene.background"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::shared_ptr<IIr77Return const> result{};

            glm::vec3 color{0.0f};
            m_animate->GetBackground(color);

            Numbers(a, "color", &color.x, 3);
            if (Failed(m_animate->SetBackground(color), result)) return result;
            m_animate->GetBackground(color);

            char text[96]{};
            std::snprintf(text, sizeof(text), "{\"color\":[%.6g,%.6g,%.6g]}", static_cast<double>(color.r), static_cast<double>(color.g),
                          static_cast<double>(color.b));
            m_data = text;

            return result;
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
            m_data = DumpJson();

            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        // the editor grid -- show true / false sets it; no show just reads it. Replies {"show":bool} either way.
        m_operations["scene.grid"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::shared_ptr<IIr77Return const> result{};

            bool visible{true};
            m_animate->GetGridVisible(visible);

            if (Failed(m_animate->SetGridVisible(Flag(a, "show", visible)), result)) return result;
            m_animate->GetGridVisible(visible);

            m_data = std::string{"{\"show\":"} + (visible ? "true" : "false") + "}";

            return result;
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

    // Entities -- files in the world, each placed as a whole -- and the nodes inside one
    void DefineEntityOperations() {
        m_operations["entity.import"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::shared_ptr<IIr77Return const> result{};

            std::string model{Text(a, "model")};
            std::string path{Text(a, "path")};

            // a file from anywhere -> into the project's Models/ first, so it's a project model and the scene graph can save it
            if (model.empty() && !path.empty() && !m_project.empty())
                if (!AdoptModel(path, model)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: " + m_error);

            if (!model.empty()) path = ModelPath(model);

            std::uint32_t entity{0};
            if (Failed(m_animate->ImportEntity(path, Flag(a, "use_mesh", false), Flag(a, "lights", SCENE_IMPORT_LIGHTS), entity), result)) return result;

            if (!model.empty()) m_entity_models[entity] = model;

            m_data = "{\"entity\":" + std::to_string(entity) + ",\"model\":\"" + Escape(model) + "\"}";

            return result;
        };

        m_operations["entity.remove"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::shared_ptr<IIr77Return const> result{};
            if (Failed(m_animate->RemoveEntity(Entity(a)), result)) return result;

            m_entity_models.erase(Entity(a));

            return result;
        };

        m_operations["entity.transform"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::vector<std::uint32_t> ids{};
            std::vector<std::string> paths{};
            std::vector<glm::vec3> translations{};
            std::vector<glm::quat> rotations{};
            std::vector<glm::vec3> scales{};
            m_animate->GetEntities(ids, paths, translations, rotations, scales);

            std::uint32_t const entity = Entity(a);

            auto const found = std::find(ids.begin(), ids.end(), entity);
            if (found == ids.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: no entity " + std::to_string(entity) + ".");

            std::size_t const at = static_cast<std::size_t>(found - ids.begin());

            glm::vec3 translation = translations[at];
            glm::quat rotation = rotations[at];
            glm::vec3 scale = scales[at];
            Placement(a, translation, rotation, scale);

            return m_animate->SetEntityTransform(entity, translation, rotation, scale);
        };

        m_operations["entity.list"] = [this](Ir77SceneArgs const&) -> std::shared_ptr<IIr77Return const> {
            std::vector<std::uint32_t> ids{};
            std::vector<std::string> paths{};
            std::vector<glm::vec3> translations{};
            std::vector<glm::quat> rotations{};
            std::vector<glm::vec3> scales{};
            m_animate->GetEntities(ids, paths, translations, rotations, scales);

            std::string entities{};
            for (std::size_t i = 0; i < ids.size(); i++) {
                if (!entities.empty()) entities += ",";
                entities += "{\"entity\":" + std::to_string(ids[i]) + ",\"path\":\"" + Escape(paths[i]) + "\"," +
                            PlacementJson(translations[i], rotations[i], scales[i]) + "}";
            }

            m_data = "{\"entities\":[" + entities + "]}";

            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        m_operations["node.list"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::shared_ptr<IIr77Return const> result{};
            std::shared_ptr<IIr77Pose> pose{};
            if (Failed(m_animate->GetPose(Entity(a), pose), result)) return result;

            std::string const nodes = NodesJson(pose);

            m_data = "{\"nodes\":[" + nodes + "]}";

            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        m_operations["node.transform"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::shared_ptr<IIr77Return const> result{};
            std::shared_ptr<IIr77Pose> pose{};
            if (Failed(m_animate->GetPose(Entity(a), pose), result)) return result;

            double node_number{-1.0};
            if (a["node"].get(node_number) != simdjson::SUCCESS || node_number < 0.0)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: node.transform needs node.");

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

    // World lights -- the scene graph's, placed by the UI or imported with a file
    void DefineLightOperations() {
        m_operations["light.add"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::shared_ptr<IIr77Return const> result{};

            Ir77PVLight light{};
            light.type = LIGHT_POINT;
            light.color = glm::vec3(1.0f);
            light.intensity = 683.0f;
            light.outerConeAngle = glm::quarter_pi<float>();

            glm::vec3 translation{0.0f};
            glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
            glm::vec3 scale{1.0f};

            if (Failed(LightFields(a, light), result)) return result;
            Placement(a, translation, rotation, scale);

            std::uint32_t id{0};
            if (Failed(m_animate->AddLight(light, translation, rotation, id), result)) return result;

            m_data = "{\"light\":" + std::to_string(id) + "}";

            return result;
        };

        m_operations["light.set"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::shared_ptr<IIr77Return const> result{};

            double number{-1.0};
            if (a["light"].get(number) != simdjson::SUCCESS || number < 0.0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: light.set needs light.");

            std::uint32_t const id = static_cast<std::uint32_t>(number);

            Ir77PVLight light{};
            glm::vec3 translation{0.0f};
            glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
            glm::vec3 scale{1.0f};
            std::uint32_t entity{UINT32_MAX};
            if (Failed(m_animate->GetLight(id, light, translation, rotation, entity), result)) return result;

            if (Failed(LightFields(a, light), result)) return result;
            Placement(a, translation, rotation, scale);

            return m_animate->SetLight(id, light, translation, rotation);
        };

        m_operations["light.remove"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            double number{-1.0};
            if (a["light"].get(number) != simdjson::SUCCESS || number < 0.0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: light.remove needs light.");

            return m_animate->RemoveLight(static_cast<std::uint32_t>(number));
        };

        m_operations["light.list"] = [this](Ir77SceneArgs const&) -> std::shared_ptr<IIr77Return const> {
            std::string const lights = LightsJson();

            m_data = "{\"lights\":[" + lights + "]}";

            return Ir77RETURN<Ir77OperationSucceeded>();
        };
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

    // One entity's nodes, comma-separated -- node.list and scene.dump
    std::string NodesJson(std::shared_ptr<IIr77Pose> const& pose) const {
        std::uint32_t count{0};
        pose->GetNodeCount(count);

        std::string nodes{};
        for (std::uint32_t n = 0; n < count; n++) {
            std::string name{};
            std::uint32_t parent{UINT32_MAX};
            glm::vec3 translation{0.0f};
            glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
            glm::vec3 scale{1.0f};
            pose->GetNode(n, name, parent, translation, rotation, scale);

            if (!nodes.empty()) nodes += ",";
            nodes += "{\"node\":" + std::to_string(n) + ",\"name\":\"" + Escape(name) + "\",\"parent\":" +
                     (parent == UINT32_MAX ? std::string{"-1"} : std::to_string(parent)) + "," + PlacementJson(translation, rotation, scale) + "}";
        }

        return nodes;
    }

    // Every world light, comma-separated -- light.list and scene.dump
    std::string LightsJson() const {
        std::vector<std::uint32_t> ids{};
        m_animate->GetLightIds(ids);

        std::string lights{};
        for (std::uint32_t const id : ids) {
            Ir77PVLight light{};
            glm::vec3 translation{0.0f};
            glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
            std::uint32_t entity{UINT32_MAX};
            if (m_animate->GetLight(id, light, translation, rotation, entity)->ID() != GUIDIr77OperationSucceeded) continue;

            if (!lights.empty()) lights += ",";
            lights += "{\"light\":" + std::to_string(id) + ",\"type\":\"" + LightTypeName(light.type) + "\",\"color\":[" + JsonNumber(light.color.r) +
                      "," + JsonNumber(light.color.g) + "," + JsonNumber(light.color.b) + "],\"intensity\":" + JsonNumber(light.intensity) +
                      ",\"range\":" + JsonNumber(light.range) + ",\"inner\":" + JsonNumber(light.innerConeAngle) +
                      ",\"outer\":" + JsonNumber(light.outerConeAngle) + "," + PlacementJson(translation, rotation, glm::vec3(1.0f)) + ",\"entity\":" +
                      (entity == UINT32_MAX ? std::string{"-1"} : std::to_string(entity)) + "}";
        }

        return lights;
    }

    // One entity's clips (what its file has) and players (what's running), comma-separated
    void ClipsJson(std::shared_ptr<IIr77Pose> const& pose, std::string& clips, std::string& playing) const {
        clips.clear();
        playing.clear();

        std::shared_ptr<IIr77Animation> animations{};
        std::uint32_t clip_count{0};
        if (pose->GetAnimations(animations)->ID() == GUIDIr77OperationSucceeded && animations) animations->GetClipCount(clip_count);

        std::vector<std::string> names(clip_count);
        for (std::uint32_t c = 0; c < clip_count; c++) {
            float duration{0.0f};
            animations->GetClipName(c, names[c]);
            animations->GetClipDuration(c, duration);

            if (!clips.empty()) clips += ",";
            clips += "{\"clip\":" + std::to_string(c) + ",\"name\":\"" + Escape(names[c]) + "\",\"duration\":" + JsonNumber(duration) + "}";
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

            if (!playing.empty()) playing += ",";
            playing += "{\"clip\":" + std::to_string(clip) + ",\"name\":\"" + Escape(clip < names.size() ? names[clip] : std::string{}) +
                       "\",\"time\":" + JsonNumber(time) + ",\"speed\":" + JsonNumber(speed) + ",\"loop\":" + (loop ? "true" : "false") + "}";
        }
    }

    // The whole scene graph -- what the UI builds its outliner from at start, and what a save turns into a scene document
    std::string DumpJson() const {
        glm::vec3 background{0.0f};
        m_animate->GetBackground(background);

        bool grid{true};
        m_animate->GetGridVisible(grid);

        std::vector<std::uint32_t> ids{};
        std::vector<std::string> paths{};
        std::vector<glm::vec3> translations{};
        std::vector<glm::quat> rotations{};
        std::vector<glm::vec3> scales{};
        m_animate->GetEntities(ids, paths, translations, rotations, scales);

        std::string entities{};
        for (std::size_t i = 0; i < ids.size(); i++) {
            auto const named = m_entity_models.find(ids[i]);
            std::string const model = named == m_entity_models.end() ? std::string{} : named->second;

            std::string nodes{};
            std::string clips{};
            std::string playing{};

            std::shared_ptr<IIr77Pose> pose{};
            if (m_animate->GetPose(ids[i], pose)->ID() == GUIDIr77OperationSucceeded && pose) {
                nodes = NodesJson(pose);
                ClipsJson(pose, clips, playing);
            }

            if (!entities.empty()) entities += ",";
            entities += "{\"entity\":" + std::to_string(ids[i]) + ",\"model\":\"" + Escape(model) + "\",\"path\":\"" + Escape(paths[i]) + "\"," +
                        PlacementJson(translations[i], rotations[i], scales[i]) + ",\"clips\":[" + clips + "],\"playing\":[" + playing +
                        "],\"nodes\":[" + nodes + "]}";
        }

        return "{\"project\":\"" + Escape(m_project) + "\",\"startup\":\"" + Escape(m_startup_scene) + "\",\"document\":{\"id\":\"" +
               Escape(m_document_id) + "\",\"name\":\"" + Escape(m_document_name) + "\"},\"background\":[" + JsonNumber(background.r) + "," + JsonNumber(background.g) + "," +
               JsonNumber(background.b) + "],\"grid\":" + (grid ? "true" : "false") + ",\"entities\":[" + entities + "],\"lights\":[" + LightsJson() +
               "]}";
    }

    // A .glb from anywhere -> a model of the open project: copied into <project>/Models/ unless it's already there, model = its file name
    // without .glb. A different file already under that name is refused (rename one) -- nothing in Models/ is ever overwritten. A .gltf is
    // refused: its buffers and textures are separate files the copy would leave behind.
    bool AdoptModel(std::string const& source_text, std::string& model) {
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

        model = source.stem().string();

        std::filesystem::path const models = std::filesystem::path{m_project} / "Models";
        std::filesystem::path const target = models / (model + ".glb");

        if (std::filesystem::exists(target, ec)) {
            if (std::filesystem::equivalent(source, target, ec) || SameContents(source, target)) return true;  // already the project's

            m_error = "Models/ already has a different " + target.filename().string() + " -- rename the file you're importing";
            return false;
        }

        std::filesystem::create_directories(models, ec);
        if (!std::filesystem::copy_file(source, target, ec)) {
            m_error = "couldn't copy " + source.string() + " into " + models.string() + " (" + ec.message() + ")";
            return false;
        }

        return true;
    }

    // byte for byte -- sizes first
    static bool SameContents(std::filesystem::path const& a, std::filesystem::path const& b) {
        std::error_code ec{};
        if (std::filesystem::file_size(a, ec) != std::filesystem::file_size(b, ec) || ec) return false;

        std::ifstream fa{a, std::ios::binary};
        std::ifstream fb{b, std::ios::binary};

        std::array<char, 65536> ba{};
        std::array<char, 65536> bb{};

        while (fa && fb) {
            fa.read(ba.data(), ba.size());
            fb.read(bb.data(), bb.size());

            if (fa.gcount() != fb.gcount() || std::memcmp(ba.data(), bb.data(), static_cast<std::size_t>(fa.gcount())) != 0) return false;
        }

        return true;
    }

    // A project model by name -- <project>/Models/<model>.glb
    std::string ModelPath(std::string const& model) const { return (std::filesystem::path{m_project} / "Models" / (model + ".glb")).string(); }

    static std::string JsonNumber(float const value) {
        char text[32]{};
        std::snprintf(text, sizeof(text), "%.6g", static_cast<double>(value));
        return std::string{text};
    }

    void DefineAnimationOperations() {
        m_operations["animation.play"] = [this](Ir77SceneArgs const& a) {
            std::shared_ptr<IIr77Return const> result{};
            std::shared_ptr<IIr77Pose> pose{};
            if (Failed(m_animate->GetPose(Entity(a), pose), result)) return result;

            return pose->PlayAnimation(std::string{Text(a, "name")}, Flag(a, "loop", true), Number(a, "speed", 1.0f));
        };
        m_operations["animation.stop"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::shared_ptr<IIr77Return const> result{};
            std::shared_ptr<IIr77Pose> pose{};
            if (Failed(m_animate->GetPose(Entity(a), pose), result)) return result;

            return pose->StopAnimation(std::string{Text(a, "name")});
        };
        m_operations["animation.stop_all"] = [this](Ir77SceneArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::shared_ptr<IIr77Return const> result{};
            std::shared_ptr<IIr77Pose> pose{};
            if (Failed(m_animate->GetPose(Entity(a), pose), result)) return result;

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

            std::string names{};
            for (auto const& [name, entry] : entries) {
                if (!names.empty()) names += ",";
                names += "\"" + Escape(name) + "\"";
            }

            m_data = "{\"names\":[" + names + "]}";

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

            m_data = "{\"name\":\"" + Escape(name) + "\",\"field\":" + found->second + "}";

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
                out << "  \"" << Escape(name) << "\": " << entry << (written < entries.size() ? ",\n" : "\n");
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

    // {"t":[...],"r":[...],"s":[...]} -- r as x, y, z, w
    std::string PlacementJson(glm::vec3 const& translation, glm::quat const& rotation, glm::vec3 const& scale) const {
        auto const number = [](float const value) {
            char text[32]{};
            std::snprintf(text, sizeof(text), "%.6g", static_cast<double>(value));
            return std::string{text};
        };

        return "\"t\":[" + number(translation.x) + "," + number(translation.y) + "," + number(translation.z) + "],\"r\":[" + number(rotation.x) + "," +
               number(rotation.y) + "," + number(rotation.z) + "," + number(rotation.w) + "],\"s\":[" + number(scale.x) + "," + number(scale.y) + "," +
               number(scale.z) + "]";
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

    // JSON string body -- quotes, backslashes and control characters escaped (compile errors carry newlines)
    std::string Escape(std::string_view const text) const {
        std::string out{};
        out.reserve(text.size() + 8);

        for (char const c : text) {
            switch (c) {
                case '"':
                    out += "\\\"";
                    break;
                case '\\':
                    out += "\\\\";
                    break;
                case '\n':
                    out += "\\n";
                    break;
                case '\r':
                    out += "\\r";
                    break;
                case '\t':
                    out += "\\t";
                    break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20) {
                        char code[8]{};
                        std::snprintf(code, sizeof(code), "\\u%04x", static_cast<unsigned>(static_cast<unsigned char>(c)));
                        out += code;
                    } else {
                        out += c;
                    }
            }
        }

        return out;
    }

   private:
    std::shared_ptr<IIr77Animate> m_animate{};  // the module's work -- only this server holds it

    std::shared_ptr<IIr77View> m_view{};  // Animate's -- camera.* ops

    std::shared_ptr<IIr77Field> m_field{};  // Animate's -- field.* ops

    std::shared_ptr<NSIr77PeregrineV::Ir77Server> m_renderer{};  // PeregrineV's server -- scene.pipeline looks names up through it

    // Ir77PVLight::type -- as Ir77Light writes them from the glTF
    static constexpr int LIGHT_DIRECTIONAL{0};

    static constexpr int LIGHT_POINT{1};

    static constexpr int LIGHT_SPOT{2};

    std::function<void(std::string)> m_post{};

    std::string m_error{};  // a handler's detail for the reply's "error" -- cleared before every message

    std::string m_data{"{}"};  // a handler's result for the reply's "data" -- a JSON object, reset before every message

    std::string m_project{};  // scene.project -- the folder fields.json and models/ live in

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