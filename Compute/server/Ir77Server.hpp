#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <simdjson.h>

#include "../../Ir77RT/types/Ir77Types.hpp"

#include "../../Ir77RT/runtime/Ir77Json.hpp"
#include "../../Ir77RT/runtime/Ir77Return.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../dictionary/IDIIr77Compute.hpp"

#include "../interface/IIr77Device.hpp"
#include "../interface/IIr77Program.hpp"

#include "../runtime/Ir77Device.hpp"
#include "../runtime/Ir77Program.hpp"

#include "../services/Ir77Basis.hpp"
#include "../services/Ir77Scf.hpp"

#include "../types/Ir77Types.hpp"

using namespace NSIr77RT;

namespace NSIr77Compute {

// =============================================================================================================================================
// Ir77Server -- the compute module's switchboard. Owns the OpenCL device and the program worker, held through their interfaces.
//
//   pipe:        Sink(json) in, the post function out (SetPost) -- address "compute". A run answers at once ({started}) and finishes later:
//                Update (the frame loop, main thread) sees it done, writes its outputs to <project>/data/ and posts the event "compute.run".
//   programs:    one OpenCL source each -- its constants, buffers, kernels and script are declared in it (services/Ir77Stages.hpp). Compute is
//                a generator: what a program makes leaves as data files, {"format", "version", "summary" {...}, "sliders" [...], "values"
//                [floats]} (formats in Ir77Types.hpp). Fields reads them when its panel asks; Compute never talks to Fields. An output under
//                the same name overwrites the file (temp file, then rename).
//
// ops:
//   compute.devices                              -- data { devices: ["platform: device (doubles)" ...], active, error } -- opens the device
//                                                   first if it isn't (the first use does too)
//   compute.open                                 -- the device found again (a driver installed since) -- data as compute.devices
//   compute.project path                         -- the project folder -- outputs go to its data/ ('' = none: a run is refused); the
//                                                   built-in programs it lacks go into its compute.json
//   compute.run     source, program ('' -- its name, reported back)
//                                                -- built and run on the worker; data { started } -- the end comes as the event below
//   compute.cancel                               -- the running program stops at its next step
//   compute.status                               -- data { state "idle" | "running" | "done" | "failed" | "cancelled", program, last (the
//                                                   last run's event data, null before the first) }
//   compute.buffer  name, slice (0)              -- one buffer of the last run, sampled for drawing: data { name, dims [d0 fastest ...], ints,
//                                                   size, minimum, maximum, kind "plot" | "matrix", series, length (plot: series rows of
//                                                   length), rows, cols (matrix), step (sampling, both), slice, slices (3 dims: the d2 index
//                                                   shown, how many), low, high (what's drawn), values [floats] }
//   compute.library.root path                    -- the shader root -- the built-in programs are <root>/compute/compute.json, copied
//                                                   into each project's compute.json (the ones it lacks) when it opens
//   compute.list / compute.load name / compute.save name, program {source} / compute.delete name
//                                                -- the project's saved programs, <project>/compute.json
//
// event "compute.run" (to "ui"): { ok, state, error, program, seconds, device, report {symbol: value}, outputs [{name, format, path,
//   written, error, summary}], buffers [{name, dims, ints, size, minimum, maximum, kept}] } -- summary: an orbitals file's (energy, homo,
//   lumo, functions, orbital_energies ...) or a grid's (size, domain, channels, largest)
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

        m_device = std::static_pointer_cast<IIr77Device>(std::make_shared<Ir77Device>());

        m_program = std::static_pointer_cast<IIr77Program>(std::make_shared<Ir77Program>());
        m_program->SetDevice(m_device);

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
        seat_shared_uuid<&GUIDIIr77Compute>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Compute>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77Compute)
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
            Ir77RETURN<Ir77OperationFailed>(this, "[WARN] compute: not JSON: " + json);
            return;
        }

        // a reply to something this module sent -- never answered
        if (root["ok"].error() == simdjson::SUCCESS) return;

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
        std::string const error = ok ? "" : !known ? "unknown op" : m_error.empty() ? "operation failed" : m_error;

        if (!ok) Ir77RETURN<Ir77OperationFailed>(this, "[WARN] compute: " + error + " : " + json);

        if (from.empty() || !m_post) return;

        Ir77Json reply = Ir77Json::Object();
        reply["id"] = id;
        reply["from"] = "compute";
        reply["to"] = from;
        reply["op"] = op;
        reply["ok"] = ok;
        reply["error"] = error;
        reply["data"] = ok ? m_data : Ir77Json::Object();

        m_post(reply.Write());
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // the frame loop
    // -------------------------------------------------------------------------------------------------------------------------------------
   public:
    // Once a frame, main thread -- a run that finished since: its outputs written, the "compute.run" event out
    void Update() {
        Ir77JobState state{Ir77JobState::Idle};
        m_program->GetState(state);
        if (state == Ir77JobState::Running || state == Ir77JobState::Idle || !m_pending) return;
        m_pending = false;

        Ir77ProgramJob job{};
        Ir77ProgramResult result{};
        std::string error{};
        m_program->TakeResult(job, result, error);

        Ir77Json data = Ir77Json::Object();
        data["ok"] = state == Ir77JobState::Done;
        data["state"] = StateName(state);
        data["error"] = error;
        data["program"] = job.program;
        data["seconds"] = result.seconds;
        data["device"] = result.device;

        Ir77Json& report = data["report"] = Ir77Json::Object();
        for (auto const& [symbol, value] : result.report) report[symbol] = value;

        // the outputs -- written only when the whole script ran
        Ir77Json& outputs = data["outputs"] = Ir77Json::Array();
        for (Ir77ProgramOutput& output : result.outputs) {
            Ir77Json row = Ir77Json::Object();
            row["name"] = output.name;
            row["format"] = output.format;
            std::string path{}, write_error{};
            bool written = false;
            if (output.format == ORBITALS_FORMAT) {
                output.scf.seconds = result.seconds;
                output.scf.device = result.device;
                Ir77Json const summary = SolveData(output.molecule, output.basis, output.scf);
                row["summary"] = summary;
                if (state == Ir77JobState::Done)
                    written = WriteFile(output.name, ORBITALS_FORMAT, ORBITALS_LAYOUT_VERSION, summary, Sliders(output.scf), {},
                                        Ir77Scf::Layout(output.molecule, output.basis, output.scf), path, write_error);
            } else {
                Ir77Json const summary = GridData(output);
                row["summary"] = summary;
                if (state == Ir77JobState::Done) written = WriteGrid(output, summary, path, write_error);
            }
            if (state != Ir77JobState::Done) write_error = "not written -- the run didn't finish";
            row["written"] = written;
            row["path"] = path;
            row["error"] = write_error;
            if (!written && state == Ir77JobState::Done) Ir77RETURN<Ir77OperationFailed>(this, "[WARN] compute: " + write_error);
            outputs.Push(row);
        }

        // the buffers -- kept for compute.buffer until the next run
        m_buffers = std::move(result.buffers);
        Ir77Json& buffers = data["buffers"] = Ir77Json::Array();
        for (Ir77ProgramBuffer const& b : m_buffers) {
            Ir77Json row = Ir77Json::Object();
            row["name"] = b.name;
            row["dims"] = Ir77Json::Numbers(b.dims.data(), b.dims.size());
            row["ints"] = b.ints;
            row["size"] = static_cast<std::uint64_t>(b.size);
            row["minimum"] = b.minimum;
            row["maximum"] = b.maximum;
            row["kept"] = !b.values.empty() || b.size == 0;
            buffers.Push(row);
        }

        m_last = data;
        if (!m_post) return;
        Ir77Json event = Ir77Json::Object();
        event["from"] = "compute";
        event["to"] = "ui";
        event["event"] = "compute.run";
        event["data"] = data;
        m_post(event.Write());
    }

   private:
    using Ir77ComputeArgs = simdjson::dom::object;

    using Ir77ComputeOperation = std::function<std::shared_ptr<IIr77Return const>(Ir77ComputeArgs const&)>;

    struct Ir77ComputeOpHash {
        using is_transparent = void;
        std::size_t operator()(std::string_view const& op) const { return std::hash<std::string_view>{}(op); }
    };

    void RegisterPipeOperations() {
        m_operations["compute.devices"] = [this](Ir77ComputeArgs const&) {
            if (!m_device_tried) OpenDevice();
            return DeviceData();
        };

        m_operations["compute.open"] = [this](Ir77ComputeArgs const&) {
            OpenDevice();
            return DeviceData();
        };

        m_operations["compute.project"] = [this](Ir77ComputeArgs const& a) -> std::shared_ptr<IIr77Return const> {
            m_project = std::string{Text(a, "path")};
            return Seed();
        };

        m_operations["compute.run"] = [this](Ir77ComputeArgs const& a) -> std::shared_ptr<IIr77Return const> {
            if (m_project.empty()) {
                m_error = "no project open -- a program's outputs go to <project>/" + std::string{COMPUTE_DATA_FOLDER};
                return Ir77RETURN<Ir77NotConfigured>(this, m_error);
            }

            Ir77ProgramJob job{};
            job.source = std::string{Text(a, "source")};
            job.program = std::string{Text(a, "program")};
            if (job.source.find_first_not_of(" \t\r\n") == std::string::npos) {
                m_error = "no program -- pick one from the list or write one";
                return Ir77RETURN<Ir77InvalidArgument>(this, m_error);
            }

            if (!m_device_tried) OpenDevice();
            bool opened{false};
            std::string device{}, device_error{};
            m_device->GetDevice(opened, device, device_error);
            if (!opened) {
                m_error = "programs run on an OpenCL device -- there is none (" + device_error + ")";
                return Ir77RETURN<Ir77NotConfigured>(this, m_error);
            }

            std::shared_ptr<IIr77Return const> result{};
            if (Failed(m_program->Start(job), result)) {
                m_error = "a program is already running";
                return result;
            }
            m_pending = true;
            m_running = job.program;

            m_data["started"] = true;
            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        m_operations["compute.cancel"] = [this](Ir77ComputeArgs const&) { return m_program->Cancel(); };

        m_operations["compute.status"] = [this](Ir77ComputeArgs const&) -> std::shared_ptr<IIr77Return const> {
            Ir77JobState state{Ir77JobState::Idle};
            m_program->GetState(state);
            m_data["state"] = StateName(state);
            m_data["program"] = m_running;
            m_data["last"] = m_last;
            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        m_operations["compute.buffer"] = [this](Ir77ComputeArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::string const name{Text(a, "name")};
            for (Ir77ProgramBuffer const& b : m_buffers)
                if (b.name == name) return BufferView(b, static_cast<std::size_t>(std::max(Number(a, "slice", 0.0), 0.0)));
            m_error = "no buffer '" + name + "' in the last run";
            return Ir77RETURN<Ir77NotConfigured>(this, m_error);
        };

        DefineLibraryOperations();
        DefineStoreOperations();
    }

    // One buffer sampled for drawing -- a vector (or a few of them) as a plot, a matrix as an image, a 3-dimensional buffer one d2 slice at a
    // time. d0 is fastest: a matrix's columns, a plot's points.
    std::shared_ptr<IIr77Return const> BufferView(Ir77ProgramBuffer const& b, std::size_t const slice_asked) {
        if (b.values.empty() && b.size > 0) {
            m_error = b.name + " holds " + std::to_string(b.size) + " -- more than the " + std::to_string(COMPUTE_KEEP_LIMIT) + " kept for viewing";
            return Ir77RETURN<Ir77NotConfigured>(this, m_error);
        }
        std::size_t const d0 = b.dims.size() > 0 ? b.dims[0] : 1, d1 = b.dims.size() > 1 ? b.dims[1] : 1, d2 = b.dims.size() > 2 ? b.dims[2] : 1;
        std::size_t const slice = std::min(slice_asked, d2 > 0 ? d2 - 1 : 0);
        std::size_t const offset = slice * d0 * d1;

        m_data["name"] = b.name;
        m_data["dims"] = Ir77Json::Numbers(b.dims.data(), b.dims.size());
        m_data["ints"] = b.ints;
        m_data["size"] = static_cast<std::uint64_t>(b.size);
        m_data["minimum"] = b.minimum;
        m_data["maximum"] = b.maximum;
        m_data["slice"] = static_cast<std::uint64_t>(slice);
        m_data["slices"] = static_cast<std::uint64_t>(b.dims.size() > 2 ? d2 : 1);

        std::vector<float> values{};
        double low = 1e300, high = -1e300;
        auto const take = [&](std::size_t const at) {
            double const v = at < b.values.size() ? b.values[at] : 0.0;
            low = std::min(low, v);
            high = std::max(high, v);
            values.push_back(static_cast<float>(v));
        };

        bool const plot = b.dims.size() == 1 || (b.dims.size() == 2 && d1 <= 8);
        if (plot) {
            std::size_t const step = std::max<std::size_t>((d0 + COMPUTE_VIEW_LENGTH - 1) / COMPUTE_VIEW_LENGTH, 1);
            std::size_t length = 0;
            for (std::size_t s = 0; s < d1; s++) {
                length = 0;
                for (std::size_t k = 0; k < d0; k += step, length++) take(s * d0 + k);
            }
            m_data["kind"] = "plot";
            m_data["series"] = static_cast<std::uint64_t>(d1);
            m_data["length"] = static_cast<std::uint64_t>(length);
            m_data["step"] = static_cast<std::uint64_t>(step);
        } else {
            std::size_t const step = std::max<std::size_t>((std::max(d0, d1) + COMPUTE_VIEW_SIDE - 1) / COMPUTE_VIEW_SIDE, 1);
            std::size_t rows = 0, cols = 0;
            for (std::size_t r = 0; r < d1; r += step, rows++) {
                cols = 0;
                for (std::size_t c = 0; c < d0; c += step, cols++) take(offset + r * d0 + c);
            }
            m_data["kind"] = "matrix";
            m_data["rows"] = static_cast<std::uint64_t>(rows);
            m_data["cols"] = static_cast<std::uint64_t>(cols);
            m_data["step"] = static_cast<std::uint64_t>(step);
        }
        m_data["low"] = values.empty() ? 0.0 : low;
        m_data["high"] = values.empty() ? 0.0 : high;
        m_data["values"] = Ir77Json::Numbers(values.data(), values.size());
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // a grid output's summary
    static Ir77Json GridData(Ir77ProgramOutput const& output) {
        Ir77Json data = Ir77Json::Object();
        data["size"] = Ir77Json::Numbers(output.size.data(), output.size.size());
        data["domain"] = Ir77Json::Numbers(output.domain.data(), output.domain.size());
        Ir77Json& channels = data["channels"] = Ir77Json::Array();
        for (std::string const& c : output.channels) channels.Push(Ir77Json(c));
        data["largest"] = Ir77Json::Numbers(output.largest.data(), output.largest.size());
        data["passes"] = static_cast<std::uint64_t>(output.passes);
        data["residual"] = output.residual;
        data["converged"] = output.converged;
        return data;
    }

    // A grid to <project>/data/<name>.json -- format "grid": header (GRID_HEADER) version, header length, nx, ny, nz, stride, x0, x1, y0, y1,
    // z0, z1, passes, residual, 0, converged; then each channel's largest |value|; then the points (x fastest), stride floats each.
    bool WriteGrid(Ir77ProgramOutput const& output, Ir77Json const& summary, std::string& path, std::string& error) {
        std::uint32_t const stride = static_cast<std::uint32_t>(output.channels.size());
        std::vector<float> header{static_cast<float>(GRID_VERSION),
                                  static_cast<float>(GRID_HEADER + stride),
                                  static_cast<float>(output.size[0]),
                                  static_cast<float>(output.size[1]),
                                  static_cast<float>(output.size[2]),
                                  static_cast<float>(stride),
                                  output.domain[0], output.domain[1], output.domain[2], output.domain[3], output.domain[4], output.domain[5],
                                  static_cast<float>(output.passes),
                                  static_cast<float>(output.residual),
                                  0.0f,
                                  output.converged ? 1.0f : 0.0f};
        for (double const largest : output.largest) header.push_back(static_cast<float>(largest));

        // Fields' grid program has a channel slider -- named after the program's buffers
        Ir77Json sliders = Ir77Json::Array();
        Ir77Json channel = Ir77Json::Object();
        channel["index"] = 0;
        channel["min"] = 0;
        channel["max"] = static_cast<std::uint64_t>(stride > 0 ? stride - 1 : 0);
        channel["step"] = 1;
        Ir77Json& names = channel["labels"] = Ir77Json::Array();
        for (std::uint32_t c = 0; c < stride; c++) names.Push(Ir77Json(std::to_string(c) + "  " + output.channels[c]));
        sliders.Push(channel);

        return WriteFile(output.name, GRID_FORMAT, GRID_VERSION, summary, sliders, header, output.values, path, error);
    }

    // <project>/data/<name>.json -- {format, version, summary, sliders, values: head then body}, via .tmp and a rename
    bool WriteFile(std::string const& name, char const* format, std::uint32_t const version, Ir77Json const& summary, Ir77Json const& sliders,
                   std::vector<float> const& head, std::vector<float> const& body, std::string& path, std::string& error) {
        if (m_project.empty()) {
            error = "the project closed before the run finished -- nothing written";
            return false;
        }

        std::filesystem::path const folder = std::filesystem::path{m_project} / COMPUTE_DATA_FOLDER;
        std::filesystem::path const file = folder / (name + ".json");
        std::filesystem::path temp = file;
        temp += ".tmp";
        path = file.string();

        std::error_code ec{};
        std::filesystem::create_directories(folder, ec);
        if (ec) {
            error = "cannot make " + folder.string() + ": " + ec.message();
            return false;
        }

        {
            std::ofstream out{temp, std::ios::trunc};
            if (!out) {
                error = "cannot write " + temp.string();
                return false;
            }

            out << "{\"format\": " << Ir77Json::Quote(format) << ", \"version\": " << version << ",\n";
            out << "\"summary\": " << summary.Write() << ",\n";
            out << "\"sliders\": " << sliders.Write() << ",\n";
            out << "\"values\": [";

            char number[32];
            std::size_t written = 0;
            for (std::vector<float> const* part : {&head, &body})
                for (float const v : *part) {
                    std::snprintf(number, sizeof(number), "%.9g", static_cast<double>(v));
                    out << (written == 0 ? "" : written % 16 == 0 ? ",\n" : ", ") << number;
                    written++;
                }
            out << "]}\n";
            out.flush();
            if (!out) {
                error = "cannot write " + temp.string();
                return false;
            }
        }

        std::filesystem::rename(temp, file, ec);
        if (ec) {
            error = "cannot replace " + file.string() + ": " + ec.message();
            return false;
        }
        return true;
    }


    // ---- the built-in programs -- <root>/compute/compute.json, in the project store's own form ({name: {source}}) ----------------------
    // A project gets each one it lacks when it opens (compute.project, or compute.library.root after it): from then on they're its saved
    // programs, edited, saved and deleted like any other. An edited one is never overwritten; a deleted one comes back the next time the
    // project opens.

    void DefineLibraryOperations() {
        m_operations["compute.library.root"] = [this](Ir77ComputeArgs const& a) -> std::shared_ptr<IIr77Return const> {
            m_library = std::string{Text(a, "path")};
            return Seed();
        };
    }

    // the built-ins the project's compute.json lacks, written into it
    std::shared_ptr<IIr77Return const> Seed() {
        if (m_library.empty() || m_project.empty()) return Ir77RETURN<Ir77OperationSucceeded>();

        std::filesystem::path const file = std::filesystem::path{m_library} / "compute" / "compute.json";
        std::error_code missing{};
        if (!std::filesystem::exists(file, missing)) return Ir77RETURN<Ir77OperationSucceeded>();

        simdjson::dom::element root{};
        simdjson::dom::object builtins{};
        if (m_file_parser.load(file.string()).get(root) != simdjson::SUCCESS || root.get(builtins) != simdjson::SUCCESS) {
            m_error = file.string() + " isn't a JSON object";
            return Ir77RETURN<Ir77OperationFailed>(this, m_error);
        }
        std::map<std::string, std::string> wanted{};
        for (auto const entry : builtins) wanted[std::string{entry.key}] = simdjson::minify(entry.value);

        std::map<std::string, std::string> entries{};
        if (!ReadStore(entries)) return Ir77RETURN<Ir77NotConfigured>(this, m_error);

        bool added = false;
        for (auto const& [name, entry] : wanted)
            if (entries.emplace(name, entry).second) added = true;

        return added ? WriteStore(entries) : Ir77RETURN<Ir77OperationSucceeded>();
    }

    // ---- the project's saved programs -- <project>/compute.json, one object keyed by name ({source}, never looked inside) ---------

    void DefineStoreOperations() {
        m_operations["compute.list"] = [this](Ir77ComputeArgs const&) -> std::shared_ptr<IIr77Return const> {
            std::map<std::string, std::string> entries{};
            if (!ReadStore(entries)) return Ir77RETURN<Ir77NotConfigured>(this, m_error);
            Ir77Json& names = m_data["names"] = Ir77Json::Array();
            for (auto const& [name, entry] : entries) names.Push(name);
            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        m_operations["compute.load"] = [this](Ir77ComputeArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::string const name{Text(a, "name")};
            std::map<std::string, std::string> entries{};
            if (!ReadStore(entries)) return Ir77RETURN<Ir77NotConfigured>(this, m_error);
            auto const found = entries.find(name);
            if (found == entries.end()) {
                m_error = "no saved program '" + name + "'";
                return Ir77RETURN<Ir77NotConfigured>(this, m_error);
            }
            m_data["name"] = name;
            m_data["program"] = Ir77Json::Raw(found->second);
            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        m_operations["compute.save"] = [this](Ir77ComputeArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::string const name{Text(a, "name")};
            if (name.empty()) {
                m_error = "a program needs a name";
                return Ir77RETURN<Ir77InvalidArgument>(this, m_error);
            }
            simdjson::dom::element program{};
            if (a["program"].get(program) != simdjson::SUCCESS || !program.is_object()) {
                m_error = "compute.save needs \"program\" as an object";
                return Ir77RETURN<Ir77InvalidArgument>(this, m_error);
            }
            std::string const entry = simdjson::minify(program);  // before the file's parse reuses nothing of the message's

            std::map<std::string, std::string> entries{};
            if (!ReadStore(entries)) return Ir77RETURN<Ir77NotConfigured>(this, m_error);
            entries[name] = entry;
            return WriteStore(entries);
        };

        m_operations["compute.delete"] = [this](Ir77ComputeArgs const& a) -> std::shared_ptr<IIr77Return const> {
            std::string const name{Text(a, "name")};
            std::map<std::string, std::string> entries{};
            if (!ReadStore(entries)) return Ir77RETURN<Ir77NotConfigured>(this, m_error);
            if (entries.erase(name) == 0) {
                m_error = "no saved program '" + name + "'";
                return Ir77RETURN<Ir77NotConfigured>(this, m_error);
            }
            return WriteStore(entries);
        };
    }

    bool ReadStore(std::map<std::string, std::string>& entries) {
        entries.clear();
        if (m_project.empty()) {
            m_error = "no project open";
            return false;
        }
        std::filesystem::path const file = std::filesystem::path{m_project} / "compute.json";
        std::error_code ec{};
        if (!std::filesystem::exists(file, ec)) return true;

        simdjson::dom::element root{};
        simdjson::dom::object programs{};
        if (m_file_parser.load(file.string()).get(root) != simdjson::SUCCESS || root.get(programs) != simdjson::SUCCESS) {
            m_error = file.string() + " must be one JSON object keyed by program name";
            return false;
        }
        for (auto const program : programs) entries[std::string{program.key}] = simdjson::minify(program.value);
        return true;
    }

    std::shared_ptr<IIr77Return const> WriteStore(std::map<std::string, std::string> const& entries) {
        std::filesystem::path const file = std::filesystem::path{m_project} / "compute.json";
        std::filesystem::path temp = file;
        temp += ".tmp";
        {
            std::ofstream out{temp, std::ios::trunc};
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
                return Ir77RETURN<Ir77OperationFailed>(this, m_error);
            }
        }
        std::error_code ec{};
        std::filesystem::rename(temp, file, ec);
        if (ec) {
            m_error = "cannot replace " + file.string() + ": " + ec.message();
            return Ir77RETURN<Ir77OperationFailed>(this, m_error);
        }
        return Ir77RETURN<Ir77OperationSucceeded>();
    }


    // ---- the device -----------------------------------------------------------------------------------------------------------------------

    void OpenDevice() {
        m_device_tried = true;
        m_device->Open();  // its own warning when there's none -- the CPU computes
    }

    std::shared_ptr<IIr77Return const> DeviceData() {
        std::vector<std::string> devices{};
        m_device->ListDevices(devices);

        bool opened{false};
        std::string name{};
        std::string error{};
        m_device->GetDevice(opened, name, error);

        Ir77Json& list = m_data["devices"] = Ir77Json::Array();
        for (std::string const& d : devices) list.Push(Ir77Json(d));
        m_data["active"] = name;
        m_data["error"] = error;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }


    // ---- an orbitals output ---------------------------------------------------------------------------------------------------------------

    // an orbitals output's summary -- what the panel shows of it, and the data file's "summary"
    static Ir77Json SolveData(Ir77Molecule const& molecule, Ir77Basis const& basis, Ir77ScfResult const& result) {
        Ir77Json data = Ir77Json::Object();
        data["converged"] = result.converged;
        data["energy"] = result.energy;
        data["nuclear_repulsion"] = result.nuclear_repulsion;
        data["iterations"] = result.iterations;
        data["electrons"] = result.electrons;
        data["occupied"] = result.occupied;
        data["functions"] = static_cast<std::uint64_t>(basis.functions.size());
        data["orbital_energies"] = Ir77Json::Numbers(result.orbital_energies.data(), result.orbital_energies.size());
        // occupations -- the solver's own when it has them (a Slater atom's spread open shell), else 2 / 0 (restricted) or 1 / 0 a spin
        std::vector<double> occupations(result.orbital_energies.size(), 0.0);
        std::vector<double> beta_occupations(result.beta_orbital_energies.size(), 0.0);
        if (result.occupations.size() == occupations.size()) {
            occupations = result.occupations;
        } else {
            for (std::size_t k = 0; k < occupations.size() && k < (result.unrestricted ? result.alpha : result.occupied); k++)
                occupations[k] = result.unrestricted ? 1.0 : 2.0;
        }
        if (result.beta_occupations.size() == beta_occupations.size()) {
            beta_occupations = result.beta_occupations;
        } else {
            for (std::size_t k = 0; k < beta_occupations.size() && k < result.beta; k++) beta_occupations[k] = 1.0;
        }

        // HOMO / LUMO -- across both spins when unrestricted: the highest with electrons, the lowest without
        double homo = -1e300;
        double lumo = 1e300;
        auto const frontier = [&](std::vector<double> const& energies, std::vector<double> const& filled) {
            for (std::size_t k = 0; k < energies.size(); k++) {
                if (filled[k] > 0.0) homo = std::max(homo, energies[k]);
                else lumo = std::min(lumo, energies[k]);
            }
        };
        frontier(result.orbital_energies, occupations);
        if (result.unrestricted) frontier(result.beta_orbital_energies, beta_occupations);
        data["homo"] = homo > -1e300 ? homo : 0.0;
        data["lumo"] = lumo < 1e300 ? lumo : 0.0;
        data["device"] = result.device;
        data["seconds"] = result.seconds;
        data["basis"] = molecule.basis;
        data["charge"] = static_cast<std::int64_t>(molecule.charge);

        data["functions_kind"] = result.function_kind == ORBITALS_FUNCTION_SLATER ? "slater" : "gaussian";
        data["occupations"] = Ir77Json::Numbers(occupations.data(), occupations.size());

        // the orbitals' names -- the solver's ("2p x", a Slater atom) or MO k
        Ir77Json& names = data["orbital_names"] = Ir77Json::Array();
        for (std::size_t k = 0; k < result.orbital_energies.size(); k++)
            names.Push(Ir77Json(k < result.orbital_names.size() ? result.orbital_names[k] : "MO " + std::to_string(k + 1)));

        data["unrestricted"] = result.unrestricted;
        data["multiplicity"] = static_cast<std::uint64_t>(result.multiplicity);
        data["alpha"] = static_cast<std::uint64_t>(result.alpha);
        data["beta"] = static_cast<std::uint64_t>(result.beta);
        data["s2"] = result.s2;
        data["beta_energies"] = Ir77Json::Numbers(result.beta_orbital_energies.data(), result.beta_orbital_energies.size());
        data["beta_occupations"] = Ir77Json::Numbers(beta_occupations.data(), beta_occupations.size());

        Ir77Json& atoms = data["atoms"] = Ir77Json::Array();
        for (Ir77Atom const& atom : molecule.atoms) {
            Ir77Json row = Ir77Json::Object();
            row["element"] = atom.number < std::size(Ir77BasisBuilder::ELEMENTS) ? Ir77BasisBuilder::ELEMENTS[atom.number] : "";
            row["number"] = static_cast<std::uint64_t>(atom.number);
            row["x"] = atom.x;
            row["y"] = atom.y;
            row["z"] = atom.z;
            atoms.Push(row);
        }

        return data;
    }


    // What the data file tells the panel about its sliders -- Shaders/fields/orbitals.glsl's: p[0] orbital runs 0 .. M, each value named
    // ("density", "16  4p x  up-down  -0.4775"), p[2] spin named. Fields passes this through untouched (field.data.load / field.compile).
    static Ir77Json Sliders(Ir77ScfResult const& result) {
        std::size_t const m = result.orbital_energies.size();

        auto const occupancy = [](double const alpha, double const beta) {
            // arrows -- a fraction (a spread open subshell) as its number
            auto const one = [](double const q, char const* arrow) -> std::string {
                if (q <= 0.0) return "";
                if (fabs(q - 1.0) < 1e-9) return arrow;
                char text[32];
                std::snprintf(text, sizeof(text), "%s%.2g", arrow, q);
                return text;
            };
            return one(alpha, "\u2191") + one(beta, "\u2193");
        };

        Ir77Json labels = Ir77Json::Array();
        labels.Push(Ir77Json(std::string{"density"}));
        for (std::size_t k = 0; k < m; k++) {
            std::string const name = k < result.orbital_names.size() ? result.orbital_names[k] : "MO " + std::to_string(k + 1);

            double alpha = 0.0, beta = 0.0;
            if (result.unrestricted) {
                alpha = k < result.occupations.size() ? result.occupations[k] : (k < result.alpha ? 1.0 : 0.0);
                beta = k < result.beta_occupations.size() ? result.beta_occupations[k] : (k < result.beta ? 1.0 : 0.0);
            } else {
                double const q = k < result.occupations.size() ? result.occupations[k] : (k < result.occupied ? 2.0 : 0.0);
                alpha = beta = 0.5 * q;
            }

            char energy[32];
            std::snprintf(energy, sizeof(energy), "%.4f", result.orbital_energies[k]);
            std::string label = std::to_string(k + 1) + "  " + name + "  " + occupancy(alpha, beta) + "  " + energy;
            if (result.unrestricted && k < result.beta_orbital_names.size() && result.beta_orbital_names[k] != name)
                label += "  (\u03b2 " + result.beta_orbital_names[k] + ")";
            labels.Push(Ir77Json(label));
        }

        Ir77Json orbital = Ir77Json::Object();
        orbital["index"] = 0;
        orbital["min"] = 0;
        orbital["max"] = static_cast<std::uint64_t>(m);
        orbital["step"] = 1;
        orbital["labels"] = labels;

        Ir77Json spin = Ir77Json::Object();
        spin["index"] = 2;
        spin["min"] = 0;
        spin["max"] = 2;
        spin["step"] = 1;
        Ir77Json spins = Ir77Json::Array();
        spins.Push(Ir77Json(std::string{result.unrestricted ? "\u03b1" : "both"}));
        spins.Push(Ir77Json(std::string{result.unrestricted ? "\u03b2" : "both"}));
        spins.Push(Ir77Json(std::string{result.unrestricted ? "\u03b1 - \u03b2" : "none (closed shell)"}));
        spin["labels"] = spins;

        Ir77Json sliders = Ir77Json::Array();
        sliders.Push(orbital);
        sliders.Push(spin);
        return sliders;
    }

    // a file name with no folder in it -- letters, digits, '-', '_', '.' (not leading)
    static bool PlainName(std::string const& name) {
        if (name.empty() || name.front() == '.' || name.size() > 128) return false;
        for (char const c : name)
            if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '_' && c != '.') return false;
        return true;
    }

    static char const* StateName(Ir77JobState const state) {
        switch (state) {
            case Ir77JobState::Running:
                return "running";
            case Ir77JobState::Done:
                return "done";
            case Ir77JobState::Failed:
                return "failed";
            case Ir77JobState::Cancelled:
                return "cancelled";
            default:
                return "idle";
        }
    }

    // ---- args -----------------------------------------------------------------------------------------------------------------------------

    double Number(Ir77ComputeArgs const& args, char const* key, double const fallback) const {
        double value{0.0};
        return args[key].get(value) == simdjson::SUCCESS ? value : fallback;
    }


    std::string_view Text(Ir77ComputeArgs const& args, char const* key) const {
        std::string_view value{};
        return args[key].get(value) == simdjson::SUCCESS ? value : std::string_view{};
    }


   private:
    std::shared_ptr<IIr77Device> m_device{};  // the module's work -- only this server holds them

    std::shared_ptr<IIr77Program> m_program{};

    bool m_pending{false};  // a run started that Update hasn't reported yet

    std::string m_running{};  // the last run's program name

    std::vector<Ir77ProgramBuffer> m_buffers{};  // the last run's buffers -- compute.buffer

    Ir77Json m_last{};  // the last run's event data -- compute.status gives it to a panel that opens later (null before the first)

    std::string m_library{};  // compute.library.root -- the shader root; the built-in programs are its compute/compute.json

    simdjson::dom::parser m_file_parser{};  // files (library, store) -- apart from m_parser, which holds the message being handled

    bool m_device_tried{false};  // the device is opened on first use, not at startup (a driver can take seconds)

    std::string m_project{};  // compute.project -- where data/ is

    std::function<void(std::string)> m_post{};

    std::unordered_map<std::string, Ir77ComputeOperation, Ir77ComputeOpHash, std::equal_to<>> m_operations{};

    simdjson::dom::parser m_parser{};

    simdjson::dom::parser m_empty_parser{};

    simdjson::dom::object m_empty_args{};

    Ir77Json m_data{};

    std::string m_error{};
};
}  // namespace NSIr77Compute
