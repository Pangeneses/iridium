#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace NSIr77Compute {

// =============================================================================================================================================
// Compute's types -- molecules, basis functions, a program's run and what it made. Units: bohr, hartree.
// =============================================================================================================================================

inline constexpr double BOHR_PER_ANGSTROM{1.8897261254578281};

// atomic numbers below this have a slot in per-element tables (the embedded sets reach Ar, 18)
inline constexpr std::uint32_t BASIS_ELEMENT_LIMIT{19};

// a molecule step's -- atoms by atomic number, positions in bohr (world axes, Y up)
struct Ir77Atom {
    std::uint32_t number{1};
    double x{0.0};
    double y{0.0};
    double z{0.0};
};

struct Ir77Molecule {
    std::vector<Ir77Atom> atoms{};
    std::int32_t charge{0};
    std::uint32_t multiplicity{0};  // 2S + 1 -- 0: the lowest the electron count allows (1 even, 2 odd)
    std::string basis{"sto-3g"};
};

// One Cartesian Gaussian basis function -- x^a y^b z^c exp(-alpha r^2) about its atom, contracted. coefficients carry the normalized
// primitive's factor for this component (Compute/kernels/Ir77Integrals.cl); contraction holds the published coefficients, renormalized --
// what Shaders/fields/orbitals.glsl's layout stores (it applies the component factor itself).
struct Ir77BasisFunction {
    std::array<std::int32_t, 3> powers{0, 0, 0};
    std::uint32_t atom{0};
    std::uint32_t shell{0};
    std::vector<double> exponents{};
    std::vector<double> coefficients{};  // contraction x the component's primitive normalization
};

// a shell -- one l on one atom, its components the functions [first_function, + (l+1)(l+2)/2)
struct Ir77BasisShell {
    std::uint32_t atom{0};
    std::uint32_t l{0};
    std::uint32_t first_function{0};
    std::vector<double> exponents{};
    std::vector<double> contraction{};  // renormalized, on normalized primitives
};

struct Ir77Basis {
    std::vector<Ir77BasisShell> shells{};
    std::vector<Ir77BasisFunction> functions{};
};

// what a solve answers
struct Ir77ScfResult {
    bool converged{false};
    std::uint32_t iterations{0};
    double energy{0.0};               // total, hartree
    double nuclear_repulsion{0.0};
    double error{0.0};                // the last DIIS error
    std::uint32_t electrons{0};
    std::uint32_t occupied{0};        // restricted: doubly occupied orbitals; unrestricted: alpha electrons
    std::vector<double> orbital_energies{};  // ascending -- unrestricted: the alpha orbitals'
    std::vector<double> coefficients{};      // functions x orbitals, column-major: orbital k's are [k n, k n + n) -- unrestricted: alpha
    bool unrestricted{false};
    std::uint32_t multiplicity{1};
    std::uint32_t alpha{0};           // electrons of each spin
    std::uint32_t beta{0};
    std::vector<double> beta_orbital_energies{};  // unrestricted only
    std::vector<double> beta_coefficients{};
    double s2{0.0};                   // <S^2> -- exact S(S+1) when restricted; above it, the spin contamination
    std::uint32_t function_kind{0};   // ORBITALS_FUNCTION_GAUSSIAN / _SLATER
    std::vector<double> occupations{};  // per orbital when they aren't 2 / 0 or 1 / 0 (a Slater atom's spread subshells) -- empty: from occupied
    std::vector<double> beta_occupations{};  // the same for the beta set -- empty: from beta
    std::vector<std::string> orbital_names{};       // per orbital when the solver knows them ("2p x", "3d z2" -- a Slater atom); empty: MO k
    std::vector<std::string> beta_orbital_names{};
    bool used_device{false};
    std::string device{};
    double seconds{0.0};
};

// The "orbitals" data format -- what a solve writes to <project>/data/<name>.json "values" (read by Shaders/fields/orbitals.glsl). It describes
// itself: the header carries its own length and every block's stride, so a reader takes them from there, not from these.
//   header     16  version, header length, atoms A, shells S, primitives P, functions N, orbitals M, total energy, atom stride, shell stride,
//                  primitive stride, orbital stride, function kind (0 Gaussian), electrons, occupied, 0
//   atoms      A x ORBITALS_ATOM_STRIDE       x, y, z (bohr), Z
//   shells     S x ORBITALS_SHELL_STRIDE      atom, l, first primitive, primitive count
//   primitives P x ORBITALS_PRIMITIVE_STRIDE  exponent, coefficient (on the normalized primitive)
//   (header 17: the density's size -- one atom, bohr holding 90% of the electrons; 0 otherwise)
//   orbitals   M x (4 + N)                    occupation, energy, N coefficients, the orbital's peak |psi|, its size (one atom: bohr holding 90%
//                                             of it; 0 otherwise) (Ir77Peaks) -- unrestricted: 2M rows
inline constexpr char const* ORBITALS_FORMAT{"orbitals"};
inline constexpr std::uint32_t ORBITALS_LAYOUT_VERSION{2};
inline constexpr std::uint32_t ORBITALS_HEADER{16};
inline constexpr std::uint32_t ORBITALS_ATOM_STRIDE{4};
inline constexpr std::uint32_t ORBITALS_SHELL_STRIDE{4};
inline constexpr std::uint32_t ORBITALS_PRIMITIVE_STRIDE{2};
inline constexpr std::uint32_t ORBITALS_FUNCTION_GAUSSIAN{0};
inline constexpr std::uint32_t ORBITALS_FUNCTION_SLATER{1};  // one primitive a shell: N_l(zeta) exp(-zeta r) x the Cartesian component

// where programs write -- <project>/data/<name>.json (Fields' field.data.load reads the same folder)
inline constexpr char const* COMPUTE_DATA_FOLDER{"data"};

// ---- programs -- one OpenCL source each, its constants, buffers, kernels and script in it (services/Ir77Stages.hpp) -----------------------

inline constexpr std::uint64_t COMPUTE_STAGE_CHUNK{1u << 18};     // a kernel's work items per launch -- every launch short (a display driver
                                                                  // kills kernels that run for seconds)
inline constexpr std::uint32_t COMPUTE_DIIS_SIZE{8};
inline constexpr double COMPUTE_ORTHOGONAL_DROP{1e-7};       // orthogonalize -- S's eigenvalues under this are dropped (unless the step says)
inline constexpr std::uint64_t COMPUTE_KEEP_LIMIT{16u * 1024u * 1024u};  // a buffer's values kept for the inspector up to this many
inline constexpr std::uint32_t COMPUTE_VIEW_SIDE{256};            // compute.buffer -- a matrix or slice sampled down to this many a side
inline constexpr std::uint32_t COMPUTE_VIEW_LENGTH{4096};         // ... a vector to this many
inline constexpr std::uint64_t COMPUTE_MAX_POINTS{16u * 1024u * 1024u};  // a grid output's points
inline constexpr std::uint32_t COMPUTE_MAX_CHANNELS{8};           // a grid output's channels
inline constexpr char const* GRID_FORMAT{"grid"};                 // a grid output -- Shaders/fields/grid.glsl reads it
inline constexpr std::uint32_t GRID_VERSION{1};
inline constexpr std::uint32_t GRID_HEADER{16};                   // then a float a channel: its largest |value|

// one run
struct Ir77ProgramJob {
    std::string source{};
    std::string program{};            // its name, as the panel had it -- reported back
};

// a data file a program made -- <project>/data/<name>.json
struct Ir77ProgramOutput {
    std::string name{};
    std::string format{};             // ORBITALS_FORMAT / GRID_FORMAT
    // orbitals
    Ir77Molecule molecule{};
    Ir77Basis basis{};
    Ir77ScfResult scf{};
    // grid -- values size[0] x size[1] x size[2] x channels, x fastest
    std::array<std::uint32_t, 3> size{1, 1, 1};
    std::array<float, 6> domain{-1, 1, -1, 1, -1, 1};
    std::vector<std::string> channels{};
    std::vector<float> values{};
    std::vector<double> largest{};    // a channel's largest |value|
    std::uint32_t passes{0};
    double residual{0.0};
    bool converged{false};
};

// a buffer as the run left it -- the inspector's
struct Ir77ProgramBuffer {
    std::string name{};
    bool ints{false};
    std::vector<std::size_t> dims{};  // d0 fastest
    std::size_t size{0};
    double minimum{0.0};
    double maximum{0.0};
    std::vector<double> values{};     // empty above COMPUTE_KEEP_LIMIT
};

// what it made
struct Ir77ProgramResult {
    double seconds{0.0};
    std::string device{};
    std::vector<std::pair<std::string, double>> report{};  // its @report symbols, in order
    std::vector<Ir77ProgramOutput> outputs{};
    std::vector<Ir77ProgramBuffer> buffers{};
};

// the job queue's state
enum class Ir77JobState : std::uint8_t { Idle, Running, Done, Failed, Cancelled };

}  // namespace NSIr77Compute
