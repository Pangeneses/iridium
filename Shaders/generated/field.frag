#version 460
#extension GL_GOOGLE_include_directive : require
#define IR77_STAGE_FRAGMENT
#define IR77_HAS_MARKERS
#include "include/bindless.glsl"
#include "include/field.glsl"
#line 1
// =============================================================================================================================================
// orbitals -- a molecule's orbitals and electron density, from a data file Compute wrote (<project>/data/<name>.json, format "orbitals").
// Tick "data" in the panel and pick the file; every compile reads it again, so a re-solve under the same name shows on the next compile.
// Topology picks the view (include/view.glsl): isosurface -- the orbital's lobes at |psi| = level; particles -- a cloud; surface -- a slice.
// The nuclei are markers -- lit spheres in CPK colours, radius (nucleus slider) x Z^(1/3); a real nucleus is ~1e-5 bohr, so they're drawn
// far larger than life. Cut a half away to see them inside the lobes.
//
// ---- the data (version 2) -- every value a float (counts and indices are whole numbers, exact to 2^24) ------------------------------------
//   header     H          version 2, H (16), atoms A, shells S, primitives P, functions N, orbitals M, total energy,
//                         atom stride, shell stride, primitive stride, orbital stride, function kind (0 Gaussian), electrons, occupied
//                         (restricted: pairs; unrestricted: alpha electrons), spin (0 restricted, 1 unrestricted)
//   atoms      A x stride x, y, z, Z                             -- bohr
//   shells     S x stride atom, l (0 s, 1 p, 2 d), first primitive, primitive count
//   primitives P x stride exponent alpha, coefficient d (on the normalized primitive, contraction included)
//   orbitals   M x stride occupation, energy (hartree), then N coefficients, then (stride 3 + N) the orbital's peak |psi|, (stride 4 + N)
//              its size -- one atom: the bohr holding 90% of it -- unrestricted: M alpha rows, then M beta rows (occupations 1 / 0)
//   (a header of 17 or more: [16] the density's size, one atom)
// Each shell's Cartesian components in order: s; p x, y, z; d xx, xy, xz, yy, yz, zz; f xxx, xxy, xxz, xyy, xyz, xzz, yyy, yyz, yzz, zzz. Each
// carries 1 / sqrt((2a-1)!! (2b-1)!! (2c-1)!!): 1, except d's squares 1/sqrt(3), f's cubes 1/sqrt(15) and its x2y-likes 1/sqrt(3).
// Function kind 0 Gaussian -- primitive norm (2a/pi)^(3/4) (4a)^(l/2) exp(-a r^2); kind 1 Slater (a Compute "slater" atom: the nuclear cusp)
// -- N_l exp(-zeta r), N_l = sqrt((2l+1)!! (2 zeta)^(2l+3) / (4 pi (2l+2)!)).
// The strides come from the header, so a writer may add columns without breaking this reader.
// =============================================================================================================================================

// @param 0 orbital 0 40 0 step=1
// @param 1 sign 0 1 0 step=1
// @param 2 spin 0 2 0 step=1
// @param 3 level 0.0001 1 0.1 log isosurface
// @param 4 scale 0.05 20 1 log
// @param 5 nucleus 0 1 0.12 isosurface
// @param 6 zoom 0.01 100 1 log isosurface
// @param 7 cut 0 3 0 step=1 particles isosurface
// @param 3 brightness 0.001 1 0.04 log particles
// @param 5 size 0.001 0.5 0.015 log particles
// @param 6 step 0.01 4 0.3 log particles
// @param 3 height -10 10 0 surface
// @param 5 plane 0 2 0 step=1 surface
// @param 6 offset -20 20 0 surface
//
// p[0] orbital: 0 the density, 1 .. M the orbitals lowest first (past M: the last). An orbital is shown over its own peak |psi| when the data
//      carries it, so the level is a fraction of the peak -- 0.05 draws the lobes of a tight 1s and a faint Rydberg 4f alike; the density's
//      level stays absolute (electrons per cubic bohr), so sweeping it walks out through the shells
// p[1] sign: 1 flips the orbital's overall sign (a solver's sign is arbitrary -- this keeps colours steady between solves)
// p[2] spin: 0 alpha (restricted: the orbital), 1 beta, 2 the spin density alpha - beta (orange where alpha is in excess, blue where beta)
//      -- with orbital 0 and spin 0 / 1: the total density
// p[5] nucleus: the markers' radius (box units) for Z = 1; isosurface only -- the cloud uses 0.12, the slice none
// p[6] zoom: one atom -- each orbital (and the density) is framed to fill FIT of the box (its size from the data), so a core 1s and a
//      Rydberg 5f both fill it; zoom magnifies from there (10: a tenth of the orbital fills the box -- its inner layers). A molecule is
//      shown at its own size; zoom magnifies it. Isosurface only (the other views use p[6] themselves)
// p[7] cut: 0 none, 1 / 2 / 3 the +x / +y / +z half cut away

#define ORBITALS_VERSION 2u
#define ORBITALS_MAX_L 3u
#define ORBITALS_MAX_SHELLS 64u  // shells whose radial parts are worked out once a point (past it: once a row)
#define ORBITALS_FIT 0.8          // the share of the box's half width an atom's orbital is framed to

struct Ir77Orbitals {
    bool valid;
    bool unrestricted;  // M alpha rows, then M beta
    bool slater;        // function kind 1
    uint atoms;
    uint shells;
    uint primitives;
    uint functions;
    uint orbitals;
    uint atom_stride;
    uint shell_stride;
    uint primitive_stride;
    uint orbital_stride;
    uint atom_at;
    uint shell_at;
    uint primitive_at;
    uint orbital_at;
};

uint Ir77OrbitalsWhole(uint i) { return uint(max(Ir77FieldData(i), 0.0) + 0.5); }

Ir77Orbitals Ir77OrbitalsLayout() {
    Ir77Orbitals o;
    o.valid = false;
    o.unrestricted = false;
    o.slater = false;
    o.atoms = 0u;
    o.shells = 0u;
    o.primitives = 0u;
    o.functions = 0u;
    o.orbitals = 0u;
    o.atom_stride = 4u;
    o.shell_stride = 4u;
    o.primitive_stride = 2u;
    o.orbital_stride = 2u;
    o.atom_at = 0u;
    o.shell_at = 0u;
    o.primitive_at = 0u;
    o.orbital_at = 0u;

    if (Ir77FieldDataCount() < 16u || Ir77OrbitalsWhole(0u) != ORBITALS_VERSION) return o;

    uint header = Ir77OrbitalsWhole(1u);
    o.atoms = Ir77OrbitalsWhole(2u);
    o.shells = Ir77OrbitalsWhole(3u);
    o.primitives = Ir77OrbitalsWhole(4u);
    o.functions = Ir77OrbitalsWhole(5u);
    o.orbitals = Ir77OrbitalsWhole(6u);
    o.atom_stride = Ir77OrbitalsWhole(8u);
    o.shell_stride = Ir77OrbitalsWhole(9u);
    o.primitive_stride = Ir77OrbitalsWhole(10u);
    o.orbital_stride = Ir77OrbitalsWhole(11u);
    o.unrestricted = Ir77OrbitalsWhole(15u) == 1u;

    o.slater = Ir77OrbitalsWhole(12u) == 1u;
    if (Ir77OrbitalsWhole(12u) > 1u || o.atom_stride < 4u || o.shell_stride < 4u || o.primitive_stride < 2u || o.orbital_stride < 2u + o.functions)
        return o;

    o.atom_at = header;
    o.shell_at = o.atom_at + o.atoms * o.atom_stride;
    o.primitive_at = o.shell_at + o.shells * o.shell_stride;
    o.orbital_at = o.primitive_at + o.primitives * o.primitive_stride;

    o.valid = o.orbital_at + o.orbitals * (o.unrestricted ? 2u : 1u) * o.orbital_stride <= Ir77FieldDataCount();
    return o;
}

vec3 Ir77OrbitalAtom(Ir77Orbitals o, uint atom) {
    uint at = o.atom_at + atom * o.atom_stride;
    return vec3(Ir77FieldData(at), Ir77FieldData(at + 1u), Ir77FieldData(at + 2u));
}

float Ir77OrbitalOccupation(Ir77Orbitals o, uint orbital) { return Ir77FieldData(o.orbital_at + orbital * o.orbital_stride); }

// a row's size (bohr) -- 0 when the data doesn't carry it; row -1: the density's
float Ir77OrbitalExtent(Ir77Orbitals o, int row) {
    if (row < 0) return o.atom_at >= 17u ? Ir77FieldData(16u) : 0.0;
    if (o.orbital_stride < 4u + o.functions) return 0.0;
    return Ir77FieldData(o.orbital_at + uint(row) * o.orbital_stride + 3u + o.functions);
}

// the row on show -- -1 the density (orbital 0 or the spin density), else the alpha or beta row
int Ir77OrbitalShown(Ir77Orbitals o, float params[8]) {
    int what = min(int(params[0] + 0.5), int(o.orbitals));
    int spin = clamp(int(params[2] + 0.5), 0, 2);
    if (spin == 2 || what <= 0) return -1;
    return what - 1 + (o.unrestricted && spin == 1 ? int(o.orbitals) : 0);
}

// function units per box unit -- one atom: the shown orbital's size over FIT of the box's half width, over the zoom; a molecule: 1 / zoom
float Ir77OrbitalFrame(Ir77Orbitals o, float params[8]) {
    float zoom = Ir77FieldTopology() == 2u && params[6] > 0.0 ? params[6] : 1.0;
    float extent = o.atoms == 1u ? Ir77OrbitalExtent(o, Ir77OrbitalShown(o, params)) : 0.0;
    if (extent <= 0.0) return 1.0 / zoom;

    Ir77FieldConfig cfg = ir77_field_configs[IR77_FIELD_CONFIG_SLOT].data[0];
    float half_width = 0.5 * min(min(cfg.domain_x.y - cfg.domain_x.x, cfg.domain_x.w - cfg.domain_x.z), cfg.domain_z.y - cfg.domain_z.x);
    return extent / (ORBITALS_FIT * max(half_width, 1e-6)) / zoom;
}

// a row's peak |psi| -- 1 when the data doesn't carry it (stride 2 + N)
float Ir77OrbitalPeak(Ir77Orbitals o, uint row) {
    if (o.orbital_stride < 3u + o.functions) return 1.0;
    return max(Ir77FieldData(o.orbital_at + row * o.orbital_stride + 2u + o.functions), 1e-30);
}

// a shell's radial part at r^2 from its atom -- Gaussian: sum_k d_k (2 alpha / pi)^(3/4) (4 alpha)^(l/2) exp(-alpha r^2); Slater: sum_k d_k
// N_l(zeta) exp(-zeta r)
float Ir77ShellRadial(Ir77Orbitals o, uint first, uint count, uint l, float r2) {
    float sum = 0.0;
    float r = sqrt(r2);

    // Slater: (2l+1)!! / (4 pi (2l+2)!)
    const float SLATER[4] = float[4](1.0 / (4.0 * 3.14159265359 * 2.0), 3.0 / (4.0 * 3.14159265359 * 24.0), 15.0 / (4.0 * 3.14159265359 * 720.0),
                                     105.0 / (4.0 * 3.14159265359 * 40320.0));

    for (uint k = 0u; k < count; k++) {
        uint at = o.primitive_at + (first + k) * o.primitive_stride;
        float alpha = Ir77FieldData(at);
        float d = Ir77FieldData(at + 1u);

        if (o.slater) {
            float norm = sqrt(SLATER[min(l, 3u)] * pow(2.0 * alpha, float(2u * l + 3u)));
            sum += d * norm * exp(-alpha * r);
        } else {
            float norm = pow(2.0 * alpha / 3.14159265359, 0.75) * pow(4.0 * alpha, 0.5 * float(l));
            sum += d * norm * exp(-alpha * r2);
        }
    }

    return sum;
}

// every shell's radial part at p (up to ORBITALS_MAX_SHELLS) -- worked out once, shared by every orbital row
void Ir77OrbitalRadials(Ir77Orbitals o, vec3 p, out float radial[ORBITALS_MAX_SHELLS]) {
    for (uint s = 0u; s < ORBITALS_MAX_SHELLS; s++) {
        radial[s] = 0.0;
        if (s >= o.shells) continue;

        uint at = o.shell_at + s * o.shell_stride;
        vec3 x = p - Ir77OrbitalAtom(o, Ir77OrbitalsWhole(at));
        radial[s] = Ir77ShellRadial(o, Ir77OrbitalsWhole(at + 2u), Ir77OrbitalsWhole(at + 3u), Ir77OrbitalsWhole(at + 1u), dot(x, x));
    }
}

// orbital rows -- restricted: M; unrestricted: 2M (alpha then beta)
uint Ir77OrbitalRows(Ir77Orbitals o) { return o.orbitals * (o.unrestricted ? 2u : 1u); }

// one orbital row at p -- signed. radial: Ir77OrbitalRadials at p (shells past ORBITALS_MAX_SHELLS are worked out here)
float Ir77OrbitalWith(Ir77Orbitals o, uint row, vec3 p, float radial[ORBITALS_MAX_SHELLS]) {
    if (!o.valid || row >= Ir77OrbitalRows(o)) return 0.0;

    uint c = o.orbital_at + row * o.orbital_stride + 2u;
    uint mu = 0u;
    float psi = 0.0;

    for (uint s = 0u; s < o.shells; s++) {
        uint at = o.shell_at + s * o.shell_stride;
        uint atom = Ir77OrbitalsWhole(at);
        uint l = Ir77OrbitalsWhole(at + 1u);
        uint first = Ir77OrbitalsWhole(at + 2u);
        uint count = Ir77OrbitalsWhole(at + 3u);

        if (l > ORBITALS_MAX_L) {  // past f -- its (l+1)(l+2)/2 functions skipped
            mu += (l + 1u) * (l + 2u) / 2u;
            continue;
        }

        vec3 x = p - Ir77OrbitalAtom(o, atom);
        float rad = s < ORBITALS_MAX_SHELLS ? radial[s] : Ir77ShellRadial(o, first, count, l, dot(x, x));
        if (rad == 0.0) {
            mu += (l + 1u) * (l + 2u) / 2u;
            continue;
        }

        if (l == 0u) {
            psi += Ir77FieldData(c + mu) * rad;
            mu += 1u;
        } else if (l == 1u) {
            psi += rad * (Ir77FieldData(c + mu) * x.x + Ir77FieldData(c + mu + 1u) * x.y + Ir77FieldData(c + mu + 2u) * x.z);
            mu += 3u;
        } else if (l == 2u) {
            const float SQUARE = 0.57735027;  // xx yy zz carry 1 / sqrt(3!!)
            psi += rad * (Ir77FieldData(c + mu) * x.x * x.x * SQUARE + Ir77FieldData(c + mu + 1u) * x.x * x.y + Ir77FieldData(c + mu + 2u) * x.x * x.z +
                             Ir77FieldData(c + mu + 3u) * x.y * x.y * SQUARE + Ir77FieldData(c + mu + 4u) * x.y * x.z +
                             Ir77FieldData(c + mu + 5u) * x.z * x.z * SQUARE);
            mu += 6u;
        } else {
            const float CUBE = 0.25819889;  // xxx yyy zzz carry 1 / sqrt(5!!)
            const float PAIR = 0.57735027;  // xxy and its kind 1 / sqrt(3!!)
            psi += rad * (Ir77FieldData(c + mu) * x.x * x.x * x.x * CUBE + Ir77FieldData(c + mu + 1u) * x.x * x.x * x.y * PAIR +
                          Ir77FieldData(c + mu + 2u) * x.x * x.x * x.z * PAIR + Ir77FieldData(c + mu + 3u) * x.x * x.y * x.y * PAIR +
                          Ir77FieldData(c + mu + 4u) * x.x * x.y * x.z + Ir77FieldData(c + mu + 5u) * x.x * x.z * x.z * PAIR +
                          Ir77FieldData(c + mu + 6u) * x.y * x.y * x.y * CUBE + Ir77FieldData(c + mu + 7u) * x.y * x.y * x.z * PAIR +
                          Ir77FieldData(c + mu + 8u) * x.y * x.z * x.z * PAIR + Ir77FieldData(c + mu + 9u) * x.z * x.z * x.z * CUBE);
            mu += 10u;
        }
    }

    return psi;
}

// one orbital row at p
float Ir77Orbital(Ir77Orbitals o, uint row, vec3 p) {
    float radial[ORBITALS_MAX_SHELLS];
    Ir77OrbitalRadials(o, p, radial);
    return Ir77OrbitalWith(o, row, p, radial);
}

// the density over rows [from, to) -- sum of occupation x psi^2 (electrons per cubic bohr)
float Ir77OrbitalDensityWith(Ir77Orbitals o, uint from, uint to, vec3 p, float radial[ORBITALS_MAX_SHELLS]) {
    float rho = 0.0;

    for (uint i = from; i < to; i++) {
        float occupation = Ir77OrbitalOccupation(o, i);
        if (occupation <= 0.0) continue;

        float psi = Ir77OrbitalWith(o, i, p, radial);
        rho += occupation * psi * psi;
    }

    return rho;
}

Ir77Sample Sample(vec3 box, float params[8]) {
    Ir77Orbitals o = Ir77OrbitalsLayout();
    int what = min(int(params[0] + 0.5), int(o.orbitals));
    int spin = clamp(int(params[2] + 0.5), 0, 2);
    vec3 p = box * Ir77OrbitalFrame(o, params);  // the box's point in the function's units (bohr)

    float radial[ORBITALS_MAX_SHELLS];
    Ir77OrbitalRadials(o, p, radial);

    Ir77Sample s;
    if (spin == 2) {
        // alpha - beta: restricted has none
        s.value = o.unrestricted ? Ir77OrbitalDensityWith(o, 0u, o.orbitals, p, radial) - Ir77OrbitalDensityWith(o, o.orbitals, 2u * o.orbitals, p, radial) : 0.0;
        s.weight = abs(s.value);
    } else if (what <= 0) {
        s.value = Ir77OrbitalDensityWith(o, 0u, Ir77OrbitalRows(o), p, radial);
        s.weight = s.value;
    } else {
        uint row = uint(what - 1) + (o.unrestricted && spin == 1 ? o.orbitals : 0u);
        s.value = Ir77OrbitalWith(o, row, p, radial) / Ir77OrbitalPeak(o, row) * (params[1] > 0.5 ? -1.0 : 1.0);
        s.weight = s.value * s.value;
    }

    return s;
}

// CPK-ish colours -- H white, C grey, N blue, O red, F / Cl green, S yellow, P orange, the rest pink
vec3 Ir77ElementColor(uint z) {
    if (z == 1u) return vec3(0.95);
    if (z == 6u) return vec3(0.35);
    if (z == 7u) return vec3(0.15, 0.3, 0.95);
    if (z == 8u) return vec3(0.95, 0.1, 0.1);
    if (z == 9u || z == 17u) return vec3(0.2, 0.9, 0.2);
    if (z == 16u) return vec3(0.95, 0.85, 0.2);
    if (z == 15u) return vec3(1.0, 0.5, 0.0);
    return vec3(1.0, 0.4, 0.7);
}

// the nuclei -- one marker an atom
vec4 Marker(uint i, float params[8], out vec4 color) {
    color = vec4(1.0);

    Ir77Orbitals o = Ir77OrbitalsLayout();
    uint topology = Ir77FieldTopology();
    if (!o.valid || i >= o.atoms || topology == 0u) return vec4(0.0);

    uint z = Ir77OrbitalsWhole(o.atom_at + i * o.atom_stride + 3u);
    float radius = (topology == 2u ? params[5] : 0.12) * pow(float(max(z, 1u)), 1.0 / 3.0);

    color = vec4(pow(Ir77ElementColor(z), vec3(2.2)), 1.0);  // linear, as the lit pass expects
    return vec4(Ir77OrbitalAtom(o, i) / Ir77OrbitalFrame(o, params), radius);  // where the atom shows in the box -- its size stays the box's
}

#include "include/view.glsl"

#include "include/field_fragment.glsl"
