#pragma once

#include <math.h>

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include "../types/Ir77Types.hpp"
#include "Ir77Basis.hpp"
#include "Ir77Integrals.hpp"
#include "Ir77Linear.hpp"
#include "Ir77Peaks.hpp"

namespace NSIr77Compute {

// =============================================================================================================================================
// Ir77Scf -- what a Hartree-Fock program's engine steps share: the superposition-of-atomic-densities guess (each element's own spherically
// averaged SCF -- the bare core guess lands some molecules, N2 among them, in the wrong state) and the orbitals file's layout. Pure
// computation -- no engine, no threads.
// =============================================================================================================================================
class Ir77Scf {
   public:
    // the superposition-of-atomic-densities guess -- for a staged program's "guess" step too (density: n x n, the total)
    static bool Guess(Ir77Molecule const& molecule, Ir77Basis const& basis, std::atomic<bool> const& cancel, std::vector<double>& density) {
        density.assign(basis.functions.size() * basis.functions.size(), 0.0);
        return AtomicDensities(molecule, basis, cancel, density);
    }

    // The result in the "orbitals" format (Ir77Types.hpp) -- a data file's "values"
    static std::vector<float> Layout(Ir77Molecule const& molecule, Ir77Basis const& basis, Ir77ScfResult const& result) {
        std::size_t const n = basis.functions.size();
        std::size_t const m = result.orbital_energies.size();

        std::uint32_t primitives = 0;
        for (Ir77BasisShell const& shell : basis.shells) primitives += static_cast<std::uint32_t>(shell.exponents.size());

        std::vector<float> out{};
        out.reserve(ORBITALS_HEADER + molecule.atoms.size() * ORBITALS_ATOM_STRIDE + basis.shells.size() * ORBITALS_SHELL_STRIDE +
                    primitives * ORBITALS_PRIMITIVE_STRIDE + m * (2 + n) * (result.unrestricted ? 2 : 1));

        out.insert(out.end(), {static_cast<float>(ORBITALS_LAYOUT_VERSION), static_cast<float>(ORBITALS_HEADER), static_cast<float>(molecule.atoms.size()),
                               static_cast<float>(basis.shells.size()), static_cast<float>(primitives), static_cast<float>(n), static_cast<float>(m),
                               static_cast<float>(result.energy), static_cast<float>(ORBITALS_ATOM_STRIDE), static_cast<float>(ORBITALS_SHELL_STRIDE),
                               static_cast<float>(ORBITALS_PRIMITIVE_STRIDE), static_cast<float>(2 + n), static_cast<float>(result.function_kind),
                               static_cast<float>(result.electrons), static_cast<float>(result.occupied), result.unrestricted ? 1.0f : 0.0f});

        for (Ir77Atom const& atom : molecule.atoms)
            out.insert(out.end(), {static_cast<float>(atom.x), static_cast<float>(atom.y), static_cast<float>(atom.z), static_cast<float>(atom.number)});

        std::uint32_t first = 0;
        for (Ir77BasisShell const& shell : basis.shells) {
            out.insert(out.end(), {static_cast<float>(shell.atom), static_cast<float>(shell.l), static_cast<float>(first), static_cast<float>(shell.exponents.size())});
            first += static_cast<std::uint32_t>(shell.exponents.size());
        }

        for (Ir77BasisShell const& shell : basis.shells)
            for (std::size_t k = 0; k < shell.exponents.size(); k++) out.insert(out.end(), {static_cast<float>(shell.exponents[k]), static_cast<float>(shell.contraction[k])});

        // restricted: occupation 2 for the pairs; unrestricted: the alpha rows, then the beta rows, 1 each
        auto const rows = [&](std::vector<double> const& energies, std::vector<double> const& coefficients, std::size_t const filled, float const occupation) {
            // the solver's own occupations when it gave them (a Slater atom's spread subshells)
            std::vector<double> const& own = &energies == &result.beta_orbital_energies ? result.beta_occupations : result.occupations;
            for (std::size_t k = 0; k < m; k++) {
                out.push_back(own.size() == m ? static_cast<float>(own[k]) : k < filled ? occupation : 0.0f);
                out.push_back(static_cast<float>(energies[k]));
                for (std::size_t i = 0; i < n; i++) out.push_back(static_cast<float>(coefficients[k * n + i]));
            }
        };

        std::size_t const orbital_at = out.size();
        if (!result.unrestricted) {
            rows(result.orbital_energies, result.coefficients, result.occupied, 2.0f);
        } else {
            rows(result.orbital_energies, result.coefficients, result.alpha, 1.0f);
            rows(result.beta_orbital_energies, result.beta_coefficients, result.beta, 1.0f);
        }

        // each row's peak |psi| and size as two more columns (stride 4 + N), the density's size as one more header value (17) -- the reader
        // shows an orbital relative to its peak and frames it in its box
        std::size_t const count = m * (result.unrestricted ? 2 : 1);
        Ir77Peaks::Ir77Found const found = Ir77Peaks::Find(out, count);

        std::vector<float> widened(out.begin(), out.begin() + static_cast<std::ptrdiff_t>(ORBITALS_HEADER));
        widened[1] = static_cast<float>(ORBITALS_HEADER + 1);
        widened[11] = static_cast<float>(4 + n);
        widened.push_back(static_cast<float>(found.density_extent));
        widened.insert(widened.end(), out.begin() + static_cast<std::ptrdiff_t>(ORBITALS_HEADER), out.begin() + static_cast<std::ptrdiff_t>(orbital_at));
        for (std::size_t row = 0; row < count; row++) {
            auto const from = out.begin() + static_cast<std::ptrdiff_t>(orbital_at + row * (2 + n));
            widened.insert(widened.end(), from, from + static_cast<std::ptrdiff_t>(2 + n));
            widened.push_back(static_cast<float>(found.peak[row]));
            widened.push_back(static_cast<float>(found.extent[row]));
        }

        return widened;
    }

   private:
    // The guess: per element, one spherically averaged atomic SCF (fractional occupation, spread evenly over degenerate orbitals) in that atom's
    // own functions; each atom's block of the molecule's density is its element's. false: cancelled.
    static bool AtomicDensities(Ir77Molecule const& molecule, Ir77Basis const& basis, std::atomic<bool> const& cancel, std::vector<double>& density) {
        std::size_t const n = basis.functions.size();
        std::vector<std::vector<double>> by_element(BASIS_ELEMENT_LIMIT);  // the element's atomic density, its own functions

        for (std::uint32_t a = 0; a < molecule.atoms.size(); a++) {
            std::uint32_t const z = molecule.atoms[a].number;

            // this atom's functions -- contiguous, in atom order
            std::vector<std::size_t> mine{};
            for (std::size_t i = 0; i < n; i++)
                if (basis.functions[i].atom == a) mine.push_back(i);
            if (mine.empty()) continue;

            if (z < BASIS_ELEMENT_LIMIT && by_element[z].empty()) {
                if (cancel) return false;
                by_element[z] = AtomDensity(molecule, basis, a);
            }

            std::vector<double> const& atomic = by_element[z];
            if (atomic.size() != mine.size() * mine.size()) continue;
            for (std::size_t i = 0; i < mine.size(); i++)
                for (std::size_t j = 0; j < mine.size(); j++) density[mine[i] * n + mine[j]] = atomic[i * mine.size() + j];
        }

        return true;
    }

    // one atom alone (at the origin -- its density doesn't care where), Z electrons, its own functions -> its density
    static std::vector<double> AtomDensity(Ir77Molecule const& molecule, Ir77Basis const& basis, std::uint32_t const atom) {
        Ir77Molecule alone{};
        alone.basis = molecule.basis;
        alone.atoms.push_back(Ir77Atom{molecule.atoms[atom].number, 0.0, 0.0, 0.0});

        Ir77Basis own{};
        std::string error{};
        if (!Ir77BasisBuilder::Build(alone, own, error)) return {};

        std::size_t const n = own.functions.size();
        double const electrons = static_cast<double>(molecule.atoms[atom].number);

        std::vector<double> s, t, v, eri;
        Ir77Integrals::OneElectron(alone, own, s, t, v);
        Ir77Integrals::TwoElectron(Ir77Integrals::Flatten(alone, own), n, eri);

        std::vector<double> h(n * n);
        for (std::size_t i = 0; i < n * n; i++) h[i] = t[i] + v[i];

        std::vector<double> sv, su;
        Ir77Linear::Eigen(s, n, sv, su);
        std::vector<double> x(n * n);
        for (std::size_t i = 0; i < n; i++)
            for (std::size_t c = 0; c < n; c++) x[i * n + c] = su[i * n + c] / sqrt(std::max(sv[c], 1e-12));
        std::vector<double> const xt = Ir77Linear::Transpose(x, n, n);

        std::vector<double> density(n * n, 0.0);
        for (int iteration = 0; iteration < 64; iteration++) {
            std::vector<double> fock = h;
            if (iteration > 0) AddTwoElectron(eri, density, n, fock);

            std::vector<double> energies, cp;
            Ir77Linear::Eigen(Ir77Linear::Multiply(Ir77Linear::Multiply(xt, fock, n, n, n), x, n, n, n), n, energies, cp);
            std::vector<double> const c = Ir77Linear::Multiply(x, cp, n, n, n);

            // occupations -- fill up, a degenerate group sharing what's left evenly (the spherical average of an open shell)
            std::vector<double> occupation(n, 0.0);
            double left = electrons;
            for (std::size_t k = 0; k < n && left > 1e-12;) {
                std::size_t end = k + 1;
                while (end < n && fabs(energies[end] - energies[k]) < 1e-4) end++;

                double const share = std::min(2.0 * (end - k), left) / static_cast<double>(end - k);
                for (std::size_t q = k; q < end; q++) occupation[q] = share;
                left -= share * (end - k);
                k = end;
            }

            std::vector<double> next(n * n, 0.0);
            for (std::size_t i = 0; i < n; i++)
                for (std::size_t j = 0; j < n; j++) {
                    double sum = 0.0;
                    for (std::size_t k = 0; k < n; k++) sum += occupation[k] * c[i * n + k] * c[j * n + k];
                    next[i * n + j] = sum;
                }

            // damped -- the fractional atom oscillates otherwise
            double change = 0.0;
            for (std::size_t i = 0; i < n * n; i++) {
                double const blended = iteration == 0 ? next[i] : 0.5 * density[i] + 0.5 * next[i];
                change = std::max(change, fabs(blended - density[i]));
                density[i] = blended;
            }
            if (iteration > 0 && change < 1e-6) break;
        }

        return density;
    }

    // F += J - K/2 from the unique integrals -- each applied once per distinct permutation of its indices (up to 8), so the full tensor is
    // summed exactly once
    static void AddTwoElectron(std::vector<double> const& eri, std::vector<double> const& density, std::size_t const n, std::vector<double>& fock) {
        std::vector<double> j(n * n, 0.0), k(n * n, 0.0);

        for (std::uint64_t q = 0; q < eri.size(); q++) {
            double const value = eri[q];
            if (fabs(value) < 1e-14) continue;

            int a, b, c, d;
            Ir77QuartetFunctions(static_cast<long>(q), &a, &b, &c, &d);

            int const perms[8][4]{{a, b, c, d}, {b, a, c, d}, {a, b, d, c}, {b, a, d, c}, {c, d, a, b}, {d, c, a, b}, {c, d, b, a}, {d, c, b, a}};
            for (int p = 0; p < 8; p++) {
                bool seen = false;
                for (int r = 0; r < p && !seen; r++)
                    seen = perms[r][0] == perms[p][0] && perms[r][1] == perms[p][1] && perms[r][2] == perms[p][2] && perms[r][3] == perms[p][3];
                if (seen) continue;

                int const w = perms[p][0], x = perms[p][1], y = perms[p][2], z = perms[p][3];
                j[w * n + x] += density[y * n + z] * value;  // (wx|yz) P_yz
                k[w * n + y] += density[x * n + z] * value;  // (wx|yz) P_xz -> K_wy
            }
        }

        for (std::size_t i = 0; i < n * n; i++) fock[i] += j[i] - 0.5 * k[i];
    }
};

}  // namespace NSIr77Compute
