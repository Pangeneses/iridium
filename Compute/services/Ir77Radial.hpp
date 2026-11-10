#pragma once

#include <math.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "../types/Ir77Types.hpp"

namespace NSIr77Compute {

// =============================================================================================================================================
// Ir77Radial -- one atom's orbitals over radial functions R(r) = N r^l exp(-zeta r) Y_lm (Slater functions) as the orbitals file holds them
// (function kind 1). The basis: per l, per zeta, one shell (one primitive, coefficient 1) with its Cartesian components; each radial orbital
// becomes its 2l+1 real orbitals over those components (p: x y z; d: xy xz yz, x2-y2, z2; f: z3 xz2 yz2 z(x2-y2) xyz x(x2-3y2) y(3x2-y2)),
// the radial orbital's electrons spread evenly over them, every row named ("2p x"). The rows come out in energy order.
// =============================================================================================================================================
class Ir77Radial {
   public:
    static constexpr std::uint32_t MAX_L{3};

    // one l's radial orbitals -- what a program's buffers hold for it
    struct Ir77RadialBlock {
        std::uint32_t l{0};
        std::vector<double> zeta{};          // k
        std::size_t m{0};                    // orbitals
        std::vector<double> coefficients{};  // k x m -- function i, orbital c at i m + c
        std::vector<double> energies{};      // m
        std::vector<double> occupations{};   // the radial orbitals' electrons (this spin, or both when restricted) -- past its end: 0
        std::size_t keep{0};                 // the radial orbitals written (the first keep)
    };

    // The basis (one per atom -- both spins share it) and one spin's rows: energies, occupations, coefficients (orbital k's n at [k n, k n + n)),
    // names
    static void Build(std::array<Ir77RadialBlock, MAX_L + 1> const& blocks, Ir77Basis& basis, std::vector<double>& energies, std::vector<double>& occupations,
                      std::vector<double>& coefficients, std::vector<std::string>& names) {
        basis = Ir77Basis{};
        std::array<std::size_t, MAX_L + 1> first_of{};
        std::uint32_t function = 0;
        for (Ir77RadialBlock const& b : blocks) {
            first_of[b.l] = function;
            std::uint32_t const components = (b.l + 1) * (b.l + 2) / 2;
            for (double const zeta : b.zeta) {
                Ir77BasisShell shell{};
                shell.atom = 0;
                shell.l = b.l;
                shell.first_function = function;
                shell.exponents = {zeta};
                shell.contraction = {1.0};
                basis.shells.push_back(shell);

                for (std::uint32_t c = 0; c < components; c++) {
                    Ir77BasisFunction f{};
                    f.atom = 0;
                    f.shell = static_cast<std::uint32_t>(basis.shells.size() - 1);
                    f.exponents = {zeta};
                    f.coefficients = {1.0};
                    basis.functions.push_back(f);
                }
                function += components;
            }
        }
        std::size_t const n = basis.functions.size();

        std::array<std::vector<std::vector<double>>, MAX_L + 1> const angular{RealHarmonics(0), RealHarmonics(1), RealHarmonics(2), RealHarmonics(3)};
        std::array<std::vector<char const*>, MAX_L + 1> const shapes{
            std::vector<char const*>{""},
            std::vector<char const*>{"x", "y", "z"},
            std::vector<char const*>{"xy", "xz", "yz", "x2-y2", "z2"},
            std::vector<char const*>{"z3", "xz2", "yz2", "z(x2-y2)", "xyz", "x(x2-3y2)", "y(3x2-y2)"},
        };

        struct Row {
            double energy;
            double occupation;
            std::vector<double> c;
            std::string name;
        };
        std::vector<Row> rows{};
        for (Ir77RadialBlock const& b : blocks) {
            std::size_t const components = (b.l + 1) * (b.l + 2) / 2;
            std::size_t const k = b.zeta.size();
            for (std::size_t i = 0; i < std::min(b.keep, b.m); i++) {
                double const q = i < b.occupations.size() ? b.occupations[i] : 0.0;
                for (std::size_t s = 0; s < angular[b.l].size(); s++) {
                    std::vector<double> const& a = angular[b.l][s];
                    std::string name = std::to_string(b.l + 1 + i) + "spdf"[b.l];
                    if (b.l > 0) name += std::string{" "} + shapes[b.l][s];
                    Row row{b.energies[i], q / (2.0 * b.l + 1.0), std::vector<double>(n, 0.0), name};
                    for (std::size_t z = 0; z < k; z++)
                        for (std::size_t c = 0; c < components; c++) row.c[first_of[b.l] + z * components + c] = b.coefficients[z * b.m + i] * a[c];
                    rows.push_back(std::move(row));
                }
            }
        }
        std::stable_sort(rows.begin(), rows.end(), [](Row const& a, Row const& b) { return a.energy < b.energy; });

        std::size_t const m = rows.size();
        energies.resize(m);
        occupations.resize(m);
        names.resize(m);
        coefficients.assign(m * n, 0.0);
        for (std::size_t r = 0; r < m; r++) {
            energies[r] = rows[r].energy;
            occupations[r] = rows[r].occupation;
            names[r] = rows[r].name;
            for (std::size_t i = 0; i < n; i++) coefficients[r * n + i] = rows[r].c[i];
        }
    }

    // The real orbitals of one l as weights on its Cartesian components (a b c, a from l down, then b: s; x y z; xx xy xz yy yz zz; xxx xxy
    // xxz xyy xyz xzz yyy yyz yzz zzz), each component normalized (x^a y^b z^c / sqrt((2a-1)!! (2b-1)!! (2c-1)!!)), each orbital normalized:
    //   p: x y z.  d: xy xz yz, x2-y2, 2z2-x2-y2.  f: z(2z2-3x2-3y2), x(4z2-x2-y2), y(4z2-x2-y2), z(x2-y2), xyz, x(x2-3y2), y(3x2-y2)
    static std::vector<std::vector<double>> RealHarmonics(int const l) {
        std::vector<std::array<int, 3>> components{};
        for (int a = l; a >= 0; a--)
            for (int b = l - a; b >= 0; b--) components.push_back({a, b, l - a - b});

        auto const df = [](int n) {  // (n)!! with (-1)!! = 1
            double r = 1.0;
            for (int i = n; i > 1; i -= 2) r *= i;
            return r;
        };
        auto const angular = [&](int a, int b, int c) {  // the sphere's integral of x^a y^b z^c, over 4 pi / (a+b+c+1)!!
            return a % 2 || b % 2 || c % 2 ? 0.0 : df(a - 1) * df(b - 1) * df(c - 1);
        };

        using Poly = std::vector<std::pair<std::array<int, 3>, double>>;
        std::vector<Poly> polys{};
        if (l == 0) polys = {{{{0, 0, 0}, 1.0}}};
        if (l == 1) polys = {{{{1, 0, 0}, 1.0}}, {{{0, 1, 0}, 1.0}}, {{{0, 0, 1}, 1.0}}};
        if (l == 2)
            polys = {{{{1, 1, 0}, 1.0}}, {{{1, 0, 1}, 1.0}}, {{{0, 1, 1}, 1.0}}, {{{2, 0, 0}, 1.0}, {{0, 2, 0}, -1.0}},
                     {{{0, 0, 2}, 2.0}, {{2, 0, 0}, -1.0}, {{0, 2, 0}, -1.0}}};
        if (l == 3)
            polys = {{{{0, 0, 3}, 2.0}, {{2, 0, 1}, -3.0}, {{0, 2, 1}, -3.0}},
                     {{{1, 0, 2}, 4.0}, {{3, 0, 0}, -1.0}, {{1, 2, 0}, -1.0}},
                     {{{0, 1, 2}, 4.0}, {{2, 1, 0}, -1.0}, {{0, 3, 0}, -1.0}},
                     {{{2, 0, 1}, 1.0}, {{0, 2, 1}, -1.0}},
                     {{{1, 1, 1}, 1.0}},
                     {{{3, 0, 0}, 1.0}, {{1, 2, 0}, -3.0}},
                     {{{2, 1, 0}, 3.0}, {{0, 3, 0}, -1.0}}};

        std::vector<std::vector<double>> out{};
        for (Poly const& poly : polys) {
            // a monomial is sqrt(df) x its normalized component
            std::vector<double> w(components.size(), 0.0);
            for (auto const& [powers, coefficient] : poly)
                for (std::size_t i = 0; i < components.size(); i++)
                    if (components[i] == powers) w[i] += coefficient * sqrt(angular(2 * powers[0], 2 * powers[1], 2 * powers[2]));

            // its norm over the components' overlaps
            double norm = 0.0;
            for (std::size_t i = 0; i < components.size(); i++)
                for (std::size_t j = 0; j < components.size(); j++) {
                    auto const& p = components[i];
                    auto const& q = components[j];
                    double const s = angular(p[0] + q[0], p[1] + q[1], p[2] + q[2]) /
                                     sqrt(angular(2 * p[0], 2 * p[1], 2 * p[2]) * angular(2 * q[0], 2 * q[1], 2 * q[2]));
                    norm += w[i] * w[j] * s;
                }
            for (double& x : w) x /= sqrt(norm);
            out.push_back(std::move(w));
        }
        return out;
    }
};

}  // namespace NSIr77Compute
