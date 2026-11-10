#pragma once

#include <math.h>

#include <cstdint>
#include <vector>

#include "../types/Ir77Types.hpp"

namespace NSIr77Compute {

// the shared McMurchie-Davidson source -- the same text the OpenCL kernel is built from (its functions land in this namespace)
#include "../kernels/Ir77Integrals.cl"

// =============================================================================================================================================
// Ir77Integrals -- the integrals over a basis on the CPU: overlap, kinetic, nuclear attraction (always here -- N^2, cheap), and the two-electron
// integrals when there's no device (N^4 / 8 unique). The flat arrays are what the kernel reads too.
// =============================================================================================================================================
class Ir77Integrals {
   public:
    // the basis and atoms as the kernel reads them -- functions (IR77_FUNCTION ints each), primitives (alpha, c), atoms (x, y, z, Z)
    struct Ir77Flat {
        std::vector<std::int32_t> functions{};
        std::vector<double> primitives{};
        std::vector<double> atoms{};
    };

    static Ir77Flat Flatten(Ir77Molecule const& molecule, Ir77Basis const& basis) {
        Ir77Flat flat{};

        for (Ir77Atom const& atom : molecule.atoms) flat.atoms.insert(flat.atoms.end(), {atom.x, atom.y, atom.z, static_cast<double>(atom.number)});

        for (Ir77BasisFunction const& f : basis.functions) {
            std::int32_t const first = static_cast<std::int32_t>(flat.primitives.size() / 2);
            for (std::size_t k = 0; k < f.exponents.size(); k++) flat.primitives.insert(flat.primitives.end(), {f.exponents[k], f.coefficients[k]});

            flat.functions.insert(flat.functions.end(),
                                  {f.powers[0], f.powers[1], f.powers[2], first, static_cast<std::int32_t>(f.exponents.size()), static_cast<std::int32_t>(f.atom)});
        }

        return flat;
    }

    // S, T, V (n x n, row-major) -- V over every nucleus
    static void OneElectron(Ir77Molecule const& molecule, Ir77Basis const& basis, std::vector<double>& s, std::vector<double>& t, std::vector<double>& v) {
        std::size_t const n = basis.functions.size();
        s.assign(n * n, 0.0);
        t.assign(n * n, 0.0);
        v.assign(n * n, 0.0);

        double e[IR77_E_I * IR77_E_I * IR77_E_T];
        double scratch[IR77_R_SIZE];
        double r[IR77_R_SIZE];

        for (std::size_t i = 0; i < n; i++)
            for (std::size_t j = 0; j <= i; j++) {
                Ir77BasisFunction const& fi = basis.functions[i];
                Ir77BasisFunction const& fj = basis.functions[j];
                Ir77Atom const& ai = molecule.atoms[fi.atom];
                Ir77Atom const& aj = molecule.atoms[fj.atom];
                double const A[3]{ai.x, ai.y, ai.z};
                double const B[3]{aj.x, aj.y, aj.z};

                double sum_s = 0.0, sum_t = 0.0, sum_v = 0.0;

                for (std::size_t pa = 0; pa < fi.exponents.size(); pa++)
                    for (std::size_t pb = 0; pb < fj.exponents.size(); pb++) {
                        double const a = fi.exponents[pa];
                        double const b = fj.exponents[pb];
                        double const p = a + b;
                        double const c = fi.coefficients[pa] * fj.coefficients[pb];
                        double const P[3]{(a * A[0] + b * B[0]) / p, (a * A[1] + b * B[1]) / p, (a * A[2] + b * B[2]) / p};

                        // per axis: E^{ij}_t up to j + 2 (the kinetic integral), the 1D overlaps and kinetic parts
                        double ex[3][IR77_E_T]{};
                        double s1[3]{}, t1[3]{};
                        for (int axis = 0; axis < 3; axis++) {
                            int const ia = fi.powers[axis], jb = fj.powers[axis];
                            Ir77Hermite(ia, jb + 2, a, b, A[axis] - B[axis], e);

                            auto const E = [&](int i, int j, int tt) { return e[(i * IR77_E_I + j) * IR77_E_T + tt]; };
                            double const root = sqrt(IR77_PI / p);

                            s1[axis] = E(ia, jb, 0) * root;
                            double kin = -2.0 * b * b * E(ia, jb + 2, 0) * root + b * (2.0 * jb + 1.0) * E(ia, jb, 0) * root;
                            if (jb >= 2) kin -= 0.5 * jb * (jb - 1) * E(ia, jb - 2, 0) * root;
                            t1[axis] = kin;

                            for (int tt = 0; tt < IR77_E_T; tt++) ex[axis][tt] = E(ia, jb, tt);
                        }

                        sum_s += c * s1[0] * s1[1] * s1[2];
                        sum_t += c * (t1[0] * s1[1] * s1[2] + s1[0] * t1[1] * s1[2] + s1[0] * s1[1] * t1[2]);

                        int const tx = fi.powers[0] + fj.powers[0], ty = fi.powers[1] + fj.powers[1], tz = fi.powers[2] + fj.powers[2];
                        for (Ir77Atom const& nucleus : molecule.atoms) {
                            Ir77Coulomb(tx, ty, tz, p, P[0] - nucleus.x, P[1] - nucleus.y, P[2] - nucleus.z, scratch, r);

                            double sum = 0.0;
                            for (int a1 = 0; a1 <= tx; a1++)
                                for (int b1 = 0; b1 <= ty; b1++)
                                    for (int c1 = 0; c1 <= tz; c1++) sum += ex[0][a1] * ex[1][b1] * ex[2][c1] * r[(a1 * (ty + 1) + b1) * (tz + 1) + c1];

                            sum_v += -static_cast<double>(nucleus.number) * c * 2.0 * IR77_PI / p * sum;
                        }
                    }

                s[i * n + j] = s[j * n + i] = sum_s;
                t[i * n + j] = t[j * n + i] = sum_t;
                v[i * n + j] = v[j * n + i] = sum_v;
            }
    }

    // how many unique (ij|kl) there are -- i >= j, k >= l, ij >= kl
    static std::uint64_t QuartetCount(std::size_t n) {
        std::uint64_t const pairs = static_cast<std::uint64_t>(n) * (n + 1) / 2;
        return pairs * (pairs + 1) / 2;
    }

    // every unique (ij|kl) on the CPU, in quartet order (Ir77QuartetFunctions)
    static void TwoElectron(Ir77Flat const& flat, std::size_t n, std::vector<double>& eri) {
        std::uint64_t const count = QuartetCount(n);
        eri.assign(count, 0.0);

        for (std::uint64_t q = 0; q < count; q++) {
            int i, j, k, l;
            Ir77QuartetFunctions(static_cast<long>(q), &i, &j, &k, &l);
            eri[q] = Ir77EriFunctions(i, j, k, l, flat.functions.data(), flat.primitives.data(), flat.atoms.data());
        }
    }

    static double NuclearRepulsion(Ir77Molecule const& molecule) {
        double e = 0.0;
        for (std::size_t a = 0; a < molecule.atoms.size(); a++)
            for (std::size_t b = 0; b < a; b++) {
                Ir77Atom const& p = molecule.atoms[a];
                Ir77Atom const& q = molecule.atoms[b];
                double const dx = p.x - q.x, dy = p.y - q.y, dz = p.z - q.z;
                e += static_cast<double>(p.number) * static_cast<double>(q.number) / sqrt(dx * dx + dy * dy + dz * dz);
            }
        return e;
    }
};

}  // namespace NSIr77Compute
