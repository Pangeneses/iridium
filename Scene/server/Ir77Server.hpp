#pragma once

#include <algorithm>
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

using namespace NSIr77RT;

namespace NSIr77Scene {

// =============================================================================================================================================
// Ir77Server -- the scene module's switchboard. Owns Ir77Animate (the work) -- created here, held through IIr77Animate, handed to no one.
// Animate owns Pose, View, Lighting and Field; this server holds the three the ops reach (Pose, View, Field) through their interfaces. The
// module's one door:
//
//   pipe:        Sink(json) in, the post function out (SetPost). Every command arrives here as JSON and every reply leaves the same way.
//   direct link: only what can't travel as JSON -- the renderer's server (SetRenderer) and the per-frame calls (Update / CreateFrame / stats).
//                Everything else is an op, setup included.
//
// Message in:  {"id":7, "from":"ui", "to":"scene", "op":"camera.move", "args":{"right":1, "up":0, "forward":0}}
// Reply out:   {"id":7, "from":"scene", "to":"ui", "op":"camera.move", "ok":true, "error":""}   (only when the message names a "from")
//
// ops:
//   scene.create    path, use_mesh (false)        -- loads a glTF ("" = empty scene); a file that won't load leaves an empty scene
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
//   animation.play  name, loop (true), speed (1)
//   animation.stop  name
//   animation.stop_all
//   field.compile   source, topology "surface" | "particles", dims [x, y, z], x / y / z [min, max], value [min, max],
//                   shading "lit" | "colormap" | "contours", colormap "viridis" | "turbo" | "grey", contour {spacing, width}, params [8],
//                   canvas (size), pen {target "canvas" | "points", trace "time" | "sweep", speed, sweep, decay, wrap "fade" | "clear", steps,
//                   pens [{color [r,g,b,a], width}]} -- width in pixels on the canvas, in grid cells as point ink
//                   -- a failed compile answers with the compiler's messages in "error" (lines counted from the program's first line)
//   field.params    params [8]                     -- live values, no recompile
//   field.clear                                    -- blank the canvas
//   field.reset                                    -- the program's first frame again (f.first), blank canvas, pens at t = 0
//   field.show      value true | false
//   scene.project   path                          -- the project folder -- fields.json lives here
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

        // Animate's peers exist from its construction -- the ops go straight to them
        m_animate->GetPose(m_pose);
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
        DefineAnimationOperations();
        DefineFieldOperations();
        DefineFieldStoreOperations();
    }

    // setup -- iridium sends these at startup; the UI can send scene.create to open another file
    void DefineSceneOperations() {
        m_operations["scene.create"] = [this](Ir77SceneArgs const& a) {
            return m_animate->CreateScene(std::string{Text(a, "path")}, Flag(a, "use_mesh", false));
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
    }

    void DefineAnimationOperations() {
        m_operations["animation.play"] = [this](Ir77SceneArgs const& a) {
            return m_pose->PlayAnimation(std::string{Text(a, "name")}, Flag(a, "loop", true), Number(a, "speed", 1.0f));
        };
        m_operations["animation.stop"] = [this](Ir77SceneArgs const& a) { return m_pose->StopAnimation(std::string{Text(a, "name")}); };
        m_operations["animation.stop_all"] = [this](Ir77SceneArgs const&) { return m_pose->StopAllAnimations(); };
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

    std::shared_ptr<IIr77Pose> m_pose{};  // Animate's -- animation.* ops

    std::shared_ptr<IIr77View> m_view{};  // Animate's -- camera.* ops

    std::shared_ptr<IIr77Field> m_field{};  // Animate's -- field.* ops

    std::shared_ptr<NSIr77PeregrineV::Ir77Server> m_renderer{};  // PeregrineV's server -- scene.pipeline looks names up through it

    std::function<void(std::string)> m_post{};

    std::string m_error{};  // a handler's detail for the reply's "error" -- cleared before every message

    std::string m_data{"{}"};  // a handler's result for the reply's "data" -- a JSON object, reset before every message

    std::string m_project{};  // scene.project -- the folder fields.json lives in

    simdjson::dom::parser m_fields_parser{};  // fields.json -- separate from m_parser, which holds the message being handled

    simdjson::dom::parser m_parser{};

    simdjson::dom::parser m_empty_parser{};  // owns m_empty_args

    Ir77SceneArgs m_empty_args{};

    std::unordered_map<std::string, Ir77SceneOperation, Ir77SceneOpHash, std::equal_to<>> m_operations{};
};

}  // namespace NSIr77Scene