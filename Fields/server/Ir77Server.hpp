#pragma once

#include <algorithm>
#include <cctype>
#include <iterator>
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
//                its frame (AppendDispatches / AppendDraws / GetBounds). Fields never sees the scene; the scene calls in. No module compiles
//                a field or hands it data behind the panel's back: data comes from files (field.data.load), written by whoever made it.
//
// Message in:  {"id":7, "from":"ui", "to":"fields", "op":"field.params", "args":{"params":[0, 1, 0, 0, 0, 0, 0, 0]}}
// Reply out:   {"id":7, "from":"fields", "to":"ui", "op":"field.params", "ok":true, "error":""}   (only when the message names a "from")
//
// ops:
//   field.compile   source, topology "surface" | "particles" | "isosurface", dims [x, y, z], x / y / z [min, max], value [min, max],
//                   shading "lit" | "colormap" | "contours", colormap "viridis" | "turbo" | "grey", contour {spacing, width}, params [8],
//                   canvas (size), pen {target "canvas" | "points", trace "time" | "sweep", speed, sweep, decay, wrap "fade" | "clear", steps,
//                   pens [{color [r,g,b,a], width}]} -- width in pixels on the canvas, in grid cells as point ink,
//                   data "name" (field.data.load first -- read again on every compile) | "" (no data) | left out (the data stays as it is)
//                   -- a failed compile answers with the compiler's messages in "error" (lines counted from the program's first line)
//   field.params    params [8]                     -- live values, no recompile
//   field.data      values [floats]                -- the program's own data, read with Ir77FieldData(i); replaces what was there, [] clears it.
//                                                    Kept across recompiles, released with the field -- data { count }
//   field.data.list                               -- data { files: [{name, format, version, count}] } -- <project>/data/*.json
//   field.data.load name                          -- <project>/data/<name>.json's "values" into field.data -- data { name, format, version,
//                                                    count, summary, sliders }. A data file: {"format", "version", "summary" {...}, "sliders"
//                                                    [{index, min, max, step, labels}], "values" [floats]} -- whoever writes it (Compute) owns
//                                                    the format; Fields reads "values" and passes the rest to the panel (sliders: a range and
//                                                    a name per value for the program's sliders, fitted to this data)
//   field.library.root path                       -- the shader root -- the built-in programs are <root>/fields/fields.json, copied into
//                                                    each project's fields.json (the ones it lacks) when it opens
//   field.clear                                    -- blank the canvas
//   field.reset                                    -- the program's first frame again (f.first), blank canvas, pens at t = 0
//   field.show      value true | false
//   field.release                                  -- the program out, its buffers freed (switching away from the fields app)
//   field.project   path                          -- the project folder -- fields.json lives here; the built-ins it lacks go into it
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
        DefineDataOperations();
        DefineLibraryOperations();
    }

    // the running field -- one program at a time
    void DefineFieldOperations() {
        m_operations["field.compile"] = [this](Ir77FieldsArgs const& a) {
            Ir77FieldSettings settings{};
            ReadSettings(a, settings);

            // the data file first -- read fresh, so a solve written since shows; a missing file fails the compile
            std::string_view data{};
            if (a["data"].get(data) == simdjson::SUCCESS) {
                std::shared_ptr<IIr77Return const> loaded{};
                if (data.empty()) {
                    if (Failed(m_field->SetFieldData({}), loaded)) return loaded;
                } else if (Failed(LoadData(std::string{data}), loaded)) {
                    return loaded;
                }
            }

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

            return Seed();
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

    // Data files -- <project>/data/<name>.json. Fields reads "values"; "format", "version" and "summary" pass through to the panel.
    void DefineDataOperations() {
        m_operations["field.data.list"] = [this](Ir77FieldsArgs const&) -> std::shared_ptr<IIr77Return const> {
            Ir77Json& files = m_data["files"] = Ir77Json::Array();

            if (m_project.empty()) return Ir77RETURN<Ir77OperationSucceeded>();  // no project -- no files

            std::filesystem::path const folder = std::filesystem::path{m_project} / FIELD_DATA_FOLDER;

            std::error_code error{};
            if (!std::filesystem::is_directory(folder, error)) return Ir77RETURN<Ir77OperationSucceeded>();

            std::map<std::string, std::filesystem::path> found{};  // sorted by name
            for (auto const& entry : std::filesystem::directory_iterator{folder, error})
                if (entry.is_regular_file(error) && entry.path().extension() == ".json") found[entry.path().stem().string()] = entry.path();

            for (auto const& [name, path] : found) {
                Ir77Json file = Ir77Json::Object();
                file["name"] = name;

                simdjson::dom::element root{};
                if (m_store_parser.load(path.string()).get(root) == simdjson::SUCCESS) {
                    std::string_view format{};
                    std::int64_t version{0};
                    simdjson::dom::array values{};
                    file["format"] = root["format"].get(format) == simdjson::SUCCESS ? std::string{format} : std::string{};
                    file["version"] = root["version"].get(version) == simdjson::SUCCESS ? version : 0;
                    file["count"] = root["values"].get(values) == simdjson::SUCCESS ? static_cast<std::uint64_t>(values.size()) : 0u;
                } else {
                    file["format"] = "";
                    file["version"] = 0;
                    file["count"] = 0u;
                    file["error"] = "not valid JSON";
                }

                files.Push(file);
            }

            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        m_operations["field.data.load"] = [this](Ir77FieldsArgs const& a) { return LoadData(std::string{Text(a, "name")}); };
    }

    // <project>/data/<name>.json -> field.data; m_data gets {name, format, version, count, summary}
    std::shared_ptr<IIr77Return const> LoadData(std::string const& name) {
        if (m_project.empty()) {
            m_error = "no project open -- data files live in <project>/" + std::string{FIELD_DATA_FOLDER};
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: " + m_error);
        }

        if (!PlainName(name)) {
            m_error = "data name '" + name + "' -- letters, digits, '-', '_' and '.' only";
            return Ir77RETURN<Ir77InvalidArgument>(this, "Ir77Server: " + m_error);
        }

        std::filesystem::path const file = std::filesystem::path{m_project} / FIELD_DATA_FOLDER / (name + ".json");

        simdjson::dom::element root{};
        if (m_store_parser.load(file.string()).get(root) != simdjson::SUCCESS) {
            std::error_code error{};
            m_error = std::filesystem::exists(file, error) ? file.string() + " is not valid JSON" : "no data file " + file.string();
            return Ir77RETURN<Ir77OperationFailed>(this, "Ir77Server: " + m_error);
        }

        simdjson::dom::array values{};
        if (root["values"].get(values) != simdjson::SUCCESS) {
            m_error = file.string() + " has no \"values\" array";
            return Ir77RETURN<Ir77InvalidArgument>(this, "Ir77Server: " + m_error);
        }

        std::vector<float> floats{};
        floats.reserve(values.size());
        for (auto const element : values) {
            double value{0.0};
            if (element.get(value) != simdjson::SUCCESS) {
                m_error = file.string() + ": values are numbers -- element " + std::to_string(floats.size()) + " isn't";
                return Ir77RETURN<Ir77InvalidArgument>(this, "Ir77Server: " + m_error);
            }
            floats.push_back(static_cast<float>(value));
        }

        std::shared_ptr<IIr77Return const> result{};
        if (Failed(m_field->SetFieldData(floats), result)) {
            m_error = file.string() + ": " + std::to_string(floats.size()) + " floats -- at most " + std::to_string(FIELD_MAX_DATA);
            return result;
        }

        std::string_view format{};
        std::int64_t version{0};
        simdjson::dom::element summary{};

        m_data["name"] = name;
        m_data["format"] = root["format"].get(format) == simdjson::SUCCESS ? std::string{format} : std::string{};
        m_data["version"] = root["version"].get(version) == simdjson::SUCCESS ? version : 0;
        m_data["count"] = static_cast<std::uint64_t>(floats.size());
        m_data["summary"] = root["summary"].get(summary) == simdjson::SUCCESS ? Ir77Json::Raw(simdjson::minify(summary)) : Ir77Json::Object();

        // what the writer says about the program's sliders for this data ([{index, min, max, step, labels}]) -- the panel applies it
        simdjson::dom::element sliders{};
        m_data["sliders"] = root["sliders"].get(sliders) == simdjson::SUCCESS && sliders.is_array() ? Ir77Json::Raw(simdjson::minify(sliders)) : Ir77Json::Array();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The built-in programs -- <root>/fields/fields.json, in the project store's own form (name -> the panel's form, its source in it). A
    // project gets each one it lacks when it opens (field.project, or field.library.root after it): from then on they're its saved fields,
    // edited, saved and deleted like any other. An edited one is never overwritten; a deleted one comes back the next time the project opens.
    void DefineLibraryOperations() {
        m_operations["field.library.root"] = [this](Ir77FieldsArgs const& a) -> std::shared_ptr<IIr77Return const> {
            m_library = std::string{Text(a, "path")};

            return Seed();
        };
    }

    // the built-ins the project's fields.json lacks, written into it
    std::shared_ptr<IIr77Return const> Seed() {
        if (m_library.empty() || m_project.empty()) return Ir77RETURN<Ir77OperationSucceeded>();

        std::filesystem::path const file = std::filesystem::path{m_library} / FIELD_LIBRARY_FOLDER / "fields.json";
        std::error_code missing{};
        if (!std::filesystem::exists(file, missing)) return Ir77RETURN<Ir77OperationSucceeded>();

        simdjson::dom::element root{};
        simdjson::dom::object builtins{};
        if (m_store_parser.load(file.string()).get(root) != simdjson::SUCCESS || root.get(builtins) != simdjson::SUCCESS) {
            m_error = file.string() + " isn't a JSON object";
            return Ir77RETURN<Ir77OperationFailed>(this, "Ir77Server: " + m_error);
        }
        std::map<std::string, std::string> wanted{};
        for (auto const entry : builtins) wanted[std::string{entry.key}] = simdjson::minify(entry.value);

        std::shared_ptr<IIr77Return const> result{};
        std::map<std::string, std::string> entries{};
        if (Failed(ReadStore(entries), result)) return result;

        bool added = false;
        for (auto const& [name, entry] : wanted)
            if (entries.emplace(name, entry).second) added = true;

        return added ? WriteStore(entries) : Ir77RETURN<Ir77OperationSucceeded>();
    }

    // a file name with no folder in it -- letters, digits, '-', '_', '.' (not leading)
    static bool PlainName(std::string const& name) {
        if (name.empty() || name.front() == '.' || name.size() > 128) return false;
        for (char const c : name)
            if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '_' && c != '.') return false;
        return true;
    }

    // field.compile's args -> the settings; anything left out keeps Ir77FieldSettings' default
    void ReadSettings(Ir77FieldsArgs const& a, Ir77FieldSettings& settings) const {
        settings.source = std::string{Text(a, "source")};
        std::string_view const topology = Text(a, "topology");
        settings.topology = topology == "particles" ? FIELD_TOPOLOGY_PARTICLES : topology == "isosurface" ? FIELD_TOPOLOGY_ISOSURFACE : FIELD_TOPOLOGY_SURFACE;

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

    std::string m_project{};  // field.project -- the folder fields.json and data/ live in

    std::string m_library{};  // field.library.root -- the shader root; the built-in programs are in its fields/

    simdjson::dom::parser m_store_parser{};  // fields.json -- separate from m_parser, which holds the message being handled

    simdjson::dom::parser m_parser{};

    simdjson::dom::parser m_empty_parser{};  // owns m_empty_args

    Ir77FieldsArgs m_empty_args{};

    std::unordered_map<std::string, Ir77FieldsOperation, Ir77FieldsOpHash, std::equal_to<>> m_operations{};
};
}  // namespace NSIr77Fields
