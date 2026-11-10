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
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "../types/Ir77Types.hpp"
#include "Ir77Basis.hpp"
#include "Ir77Integrals.hpp"
#include "Ir77Linear.hpp"
#include "Ir77Radial.hpp"
#include "Ir77Scf.hpp"

namespace NSIr77Compute {

// =============================================================================================================================================
// Ir77Stages -- a Compute program: one OpenCL source. Its comment directives declare what it works with (constants, buffers, kernels) and the
// script the host runs between the kernels. The math is the program's (its kernels); the engine supplies only general steps (linear algebra,
// a molecule's basis, the file writers). A program is a generator: what it makes leaves as data files in <project>/data/ for Fields to draw.
//
// Directives (comment lines; the compiler skips them):
//   // @const <type> <name> = <value>     a constant -- the program's settings live here, nowhere else:
//                                           int, double, bool    a number (an expression of numbers and constants above it); in a kernel
//                                                                s.<name> as int / double / int (0, 1)
//                                           string               text a step reads (a basis set's name, an output file's name)
//                                           double[], int[]      numbers apart by spaces or commas -- a buffer of them, s.<name>, and
//                                                                <name>_count its length
//                                           molecule             atoms "El x y z; El x y z ..." in angstrom -- each @const molecule line of
//                                                                the same name adds its atoms
//   // @use gaussian                      the Gaussian integral library (Compute's Ir77Integrals.cl) ahead of the program: Ir77EriFunctions,
//                                         Ir77Hermite, Ir77Coulomb, Ir77Boys, Ir77QuartetFunctions, IR77_FUNCTION ...
//   // @buffer <name> [int] <d0> [d1 [d2]] a buffer of doubles (or ints), d0 fastest -- element (a, b, c) at a + d0 (b + d1 c); each dimension
//                                         a number, a symbol, or a product (n*n)
//   // @kernel <name> <d0> [d1 [d2]]      void <name>(Ir77Stage s) -- one work item an element, s.i its index (0 .. the product - 1)
//   // @report <symbol> ...               symbols the run reports
//   the script, in order:
//   // @do <step> ...                     one step (below)
//   // @repeat <passes>                   the steps down to @until, again and again -- `iteration` counts (1, 2 ...)
//   // @until <expression>                ... until it's true (sets `converged`), or the passes run out -- loops nest
//
// In a kernel: s.<buffer> is the buffer (__global double* / int*), s.<symbol> a symbol's value, s.i the element. Symbols are the constants,
// the engine's (below) and what `set` makes (doubles).
//
// The engine's symbols: iteration converged (int), error energy (double) -- and with a molecule step n atoms primitives electrons alpha beta
// multiplicity unrestricted (int), quartets (long), nuclear (double); with an orthogonalize step m (int). A @repeat inside another counts
// its own iteration; the outer one's comes back after it.
//
// Steps (a <word> that names a string constant means its text; a matrix's sizes come from its @buffer line -- d0 its columns, d1 its rows,
// d2 its layers):
//   molecule <molecule> <basis> [charge] [multiplicity] [unrestricted]
//                                         the molecule's basis -> buffers basis_functions (int, IR77_FUNCTION each), basis_primitives,
//                                         basis_atoms (Ir77Integrals.cl's layout); symbols n (functions), atoms, electrons, alpha, beta,
//                                         multiplicity (0 = the lowest the electron count allows), unrestricted (asked, or an open shell),
//                                         quartets (unique (ij|kl)), nuclear (the nuclei's repulsion). basis_atoms (x y z Z a nucleus, bohr) is
//                                         the geometry from then on: a kernel that moves the nuclei writes it, and the steps after (guess, the
//                                         orbitals file) take the atoms from it
//   run <kernel>                          the kernel over its elements
//   swap <a> <b>                          two buffers of one size trade contents (a pass's out becomes the next pass's in)
//   guess <P>                             the superposition of atomic densities -> P, split evenly over its layers (n x n each)
//   orthogonalize <S> <X> [rank] [drop]   canonical: S's (n x n) eigenvectors over sqrt(their eigenvalues), those under drop (1e-7) dropped
//                                         -> X (@buffer X <rank> <n>); the rank symbol (m) set first
//   eigen <F> <X> <C> <E>                 per layer of F: X^T F X diagonalized; C = X C' (m x n a layer), E its eigenvalues ascending (m)
//   subspace <F> <C> <E> <first> ...      per layer: F diagonalized again in the space of C's columns from first on (they're orthonormal over
//                                         S) -- those columns and their E replaced, the ones before kept; one first (an expression) a layer
//   density <C> <P> <count> ...           per layer: P = sum over the first <count> columns of C c c^T (one count a layer)
//   diis <F> <P> <S> <X> ...              Pulay's extrapolation of every F given (groups of four, one set of coefficients) from its last
//                                         COMPUTE_DIIS_SIZE, error X^T(FPS - SPF)X; symbol error (its largest element)
//   diis reset                            every diis history forgotten (a new geometry: the old Focks no longer apply)
//   set <symbol> = <expression>           + - * / ( ) < > <= >= == != && || !, abs sqrt min max exp log; sum(b) sumsq(b) maxabs(b) of a
//                                         buffer, at(b,i) one element (no spaces inside a step's words)
//   require <expression> "<message>"      the run stops with the message unless the expression holds
//   output orbitals <C> <E> <name>        <project>/data/<name>.json, format "orbitals" (Shaders/fields/orbitals.glsl, levels.glsl) -- layers:
//                                         1 restricted, 2 alpha / beta; with energy, iteration, converged, error
//   output atom <name> <Z> <charge> <q> <keep> <zeta0> <C0> <E0> ... <zeta3> <C3> <E3>
//                                         one atom over radial (Slater) functions, format "orbitals" function kind 1 (Ir77Radial) -- q the
//                                         radial orbitals' electrons (R x 4 x 2: r, l, spin), keep those written per l (4), per l its
//                                         exponents (k), C (m x k x 2), E (m x 2)
//   output grid <name> <lo> <hi> <b> ...  <project>/data/<name>.json, format "grid" (Shaders/fields/grid.glsl) -- the buffers as channels of
//                                         one d0 x d1 x d2 grid (x fastest) over the box [lo, hi]^3
// =============================================================================================================================================
class Ir77Stages {
   public:
    Ir77Stages(cl_context const context, cl_command_queue const queue, cl_device_id const device, std::string const& integrals)
        : m_context{context}, m_queue{queue}, m_device{device}, m_integrals{integrals} {}

    Ir77Stages(Ir77Stages const&) = delete;
    Ir77Stages& operator=(Ir77Stages const&) = delete;

    ~Ir77Stages() { Release(); }

    // parsed, built and run -- result: the report, the outputs, every buffer as the run left it (a failed script's too). false: error says why.
    bool Run(std::string const& source, std::atomic<bool> const& cancel, Ir77ProgramResult& result, std::string& error) {
        auto const started = std::chrono::steady_clock::now();
        m_cancel = &cancel;
        m_result = &result;

        if (!Parse(source, error) || !Build(source, error)) return false;

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
        std::size_t partner{0};       // repeat <-> until
        std::uint32_t number{0};      // its line in the source
    };

    // ---- the program's text -------------------------------------------------------------------------------------------------------------

    bool Parse(std::string const& source, std::string& error) {
        for (char const* name : {"iteration", "converged"}) Engine(name, "int");
        for (char const* name : {"error", "energy"}) Engine(name, "double");

        std::istringstream lines{source};
        std::string line{};
        std::vector<std::size_t> open{};
        std::uint32_t number = 0;

        while (std::getline(lines, line)) {
            number++;
            std::size_t const at = line.find("// @");
            if (at == std::string::npos) continue;
            std::istringstream words{line.substr(at + 4)};
            std::string what{};
            words >> what;
            std::string rest{};
            std::getline(words, rest);
            rest = Trim(rest);
            std::string const where = "line " + std::to_string(number) + ": ";

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
            } else if (what == "use") {
                if (rest != "gaussian") {
                    error = where + "@use takes gaussian";
                    return false;
                }
                m_use_gaussian = true;
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
                m_script.push_back({Ir77StageLine::Kind::Do, rest, 0, number});
            } else if (what == "repeat") {
                open.push_back(m_script.size());
                m_script.push_back({Ir77StageLine::Kind::Repeat, rest, 0, number});
            } else if (what == "until") {
                if (open.empty()) {
                    error = where + "@until without @repeat";
                    return false;
                }
                m_script.push_back({Ir77StageLine::Kind::Until, rest, open.back(), number});
                m_script[open.back()].partner = m_script.size() - 1;
                open.pop_back();
            } else {
                error = where + "no directive @" + what + " -- const use buffer kernel report do repeat until";
                return false;
            }
        }

        if (!open.empty()) {
            error = "line " + std::to_string(m_script[open.back()].number) + ": @repeat without @until";
            return false;
        }
        if (m_script.empty()) {
            error = "no script -- a program runs its // @do steps (none found)";
            return false;
        }

        // the molecule and orthogonalize steps' symbols, and the molecule step's buffers -- the engine's when the script has those steps (a
        // program without them may name its own n, m ...)
        bool molecule = false, orthogonalize = false;
        std::vector<std::string> ranks{};  // orthogonalize's own rank symbols
        for (Ir77StageLine const& l : m_script) {
            std::vector<std::string> const w = l.kind == Ir77StageLine::Kind::Do ? Words(l.text) : std::vector<std::string>{""};
            molecule = molecule || w[0] == "molecule";
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
        std::vector<std::pair<char const*, char const*>> steps{};
        if (molecule)
            steps = {{"n", "int"},     {"atoms", "int"},        {"primitives", "int"},   {"electrons", "int"}, {"alpha", "int"},
                     {"beta", "int"},  {"multiplicity", "int"}, {"unrestricted", "int"}, {"quartets", "long"}, {"nuclear", "double"}};
        if (orthogonalize) steps.emplace_back("m", "int");
        for (auto const& [name, type] : steps) {
            if (m_constants.count(name) || m_strings.count(name) || m_molecules.count(name)) {
                error = std::string{name} + " is the " + (std::string{name} == "m" ? "orthogonalize" : "molecule") + " step's symbol -- pick another name";
                return false;
            }
            Engine(name, type);
        }
        if (molecule) {
            m_buffers.push_back({"basis_functions", true, {std::to_string(IR77_FUNCTION), "n"}});
            m_buffers.push_back({"basis_primitives", false, {"2", "primitives"}});
            m_buffers.push_back({"basis_atoms", false, {"4", "atoms"}});
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
            if (m_symbols.count(b.name) || m_strings.count(b.name) || m_molecules.count(b.name)) {
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
        if (type == "molecule") {
            if (taken || m_symbols.count(name)) {
                error = name + " is already declared";
                return false;
            }
            std::istringstream atoms{value};
            std::string atom{};
            while (std::getline(atoms, atom, ';')) {
                std::istringstream words{atom};
                std::string element{};
                double x = 0.0, y = 0.0, z = 0.0;
                if (Trim(atom).empty()) continue;
                if (!(words >> element >> x >> y >> z)) {
                    error = "an atom is \"El x y z\" (angstrom), atoms apart by ; -- not \"" + Trim(atom) + "\"";
                    return false;
                }
                std::uint32_t const z_number = Ir77BasisBuilder::Element(element);
                if (z_number == 0) {
                    error = "no element \"" + element + "\"";
                    return false;
                }
                m_molecules[name].push_back(Ir77Atom{z_number, x * BOHR_PER_ANGSTROM, y * BOHR_PER_ANGSTROM, z * BOHR_PER_ANGSTROM});
            }
            if (m_molecules[name].empty()) {
                error = "molecule " + name + " has no atoms";
                return false;
            }
            return true;
        }

        if (taken || m_molecules.count(name)) {
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
            std::istringstream words{value};
            while (std::getline(words, word, ' ')) {
                std::istringstream parts{word};
                std::string part{};
                while (std::getline(parts, part, ',')) {
                    part = Trim(part);
                    if (part.empty()) continue;
                    char* end = nullptr;
                    double const v = std::strtod(part.c_str(), &end);
                    if (!end || *end != '\0' || (type == "int[]" && v != std::floor(v))) {
                        error = type + " " + name + ": \"" + part + "\" -- numbers apart by spaces or commas" + (type == "int[]" ? ", whole" : "");
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
            error = "@const type " + type + " -- int, double, bool, string, molecule, double[] or int[]";
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

    // the OpenCL program: the prelude (its struct of symbols and buffers), the integral library, the program, a wrapper kernel per @kernel
    bool Build(std::string const& source, std::string& error) {
        std::ostringstream text{};
        text << "#pragma OPENCL EXTENSION cl_khr_fp64 : enable\n";
        text << "typedef struct {\n    uint i;\n";
        for (auto const& [name, value] : m_symbols) text << "    " << m_types[name] << " " << name << ";\n";
        for (Ir77StageBuffer const& b : m_buffers) text << "    __global " << (b.ints ? "int" : "double") << "* " << b.name << ";\n";
        text << "} Ir77Stage;\n";

        // a unique quartet's index from four functions -- the order Ir77QuartetFunctions undoes
        text << "ulong Ir77Pair(uint i, uint j) { return i >= j ? (ulong)i * (i + 1) / 2 + j : (ulong)j * (j + 1) / 2 + i; }\n";
        text << "ulong Ir77Quartet(uint i, uint j, uint k, uint l) { ulong a = Ir77Pair(i, j), b = Ir77Pair(k, l); return a >= b ? a * (a + 1) / 2 + b : b * (b + 1) / 2 + a; }\n";

        if (m_use_gaussian) text << m_integrals << "\n";
        text << "#line 1\n" << source << "\n";

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
            // the driver's name for the source ("/tmp/OCL123.cl:7:43:", "<kernel>:7:43:") -> "line 7:43:" -- the program's own lines
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
            std::string const where = "line " + std::to_string(line.number) + ": ";

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
                        error = "line " + std::to_string(m_script[end].number) + ": @until " + m_script[end].text + " -- " + error;
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

        if (what == "molecule") return need(2, "molecule <molecule> <basis> [charge] [multiplicity] [unrestricted]") && Molecule(w, error);
        if (what == "run") return need(1, "run <kernel>") && RunKernel(w[1], error);
        if (what == "swap") return need(2, "swap <a> <b>") && Swap(w[1], w[2], error);
        if (what == "guess") return need(1, "guess <P>") && GuessStep(w[1], error);
        if (what == "orthogonalize") {
            double threshold = COMPUTE_ORTHOGONAL_DROP;
            if (!need(2, "orthogonalize <S> <X> [rank] [threshold]") || (w.size() > 4 && !Evaluate(w[4], threshold, error))) return false;
            return Orthogonalize(w[1], w[2], w.size() > 3 ? w[3] : "m", threshold, error);
        }
        if (what == "eigen") return need(4, "eigen <F> <X> <C> <E>") && Eigen(w[1], w[2], w[3], w[4], error);
        if (what == "subspace") return need(4, "subspace <F> <C> <E> <first> ...") && Subspace(w[1], w[2], w[3], std::vector<std::string>(w.begin() + 4, w.end()), error);
        if (what == "density") return need(3, "density <C> <P> <count> ...") && Density(w[1], w[2], std::vector<std::string>(w.begin() + 3, w.end()), error);
        if (what == "diis" && w.size() == 2 && w[1] == "reset") {
            m_focks.clear();
            m_errors.clear();
            return true;
        }
        if (what == "diis") return need(4, "diis <F> <P> <S> <X> ...") && Diis(std::vector<std::string>(w.begin() + 1, w.end()), text, error);
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
        if (what == "output" && w.size() >= 2 && w[1] == "orbitals") return need(4, "output orbitals <C> <E> <name>") && OutputOrbitals(w[2], w[3], Text(w[4]), error);
        if (what == "output" && w.size() >= 2 && w[1] == "atom")
            return need(18, "output atom <name> <Z> <charge> <q> <keep> <zeta0> <C0> <E0> ... <zeta3> <C3> <E3>") && OutputAtom(w, error);
        if (what == "output" && w.size() >= 2 && w[1] == "grid")
            return need(5, "output grid <name> <lo> <hi> <buffer> ...") && OutputGrid(Text(w[2]), w[3], w[4], std::vector<std::string>(w.begin() + 5, w.end()), error);

        error = "no step '" + what + "' -- molecule run swap guess orthogonalize eigen subspace density diis set require output";
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

    // a dimension's value -- a number, a symbol, or a product of them
    std::size_t Dimension(std::string const& text) const {
        double product = 1.0;
        std::string part{};
        std::istringstream parts{text};
        while (std::getline(parts, part, '*')) {
            part = Trim(part);
            product *= !part.empty() && std::isdigit(static_cast<unsigned char>(part[0])) ? std::atof(part.c_str()) : Symbol(part);
        }
        return static_cast<std::size_t>(std::max(product, 0.0));
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

    bool Molecule(std::vector<std::string> const& w, std::string& error) {
        auto const found = m_molecules.find(w[1]);
        if (found == m_molecules.end()) {
            error = "no @const molecule " + w[1];
            return false;
        }
        double charge = 0.0, multiplicity_asked = 0.0, unrestricted = 0.0;
        if ((w.size() > 3 && !Evaluate(w[3], charge, error)) || (w.size() > 4 && !Evaluate(w[4], multiplicity_asked, error)) ||
            (w.size() > 5 && !Evaluate(w[5], unrestricted, error)))
            return false;

        m_molecule = Ir77Molecule{};
        m_molecule.atoms = found->second;
        m_molecule.basis = Text(w[2]);
        std::transform(m_molecule.basis.begin(), m_molecule.basis.end(), m_molecule.basis.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        m_molecule.charge = static_cast<std::int32_t>(std::lround(charge));
        m_molecule.multiplicity = static_cast<std::uint32_t>(std::max(std::lround(multiplicity_asked), 0L));
        if (!Ir77BasisBuilder::Build(m_molecule, m_basis, error)) return false;

        std::int64_t electrons = -m_molecule.charge;
        for (Ir77Atom const& atom : m_molecule.atoms) electrons += atom.number;
        std::int64_t const multiplicity = m_molecule.multiplicity == 0 ? (electrons % 2 == 0 ? 1 : 2) : m_molecule.multiplicity;
        std::int64_t const unpaired = multiplicity - 1;
        if (electrons <= 0 || unpaired > electrons || (electrons + unpaired) % 2 != 0) {
            error = std::to_string(electrons) + " electrons can't have multiplicity " + std::to_string(multiplicity);
            return false;
        }
        m_molecule.multiplicity = static_cast<std::uint32_t>(multiplicity);

        Ir77Integrals::Ir77Flat const flat = Ir77Integrals::Flatten(m_molecule, m_basis);
        std::size_t const n = m_basis.functions.size();
        m_symbols["n"] = static_cast<double>(n);
        m_symbols["atoms"] = static_cast<double>(m_molecule.atoms.size());
        m_symbols["primitives"] = static_cast<double>(flat.primitives.size() / 2);
        m_symbols["electrons"] = static_cast<double>(electrons);
        m_symbols["alpha"] = static_cast<double>((electrons + unpaired) / 2);
        m_symbols["beta"] = static_cast<double>((electrons - unpaired) / 2);
        m_symbols["multiplicity"] = static_cast<double>(multiplicity);
        m_symbols["unrestricted"] = unrestricted != 0.0 || multiplicity != 1 ? 1.0 : 0.0;
        m_symbols["quartets"] = static_cast<double>(Ir77Integrals::QuartetCount(n));
        m_symbols["nuclear"] = Ir77Integrals::NuclearRepulsion(m_molecule);
        if (m_symbols["alpha"] > static_cast<double>(n)) {
            error = "more occupied orbitals than basis functions";
            return false;
        }

        Size();
        Find("basis_functions")->integers = flat.functions;
        Find("basis_primitives")->doubles = flat.primitives;
        Find("basis_atoms")->doubles = flat.atoms;
        for (char const* name : {"basis_functions", "basis_primitives", "basis_atoms"}) Wrote(*Find(name));
        m_have_molecule = true;
        return true;
    }

    // the molecule's nuclei where basis_atoms has them now -- a program may have moved them
    bool Geometry(std::string& error) {
        Ir77StageBuffer* atoms = Find("basis_atoms");
        if (!atoms || !Host(*atoms, error)) return false;
        for (std::size_t a = 0; a < m_molecule.atoms.size() && a * 4 + 2 < atoms->doubles.size(); a++) {
            m_molecule.atoms[a].x = atoms->doubles[a * 4];
            m_molecule.atoms[a].y = atoms->doubles[a * 4 + 1];
            m_molecule.atoms[a].z = atoms->doubles[a * 4 + 2];
        }
        return true;
    }

    bool GuessStep(std::string const& name, std::string& error) {
        if (!m_have_molecule) {
            error = "guess needs the molecule step first";
            return false;
        }
        if (!Geometry(error)) return false;
        std::size_t const n = m_basis.functions.size();
        std::vector<double>* p = Doubles(name, n * n, error);
        if (!p) return false;

        std::vector<double> density{};
        if (!Ir77Scf::Guess(m_molecule, m_basis, *m_cancel, density)) {
            error = "cancelled";
            return false;
        }
        std::size_t const layers = std::max<std::size_t>(p->size() / (n * n), 1);
        for (std::size_t l = 0; l < layers; l++)
            for (std::size_t i = 0; i < n * n; i++) (*p)[l * n * n + i] = density[i] / static_cast<double>(layers);
        Wrote(*Find(name));
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

    // per layer: P (n x n) = sum over the first <count> columns of C (m x n) c c^T
    bool Density(std::string const& c_name, std::string const& p_name, std::vector<std::string> const& counts, std::string& error) {
        std::size_t m = 0, n = 0, c_layers = 0, pn = 0, p_rows = 0, layers = 0;
        if (!Shape(c_name, m, n, c_layers, error) || !Shape(p_name, pn, p_rows, layers, error)) return false;
        std::vector<double>* c = Doubles(c_name, c_layers * n * m, error);
        std::vector<double>* p = c ? Doubles(p_name, layers * n * n, error) : nullptr;
        if (!p) return false;
        if (counts.size() < layers) {
            error = "density needs a count for each of " + p_name + "'s " + std::to_string(layers) + " layers";
            return false;
        }
        for (std::size_t l = 0; l < layers; l++) {
            double count = 0.0;
            if (!Evaluate(counts[l], count, error)) return false;
            std::size_t const filled = std::min(static_cast<std::size_t>(std::max(count, 0.0)), m);
            std::size_t const cl = std::min(l, c_layers - 1) * n * m;
            for (std::size_t i = 0; i < n; i++)
                for (std::size_t j = 0; j < n; j++) {
                    double sum = 0.0;
                    for (std::size_t k = 0; k < filled; k++) sum += (*c)[cl + i * m + k] * (*c)[cl + j * m + k];
                    (*p)[l * n * n + i * n + j] = sum;
                }
        }
        Wrote(*Find(p_name));
        return true;
    }

    // Pulay's extrapolation over groups of F P S X at once (one set of coefficients): each F layer's error X^T (F P S - S P F) X (P's layer
    // as F's, S and X one layer); symbol error -- the largest element. A history a step: the step's own words.
    bool Diis(std::vector<std::string> const& names, std::string const& key, std::string& error) {
        if (names.size() % 4 != 0) {
            error = "diis takes groups of four: F P S X ...";
            return false;
        }
        struct Group {
            std::vector<double>* f;
            std::size_t n, layers;
        };
        std::vector<Group> groups{};
        std::vector<double> err{}, stacked{};
        double largest = 0.0;

        for (std::size_t g = 0; g < names.size(); g += 4) {
            std::size_t n = 0, rows = 0, layers = 0, m = 0, xn = 0, xl = 0, pn = 0, pr = 0, pl = 0;
            if (!Shape(names[g], n, rows, layers, error) || !Shape(names[g + 3], m, xn, xl, error) || !Shape(names[g + 1], pn, pr, pl, error)) return false;
            if (xn != n || pn != n) {
                error = names[g] + ", " + names[g + 1] + " and " + names[g + 3] + " disagree on the function count";
                return false;
            }
            std::vector<double>* f = Doubles(names[g], layers * n * n, error);
            std::vector<double>* p = f ? Doubles(names[g + 1], pl * n * n, error) : nullptr;
            std::vector<double>* s = p ? Doubles(names[g + 2], n * n, error) : nullptr;
            std::vector<double>* x = s ? Doubles(names[g + 3], n * m, error) : nullptr;
            if (!x) return false;

            std::vector<double> const sm = Layer(*s, 0, n * n);
            std::vector<double> const xm = Layer(*x, 0, n * m);
            std::vector<double> const xt = Ir77Linear::Transpose(xm, n, m);
            for (std::size_t l = 0; l < layers; l++) {
                std::vector<double> const fl = Layer(*f, l, n * n);
                std::vector<double> const fps = Ir77Linear::Multiply(Ir77Linear::Multiply(fl, Layer(*p, std::min(l, pl - 1), n * n), n, n, n), sm, n, n, n);
                std::vector<double> commutator(n * n);
                for (std::size_t i = 0; i < n; i++)
                    for (std::size_t j = 0; j < n; j++) commutator[i * n + j] = fps[i * n + j] - fps[j * n + i];
                std::vector<double> const e = Ir77Linear::Multiply(Ir77Linear::Multiply(xt, commutator, m, n, n), xm, m, n, m);
                for (double const v : e) largest = std::max(largest, fabs(v));
                err.insert(err.end(), e.begin(), e.end());
                stacked.insert(stacked.end(), fl.begin(), fl.end());
            }
            groups.push_back({f, n, layers});
        }
        m_symbols["error"] = largest;

        std::vector<std::vector<double>>& focks = m_focks[key];
        std::vector<std::vector<double>>& errors = m_errors[key];
        focks.push_back(stacked);
        errors.push_back(err);
        if (focks.size() > COMPUTE_DIIS_SIZE) {
            focks.erase(focks.begin());
            errors.erase(errors.begin());
        }
        if (focks.size() < 2) return true;

        // B c = (0 .. 0, -1), B_ij = e_i . e_j bordered by -1
        std::size_t const count = focks.size(), size = count + 1;
        std::vector<double> b(size * size, 0.0), rhs(size, 0.0), coefficients;
        for (std::size_t i = 0; i < count; i++)
            for (std::size_t j = 0; j < count; j++) {
                double dot = 0.0;
                for (std::size_t k = 0; k < errors[i].size(); k++) dot += errors[i][k] * errors[j][k];
                b[i * size + j] = dot;
            }
        for (std::size_t i = 0; i < count; i++) b[i * size + count] = b[count * size + i] = -1.0;
        rhs[count] = -1.0;
        if (!Ir77Linear::Solve(b, rhs, size, coefficients)) return true;  // singular -- this pass keeps its own F

        std::size_t at = 0;
        for (std::size_t g = 0; g < groups.size(); g++) {
            std::size_t const length = groups[g].layers * groups[g].n * groups[g].n;
            for (std::size_t k = 0; k < length; k++) {
                double sum = 0.0;
                for (std::size_t i = 0; i < count; i++) sum += coefficients[i] * focks[i][at + k];
                (*groups[g].f)[k] = sum;
            }
            at += length;
            Wrote(*Find(names[g * 4]));
        }
        return true;
    }

    static bool FileName(std::string const& name, std::string& error) {
        bool ok = !name.empty() && name.front() != '.' && name.size() <= 128;
        for (char const c : name) ok = ok && (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.');
        if (!ok) error = "output name '" + name + "' -- letters, digits, '-', '_' and '.' only";
        return ok;
    }

    // the orbitals file -- C (n x m a layer), E (m a layer), and the molecule step's basis
    bool OutputOrbitals(std::string const& c_name, std::string const& e_name, std::string const& name, std::string& error) {
        if (!m_have_molecule) {
            error = "output orbitals needs the molecule step";
            return false;
        }
        if (!Geometry(error)) return false;
        if (!FileName(name, error)) return false;
        std::size_t const n = static_cast<std::size_t>(Symbol("n")), m = static_cast<std::size_t>(Symbol("m"));
        std::vector<double>* c = Doubles(c_name, n * m, error);
        std::vector<double>* e = c ? Doubles(e_name, m, error) : nullptr;
        if (!e) return false;
        std::size_t const layers = std::min(c->size() / (n * m), e->size() / m);
        bool const unrestricted = Symbol("unrestricted") != 0.0 && layers >= 2;

        Ir77ProgramOutput output{};
        output.name = name;
        output.format = ORBITALS_FORMAT;
        output.molecule = m_molecule;
        output.basis = m_basis;

        Ir77ScfResult& r = output.scf;
        r.energy = Symbol("energy");
        r.nuclear_repulsion = Symbol("nuclear");
        r.iterations = static_cast<std::uint32_t>(Symbol("iteration"));
        r.converged = Symbol("converged") != 0.0;
        r.error = Symbol("error");
        r.electrons = static_cast<std::uint32_t>(Symbol("electrons"));
        r.alpha = static_cast<std::uint32_t>(Symbol("alpha"));
        r.beta = static_cast<std::uint32_t>(Symbol("beta"));
        r.multiplicity = static_cast<std::uint32_t>(Symbol("multiplicity"));
        r.unrestricted = unrestricted;
        r.occupied = r.alpha;
        r.function_kind = ORBITALS_FUNCTION_GAUSSIAN;

        // column-major as Ir77ScfResult keeps them: orbital k's are [k n, k n + n)
        auto const take = [&](std::size_t const layer, std::vector<double>& energies, std::vector<double>& coefficients) {
            energies.assign(e->begin() + static_cast<std::ptrdiff_t>(layer * m), e->begin() + static_cast<std::ptrdiff_t>((layer + 1) * m));
            coefficients.assign(n * m, 0.0);
            for (std::size_t k = 0; k < m; k++)
                for (std::size_t i = 0; i < n; i++) coefficients[k * n + i] = (*c)[layer * n * m + i * m + k];
        };
        take(0, r.orbital_energies, r.coefficients);
        double const sz = 0.5 * (static_cast<double>(r.alpha) - static_cast<double>(r.beta));
        r.s2 = sz * (sz + 1.0);
        if (unrestricted) {
            take(1, r.beta_orbital_energies, r.beta_coefficients);

            // <S^2> = Sz (Sz + 1) + N_b - sum_ij |<a_i | b_j>|^2
            std::vector<double> s{}, t{}, v{};
            Ir77Integrals::OneElectron(m_molecule, m_basis, s, t, v);
            double overlap = 0.0;
            for (std::size_t i = 0; i < r.alpha; i++)
                for (std::size_t j = 0; j < r.beta; j++) {
                    double dot = 0.0;
                    for (std::size_t p = 0; p < n; p++)
                        for (std::size_t q = 0; q < n; q++) dot += r.coefficients[i * n + p] * s[p * n + q] * r.beta_coefficients[j * n + q];
                    overlap += dot * dot;
                }
            r.s2 += static_cast<double>(r.beta) - overlap;
        }

        m_result->outputs.push_back(std::move(output));
        return true;
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

    // One atom over radial (Slater) functions -- the orbitals file, function kind 1 (Ir77Radial lays it out). q: each radial orbital's
    // electrons, R x 4 x 2 (radial index fastest, then l, then spin); keep: the radial orbitals written per l (4); per l: its exponents
    // (k), C (m x k x 2 -- alpha, beta), E (m x 2). Restricted when every q's alpha and beta agree; else alpha rows, then beta.
    bool OutputAtom(std::vector<std::string> const& w, std::string& error) {
        std::string const name = Text(w[2]);
        if (!FileName(name, error)) return false;
        double z = 0.0, charge = 0.0;
        if (!Evaluate(w[3], z, error) || !Evaluate(w[4], charge, error)) return false;

        std::size_t radial = 0, ls = 0, spins = 0;
        std::vector<double> q{}, keep{};
        if (!Shape(w[5], radial, ls, spins, error) || !Values(w[5], q, error) || !Values(w[6], keep, error)) return false;
        if (ls < Ir77Radial::MAX_L + 1 || spins < 2 || keep.size() < Ir77Radial::MAX_L + 1) {
            error = w[5] + " must be <radial> 4 2 and " + w[6] + " hold 4";
            return false;
        }
        auto const occupation = [&](std::size_t const r, std::size_t const l, std::size_t const spin) { return r < radial ? q[r + radial * (l + 4 * spin)] : 0.0; };

        bool restricted = true;
        double alpha_electrons = 0.0, beta_electrons = 0.0, unpaired = 0.0;
        for (std::size_t l = 0; l <= Ir77Radial::MAX_L; l++)
            for (std::size_t r = 0; r < radial; r++) {
                double const a = occupation(r, l, 0), b = occupation(r, l, 1);
                restricted = restricted && fabs(a - b) < 1e-12;
                alpha_electrons += a;
                beta_electrons += b;
                double const room = 2.0 * (2 * l + 1);
                unpaired += std::round(std::min(a + b, room - a - b));
            }

        std::array<std::array<Ir77Radial::Ir77RadialBlock, Ir77Radial::MAX_L + 1>, 2> blocks{};
        for (std::size_t l = 0; l <= Ir77Radial::MAX_L; l++) {
            std::string const& zeta_name = w[7 + 3 * l];
            std::string const& c_name = w[8 + 3 * l];
            std::string const& e_name = w[9 + 3 * l];
            std::size_t m = 0, k = 0, layers = 0;
            std::vector<double> zeta{}, c{}, e{};
            if (!Values(zeta_name, zeta, error) || !Shape(c_name, m, k, layers, error) || !Values(c_name, c, error) || !Values(e_name, e, error)) return false;
            if (zeta.size() != k || layers < 2 || c.size() < 2 * m * k || e.size() < 2 * m) {
                error = c_name + " must be <rank> " + std::to_string(zeta.size()) + " 2 (" + zeta_name + "'s functions, two spins) and " + e_name + " <rank> 2";
                return false;
            }
            for (std::size_t spin = 0; spin < 2; spin++) {
                Ir77Radial::Ir77RadialBlock& b = blocks[spin][l];
                b.l = static_cast<std::uint32_t>(l);
                b.zeta = zeta;
                b.m = m;
                b.coefficients.assign(c.begin() + static_cast<std::ptrdiff_t>(spin * m * k), c.begin() + static_cast<std::ptrdiff_t>((spin + 1) * m * k));
                b.energies.assign(e.begin() + static_cast<std::ptrdiff_t>(spin * m), e.begin() + static_cast<std::ptrdiff_t>((spin + 1) * m));
                b.keep = static_cast<std::size_t>(std::max(keep[l], 0.0));
                for (std::size_t r = 0; r < radial; r++) b.occupations.push_back(restricted ? occupation(r, l, 0) + occupation(r, l, 1) : occupation(r, l, spin));
            }
        }

        Ir77ProgramOutput output{};
        output.name = name;
        output.format = ORBITALS_FORMAT;
        output.molecule.atoms = {Ir77Atom{static_cast<std::uint32_t>(std::lround(z)), 0.0, 0.0, 0.0}};
        output.molecule.charge = static_cast<std::int32_t>(std::lround(charge));
        output.molecule.multiplicity = static_cast<std::uint32_t>(unpaired) + 1;
        output.molecule.basis = "slater";

        Ir77ScfResult& r = output.scf;
        Ir77Radial::Build(blocks[0], output.basis, r.orbital_energies, r.occupations, r.coefficients, r.orbital_names);
        if (!restricted) {
            Ir77Basis again{};
            Ir77Radial::Build(blocks[1], again, r.beta_orbital_energies, r.beta_occupations, r.beta_coefficients, r.beta_orbital_names);
        }
        r.unrestricted = !restricted;
        r.function_kind = ORBITALS_FUNCTION_SLATER;
        r.energy = Symbol("energy");
        r.nuclear_repulsion = 0.0;
        r.iterations = static_cast<std::uint32_t>(Symbol("iteration"));
        r.converged = Symbol("converged") != 0.0;
        r.error = Symbol("error");
        r.electrons = static_cast<std::uint32_t>(std::lround(alpha_electrons + beta_electrons));
        r.multiplicity = output.molecule.multiplicity;
        r.alpha = static_cast<std::uint32_t>((r.electrons + static_cast<std::uint32_t>(unpaired)) / 2);
        r.beta = r.electrons - r.alpha;
        r.s2 = 0.25 * unpaired * (unpaired + 2.0);
        r.occupied = 0;
        for (std::size_t k = 0; k < r.occupations.size(); k++)
            if (r.occupations[k] > 0.0) r.occupied = static_cast<std::uint32_t>(k + 1);

        m_result->outputs.push_back(std::move(output));
        return true;
    }

    // the grid file -- the buffers as channels of one grid, d0 x d1 x d2 (the first buffer's), over [lo, hi]^3
    bool OutputGrid(std::string const& name, std::string const& lo_text, std::string const& hi_text, std::vector<std::string> const& names, std::string& error) {
        if (!FileName(name, error)) return false;
        double lo = 0.0, hi = 0.0;
        if (!Evaluate(lo_text, lo, error) || !Evaluate(hi_text, hi, error)) return false;
        if (names.size() > COMPUTE_MAX_CHANNELS) {
            error = "at most " + std::to_string(COMPUTE_MAX_CHANNELS) + " channels";
            return false;
        }

        Ir77ProgramOutput output{};
        output.name = name;
        output.format = GRID_FORMAT;
        output.domain = {static_cast<float>(lo), static_cast<float>(hi), static_cast<float>(lo), static_cast<float>(hi), static_cast<float>(lo), static_cast<float>(hi)};

        std::vector<std::vector<double>*> channels{};
        for (std::string const& b : names) {
            Ir77StageBuffer* buffer = Find(b);
            if (!buffer || buffer->ints) {
                error = "no double buffer " + b;
                return false;
            }
            if (channels.empty()) {
                for (std::size_t d = 0; d < 3; d++) output.size[d] = d < buffer->dims.size() ? static_cast<std::uint32_t>(Dimension(buffer->dims[d])) : 1u;
            }
            std::size_t const points = static_cast<std::size_t>(output.size[0]) * output.size[1] * output.size[2];
            std::vector<double>* values = Doubles(b, points, error);
            if (!values) return false;
            if (points > COMPUTE_MAX_POINTS) {
                error = std::to_string(points) + " grid points -- at most " + std::to_string(COMPUTE_MAX_POINTS);
                return false;
            }
            channels.push_back(values);
            output.channels.push_back(b);
        }

        std::size_t const points = static_cast<std::size_t>(output.size[0]) * output.size[1] * output.size[2];
        std::size_t const stride = channels.size();
        output.values.resize(points * stride);
        output.largest.assign(stride, 0.0);
        for (std::size_t p = 0; p < points; p++)
            for (std::size_t c = 0; c < stride; c++) {
                double const v = (*channels[c])[p];
                output.values[p * stride + c] = static_cast<float>(v);
                output.largest[c] = std::max(output.largest[c], fabs(v));
            }
        output.passes = static_cast<std::uint32_t>(Symbol("iteration"));
        output.residual = Symbol("error");
        output.converged = Symbol("converged") != 0.0;

        m_result->outputs.push_back(std::move(output));
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
    std::string m_integrals{};

    std::atomic<bool> const* m_cancel{nullptr};
    Ir77ProgramResult* m_result{nullptr};

    bool m_use_gaussian{false};
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
    std::map<std::string, std::vector<Ir77Atom>> m_molecules{};  // @const molecule (bohr)
    bool m_constant_only{false};                     // evaluating a @const -- constants only
    std::uint32_t m_depth{0};                        // @repeat loops open

    cl_program m_program{nullptr};

    bool m_have_molecule{false};
    Ir77Molecule m_molecule{};
    Ir77Basis m_basis{};

    std::map<std::string, std::vector<std::vector<double>>> m_focks{}, m_errors{};  // diis -- a history a diis step
};

}  // namespace NSIr77Compute
