// =============================================================================================================================================
// orbitals -- Compute's field program: a molecule's orbitals and electron density, read from field.data. Compute owns this format -- it writes
// the data and sends this source as field.compile with it, so the writer and the reader live together and Fields stays generic (it only runs
// the function; include/view.glsl draws it -- a cloud on particles, a slice on a surface; see its header for the view's params p[3..7]).
//
//   p[0] what   -1 the total density; then the orbitals in the data's order, evenly across to the last at +1
//
// Units: bohr, hartree. Until Compute sends it, paste it into the Fields panel and load data with the data button (h2.sto3g.json,
// p.sto3g.json beside it -- written and checked by orbitals.data.py).
//
// ---- the layout (version 1) -- field.data, every value a float (indices and counts are whole numbers, exact to 2^24) ---------------------
//
//   header    8      version (1), atoms A, shells S, primitives P, basis functions N, orbitals M, total energy, 0
//   atoms     A x 4  x, y, z, Z (nuclear charge)                         -- bohr, world axes (Y up)
//   shells    S x 4  atom, l (0 s, 1 p, 2 d), first primitive, primitive count
//   primitives P x 2 exponent alpha, coefficient d (on the normalized primitive, contraction included)
//   orbitals  M x (2 + N)  occupation, energy, then N coefficients on the basis functions
//
// Basis functions are the shells in order, each shell's Cartesian components in order:
//   s: 1      p: x, y, z      d: xx, xy, xz, yy, yz, zz        (N = the sum of 1 / 3 / 6 over the shells)
// One function: phi(r) = sum_k d_k Nk x^a y^b z^c exp(-alpha_k |r - atom|^2), x y z from its atom, Nk the normalized Cartesian primitive's
//   Nk = (2 alpha / pi)^(3/4) (4 alpha)^(l/2) / sqrt((2a-1)!! (2b-1)!! (2c-1)!!)
// so the solver's integrals must be over exactly these functions (their own normalization, their own order). Shells past d are skipped.
// =============================================================================================================================================

#define ORBITALS_VERSION 1u
#define ORBITALS_HEADER 8u
#define ORBITALS_ATOM 4u
#define ORBITALS_SHELL 4u
#define ORBITALS_PRIMITIVE 2u
#define ORBITALS_MAX_L 2u

// where everything sits in field.data -- valid = a version-1 layout that fits in what was uploaded
struct Ir77Orbitals {
    bool valid;
    uint atoms;
    uint shells;
    uint primitives;
    uint functions;
    uint orbitals;
    float energy;
    uint atom_at;
    uint shell_at;
    uint primitive_at;
    uint orbital_at;
};

Ir77Orbitals Ir77OrbitalsLayout() {
    Ir77Orbitals o;
    o.valid = false;
    o.atoms = 0u;
    o.shells = 0u;
    o.primitives = 0u;
    o.functions = 0u;
    o.orbitals = 0u;
    o.energy = 0.0;
    o.atom_at = 0u;
    o.shell_at = 0u;
    o.primitive_at = 0u;
    o.orbital_at = 0u;

    if (Ir77FieldDataCount() < ORBITALS_HEADER || uint(Ir77FieldData(0u) + 0.5) != ORBITALS_VERSION) return o;

    o.atoms = uint(Ir77FieldData(1u) + 0.5);
    o.shells = uint(Ir77FieldData(2u) + 0.5);
    o.primitives = uint(Ir77FieldData(3u) + 0.5);
    o.functions = uint(Ir77FieldData(4u) + 0.5);
    o.orbitals = uint(Ir77FieldData(5u) + 0.5);
    o.energy = Ir77FieldData(6u);

    o.atom_at = ORBITALS_HEADER;
    o.shell_at = o.atom_at + o.atoms * ORBITALS_ATOM;
    o.primitive_at = o.shell_at + o.shells * ORBITALS_SHELL;
    o.orbital_at = o.primitive_at + o.primitives * ORBITALS_PRIMITIVE;

    o.valid = o.orbital_at + o.orbitals * (2u + o.functions) <= Ir77FieldDataCount();
    return o;
}

vec3 Ir77OrbitalAtom(Ir77Orbitals o, uint atom) {
    uint at = o.atom_at + atom * ORBITALS_ATOM;
    return vec3(Ir77FieldData(at), Ir77FieldData(at + 1u), Ir77FieldData(at + 2u));
}

float Ir77OrbitalCharge(Ir77Orbitals o, uint atom) { return Ir77FieldData(o.atom_at + atom * ORBITALS_ATOM + 3u); }

float Ir77OrbitalOccupation(Ir77Orbitals o, uint orbital) { return Ir77FieldData(o.orbital_at + orbital * (2u + o.functions)); }

float Ir77OrbitalEnergy(Ir77Orbitals o, uint orbital) { return Ir77FieldData(o.orbital_at + orbital * (2u + o.functions) + 1u); }

// a shell's radial part at r^2 from its atom -- sum_k d_k (2 alpha / pi)^(3/4) (4 alpha)^(l/2) exp(-alpha r^2)
float Ir77ShellRadial(Ir77Orbitals o, uint first, uint count, uint l, float r2) {
    float sum = 0.0;

    for (uint k = 0u; k < count; k++) {
        uint at = o.primitive_at + (first + k) * ORBITALS_PRIMITIVE;
        float alpha = Ir77FieldData(at);
        float d = Ir77FieldData(at + 1u);

        float norm = pow(2.0 * alpha / 3.14159265359, 0.75) * pow(4.0 * alpha, 0.5 * float(l));
        sum += d * norm * exp(-alpha * r2);
    }

    return sum;
}

// One orbital (0-based) at p -- signed. Each shell's radial part once, its components from it.
float Ir77Orbital(Ir77Orbitals o, uint orbital, vec3 p) {
    if (!o.valid || orbital >= o.orbitals) return 0.0;

    uint coefficients = o.orbital_at + orbital * (2u + o.functions) + 2u;
    uint mu = 0u;
    float psi = 0.0;

    for (uint s = 0u; s < o.shells; s++) {
        uint at = o.shell_at + s * ORBITALS_SHELL;
        uint atom = uint(Ir77FieldData(at) + 0.5);
        uint l = uint(Ir77FieldData(at + 1u) + 0.5);
        uint first = uint(Ir77FieldData(at + 2u) + 0.5);
        uint count = uint(Ir77FieldData(at + 3u) + 0.5);

        uint components = l == 0u ? 1u : l == 1u ? 3u : 6u;
        if (l > ORBITALS_MAX_L) {  // past d -- its (l+1)(l+2)/2 functions skipped
            mu += (l + 1u) * (l + 2u) / 2u;
            continue;
        }

        vec3 x = p - Ir77OrbitalAtom(o, atom);
        float radial = Ir77ShellRadial(o, first, count, l, dot(x, x));

        if (l == 0u) {
            psi += Ir77FieldData(coefficients + mu) * radial;
        } else if (l == 1u) {
            psi += radial * (Ir77FieldData(coefficients + mu) * x.x + Ir77FieldData(coefficients + mu + 1u) * x.y + Ir77FieldData(coefficients + mu + 2u) * x.z);
        } else {
            // xx xy xz yy yz zz -- the squares carry 1 / sqrt(3!!) = 1 / sqrt(3)
            const float SQUARE = 0.57735027;
            psi += radial * (Ir77FieldData(coefficients + mu) * x.x * x.x * SQUARE + Ir77FieldData(coefficients + mu + 1u) * x.x * x.y +
                             Ir77FieldData(coefficients + mu + 2u) * x.x * x.z + Ir77FieldData(coefficients + mu + 3u) * x.y * x.y * SQUARE +
                             Ir77FieldData(coefficients + mu + 4u) * x.y * x.z + Ir77FieldData(coefficients + mu + 5u) * x.z * x.z * SQUARE);
        }

        mu += components;
    }

    return psi;
}

// The electron density at p -- sum over orbitals of occupation x psi^2 (electrons per cubic bohr)
float Ir77OrbitalDensity(Ir77Orbitals o, vec3 p) {
    float rho = 0.0;

    for (uint i = 0u; i < o.orbitals; i++) {
        float occupation = Ir77OrbitalOccupation(o, i);
        if (occupation <= 0.0) continue;

        float psi = Ir77Orbital(o, i, p);
        rho += occupation * psi * psi;
    }

    return rho;
}

// ---- the function -------------------------------------------------------------------------------------------------------------------------

Ir77Sample Sample(vec3 p, float params[8]) {
    Ir77Orbitals o = Ir77OrbitalsLayout();
    int what = int((params[0] + 1.0) * 0.5 * float(o.orbitals) + 0.5);  // 0 density, 1.. orbital

    Ir77Sample s;
    if (what <= 0) {
        s.value = Ir77OrbitalDensity(o, p);
        s.weight = s.value;
    } else {
        s.value = Ir77Orbital(o, uint(what - 1), p);
        s.weight = s.value * s.value;
    }

    return s;
}

#include "include/view.glsl"
