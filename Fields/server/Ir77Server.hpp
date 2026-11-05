#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
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

#include <simdjson.h>

#include "../../Ir77RT/types/Ir77Types.hpp"

#include "../../Ir77RT/runtime/Ir77Json.hpp"
#include "../../Ir77RT/runtime/Ir77Return.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../dictionary/IDIIr77Fields.hpp"

#include "../interface/IIr77Field.hpp"

#include "../runtime/Ir77Field.hpp"

#include "../types/Ir77Types.hpp"

#include "../../PeregrineV/server/Ir77Server.hpp"

using namespace NSIr77RT;

namespace NSIr77Fields {

// =============================================================================================================================================
// Ir77Server -- the fields module's switchboard. Owns Ir77Field (the work) -- created here, held through IIr77Field, handed to no one.
//
//   pipe:        Sink(json) in, the post function out (SetPost) -- address "fields". Every command arrives here as JSON and every reply leaves
//                the same way.
//   direct link: only what can't travel as JSON -- the renderer's server (SetRenderer), and the per-frame calls the scene makes while it builds
//                its frame (AppendDispatches / AppendDraws / GetBounds). Fields never sees the scene; the scene calls in.
//
// Message in:  {"id":7, "from":"ui", "to":"fields", "op":"field.params", "args":{"params":[0, 1, 0, 0, 0, 0, 0, 0]}}
// Reply out:   {"id":7, "from":"fields", "to":"ui", "op":"field.params", "ok":true, "error":""}   (only when the message names a "from")
//
// ops:
//   field.compile   source, topology "surface" | "particles", dims [x, y, z], x / y / z [min, max], value [min, max],
//                   shading "lit" | "colormap" | "contours", colormap "viridis" | "turbo" | "grey", contour {spacing, width}, params [8],
//                   canvas (size), pen {target "canvas" | "points", trace "time" | "sweep", speed, sweep, decay, wrap "fade" | "clear", steps,
//                   pens [{color [r,g,b,a], width}]} -- width in pixels on the canvas, in grid cells as point ink
//                   -- a failed compile answers with the compiler's messages in "error" (lines counted from the program's first line)
//   field.params    params [8]                     -- live values, no recompile
//   field.data      values [floats]                -- the program's own data, read with Ir77FieldData(i); replaces what was there, [] clears it.
//                                                    Kept across recompiles, released with the field -- data { count }
//   field.clear                                    -- blank the canvas
//   field.reset                                    -- the program's first frame again (f.first), blank canvas, pens at t = 0
//   field.show      value true | false
//   field.release                                  -- the program out, its buffers freed (switching away from the fields app)
//   field.project   path                          -- the project folder -- fields.json lives here
//   field.list                                    -- data { names: [...] } from <project>/fields.json (none yet -> [])
//   field.load      name                          -- data { name, field } -- the panel's form, as saved
//   field.save      name, field                   -- adds or replaces that entry; the file is rewritten (temp file, then rename)
//   field.delete    name
//
// A handler that returns something leaves it in m_data -- the reply's "data" (an Ir77Json object; {} otherwise). Ops live in a table
// (op -> handler), filled once by RegisterPipeOperations.
// =============================================================================================================================================
class Ir77Server : public Ir77Enlisted, public std::enable_shared_from_this<Ir77Server> {
   public:
    Ir77Server() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();

        m_field = std::static_pointer_cast<IIr77Field>(std::make_shared<Ir77Field>());

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
        seat_shared_uuid<&GUIDIIr77Fields>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Fields>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIr77Fields)
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
            Ir77RETURN<Ir77OperationFailed>(this, "[WARN] fields: not JSON: " + json);
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

        if (!ok) Ir77RETURN<Ir77OperationFailed>(this, "[WARN] fields: " + error + " : " + json);

        if (from.empty() || !m_post) return;  // nobody to answer

        Ir77Json reply = Ir77Json::Object();
        reply["id"] = id;
        reply["from"] = "fields";
        reply["to"] = from;
        reply["op"] = op;
        reply["ok"] = ok;
        reply["error"] = error;
        reply["data"] = ok ? m_data : Ir77Json::Object();

        m_post(reply.Write());
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // direct link -- setup (Fields -> PeregrineV) and the scene's frame
    // -------------------------------------------------------------------------------------------------------------------------------------
    // PeregrineV's server -- the field's only way into the renderer. Before field.compile.
    std::shared_ptr<IIr77Return const> SetRenderer(std::shared_ptr<NSIr77PeregrineV::Ir77Server> const& renderer) { return m_field->SetRenderer(renderer); }

    // The scene's frame calls these while it builds it -- the field's compute once, its draws per window and pass, its box for the shadows
    std::shared_ptr<IIr77Return const> AppendDispatches(std::vector<Ir77PVDispatch>& dispatches, float const& dt) {
        return m_field->AppendDispatches(dispatches, dt);
    }

    std::shared_ptr<IIr77Return const> AppendDraws(std::size_t const& window, Ir77PVPass const& pass, std::uint32_t const& shadows,
                                                   std::vector<Ir77PVDrawItem>& items) {
        return m_field->AppendDraws(window, pass, shadows, items);
    }

    std::shared_ptr<IIr77Return const> GetBounds(bool& active, glm::vec3& low, glm::vec3& high) { return m_field->GetBounds(active, low, high); }

    // field.data over the direct link -- for a module whose data is too big to send as JSON (a solver's output: basis, coefficients)
    std::shared_ptr<IIr77Return const> SetFieldData(std::vector<float> const& values) { return m_field->SetFieldData(values); }

   private:
    using Ir77FieldsArgs = simdjson::dom::object;

    using Ir77FieldsOperation = std::function<std::shared_ptr<IIr77Return const>(Ir77FieldsArgs const&)>;

    // heterogeneous lookup -- find by the string_view out of the parser, no std::string per message
    struct Ir77FieldsOpHash {
        using is_transparent = void;
        std::size_t operator()(std::string_view const& op) const { return std::hash<std::string_view>{}(op); }
    };

    void RegisterPipeOperations() {
        DefineFieldOperations();
        DefineFieldStoreOperations();
    }

    // the running field -- one program at a time
    void DefineFieldOperations() {
        m_operations["field.compile"] = [this](Ir77FieldsArgs const& a) {
            Ir77FieldSettings settings{};
            ReadSettings(a, settings);

            std::string errors{};
            auto const result = m_field->CompileField(settings, errors);

            m_error = errors;

            return result;
        };

        m_operations["field.params"] = [this](Ir77FieldsArgs const& a) {
            std::array<float, 8> params{};
            Numbers(a, "params", params.data(), params.size());

            return m_field->SetFieldParams(params);
        };

        m_operations["field.data"] = [this](Ir77FieldsArgs const& a) -> std::shared_ptr<IIr77Return const> {
            simdjson::dom::array values{};
            if (a["values"].get(values) != simdjson::SUCCESS) return Ir77RETURN<Ir77InvalidArgument>(this, "field.data needs values [floats]");

            std::vector<float> floats{};
            floats.reserve(values.size());
            for (auto const element : values) {
                double value{0.0};
                if (element.get(value) != simdjson::SUCCESS) return Ir77RETURN<Ir77InvalidArgument>(this, "field.data values are numbers -- element " + std::to_string(floats.size()) + " isn't");
                floats.push_back(static_cast<float>(value));
            }

            std::shared_ptr<IIr77Return const> result{};
            if (Failed(m_field->SetFieldData(floats), result)) return result;

            m_data["count"] = static_cast<std::uint64_t>(floats.size());
            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        m_operations["field.clear"] = [this](Ir77FieldsArgs const&) { return m_field->ClearFieldCanvas(); };
        m_operations["field.reset"] = [this](Ir77FieldsArgs const&) { return m_field->ResetField(); };
        m_operations["field.show"] = [this](Ir77FieldsArgs const& a) { return m_field->ShowField(Flag(a, "value", true)); };
        m_operations["field.release"] = [this](Ir77FieldsArgs const&) { return m_field->ReleaseField(); };
    }

    // A project's saved fields -- <project>/fields.json, one object keyed by name. Each value is the panel's form as it sent it; this
    // server never looks inside one. Read fresh on every op, so a hand edit to the file shows up.
    void DefineFieldStoreOperations() {
        m_operations["field.project"] = [this](Ir77FieldsArgs const& a) -> std::shared_ptr<IIr77Return const> {
            m_project = std::string{Text(a, "path")};

            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        m_operations["field.list"] = [this](Ir77FieldsArgs const&) -> std::shared_ptr<IIr77Return const> {
            std::shared_ptr<IIr77Return const> result{};

            std::map<std::string, std::string> entries{};
            if (Failed(ReadStore(entries), result)) return result;

            Ir77Json& names = m_data["names"] = Ir77Json::Array();
            for (auto const& [name, entry] : entries) names.Push(name);

            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        m_operations["field.load"] = [this](Ir77FieldsArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::shared_ptr<IIr77Return const> result{};

            std::string const name{Text(a, "name")};

            std::map<std::string, std::string> entries{};
            if (Failed(ReadStore(entries), result)) return result;

            auto const found = entries.find(name);
            if (found == entries.end()) {
                m_error = "no field '" + name + "'";
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: " + m_error);
            }

            m_data["name"] = name;
            m_data["field"] = Ir77Json::Raw(found->second);  // stored as the panel sent it

            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        m_operations["field.save"] = [this](Ir77FieldsArgs const& a) -> std::shared_ptr<IIr77Return const> {
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
            if (Failed(ReadStore(entries), result)) return result;

            entries[name] = entry;

            return WriteStore(entries);
        };

        m_operations["field.delete"] = [this](Ir77FieldsArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::shared_ptr<IIr77Return const> result{};

            std::string const name{Text(a, "name")};

            std::map<std::string, std::string> entries{};
            if (Failed(ReadStore(entries), result)) return result;

            if (entries.erase(name) == 0) {
                m_error = "no field '" + name + "'";
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: " + m_error);
            }

            return WriteStore(entries);
        };
    }

    // field.compile's args -> the settings; anything left out keeps Ir77FieldSettings' default
    void ReadSettings(Ir77FieldsArgs const& a, Ir77FieldSettings& settings) const {
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
        if (a["pen"].get(pen) != simdjson::SUCCESS) return;

        bool const sweep = Text(pen, "trace") == "sweep";

        settings.pen_target = Text(pen, "target") == "points" ? 1u : 0u;

        settings.pen_speed = Number(pen, "speed", settings.pen_speed);
        settings.sweep = sweep ? std::max(Number(pen, "sweep", 1.0f), 1e-4f) : 0.0f;
        settings.decay = std::max(Number(pen, "decay", 0.0f), 0.0f);
        settings.wrap_clear = Text(pen, "wrap") == "clear";
        settings.steps = static_cast<std::uint32_t>(std::clamp(Number(pen, "steps", 256.0f), 1.0f, 65536.0f));

        simdjson::dom::array pens{};
        if (pen["pens"].get(pens) != simdjson::SUCCESS) return;

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

    // <project>/fields.json -> name -> entry (minified JSON). No file yet is an empty list, not an error.
    std::shared_ptr<IIr77Return const> ReadStore(std::map<std::string, std::string>& entries) {
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
        if (m_store_parser.load(file.string()).get(root) != simdjson::SUCCESS) {
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
    std::shared_ptr<IIr77Return const> WriteStore(std::map<std::string, std::string> const& entries) {
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
    float Number(Ir77FieldsArgs const& args, char const* key, float const fallback) const {
        double value{0.0};
        return args[key].get(value) == simdjson::SUCCESS ? static_cast<float>(value) : fallback;
    }

    bool Flag(Ir77FieldsArgs const& args, char const* key, bool const fallback) const {
        bool value{false};
        return args[key].get(value) == simdjson::SUCCESS ? value : fallback;
    }

    std::string_view Text(Ir77FieldsArgs const& args, char const* key) const {
        std::string_view value{};
        return args[key].get(value) == simdjson::SUCCESS ? value : std::string_view{};
    }

    // An array of numbers into out[0 .. count) -- entries past the array (or not numbers) keep what out held
    void Numbers(Ir77FieldsArgs const& args, char const* key, float* out, std::size_t const count) const {
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
    std::shared_ptr<IIr77Field> m_field{};  // the module's work -- only this server holds it

    std::function<void(std::string)> m_post{};

    std::string m_error{};  // a handler's detail for the reply's "error" -- cleared before every message

    Ir77Json m_data{Ir77Json::Object()};  // a handler's result for the reply's "data" -- reset to {} before every message

    std::string m_project{};  // field.project -- the folder fields.json lives in

    simdjson::dom::parser m_store_parser{};  // fields.json -- separate from m_parser, which holds the message being handled

    simdjson::dom::parser m_parser{};

    simdjson::dom::parser m_empty_parser{};  // owns m_empty_args

    Ir77FieldsArgs m_empty_args{};

    std::unordered_map<std::string, Ir77FieldsOperation, Ir77FieldsOpHash, std::equal_to<>> m_operations{};
};
}  // namespace NSIr77Fields
