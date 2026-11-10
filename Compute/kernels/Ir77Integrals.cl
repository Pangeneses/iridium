// =============================================================================================================================================
// Ir77Integrals.cl -- Gaussian integrals by McMurchie-Davidson, one source for both sides: Ir77Integrals.hpp includes it as C++ (the CPU
// path and the one-electron integrals), Ir77Device builds it as OpenCL C (the two-electron kernel, Ir77Eri at the bottom). Plain C99, doubles,
// fixed private arrays -- nothing either side lacks.
//
// A basis function (what Compute calls one, and what Shaders/fields/orbitals.glsl evaluates):
//   phi(r) = sum_k c_k x^a y^b z^c exp(-alpha_k |r - A|^2)       c_k = the contraction coefficient times the normalized Cartesian primitive's
//                                                                   factor (2 alpha / pi)^(3/4) (4 alpha)^(l/2) / sqrt((2a-1)!! (2b-1)!! (2c-1)!!)
// Functions: IR77_FUNCTION ints each -- a, b, c, first primitive, primitive count, atom. Primitives: 2 doubles -- alpha, c. Atoms: 4 doubles
// -- x, y, z, Z. Up to d (l <= 2): an ERI's Hermite orders reach 8.
// =============================================================================================================================================
#ifndef IR77_INTEGRALS_CL
#define IR77_INTEGRALS_CL

#ifdef __OPENCL_VERSION__
#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#define IR77_FN
#define IR77_GLOBAL __global
#else
#define IR77_FN inline
#define IR77_GLOBAL
#endif

#define IR77_PI 3.14159265358979323846

#define IR77_FUNCTION 6       // ints per function
#define IR77_MAX_L 2          // d
#define IR77_E_I 5            // Hermite expansion table -- i, j up to 4 (the kinetic integral reaches l + 2)
#define IR77_E_T 9            // t up to i + j
#define IR77_MAX_N 9          // Hermite order of an ERI -- 4 l, + 1 for a derivative (a gradient's d/dA raises one function's l)
#define IR77_R_SIZE 640       // (N + 1) (Tx + 1) (Ty + 1) (Tz + 1) at worst for Tx + Ty + Tz = N = 9 (3 3 3)

// ---- the Boys function F_n(T), n = 0 .. m, into f[] -------------------------------------------------------------------------------------
IR77_FN void Ir77Boys(int m, double t, double* f) {
    if (t < 1e-14) {
        for (int n = 0; n <= m; n++) f[n] = 1.0 / (2.0 * n + 1.0);
        return;
    }

    double const e = exp(-t);

    if (t > 35.0) {
        // large T: F_0 = sqrt(pi / T) / 2 (erf(sqrt T) = 1 to double precision), then upward -- stable out here
        f[0] = 0.5 * sqrt(IR77_PI / t);
        for (int n = 0; n < m; n++) f[n + 1] = ((2.0 * n + 1.0) * f[n] - e) / (2.0 * t);
        return;
    }

    // the series at the top order, then downward: F_m = e^-T sum_k (2T)^k / ((2m+1)(2m+3)..(2m+2k+1))
    double term = 1.0 / (2.0 * m + 1.0);
    double sum = term;
    for (int k = 1; k < 200; k++) {
        term *= 2.0 * t / (2.0 * m + 2.0 * k + 1.0);
        sum += term;
        if (term < sum * 1e-17) break;
    }

    f[m] = e * sum;
    for (int n = m; n > 0; n--) f[n - 1] = (2.0 * t * f[n] + e) / (2.0 * n - 1.0);
}

// ---- Hermite expansion coefficients E^{ij}_t along one axis -- e[i][j][t] for i <= ia, j <= jb ------------------------------------------
// a, b the exponents, qx = A - B along the axis. E^{00}_0 = exp(-a b / p qx^2).
IR77_FN void Ir77Hermite(int ia, int jb, double a, double b, double qx, double* e) {
    double const p = a + b;
    double const q = a * b / p;

#define IR77_E(i, j, t) e[((i) * IR77_E_I + (j)) * IR77_E_T + (t)]
    for (int i = 0; i <= ia; i++)
        for (int j = 0; j <= jb; j++)
            for (int t = 0; t < IR77_E_T; t++) IR77_E(i, j, t) = 0.0;

    IR77_E(0, 0, 0) = exp(-q * qx * qx);

    // up in i along j = 0
    for (int i = 0; i < ia; i++)
        for (int t = 0; t <= i + 1; t++) {
            double v = -q * qx / a * IR77_E(i, 0, t);
            if (t > 0) v += IR77_E(i, 0, t - 1) / (2.0 * p);
            if (t + 1 <= i) v += (t + 1) * IR77_E(i, 0, t + 1);
            IR77_E(i + 1, 0, t) = v;
        }

    // up in j for every i
    for (int i = 0; i <= ia; i++)
        for (int j = 0; j < jb; j++)
            for (int t = 0; t <= i + j + 1; t++) {
                double v = q * qx / b * IR77_E(i, j, t);
                if (t > 0) v += IR77_E(i, j, t - 1) / (2.0 * p);
                if (t + 1 <= i + j) v += (t + 1) * IR77_E(i, j, t + 1);
                IR77_E(i, j + 1, t) = v;
            }
#undef IR77_E
}

// ---- Hermite Coulomb integrals R_{tuv}(alpha, X Y Z), t <= tx, u <= ty, v <= tz -- out[(t (ty+1) + u) (tz+1) + v] ---------------------
// r[] is scratch of IR77_R_SIZE. R^n_000 = (-2 alpha)^n F_n(alpha |XYZ|^2), then down in n and up in v, u, t.
IR77_FN void Ir77Coulomb(int tx, int ty, int tz, double alpha, double x, double y, double z, double* r, double* out) {
    int const nmax = tx + ty + tz;
    int const sx = tx + 1, sy = ty + 1, sz = tz + 1;

    double f[IR77_MAX_N + 1];
    Ir77Boys(nmax, alpha * (x * x + y * y + z * z), f);

#define IR77_R(n, t, u, v) r[(((n) * sx + (t)) * sy + (u)) * sz + (v)]
    double scale = 1.0;
    for (int n = 0; n <= nmax; n++) {
        IR77_R(n, 0, 0, 0) = scale * f[n];
        scale *= -2.0 * alpha;
    }

    for (int n = nmax - 1; n >= 0; n--) {
        int const room = nmax - n;  // t + u + v <= room at this n

        for (int t = 0; t <= tx && t <= room; t++)
            for (int u = 0; u <= ty && t + u <= room; u++)
                for (int v = 0; v <= tz && t + u + v <= room; v++) {
                    if (t + u + v == 0) continue;

                    double value;
                    if (t > 0) {
                        value = x * IR77_R(n + 1, t - 1, u, v);
                        if (t > 1) value += (t - 1) * IR77_R(n + 1, t - 2, u, v);
                    } else if (u > 0) {
                        value = y * IR77_R(n + 1, 0, u - 1, v);
                        if (u > 1) value += (u - 1) * IR77_R(n + 1, 0, u - 2, v);
                    } else {
                        value = z * IR77_R(n + 1, 0, 0, v - 1);
                        if (v > 1) value += (v - 1) * IR77_R(n + 1, 0, 0, v - 2);
                    }

                    IR77_R(n, t, u, v) = value;
                }
    }

    for (int t = 0; t <= tx; t++)
        for (int u = 0; u <= ty; u++)
            for (int v = 0; v <= tz; v++) out[(t * sy + u) * sz + v] = IR77_R(0, t, u, v);
#undef IR77_R
}

// ---- one contracted two-electron integral (ij|kl), chemists' order ------------------------------------------------------------------------
IR77_FN double Ir77EriFunctions(int i, int j, int k, int l, IR77_GLOBAL int const* functions, IR77_GLOBAL double const* primitives,
                                IR77_GLOBAL double const* atoms) {
    IR77_GLOBAL int const* fi = functions + i * IR77_FUNCTION;
    IR77_GLOBAL int const* fj = functions + j * IR77_FUNCTION;
    IR77_GLOBAL int const* fk = functions + k * IR77_FUNCTION;
    IR77_GLOBAL int const* fl = functions + l * IR77_FUNCTION;

    double const A[3] = {atoms[fi[5] * 4], atoms[fi[5] * 4 + 1], atoms[fi[5] * 4 + 2]};
    double const B[3] = {atoms[fj[5] * 4], atoms[fj[5] * 4 + 1], atoms[fj[5] * 4 + 2]};
    double const C[3] = {atoms[fk[5] * 4], atoms[fk[5] * 4 + 1], atoms[fk[5] * 4 + 2]};
    double const D[3] = {atoms[fl[5] * 4], atoms[fl[5] * 4 + 1], atoms[fl[5] * 4 + 2]};

    int const tx = fi[0] + fj[0] + fk[0] + fl[0];
    int const ty = fi[1] + fj[1] + fk[1] + fl[1];
    int const tz = fi[2] + fj[2] + fk[2] + fl[2];
    int const sy = ty + 1, sz = tz + 1;

    double e[IR77_E_I * IR77_E_I * IR77_E_T];
    double eab[3][IR77_E_T];
    double ecd[3][IR77_E_T];
    double scratch[IR77_R_SIZE];
    double rt[IR77_R_SIZE];

    double total = 0.0;

    for (int pa = 0; pa < fi[4]; pa++)
        for (int pb = 0; pb < fj[4]; pb++) {
            double const a = primitives[(fi[3] + pa) * 2], ca = primitives[(fi[3] + pa) * 2 + 1];
            double const b = primitives[(fj[3] + pb) * 2], cb = primitives[(fj[3] + pb) * 2 + 1];
            double const p = a + b;
            double const P[3] = {(a * A[0] + b * B[0]) / p, (a * A[1] + b * B[1]) / p, (a * A[2] + b * B[2]) / p};

            for (int axis = 0; axis < 3; axis++) {
                Ir77Hermite(fi[axis], fj[axis], a, b, A[axis] - B[axis], e);
                for (int t = 0; t < IR77_E_T; t++) eab[axis][t] = e[(fi[axis] * IR77_E_I + fj[axis]) * IR77_E_T + t];
            }

            for (int pc = 0; pc < fk[4]; pc++)
                for (int pd = 0; pd < fl[4]; pd++) {
                    double const c = primitives[(fk[3] + pc) * 2], cc = primitives[(fk[3] + pc) * 2 + 1];
                    double const d = primitives[(fl[3] + pd) * 2], cd = primitives[(fl[3] + pd) * 2 + 1];
                    double const q = c + d;
                    double const Q[3] = {(c * C[0] + d * D[0]) / q, (c * C[1] + d * D[1]) / q, (c * C[2] + d * D[2]) / q};

                    for (int axis = 0; axis < 3; axis++) {
                        Ir77Hermite(fk[axis], fl[axis], c, d, C[axis] - D[axis], e);
                        for (int t = 0; t < IR77_E_T; t++) ecd[axis][t] = e[(fk[axis] * IR77_E_I + fl[axis]) * IR77_E_T + t];
                    }

                    double const alpha = p * q / (p + q);
                    Ir77Coulomb(tx, ty, tz, alpha, P[0] - Q[0], P[1] - Q[1], P[2] - Q[2], scratch, rt);

                    double sum = 0.0;
                    for (int t = 0; t <= fi[0] + fj[0]; t++)
                        for (int u = 0; u <= fi[1] + fj[1]; u++)
                            for (int v = 0; v <= fi[2] + fj[2]; v++) {
                                double const left = eab[0][t] * eab[1][u] * eab[2][v];
                                if (left == 0.0) continue;

                                for (int tau = 0; tau <= fk[0] + fl[0]; tau++)
                                    for (int nu = 0; nu <= fk[1] + fl[1]; nu++)
                                        for (int phi = 0; phi <= fk[2] + fl[2]; phi++) {
                                            double const sign = ((tau + nu + phi) & 1) ? -1.0 : 1.0;
                                            sum += left * sign * ecd[0][tau] * ecd[1][nu] * ecd[2][phi] * rt[((t + tau) * sy + (u + nu)) * sz + (v + phi)];
                                        }
                            }

                    total += ca * cb * cc * cd * 2.0 * pow(IR77_PI, 2.5) / (p * q * sqrt(p + q)) * sum;
                }
        }

    return total;
}

// a unique quartet's index -> its functions, i >= j, k >= l, ij >= kl (ij = i (i+1)/2 + j)
IR77_FN void Ir77QuartetFunctions(long index, int* i, int* j, int* k, int* l) {
    long ij = (long)((sqrt(8.0 * (double)index + 1.0) - 1.0) * 0.5);
    while (ij * (ij + 1) / 2 > index) ij--;
    while ((ij + 1) * (ij + 2) / 2 <= index) ij++;
    long const kl = index - ij * (ij + 1) / 2;

    long a = (long)((sqrt(8.0 * (double)ij + 1.0) - 1.0) * 0.5);
    while (a * (a + 1) / 2 > ij) a--;
    while ((a + 1) * (a + 2) / 2 <= ij) a++;
    *i = (int)a;
    *j = (int)(ij - a * (a + 1) / 2);

    long c = (long)((sqrt(8.0 * (double)kl + 1.0) - 1.0) * 0.5);
    while (c * (c + 1) / 2 > kl) c--;
    while ((c + 1) * (c + 2) / 2 <= kl) c++;
    *k = (int)c;
    *l = (int)(kl - c * (c + 1) / 2);
}

#ifdef __OPENCL_VERSION__
// ---- the kernel -- one unique quartet a work item, quartets [first, first + count) of this launch ------------------------------------------
__kernel void Ir77Eri(long first, long count, __global int const* functions, __global double const* primitives, __global double const* atoms,
                      __global double* out) {
    long const g = get_global_id(0);
    if (g >= count) return;

    int i, j, k, l;
    Ir77QuartetFunctions(first + g, &i, &j, &k, &l);

    out[g] = Ir77EriFunctions(i, j, k, l, functions, primitives, atoms);
}
#endif

#endif
