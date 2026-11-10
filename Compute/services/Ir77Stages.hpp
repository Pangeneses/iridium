#pragma once

#ifndef CL_TARGET_OPENCL_VERSION
#define CL_TARGET_OPENCL_VERSION 120
#endif
#include <CL/cl.h>
#include <math.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "simdjson.h"

#include "../types/Ir77Types.hpp"
#include "Ir77Linear.hpp"

namespace NSIr77Compute {

// =============================================================================================================================================
// Ir77Stages -- a Compute program: one OpenCL source. Its comment directives declare what it works with (constants, buffers, kernels) and the
// script the host runs between the kernels. Compute is a generic executor: the math -- every formula, every format -- is the program's (its
// kernels, and the libraries it takes in); the engine supplies only buffers, kernels, control and linear algebra. A program is a generator:
// what it makes leaves as data files in <project>/data/ for Fields to draw, in the format the program names.
//
// Directives (comment lines; the compiler skips them):
//   // @include <name>                    another saved program's text, here -- a library (functions, constants, even directives). Each is
//                                         taken in once; a library may include others
//   // @const <type> <name> = <value>     a constant -- the program's settings live here, nowhere else:
//                                           int, double, bool    a number (an expression of numbers and constants above it); in a kernel
//                                                                s.<name> as int / double / int (0, 1)
//                                           string               text a step reads (a file's name, a format's, label words)
//                                           double[], int[]      numbers apart by spaces, commas or ; (a group's end -- an atom's Z x y z,
//                                                                say) -- a buffer of them, s.<name>, and <name>_count its length
//   // @symbol <int|long|double> <name> ...
//                                         symbols a kernel sees with that type (set gives them their values; undeclared ones are doubles)
//   // @buffer <name> [int] <d0> [d1 [d2]] a buffer of doubles (or ints), d0 fastest -- element (a, b, c) at a + d0 (b + d1 c); each dimension
//                                         an expression of numbers and symbols, no spaces (n, n*n, 17+4*atoms)
//   // @kernel <name> <d0> [d1 [d2]]      void <name>(Ir77Stage s) -- one work item an element, s.i its index (0 .. the product - 1)
//   // @report <symbol> ...               symbols the run reports (and every write's summary carries)
//   the script, in order:
//   // @do <step> ...                     one step (below)
//   // @repeat <passes>                   the steps down to @until, again and again -- `iteration` counts (1, 2 ...)
//   // @until <expression>                ... until it's true (sets `converged`), or the passes run out -- loops nest
//
// In a kernel: s.<buffer> is the buffer (__global double* / int*), s.<symbol> a symbol's value, s.i the element. Symbols are the constants,
// the engine's (below) and what `set` makes.
//
// The engine's symbols: iteration converged (int), error (double: extrapolate's) -- and with an orthogonalize step m (int). A @repeat inside
// another counts its own iteration; the outer one's comes back after it.
//
// Steps (a <word> that names a string constant means its text; a matrix's sizes come from its @buffer line -- d0 its columns, d1 its rows,
// d2 its layers):
//   run <kernel>                          the kernel over its elements
//   swap <a> <b>                          two buffers of one size trade contents (a pass's out becomes the next pass's in)
//   fill <buffer> <expression> ...        its elements from 0, one expression each (the rest stay)
//   orthogonalize <S> <X> [rank] [drop]   canonical: S's (n x n) eigenvectors over sqrt(their eigenvalues), those under drop (1e-7) dropped
//                                         -> X (@buffer X <rank> <n>); the rank symbol (m) set first
//   eigen <F> <X> <C> <E>                 per layer of F: X^T F X diagonalized; C = X C' (m x n a layer), E its eigenvalues ascending (m)
//   subspace <F> <C> <E> <first> ...      per layer: F diagonalized again in the space of C's columns from first on (they're orthonormal over
//                                         S) -- those columns and their E replaced, the ones before kept; one first (an expression) a layer
//   extrapolate <F> <R> ...               Pulay's: every F given (each with R, its residual -- the program's measure of how far F is from
//                                         done, any shape) replaced by the mix of its last COMPUTE_EXTRAPOLATE_SIZE values whose residuals
//                                         cancel best, one set of coefficients for all the pairs; symbol error (R's largest |element|)
//   extrapolate reset                     every history forgotten (a new start: the old values no longer apply)
//   set <symbol> = <expression>           + - * / ( ) < > <= >= == != && || !, abs sqrt min max exp log; sum(b) sumsq(b) maxabs(b) of a
//                                         buffer, at(b,i) one element (no spaces inside a step's words)
//   require <expression> "<message>"      the run stops with the message unless the expression holds
//   write <name> <format> <version> <buffer> ... [-summary <name> ...] [-labels <index> <count> "<first>" "<template>"] ...
//                                         <project>/data/<name>.json: {format, version, summary, sliders, values} -- values the buffers one
//                                         after another (floats); summary the @report symbols, then each -summary name (a symbol's value, a
//                                         buffer's values, a string constant's text); sliders: each -labels names a reader parameter's values
//                                         -- "<first>" (when not "") then <count> labels from the template: {#} the row (from 1), {#0} (from
//                                         0), {<buffer>} its element at the row, {<symbol>} its value; after the name %<printf format> (%g
//                                         unless given), @<string constant> (the value picks one of its words, apart by |), ? (nothing when 0)
//   read <name> <buffer>                  <project>/data/<name>.json's values into the buffer, from element 0 (the rest stay)
// =============================================================================================================================================
class Ir77Stages {
   public:
    Ir77Stages(cl_context const context, cl_command_queue const queue, cl_device_id const device) : m_context{context}, m_queue{queue}, m_device{device} {}

    Ir77Stages(Ir77Stages const&) = delete;
    Ir77Stages& operator=(Ir77Stages const&) = delete;

    ~Ir77Stages() { Release(); }

    // its includes taken in, parsed, built and run -- result: the report, the outputs, every buffer as the run left it (a failed script's
    // too). false: error says why.
    bool Run(Ir77ProgramJob const& job, std::atomic<bool> const& cancel, Ir77ProgramResult& result, std::string& error) {
        auto const started = std::chrono::steady_clock::now();
        m_cancel = &cancel;
        m_result = &result;
        m_data = job.data;

        std::vector<Ir77SourceLine> lines{};
        std::set<std::string> taken{};
        if (!Expand(job.source, "", job.library, 0, taken, lines, error) || !Parse(lines, error) || !Build(lines, error)) return false;

        // the constant arrays into their buffers
        Size();
        for (auto const& [name, values] : m_arrays) {
            Ir77StageBuffer& b = *Find(name);
            for (std::size_t k = 0; k < values.size(); k++) {
                if (b.ints) b.integers[k] = static_cast<std::int32_t>(values[k]);
                else b.doubles[k] = values[k];
            }
            Wrote(b);
        }

        bool const ok = Script(0, m_script.size(), error);

        for (std::string const& name : m_reports) result.report.emplace_back(name, Symbol(name));
        Keep();
        result.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
        return ok;
    }

   private:
    struct Ir77StageBuffer {
        std::string name{};
        bool ints{false};
        std::vector<std::string> dims{};
        std::size_t size{0};          // elements, as last allocated
        std::vector<double> doubles{};
        std::vector<std::int32_t> integers{};
        cl_mem memory{nullptr};
        bool host{true};              // the host copy is current
        bool device{false};           // the device copy is current
    };

    struct Ir77StageKernel {
        std::string name{};
        std::vector<std::string> dims{};
        cl_kernel kernel{nullptr};
    };

    struct Ir77StageLine {
        enum class Kind { Do, Repeat, Until } kind{Kind::Do};
        std::string text{};
        std::size_t partner{0};  // repeat <-> until
        std::string where{};     // its line -- "line 12", "gaussian line 40"
    };

    // one line of the program as built -- where it came from: the program itself (origin '') or a library it took in
    struct Ir77SourceLine {
        std::string text{};
        std::string origin{};
        std::uint32_t number{0};
    };

    // ---- the program's text -------------------------------------------------------------------------------------------------------------

    // "line 12" -- the program's own; "gaussian line 40" -- a library's
    static std::string Where(Ir77SourceLine const& line) { return (line.origin.empty() ? "" : line.origin + " ") + "line " + std::to_string(line.number); }

    // The source with every // @include line replaced by the program it names -- from the project's saved programs, each taken in once (a
    // second @include of one is dropped: libraries can include what they need without doubling it). Every line keeps where it came from.
    bool Expand(std::string const& source, std::string const& origin, std::map<std::string, std::string> const& library, std::uint32_t const depth,
                std::set<std::string>& taken, std::vector<Ir77SourceLine>& out, std::string& error) {
        std::istringstream lines{source};
        std::string line{};
        std::uint32_t number = 0;
        while (std::getline(lines, line)) {
            number++;
            Ir77SourceLine here{line, origin, number};
            std::size_t const at = line.find("// @include");
            if (at == std::string::npos) {
                out.push_back(here);
                continue;
            }
            std::string const name = Trim(line.substr(at + 11));
            if (taken.count(name)) continue;
            auto const found = library.find(name);
            if (found == library.end() || name.empty()) {
                error = Where(here) + ": @include " + name + " -- no saved program of that name";
                return false;
            }
            if (depth >= COMPUTE_INCLUDE_DEPTH) {
                error = Where(here) + ": @include " + name + " -- includes nest deeper than " + std::to_string(COMPUTE_INCLUDE_DEPTH);
                return false;
            }
            taken.insert(name);
            if (!Expand(found->second, name, library, depth + 1, taken, out, error)) return false;
        }
        return true;
    }

    bool Parse(std::vector<Ir77SourceLine> const& source, std::string& error) {
        for (char const* name : {"iteration", "converged"}) Engine(name, "int");
        Engine("error", "double");

        std::vector<std::size_t> open{};

        for (Ir77SourceLine const& source_line : source) {
            std::string const& line = source_line.text;
            std::size_t const at = line.find("// @");
            if (at == std::string::npos) continue;
            std::istringstream words{line.substr(at + 4)};
            std::string what{};
            words >> what;
            std::string rest{};
            std::getline(words, rest);
            rest = Trim(rest);
            std::string const here = Where(source_line);
            std::string const where = here + ": ";

            if (what == "const") {
                std::size_t const equals = rest.find('=');
                std::vector<std::string> const w = Words(rest.substr(0, equals == std::string::npos ? rest.size() : equals));
                if (equals == std::string::npos || w.size() != 2 || !Identifier(w[1])) {
                    error = where + "@const <type> <name> = <value>";
                    return false;
                }
                if (!Constant(w[0], w[1], Trim(rest.substr(equals + 1)), error)) {
                    error = where + error;
                    return false;
                }
            } else if (what == "symbol") {
                std::vector<std::string> const w = Words(rest);
                if (w.size() < 2 || (w[0] != "int" && w[0] != "long" && w[0] != "double")) {
                    error = where + "@symbol <int | long | double> <name> ...";
                    return false;
                }
                for (std::size_t k = 1; k < w.size(); k++) {
                    if (!Identifier(w[k]) || m_constants.count(w[k]) || m_strings.count(w[k]) || m_engine.count(w[k])) {
                        error = where + "@symbol " + w[k] + " -- a name of its own (not a constant or the engine's)";
                        return false;
                    }
                    m_symbols.emplace(w[k], 0.0);
                    m_types[w[k]] = w[0];
                }
            } else if (what == "buffer") {
                std::vector<std::string> const w = Words(rest);
                Ir77StageBuffer b{};
                b.name = w[0];
                std::size_t const first = w.size() > 1 && w[1] == "int" ? 2 : 1;
                b.ints = first == 2;
                if (w.size() > first) b.dims.assign(w.begin() + static_cast<std::ptrdiff_t>(first), w.end());
                if (b.dims.empty() || b.dims.size() > 3 || !Identifier(b.name)) {
                    error = where + "@buffer <name> [int] <d0> [d1 [d2]]";
                    return false;
                }
                for (std::string const& d : b.dims) Declare(d);
                m_buffers.push_back(b);
            } else if (what == "kernel") {
                std::vector<std::string> const w = Words(rest);
                if (w.size() < 2 || w.size() > 4 || !Identifier(w[0])) {
                    error = where + "@kernel <name> <d0> [d1 [d2]]";
                    return false;
                }
                Ir77StageKernel k{};
                k.name = w[0];
                k.dims.assign(w.begin() + 1, w.end());
                for (std::string const& d : k.dims) Declare(d);
                m_kernels.push_back(k);
            } else if (what == "report") {
                for (std::string const& w : Words(rest)) {
                    if (w.empty()) continue;
                    m_reports.push_back(w);
                    Declare(w);
                }
            } else if (what == "do") {
                std::vector<std::string> const w = Words(rest);
                if (w[0] == "set" && w.size() >= 2) {
                    if (m_constants.count(w[1]) || w[1] == "iteration" || w[1] == "converged") {
                        error = where + w[1] + (m_constants.count(w[1]) ? " is a constant" : " is the engine's") + " -- set makes symbols of its own";
                        return false;
                    }
                    Declare(w[1]);
                }
                m_script.push_back({Ir77StageLine::Kind::Do, rest, 0, here});
            } else if (what == "repeat") {
                open.push_back(m_script.size());
                m_script.push_back({Ir77StageLine::Kind::Repeat, rest, 0, here});
            } else if (what == "until") {
                if (open.empty()) {
                    error = where + "@until without @repeat";
                    return false;
                }
                m_script.push_back({Ir77StageLine::Kind::Until, rest, open.back(), here});
                m_script[open.back()].partner = m_script.size() - 1;
                open.pop_back();
            } else {
                error = where + "no directive @" + what + " -- include const symbol buffer kernel report do repeat until";
                return false;
            }
        }

        if (!open.empty()) {
            error = m_script[open.back()].where + ": @repeat without @until";
            return false;
        }
        if (m_script.empty()) {
            error = "no script -- a program runs its // @do steps (none found; a library has none -- a program takes it in with // @include)";
            return false;
        }

        // the orthogonalize step's rank symbols -- m unless it names its own (the engine's only when the script has the step: a program
        // without it may name its own m)
        bool orthogonalize = false;
        std::vector<std::string> ranks{};
        for (Ir77StageLine const& l : m_script) {
            std::vector<std::string> const w = l.kind == Ir77StageLine::Kind::Do ? Words(l.text) : std::vector<std::string>{""};
            if (w[0] == "orthogonalize" && w.size() > 3) ranks.push_back(w[3]);
            else orthogonalize = orthogonalize || w[0] == "orthogonalize";
        }
        for (std::string const& rank : ranks) {
            if (!Identifier(rank) || m_constants.count(rank) || m_engine.count(rank)) {
                error = "orthogonalize's rank '" + rank + "' -- a name of its own (not a constant or the engine's)";
                return false;
            }
            m_symbols[rank] = 0.0;
            m_types[rank] = "int";
        }
        if (orthogonalize) {
            if (m_constants.count("m") || m_strings.count("m")) {
                error = "m is the orthogonalize step's rank -- pick another name, or give the step its own rank symbol";
                return false;
            }
            Engine("m", "int");
        }

        for (std::size_t a = 0; a < m_buffers.size(); a++)
            for (std::size_t b = 0; b < a; b++)
                if (m_buffers[a].name == m_buffers[b].name) {
                    error = "buffer " + m_buffers[a].name + " declared twice";
                    return false;
                }
        // the names the generated code uses itself -- the kernel's argument s, its element s.i, the wrapper's first / count / symbols / g
        for (Ir77StageBuffer const& b : m_buffers)
            if (b.name == "s" || b.name == "i" || b.name == "first" || b.name == "count" || b.name == "symbols" || b.name == "g") {
                error = "buffer " + b.name + " -- the name is the generated code's (s, i, first, count, symbols, g); pick another";
                return false;
            }
        for (auto const& [name, value] : m_symbols)
            if (name == "s" || name == "i") {
                error = "symbol " + name + " -- the name is the generated code's (s, i); pick another";
                return false;
            }
        for (Ir77StageBuffer const& b : m_buffers)
            if (m_symbols.count(b.name) || m_strings.count(b.name)) {
                error = b.name + " is both a buffer and a constant or symbol";
                return false;
            }
        return true;
    }

    void Engine(char const* name, char const* type) {
        m_symbols[name] = 0.0;
        m_types[name] = type;
        m_engine.insert(name);
    }

    // one @const -- false: error says what's wrong with it
    bool Constant(std::string const& type, std::string const& name, std::string const& value, std::string& error) {
        if (m_engine.count(name)) {
            error = name + " is the engine's symbol -- pick another name";
            return false;
        }
        bool const taken = m_constants.count(name) || m_strings.count(name);
        if (taken) {
            error = name + " is already declared";
            return false;
        }
        if (type == "string") {
            m_strings[name] = value;
            return true;
        }
        if (type == "double[]" || type == "int[]") {
            // a constant array -- a buffer filled before the script, and <name>_count its length
            std::vector<double> values{};
            std::string word{};
            std::string spaced = value;
            std::replace(spaced.begin(), spaced.end(), ';', ' ');  // ; groups (an atom's Z x y z, say) -- read as a space
            std::istringstream words{spaced};
            while (std::getline(words, word, ' ')) {
                std::istringstream parts{word};
                std::string part{};
                while (std::getline(parts, part, ',')) {
                    part = Trim(part);
                    if (part.empty()) continue;
                    char* end = nullptr;
                    double const v = std::strtod(part.c_str(), &end);
                    if (!end || *end != '\0' || (type == "int[]" && v != std::floor(v))) {
                        error = type + " " + name + ": \"" + part + "\" -- numbers apart by spaces, commas or ;" + (type == "int[]" ? ", whole" : "");
                        return false;
                    }
                    values.push_back(v);
                }
            }
            std::string const count = name + "_count";
            if (m_symbols.count(count)) {
                error = count + " is already declared";
                return false;
            }
            Ir77StageBuffer b{};
            b.name = name;
            b.ints = type == "int[]";
            b.dims = {std::to_string(values.size())};
            m_buffers.push_back(b);
            m_arrays[name] = values;
            m_symbols[count] = static_cast<double>(values.size());
            m_types[count] = "int";
            m_constants.insert(count);
            return true;
        }

        double number = 0.0;
        if (type == "bool") {
            if (value == "true" || value == "1") number = 1.0;
            else if (value == "false" || value == "0") number = 0.0;
            else {
                error = "a bool is true or false -- not \"" + value + "\"";
                return false;
            }
        } else if (type == "int" || type == "double") {
            m_constant_only = true;
            bool const ok = Evaluate(value, number, error);
            m_constant_only = false;
            if (!ok) return false;
            if (type == "int" && number != std::floor(number)) {
                error = "int " + name + " = " + std::to_string(number) + " -- not a whole number";
                return false;
            }
        } else {
            error = "@const type " + type + " -- int, double, bool, string, double[] or int[]";
            return false;
        }
        m_symbols[name] = number;
        m_types[name] = type == "double" ? "double" : "int";
        m_constants.insert(name);
        return true;
    }

    // a dimension's words -- each symbol in it becomes one (a double unless declared otherwise)
    void Declare(std::string const& text) {
        std::string word{};
        for (char const c : text + " ") {
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
                word += c;
            } else {
                if (!word.empty() && !std::isdigit(static_cast<unsigned char>(word[0])) && !Function(word)) {
                    m_symbols.emplace(word, 0.0);
                    m_types.emplace(word, "double");
                }
                word.clear();
            }
        }
    }

    // the OpenCL program: the prelude (its struct of symbols and buffers), the program with its libraries (each line where it came from --
    // the compiler's messages name it), a wrapper kernel per @kernel
    bool Build(std::vector<Ir77SourceLine> const& source, std::string& error) {
        std::ostringstream text{};
        text << "#pragma OPENCL EXTENSION cl_khr_fp64 : enable\n";
        text << "typedef struct {\n    uint i;\n";
        for (auto const& [name, value] : m_symbols) text << "    " << m_types[name] << " " << name << ";\n";
        for (Ir77StageBuffer const& b : m_buffers) text << "    __global " << (b.ints ? "int" : "double") << "* " << b.name << ";\n";
        text << "} Ir77Stage;\n";

        // the program's lines as "<source>", a library's as "library:<name>" -- a #line wherever the run of lines breaks
        Ir77SourceLine const* last = nullptr;
        for (Ir77SourceLine const& line : source) {
            if (!last || last->origin != line.origin || last->number + 1 != line.number)
                text << "#line " << line.number << " \"" << (line.origin.empty() ? std::string{"<source>"} : "library:" + line.origin) << "\"\n";
            text << line.text << "\n";
            last = &line;
        }

        for (Ir77StageKernel const& k : m_kernels) {
            text << "__kernel void Ir77Stage_" << k.name << "(ulong first, ulong count, __global const double* symbols";
            for (Ir77StageBuffer const& b : m_buffers) text << ", __global " << (b.ints ? "int" : "double") << "* " << b.name;
            text << ") {\n    ulong g = get_global_id(0);\n    if (g >= count) return;\n    Ir77Stage s;\n    s.i = (uint)(first + g);\n";
            std::size_t index = 0;
            for (auto const& [name, value] : m_symbols) text << "    s." << name << " = (" << m_types[name] << ")symbols[" << index++ << "];\n";
            for (Ir77StageBuffer const& b : m_buffers) text << "    s." << b.name << " = " << b.name << ";\n";
            text << "    " << k.name << "(s);\n}\n";
        }

        std::string const all = text.str();
        char const* chars = all.c_str();
        std::size_t const length = all.size();
        cl_int status = CL_SUCCESS;
        m_program = clCreateProgramWithSource(m_context, 1, &chars, &length, &status);
        if (status != CL_SUCCESS) {
            error = "clCreateProgramWithSource: OpenCL " + std::to_string(status);
            return false;
        }
        if (clBuildProgram(m_program, 1, &m_device, "-cl-std=CL1.2", nullptr, nullptr) != CL_SUCCESS) {
            std::size_t size = 0;
            clGetProgramBuildInfo(m_program, m_device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &size);
            error.assign(size, '\0');
            clGetProgramBuildInfo(m_program, m_device, CL_PROGRAM_BUILD_LOG, size, error.data(), nullptr);
            while (!error.empty() && error.back() == '\0') error.pop_back();
            if (error.empty()) error = "the program didn't build (no log)";
            // the driver's name for the source ("<source>:7:43:", "/tmp/OCL123.cl:7:43:") -> "line 7:43:" -- the program's own lines; a
            // library's ("library:gaussian:40:3:") -> "gaussian line 40:3:"
            error = std::regex_replace(error, std::regex{R"(\S*library:([A-Za-z0-9_.\-]+):(\d+):(\d+):)"}, "$1 line $2:$3:");
            error = std::regex_replace(error, std::regex{R"(\S*(\.cl|<kernel>|<source>):(\d+):(\d+):)"}, "line $2:$3:");
            return false;
        }
        for (Ir77StageKernel& k : m_kernels) {
            k.kernel = clCreateKernel(m_program, ("Ir77Stage_" + k.name).c_str(), &status);
            if (status != CL_SUCCESS) {
                error = "kernel " + k.name + ": OpenCL " + std::to_string(status) + " -- does the program define void " + k.name + "(Ir77Stage s)?";
                return false;
            }
        }
        return true;
    }

    // ---- the script -------------------------------------------------------------------------------------------------------------------

    bool Script(std::size_t const from, std::size_t const to, std::string& error) {
        for (std::size_t at = from; at < to; at++) {
            if (*m_cancel) {
                error = "cancelled";
                return false;
            }
            Ir77StageLine const& line = m_script[at];
            std::string const where = line.where + ": ";

            if (line.kind == Ir77StageLine::Kind::Repeat) {
                double passes = 0.0;
                if (!Evaluate(line.text, passes, error)) {
                    error = where + "@repeat " + line.text + " -- " + error;
                    return false;
                }
                std::size_t const end = line.partner;
                double const outer = m_symbols["iteration"];  // a loop inside another -- the outer one's count comes back after it
                m_depth++;
                bool done = false;
                for (std::uint32_t pass = 1; pass <= static_cast<std::uint32_t>(std::max(passes, 1.0)); pass++) {
                    m_symbols["iteration"] = pass;
                    if (!Script(at + 1, end, error)) {
                        m_depth--;
                        return false;
                    }
                    double holds = 0.0;
                    if (!Evaluate(m_script[end].text, holds, error)) {
                        error = m_script[end].where + ": @until " + m_script[end].text + " -- " + error;
                        m_depth--;
                        return false;
                    }
                    if (holds != 0.0) {
                        done = true;
                        break;
                    }
                }
                m_symbols["converged"] = done ? 1.0 : 0.0;  // this loop's -- an inner loop's own answer doesn't outlive it
                if (--m_depth > 0) m_symbols["iteration"] = outer;
                at = end;
                continue;
            }

            if (!Step(line.text, error)) {
                error = where + (Words(line.text)[0] == "require" ? error : "@do " + line.text + " -- " + error);
                return false;
            }
        }
        return true;
    }

    bool Step(std::string const& text, std::string& error) {
        std::vector<std::string> const w = Words(text);
        std::string const& what = w.front();
        auto need = [&](std::size_t count, char const* form) {
            if (w.size() - 1 >= count) return true;
            error = std::string{form};
            return false;
        };

        if (what == "run") return need(1, "run <kernel>") && RunKernel(w[1], error);
        if (what == "swap") return need(2, "swap <a> <b>") && Swap(w[1], w[2], error);
        if (what == "fill") return need(2, "fill <buffer> <expression> ...") && Fill(w[1], std::vector<std::string>(w.begin() + 2, w.end()), error);
        if (what == "orthogonalize") {
            double threshold = COMPUTE_ORTHOGONAL_DROP;
            if (!need(2, "orthogonalize <S> <X> [rank] [threshold]") || (w.size() > 4 && !Evaluate(w[4], threshold, error))) return false;
            return Orthogonalize(w[1], w[2], w.size() > 3 ? w[3] : "m", threshold, error);
        }
        if (what == "eigen") return need(4, "eigen <F> <X> <C> <E>") && Eigen(w[1], w[2], w[3], w[4], error);
        if (what == "subspace") return need(4, "subspace <F> <C> <E> <first> ...") && Subspace(w[1], w[2], w[3], std::vector<std::string>(w.begin() + 4, w.end()), error);
        if (what == "extrapolate" && w.size() == 2 && w[1] == "reset") {
            m_histories.clear();
            m_residuals.clear();
            return true;
        }
        if (what == "extrapolate") return need(2, "extrapolate <F> <R> ...") && Extrapolate(std::vector<std::string>(w.begin() + 1, w.end()), text, error);
        if (what == "require") {
            std::size_t const quote = text.find('"');
            std::string const condition = text.substr(7, quote == std::string::npos ? std::string::npos : quote - 7);
            double holds = 0.0;
            if (!Evaluate(Trim(condition), holds, error)) return false;
            if (holds != 0.0) return true;
            std::string message = quote == std::string::npos ? "" : text.substr(quote + 1);
            if (!message.empty() && message.back() == '"') message.pop_back();
            error = message.empty() ? "not so: " + Trim(condition) : message;
            return false;
        }
        if (what == "set") {
            std::size_t const equals = text.find('=');
            if (w.size() < 3 || equals == std::string::npos) {
                error = "set <symbol> = <expression>";
                return false;
            }
            double value = 0.0;
            if (!Evaluate(text.substr(equals + 1), value, error)) return false;
            m_symbols[w[1]] = value;
            return true;
        }
        if (what == "write") return need(4, "write <name> <format> <version> <buffer> ... [-summary <name> ...] [-labels <index> <count> \"<first>\" \"<template>\"]") && Write(text, error);
        if (what == "read") return need(2, "read <name> <buffer>") && Read(Text(w[1]), w[2], error);

        error = "no step '" + what + "' -- run swap fill orthogonalize eigen subspace extrapolate set require write read";
        return false;
    }

    // a word -- a string constant's text when it names one, else itself
    std::string Text(std::string const& word) const {
        auto const found = m_strings.find(word);
        return found == m_strings.end() ? word : found->second;
    }

    // ---- buffers ------------------------------------------------------------------------------------------------------------------------

    Ir77StageBuffer* Find(std::string const& name) {
        for (Ir77StageBuffer& b : m_buffers)
            if (b.name == name) return &b;
        return nullptr;
    }

    double Symbol(std::string const& name) const {
        auto const found = m_symbols.find(name);
        return found == m_symbols.end() ? 0.0 : found->second;
    }

    // a dimension's value -- an expression of numbers and symbols (n, n*n, 17+4*atoms ...); one that can't be read is 0
    std::size_t Dimension(std::string const& text) const {
        double value = 0.0;
        std::string ignored{};
        if (!const_cast<Ir77Stages*>(this)->Evaluate(text, value, ignored)) return 0;
        return static_cast<std::size_t>(std::max(value, 0.0));
    }

    std::size_t Elements(std::vector<std::string> const& dims) const {
        std::size_t count = 1;
        for (std::string const& d : dims) count *= Dimension(d);
        return count;
    }

    // every buffer at its size now (symbols may have changed) -- a resized one starts at zero
    void Size() {
        for (Ir77StageBuffer& b : m_buffers) {
            std::size_t const size = Elements(b.dims);
            if (size == b.size) continue;
            b.size = size;
            if (b.ints) b.integers.assign(size, 0);
            else b.doubles.assign(size, 0.0);
            if (b.memory) clReleaseMemObject(b.memory);
            b.memory = nullptr;
            b.host = true;
            b.device = false;
        }
    }

    // the host copy, current -- read back if a kernel wrote since
    bool Host(Ir77StageBuffer& b, std::string& error) {
        Size();
        if (b.host) return true;
        std::size_t const bytes = b.size * (b.ints ? sizeof(std::int32_t) : sizeof(double));
        cl_int const status = bytes == 0 ? CL_SUCCESS
                                         : clEnqueueReadBuffer(m_queue, b.memory, CL_TRUE, 0, bytes,
                                                               b.ints ? static_cast<void*>(b.integers.data()) : static_cast<void*>(b.doubles.data()), 0, nullptr, nullptr);
        if (status != CL_SUCCESS) {
            error = "reading " + b.name + ": OpenCL " + std::to_string(status);
            return false;
        }
        b.host = true;
        return true;
    }

    // the host wrote it -- the device's copy is stale
    static void Wrote(Ir77StageBuffer& b) {
        b.host = true;
        b.device = false;
    }

    bool Device(Ir77StageBuffer& b, std::string& error) {
        Size();
        std::size_t const bytes = std::max<std::size_t>(b.size * (b.ints ? sizeof(std::int32_t) : sizeof(double)), 8);
        cl_int status = CL_SUCCESS;
        if (!b.memory) {
            b.memory = clCreateBuffer(m_context, CL_MEM_READ_WRITE, bytes, nullptr, &status);
            if (status != CL_SUCCESS) {
                error = "buffer " + b.name + " (" + std::to_string(bytes) + " bytes): OpenCL " + std::to_string(status);
                return false;
            }
            b.device = false;
        }
        if (b.device) return true;
        if (b.size > 0) {
            status = clEnqueueWriteBuffer(m_queue, b.memory, CL_TRUE, 0, b.size * (b.ints ? sizeof(std::int32_t) : sizeof(double)),
                                          b.ints ? static_cast<void const*>(b.integers.data()) : static_cast<void const*>(b.doubles.data()), 0, nullptr, nullptr);
            if (status != CL_SUCCESS) {
                error = "writing " + b.name + ": OpenCL " + std::to_string(status);
                return false;
            }
        }
        b.device = true;
        return true;
    }

    // a buffer the step reads / writes on the host, at least `size` elements -- false: error says why
    std::vector<double>* Doubles(std::string const& name, std::size_t const size, std::string& error) {
        Ir77StageBuffer* b = Find(name);
        if (!b || b->ints) {
            error = "no double buffer " + name;
            return nullptr;
        }
        if (!Host(*b, error)) return nullptr;
        if (b->size < size) {
            error = "buffer " + name + " holds " + std::to_string(b->size) + " -- the step needs " + std::to_string(size) + " (its @buffer dimensions)";
            return nullptr;
        }
        return &b->doubles;
    }

    // every buffer as the run left it -- for the panel's inspector (values only up to COMPUTE_KEEP_LIMIT elements; above it, the shape alone)
    void Keep() {
        for (Ir77StageBuffer& b : m_buffers) {
            std::string ignored{};
            Ir77ProgramBuffer kept{};
            kept.name = b.name;
            kept.ints = b.ints;
            for (std::string const& d : b.dims) kept.dims.push_back(Dimension(d));
            if (!Host(b, ignored)) continue;
            kept.size = b.size;
            bool const values = b.size <= COMPUTE_KEEP_LIMIT;
            if (values) kept.values.reserve(b.size);
            for (std::size_t k = 0; k < b.size; k++) {
                double const v = b.ints ? static_cast<double>(b.integers[k]) : b.doubles[k];
                kept.minimum = k == 0 ? v : std::min(kept.minimum, v);
                kept.maximum = k == 0 ? v : std::max(kept.maximum, v);
                if (values) kept.values.push_back(v);
            }
            m_result->buffers.push_back(std::move(kept));
        }
    }

    // ---- steps --------------------------------------------------------------------------------------------------------------------------

    bool RunKernel(std::string const& name, std::string& error) {
        Ir77StageKernel* kernel = nullptr;
        for (Ir77StageKernel& k : m_kernels)
            if (k.name == name) kernel = &k;
        if (!kernel) {
            error = "no @kernel " + name;
            return false;
        }

        Size();
        for (Ir77StageBuffer& b : m_buffers)
            if (!Device(b, error)) return false;

        std::vector<double> symbols{};
        for (auto const& [symbol, value] : m_symbols) symbols.push_back(value);
        cl_int status = CL_SUCCESS;
        cl_mem const table = clCreateBuffer(m_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, std::max<std::size_t>(symbols.size(), 1) * sizeof(double),
                                            symbols.data(), &status);
        if (status != CL_SUCCESS) {
            error = "symbols: OpenCL " + std::to_string(status);
            return false;
        }

        std::uint64_t const total = Elements(kernel->dims);
        bool ok = true;
        for (std::uint64_t first = 0; ok && first < total; first += COMPUTE_STAGE_CHUNK) {
            if (*m_cancel) {
                error = "cancelled";
                ok = false;
                break;
            }
            cl_ulong const from = first, count = std::min<std::uint64_t>(COMPUTE_STAGE_CHUNK, total - first);
            clSetKernelArg(kernel->kernel, 0, sizeof(cl_ulong), &from);
            clSetKernelArg(kernel->kernel, 1, sizeof(cl_ulong), &count);
            clSetKernelArg(kernel->kernel, 2, sizeof(cl_mem), &table);
            cl_uint arg = 3;
            for (Ir77StageBuffer const& b : m_buffers) clSetKernelArg(kernel->kernel, arg++, sizeof(cl_mem), &b.memory);

            std::size_t const local = 64;
            std::size_t const global = static_cast<std::size_t>((count + local - 1) / local * local);
            if ((status = clEnqueueNDRangeKernel(m_queue, kernel->kernel, 1, nullptr, &global, &local, 0, nullptr, nullptr)) != CL_SUCCESS ||
                (status = clFinish(m_queue)) != CL_SUCCESS) {
                error = "kernel " + name + ": OpenCL " + std::to_string(status);
                ok = false;
            }
        }
        clReleaseMemObject(table);

        // the kernel may have written any buffer -- their host copies are stale
        for (Ir77StageBuffer& b : m_buffers) b.host = false;
        return ok;
    }

    bool Swap(std::string const& a_name, std::string const& b_name, std::string& error) {
        Ir77StageBuffer* a = Find(a_name);
        Ir77StageBuffer* b = Find(b_name);
        Size();
        if (!a || !b || a == b || a->ints != b->ints || a->size != b->size) {
            error = "swap takes two different buffers of one kind and size";
            return false;
        }
        std::swap(a->doubles, b->doubles);
        std::swap(a->integers, b->integers);
        std::swap(a->memory, b->memory);
        std::swap(a->host, b->host);
        std::swap(a->device, b->device);
        return true;
    }

    // a matrix buffer's shape from its @buffer line -- cols (d0), rows (d1), layers (d2, or 1); false: error says why
    bool Shape(std::string const& name, std::size_t& cols, std::size_t& rows, std::size_t& layers, std::string& error) {
        Ir77StageBuffer* b = Find(name);
        if (!b || b->ints) {
            error = "no double buffer " + name;
            return false;
        }
        cols = Dimension(b->dims[0]);
        rows = b->dims.size() > 1 ? Dimension(b->dims[1]) : 1;
        layers = b->dims.size() > 2 ? Dimension(b->dims[2]) : 1;
        return true;
    }

    // a layer of a host buffer -- [layer * size, + size)
    static std::vector<double> Layer(std::vector<double> const& v, std::size_t const layer, std::size_t const size) {
        return std::vector<double>(v.begin() + static_cast<std::ptrdiff_t>(layer * size), v.begin() + static_cast<std::ptrdiff_t>((layer + 1) * size));
    }

    // S (n x n) -> X (rank x n -- @buffer X <rank> <n>): S's eigenvectors over sqrt(their eigenvalues), those under the threshold dropped;
    // the rank symbol set first, so X is sized by it
    bool Orthogonalize(std::string const& s_name, std::string const& x_name, std::string const& rank, double const threshold, std::string& error) {
        std::size_t n = 0, rows = 0, layers = 0;
        if (!Shape(s_name, n, rows, layers, error)) return false;
        if (rows != n) {
            error = s_name + " is " + std::to_string(n) + " x " + std::to_string(rows) + " -- orthogonalize takes a square S";
            return false;
        }
        std::vector<double>* s = Doubles(s_name, n * n, error);
        if (!s) return false;

        std::vector<double> sv, su;
        Ir77Linear::Eigen(Layer(*s, 0, n * n), n, sv, su);
        std::vector<std::size_t> kept{};
        for (std::size_t i = 0; i < n; i++)
            if (sv[i] > threshold) kept.push_back(i);
        std::size_t const m = kept.size();
        m_symbols[rank] = static_cast<double>(m);

        std::size_t cols = 0, x_rows = 0, x_layers = 0;
        if (!Shape(x_name, cols, x_rows, x_layers, error)) return false;
        if (cols != m || x_rows != n) {
            error = "@buffer " + x_name + " must be " + rank + " " + std::to_string(n) + " (rank x functions)";
            return false;
        }
        std::vector<double>* x = Doubles(x_name, n * m, error);
        if (!x) return false;
        for (std::size_t i = 0; i < n; i++)
            for (std::size_t c = 0; c < m; c++) (*x)[i * m + c] = su[i * n + kept[c]] / sqrt(sv[kept[c]]);
        Wrote(*Find(x_name));
        return true;
    }

    // per layer of F (n x n): X^T F X diagonalized; C = X C' (m x n a layer), E its eigenvalues ascending (m a layer)
    bool Eigen(std::string const& f_name, std::string const& x_name, std::string const& c_name, std::string const& e_name, std::string& error) {
        std::size_t m = 0, n = 0, x_layers = 0, fn = 0, f_rows = 0, layers = 0;
        if (!Shape(x_name, m, n, x_layers, error) || !Shape(f_name, fn, f_rows, layers, error)) return false;
        if (fn != n || f_rows != n) {
            error = f_name + " must be " + std::to_string(n) + " x " + std::to_string(n) + " a layer (" + x_name + "'s functions)";
            return false;
        }
        std::vector<double>* f = Doubles(f_name, layers * n * n, error);
        std::vector<double>* x = f ? Doubles(x_name, n * m, error) : nullptr;
        std::vector<double>* c = x ? Doubles(c_name, layers * n * m, error) : nullptr;
        std::vector<double>* e = c ? Doubles(e_name, layers * m, error) : nullptr;
        if (!e) return false;

        std::vector<double> const xm = Layer(*x, 0, n * m);
        std::vector<double> const xt = Ir77Linear::Transpose(xm, n, m);
        for (std::size_t l = 0; l < layers; l++) {
            std::vector<double> const fp = Ir77Linear::Multiply(Ir77Linear::Multiply(xt, Layer(*f, l, n * n), m, n, n), xm, m, n, m);
            std::vector<double> w, v;
            Ir77Linear::Eigen(fp, m, w, v);
            std::vector<double> const cl = Ir77Linear::Multiply(xm, v, n, m, m);
            std::copy(cl.begin(), cl.end(), c->begin() + static_cast<std::ptrdiff_t>(l * n * m));
            std::copy(w.begin(), w.end(), e->begin() + static_cast<std::ptrdiff_t>(l * m));
        }
        Wrote(*Find(c_name));
        Wrote(*Find(e_name));
        return true;
    }

    // per layer: F (n x n) diagonalized again in the space of C's columns from `first` on (m x n a layer, orthonormal over S) -- those
    // columns and their E replaced, the ones before kept. One `first` (an expression) a layer.
    bool Subspace(std::string const& f_name, std::string const& c_name, std::string const& e_name, std::vector<std::string> const& firsts, std::string& error) {
        std::size_t m = 0, n = 0, layers = 0, fn = 0, f_rows = 0, f_layers = 0;
        if (!Shape(c_name, m, n, layers, error) || !Shape(f_name, fn, f_rows, f_layers, error)) return false;
        if (fn != n || f_rows != n || f_layers < layers) {
            error = f_name + " must be " + std::to_string(n) + " x " + std::to_string(n) + " x " + std::to_string(layers) + " (" + c_name + "'s)";
            return false;
        }
        if (firsts.size() < layers) {
            error = "subspace needs a first column for each of " + c_name + "'s " + std::to_string(layers) + " layers";
            return false;
        }
        std::vector<double>* f = Doubles(f_name, layers * n * n, error);
        std::vector<double>* c = f ? Doubles(c_name, layers * n * m, error) : nullptr;
        std::vector<double>* e = c ? Doubles(e_name, layers * m, error) : nullptr;
        if (!e) return false;

        for (std::size_t l = 0; l < layers; l++) {
            double first_value = 0.0;
            if (!Evaluate(firsts[l], first_value, error)) return false;
            std::size_t const first = static_cast<std::size_t>(std::clamp(first_value, 0.0, static_cast<double>(m)));
            std::size_t const size = m - first;
            if (size == 0) continue;

            std::vector<double> v(n * size);
            for (std::size_t i = 0; i < n; i++)
                for (std::size_t k = 0; k < size; k++) v[i * size + k] = (*c)[l * n * m + i * m + first + k];
            std::vector<double> const vt = Ir77Linear::Transpose(v, n, size);
            std::vector<double> const sub = Ir77Linear::Multiply(Ir77Linear::Multiply(vt, Layer(*f, l, n * n), size, n, n), v, size, n, size);
            std::vector<double> w, u;
            Ir77Linear::Eigen(sub, size, w, u);
            std::vector<double> const rotated = Ir77Linear::Multiply(v, u, n, size, size);
            for (std::size_t k = 0; k < size; k++) {
                (*e)[l * m + first + k] = w[k];
                for (std::size_t i = 0; i < n; i++) (*c)[l * n * m + i * m + first + k] = rotated[i * size + k];
            }
        }
        Wrote(*Find(c_name));
        Wrote(*Find(e_name));
        return true;
    }

    // Pulay's extrapolation over pairs of F and R at once (one set of coefficients): each F's whole buffer the value, its R's the residual;
    // symbol error -- R's largest |element|. A history a step: the step's own words.
    bool Extrapolate(std::vector<std::string> const& names, std::string const& key, std::string& error) {
        if (names.size() % 2 != 0) {
            error = "extrapolate takes pairs: F R ...";
            return false;
        }
        std::vector<std::vector<double>*> values{};
        std::vector<double> stacked{}, residual{};
        double largest = 0.0;

        for (std::size_t g = 0; g < names.size(); g += 2) {
            Ir77StageBuffer* f_buffer = Find(names[g]);
            Ir77StageBuffer* r_buffer = Find(names[g + 1]);
            if (!f_buffer || !r_buffer || f_buffer->ints || r_buffer->ints || f_buffer == r_buffer) {
                error = names[g] + " and " + names[g + 1] + " -- two different double buffers (the value and its residual)";
                return false;
            }
            std::vector<double>* f = Doubles(names[g], 0, error);
            std::vector<double>* r = f ? Doubles(names[g + 1], 0, error) : nullptr;
            if (!r) return false;

            for (double const v : *r) largest = std::max(largest, fabs(v));
            residual.insert(residual.end(), r->begin(), r->end());
            stacked.insert(stacked.end(), f->begin(), f->end());
            values.push_back(f);
        }
        m_symbols["error"] = largest;

        std::vector<std::vector<double>>& history = m_histories[key];
        std::vector<std::vector<double>>& residuals = m_residuals[key];
        if (!history.empty() && (history.back().size() != stacked.size() || residuals.back().size() != residual.size())) {
            history.clear();  // the buffers changed size -- the old values don't apply
            residuals.clear();
        }
        history.push_back(stacked);
        residuals.push_back(residual);
        if (history.size() > COMPUTE_EXTRAPOLATE_SIZE) {
            history.erase(history.begin());
            residuals.erase(residuals.begin());
        }
        if (history.size() < 2) return true;

        // B c = (0 .. 0, -1), B_ij = r_i . r_j bordered by -1
        std::size_t const count = history.size(), size = count + 1;
        std::vector<double> b(size * size, 0.0), rhs(size, 0.0), coefficients;
        for (std::size_t i = 0; i < count; i++)
            for (std::size_t j = 0; j < count; j++) {
                double dot = 0.0;
                for (std::size_t k = 0; k < residuals[i].size(); k++) dot += residuals[i][k] * residuals[j][k];
                b[i * size + j] = dot;
            }
        for (std::size_t i = 0; i < count; i++) b[i * size + count] = b[count * size + i] = -1.0;
        rhs[count] = -1.0;
        if (!Ir77Linear::Solve(b, rhs, size, coefficients)) return true;  // singular -- this pass keeps its own F

        std::size_t at = 0;
        for (std::size_t g = 0; g < values.size(); g++) {
            std::vector<double>& f = *values[g];
            for (std::size_t k = 0; k < f.size(); k++) {
                double sum = 0.0;
                for (std::size_t i = 0; i < count; i++) sum += coefficients[i] * history[i][at + k];
                f[k] = sum;
            }
            at += f.size();
            Wrote(*Find(names[g * 2]));
        }
        return true;
    }

    static bool FileName(std::string const& name, std::string& error) {
        bool ok = !name.empty() && name.front() != '.' && name.size() <= 128;
        for (char const c : name) ok = ok && (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.');
        if (!ok) error = "output name '" + name + "' -- letters, digits, '-', '_' and '.' only";
        return ok;
    }

    // a buffer's values as doubles (ints converted) -- false: error says why
    bool Values(std::string const& name, std::vector<double>& out, std::string& error) {
        Ir77StageBuffer* b = Find(name);
        if (!b) {
            error = "no buffer " + name;
            return false;
        }
        if (!Host(*b, error)) return false;
        if (b->ints) out.assign(b->integers.begin(), b->integers.end());
        else out = b->doubles;
        return true;
    }

    // fill <buffer> <expression> ... -- its elements from 0, one expression each
    bool Fill(std::string const& name, std::vector<std::string> const& expressions, std::string& error) {
        Ir77StageBuffer* b = Find(name);
        if (!b) {
            error = "no buffer " + name;
            return false;
        }
        if (!Host(*b, error)) return false;
        if (expressions.size() > b->size) {
            error = std::to_string(expressions.size()) + " values -- " + name + " holds " + std::to_string(b->size);
            return false;
        }
        for (std::size_t k = 0; k < expressions.size(); k++) {
            double value = 0.0;
            if (!Evaluate(expressions[k], value, error)) return false;
            if (b->ints) b->integers[k] = static_cast<std::int32_t>(std::lround(value));
            else b->doubles[k] = value;
        }
        Wrote(*b);
        return true;
    }

    // write <name> <format> <version> <buffer> ... [-summary <name> ...] [-labels <index> <count> "<first>" "<template>"] ...
    bool Write(std::string const& text, std::string& error) {
        std::vector<std::string> const w = Tokens(text);
        Ir77ProgramOutput output{};
        output.name = Text(w[1]);
        output.format = Text(w[2]);
        if (!FileName(output.name, error)) return false;
        double version = 0.0;
        if (!Evaluate(w[3], version, error)) return false;
        output.version = static_cast<std::uint32_t>(std::max(version, 0.0));

        for (std::string const& symbol : m_reports) output.summary.push_back({symbol, Ir77ProgramSummary::Kind::Number, {Symbol(symbol)}, {}});

        std::size_t at = 4;
        for (; at < w.size() && w[at].front() != '-'; at++) {
            std::vector<double> values{};
            if (!Values(w[at], values, error)) return false;
            if (output.values.size() + values.size() > COMPUTE_WRITE_LIMIT) {
                error = "more than " + std::to_string(COMPUTE_WRITE_LIMIT) + " values";
                return false;
            }
            for (double const v : values) output.values.push_back(static_cast<float>(v));
        }

        while (at < w.size()) {
            std::string const& option = w[at++];
            if (option == "-summary") {
                for (; at < w.size() && w[at].front() != '-'; at++) {
                    std::string const& name = w[at];
                    if (m_strings.count(name)) {
                        output.summary.push_back({name, Ir77ProgramSummary::Kind::Text, {}, m_strings[name]});
                    } else if (Find(name)) {
                        Ir77ProgramSummary entry{name, Ir77ProgramSummary::Kind::List, {}, {}};
                        if (!Values(name, entry.values, error)) return false;
                        output.summary.push_back(entry);
                    } else if (m_symbols.count(name)) {
                        output.summary.push_back({name, Ir77ProgramSummary::Kind::Number, {Symbol(name)}, {}});
                    } else {
                        error = "-summary " + name + " -- no symbol, buffer or string constant of that name";
                        return false;
                    }
                }
            } else if (option == "-labels") {
                if (at + 4 > w.size()) {
                    error = "-labels <index> <count> \"<first>\" \"<template>\"";
                    return false;
                }
                double index = 0.0, count = 0.0;
                if (!Evaluate(w[at], index, error) || !Evaluate(w[at + 1], count, error)) return false;
                Ir77ProgramSlider slider{};
                slider.index = static_cast<std::uint32_t>(std::max(index, 0.0));
                if (!w[at + 2].empty()) slider.labels.push_back(w[at + 2]);
                for (std::size_t row = 0; row < static_cast<std::size_t>(std::max(count, 0.0)); row++) {
                    std::string label{};
                    if (!Label(w[at + 3], row, label, error)) return false;
                    slider.labels.push_back(label);
                }
                output.sliders.push_back(slider);
                at += 4;
            } else {
                error = "write's option " + option + " -- -summary or -labels";
                return false;
            }
        }

        m_result->outputs.push_back(std::move(output));
        return true;
    }

    // One label from a template at a row -- {#} the row from 1, {#0} from 0, {<buffer>} its element at the row, {<symbol>} its value; then
    // %<printf format> (%g unless given), @<string constant> (the value picks a word of it, words apart by |), ? (nothing when the value is 0)
    bool Label(std::string const& pattern, std::size_t const row, std::string& out, std::string& error) {
        for (std::size_t at = 0; at < pattern.size(); at++) {
            if (pattern[at] != '{') {
                out += pattern[at];
                continue;
            }
            std::size_t const close = pattern.find('}', at);
            if (close == std::string::npos) {
                error = "label \"" + pattern + "\" -- a { without its }";
                return false;
            }
            std::string field = pattern.substr(at + 1, close - at - 1);
            at = close;

            bool const omit_zero = !field.empty() && field.back() == '?';
            if (omit_zero) field.pop_back();
            std::string words{};
            std::size_t const word_at = field.find('@');
            if (word_at != std::string::npos) {
                words = field.substr(word_at + 1);
                field = field.substr(0, word_at);
            }
            std::string format{"%g"};
            std::size_t const format_at = field.find('%');
            if (format_at != std::string::npos) {
                format = field.substr(format_at);
                field = field.substr(0, format_at);
            }

            double value = 0.0;
            if (field == "#") {
                value = static_cast<double>(row + 1);
            } else if (field == "#0") {
                value = static_cast<double>(row);
            } else if (Ir77StageBuffer* b = Find(field)) {
                if (!Host(*b, error)) return false;
                if (row >= b->size) {
                    error = "label {" + field + "} -- row " + std::to_string(row) + ", the buffer holds " + std::to_string(b->size);
                    return false;
                }
                value = b->ints ? static_cast<double>(b->integers[row]) : b->doubles[row];
            } else if (m_symbols.count(field)) {
                value = Symbol(field);
            } else {
                error = "label {" + field + "} -- no buffer or symbol of that name";
                return false;
            }
            if (omit_zero && value == 0.0) continue;

            if (!words.empty()) {
                auto const found = m_strings.find(words);
                if (found == m_strings.end()) {
                    error = "label @" + words + " -- no string constant of that name";
                    return false;
                }
                std::vector<std::string> list{};
                std::string word{};
                std::istringstream parts{found->second};
                while (std::getline(parts, word, '|')) list.push_back(word);
                long const pick = std::lround(value);
                if (pick >= 0 && static_cast<std::size_t>(pick) < list.size()) out += list[static_cast<std::size_t>(pick)];
                continue;
            }
            if (std::count(format.begin(), format.end(), '%') != 1 || format.find_first_of("eEfgG", 1) == std::string::npos ||
                format.find_first_of("sdinpcuxX") != std::string::npos) {
                error = "label format " + format + " -- a printf number format (%g, %.4f, %.2e ...)";
                return false;
            }
            char number[64];
            std::snprintf(number, sizeof(number), format.c_str(), value);
            out += number;
        }
        return true;
    }

    // read <name> <buffer> -- <project>/data/<name>.json's "values" into the buffer from element 0
    bool Read(std::string const& name, std::string const& buffer, std::string& error) {
        if (!FileName(name, error)) return false;
        Ir77StageBuffer* b = Find(buffer);
        if (!b) {
            error = "no buffer " + buffer;
            return false;
        }
        if (m_data.empty()) {
            error = "no project -- nothing to read";
            return false;
        }
        std::string const path = (std::filesystem::path{m_data} / (name + ".json")).string();
        simdjson::dom::parser parser{};
        simdjson::dom::element root{};
        simdjson::dom::array values{};
        if (parser.load(path).get(root) != simdjson::SUCCESS || root["values"].get(values) != simdjson::SUCCESS) {
            error = "can't read " + path + " (a data file with \"values\")";
            return false;
        }
        if (!Host(*b, error)) return false;
        std::size_t k = 0;
        for (simdjson::dom::element const v : values) {
            if (k >= b->size) break;
            double number = 0.0;
            if (v.get(number) != simdjson::SUCCESS) {
                std::int64_t whole = 0;
                if (v.get(whole) != simdjson::SUCCESS) continue;
                number = static_cast<double>(whole);
            }
            if (b->ints) b->integers[k] = static_cast<std::int32_t>(std::lround(number));
            else b->doubles[k] = number;
            k++;
        }
        Wrote(*b);
        return true;
    }

    // ---- expressions ----------------------------------------------------------------------------------------------------------------------

    static bool Function(std::string const& word) {
        return word == "abs" || word == "sqrt" || word == "min" || word == "max" || word == "exp" || word == "log" || word == "at" || Reduction(word);
    }

    static bool Reduction(std::string const& word) { return word == "sum" || word == "sumsq" || word == "maxabs"; }

    struct Ir77Parser {
        std::string const& text;
        std::size_t at{0};
        Ir77Stages& owner;
        std::string& error;

        void Space() {
            while (at < text.size() && std::isspace(static_cast<unsigned char>(text[at]))) at++;
        }
        bool Take(char const* op) {
            Space();
            std::size_t const length = std::char_traits<char>::length(op);
            if (text.compare(at, length, op) != 0) return false;
            // "<" isn't "<=", "!" isn't "!="
            if (length == 1 && at + 1 < text.size() && text[at + 1] == '=' && (op[0] == '<' || op[0] == '>' || op[0] == '!' || op[0] == '=')) return false;
            at += length;
            return true;
        }
        bool Or(double& v) {
            if (!And(v)) return false;
            while (Take("||")) {
                double r = 0;
                if (!And(r)) return false;
                v = (v != 0.0 || r != 0.0) ? 1.0 : 0.0;
            }
            return true;
        }
        bool And(double& v) {
            if (!Compare(v)) return false;
            while (Take("&&")) {
                double r = 0;
                if (!Compare(r)) return false;
                v = (v != 0.0 && r != 0.0) ? 1.0 : 0.0;
            }
            return true;
        }
        bool Compare(double& v) {
            if (!Sum(v)) return false;
            for (;;) {
                char const* ops[]{"<=", ">=", "==", "!=", "<", ">"};
                int which = -1;
                for (int i = 0; i < 6 && which < 0; i++)
                    if (Take(ops[i])) which = i;
                if (which < 0) return true;
                double r = 0;
                if (!Sum(r)) return false;
                bool const b = which == 0 ? v <= r : which == 1 ? v >= r : which == 2 ? v == r : which == 3 ? v != r : which == 4 ? v < r : v > r;
                v = b ? 1.0 : 0.0;
            }
        }
        bool Sum(double& v) {
            if (!Product(v)) return false;
            for (;;) {
                if (Take("+")) {
                    double r = 0;
                    if (!Product(r)) return false;
                    v += r;
                } else if (Take("-")) {
                    double r = 0;
                    if (!Product(r)) return false;
                    v -= r;
                } else {
                    return true;
                }
            }
        }
        bool Product(double& v) {
            if (!Unary(v)) return false;
            for (;;) {
                if (Take("*")) {
                    double r = 0;
                    if (!Unary(r)) return false;
                    v *= r;
                } else if (Take("/")) {
                    double r = 0;
                    if (!Unary(r)) return false;
                    v /= r;
                } else {
                    return true;
                }
            }
        }
        bool Unary(double& v) {
            if (Take("-")) {
                if (!Unary(v)) return false;
                v = -v;
                return true;
            }
            if (Take("!")) {
                if (!Unary(v)) return false;
                v = v == 0.0 ? 1.0 : 0.0;
                return true;
            }
            return Atom(v);
        }
        bool Atom(double& v) {
            Space();
            if (Take("(")) {
                if (!Or(v)) return false;
                if (!Take(")")) {
                    error = "expected ) in '" + text + "'";
                    return false;
                }
                return true;
            }
            if (at < text.size() && (std::isdigit(static_cast<unsigned char>(text[at])) || text[at] == '.')) {
                char* end = nullptr;
                v = std::strtod(text.c_str() + at, &end);
                at = static_cast<std::size_t>(end - text.c_str());
                return true;
            }
            std::string word{};
            while (at < text.size() && (std::isalnum(static_cast<unsigned char>(text[at])) || text[at] == '_')) word += text[at++];
            if (word.empty()) {
                error = "can't read '" + text.substr(at) + "'";
                return false;
            }
            if (!Function(word)) {
                if (owner.m_constant_only && !owner.m_constants.count(word)) {
                    error = "'" + word + "' -- a constant's value uses numbers and the constants above it";
                    return false;
                }
                if (!owner.m_symbols.count(word)) {
                    error = "no symbol '" + word + "'";
                    return false;
                }
                v = owner.m_symbols[word];
                return true;
            }
            if (!Take("(")) {
                error = word + " needs (";
                return false;
            }
            if (word == "at") {
                Space();
                std::string name{};
                while (at < text.size() && (std::isalnum(static_cast<unsigned char>(text[at])) || text[at] == '_')) name += text[at++];
                double index = 0.0;
                if (!Take(",") || !Or(index) || !Take(")")) {
                    error = "at(<buffer>, <index>)";
                    return false;
                }
                if (owner.m_constant_only) {
                    error = "at() reads a buffer -- not in a constant";
                    return false;
                }
                Ir77StageBuffer* b = owner.Find(name);
                if (!b || !owner.Host(*b, error)) {
                    if (error.empty()) error = "at: no buffer " + name;
                    return false;
                }
                std::size_t const k = static_cast<std::size_t>(std::max(index, 0.0));
                if (index < 0.0 || k >= b->size) {
                    error = "at(" + name + ", " + std::to_string(static_cast<long long>(index)) + ") -- it holds " + std::to_string(b->size);
                    return false;
                }
                v = b->ints ? static_cast<double>(b->integers[k]) : b->doubles[k];
                return true;
            }
            if (Reduction(word)) {
                Space();
                std::string name{};
                while (at < text.size() && (std::isalnum(static_cast<unsigned char>(text[at])) || text[at] == '_')) name += text[at++];
                if (!Take(")")) {
                    error = word + "(<buffer>)";
                    return false;
                }
                if (owner.m_constant_only) {
                    error = word + "() reads a buffer -- not in a constant";
                    return false;
                }
                Ir77StageBuffer* b = owner.Find(name);
                if (!b || b->ints || !owner.Host(*b, error)) {
                    if (error.empty()) error = word + ": no double buffer " + name;
                    return false;
                }
                v = 0.0;
                for (double const x : b->doubles) v = word == "sum" ? v + x : word == "sumsq" ? v + x * x : std::max(v, fabs(x));
                return true;
            }
            double a = 0.0, b = 0.0;
            if (!Or(a)) return false;
            bool const two = word == "min" || word == "max";
            if (two && (!Take(",") || !Or(b))) {
                error = word + "(a, b)";
                return false;
            }
            if (!Take(")")) {
                error = "expected ) after " + word;
                return false;
            }
            v = word == "abs" ? fabs(a) : word == "sqrt" ? sqrt(a) : word == "exp" ? exp(a) : word == "log" ? log(a) : word == "min" ? std::min(a, b) : std::max(a, b);
            return true;
        }
    };

    bool Evaluate(std::string const& text, double& value, std::string& error) {
        Ir77Parser parser{text, 0, *this, error};
        if (!parser.Or(value)) return false;
        parser.Space();
        if (parser.at != text.size()) {
            error = "can't read '" + text.substr(parser.at) + "' in '" + text + "'";
            return false;
        }
        return true;
    }

    // ---- text ---------------------------------------------------------------------------------------------------------------------------

    static std::string Trim(std::string text) {
        while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) text.pop_back();
        std::size_t start = 0;
        while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) start++;
        return text.substr(start);
    }

    static std::vector<std::string> Words(std::string const& text) {
        std::vector<std::string> out{};
        std::istringstream words{text};
        std::string w{};
        while (words >> w) out.push_back(w);
        if (out.empty()) out.push_back("");
        return out;
    }

    // a step's words, a "quoted text" one word (its quotes gone)
    static std::vector<std::string> Tokens(std::string const& text) {
        std::vector<std::string> out{};
        std::size_t at = 0;
        while (at < text.size()) {
            while (at < text.size() && std::isspace(static_cast<unsigned char>(text[at]))) at++;
            if (at >= text.size()) break;
            std::string word{};
            if (text[at] == '"') {
                std::size_t const close = text.find('"', at + 1);
                word = text.substr(at + 1, close == std::string::npos ? std::string::npos : close - at - 1);
                at = close == std::string::npos ? text.size() : close + 1;
            } else {
                while (at < text.size() && !std::isspace(static_cast<unsigned char>(text[at]))) word += text[at++];
            }
            out.push_back(word);
        }
        return out;
    }

    static bool Identifier(std::string const& text) {
        if (text.empty() || std::isdigit(static_cast<unsigned char>(text[0]))) return false;
        for (char const c : text)
            if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') return false;
        return true;
    }

    void Release() {
        for (Ir77StageKernel& k : m_kernels)
            if (k.kernel) clReleaseKernel(k.kernel);
        for (Ir77StageBuffer& b : m_buffers)
            if (b.memory) clReleaseMemObject(b.memory);
        if (m_program) clReleaseProgram(m_program);
        m_program = nullptr;
    }

    cl_context m_context{nullptr};
    cl_command_queue m_queue{nullptr};
    cl_device_id m_device{nullptr};
    std::string m_data{};  // <project>/data -- read's folder

    std::atomic<bool> const* m_cancel{nullptr};
    Ir77ProgramResult* m_result{nullptr};

    std::vector<Ir77StageBuffer> m_buffers{};
    std::vector<Ir77StageKernel> m_kernels{};
    std::vector<Ir77StageLine> m_script{};
    std::vector<std::string> m_reports{};

    std::map<std::string, double> m_symbols{};       // sorted -- the kernels' symbol table follows this order
    std::map<std::string, std::string> m_types{};    // each symbol's type in a kernel -- int, long, double
    std::set<std::string> m_constants{};             // the @const numbers
    std::set<std::string> m_engine{};                // the engine's symbols
    std::map<std::string, std::string> m_strings{};  // @const string
    std::map<std::string, std::vector<double>> m_arrays{};  // @const double[] / int[] -- their buffers' values
    bool m_constant_only{false};                     // evaluating a @const -- constants only
    std::uint32_t m_depth{0};                        // @repeat loops open

    cl_program m_program{nullptr};

    std::map<std::string, std::vector<std::vector<double>>> m_histories{}, m_residuals{};  // extrapolate -- a history a step
};

}  // namespace NSIr77Compute
