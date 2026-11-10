#pragma once

#include <math.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <string_view>

#include "../types/Ir77Types.hpp"
#include "Ir77BasisSets.hpp"

namespace NSIr77Compute {

// =============================================================================================================================================
// Ir77BasisBuilder -- a molecule's basis from the embedded sets: each element's shells, renormalized, split into Cartesian components (s 1,
// p x y z, d xx xy xz yy yz zz -- the order Shaders/fields/orbitals.glsl reads).
// =============================================================================================================================================
class Ir77BasisBuilder {
   public:
    // H to Kr -- the Gaussian sets stop at Ar (Build says so); Slater atoms go to Kr
    static constexpr char const* ELEMENTS[]{"",   "H",  "He", "Li", "Be", "B",  "C",  "N",  "O",  "F",  "Ne", "Na", "Mg",
                                           "Al", "Si", "P",  "S",  "Cl", "Ar", "K",  "Ca", "Sc", "Ti", "V",  "Cr", "Mn",
                                           "Fe", "Co", "Ni", "Cu", "Zn", "Ga", "Ge", "As", "Se", "Br", "Kr"};

    static constexpr std::uint32_t MAX_ELEMENT{36};

    // "C", "cl", "6" -> the atomic number; 0: not one Compute has
    static std::uint32_t Element(std::string_view const symbol) {
        if (!symbol.empty() && std::isdigit(static_cast<unsigned char>(symbol[0]))) {
            std::uint32_t const number = static_cast<std::uint32_t>(std::stoul(std::string{symbol}));
            return number <= MAX_ELEMENT ? number : 0u;
        }

        for (std::uint32_t z = 1; z <= MAX_ELEMENT; z++) {
            std::string_view const name{ELEMENTS[z]};
            if (name.size() == symbol.size() &&
                std::equal(name.begin(), name.end(), symbol.begin(), [](char a, char b) { return std::tolower(a) == std::tolower(b); }))
                return z;
        }

        return 0;
    }

    // the set's index by name ("sto-3g", "6-31g", "6-31g*", any case); false: no such set
    static bool Set(std::string_view const name, std::uint32_t& set) {
        for (std::uint32_t i = 0; i < BASIS_SET_COUNT; i++) {
            std::string_view const known{BASIS_SET_NAMES[i]};
            if (known.size() == name.size() &&
                std::equal(known.begin(), known.end(), name.begin(), [](char a, char b) { return std::tolower(a) == std::tolower(b); })) {
                set = i;
                return true;
            }
        }
        return false;
    }

    // (2n - 1)!!, (-1)!! = 1
    static double DoubleFactorial(int const n) {
        double r = 1.0;
        for (int k = 2 * n - 1; k > 1; k -= 2) r *= k;
        return r;
    }

    // the normalized Cartesian primitive's factor for x^a y^b z^c
    static double PrimitiveNorm(double const alpha, int const a, int const b, int const c) {
        int const l = a + b + c;
        return pow(2.0 * alpha / IR77_PI_D, 0.75) * pow(4.0 * alpha, 0.5 * l) / sqrt(DoubleFactorial(a) * DoubleFactorial(b) * DoubleFactorial(c));
    }

    // error: what's wrong (an element or a set Compute doesn't have)
    static bool Build(Ir77Molecule const& molecule, Ir77Basis& basis, std::string& error) {
        basis = Ir77Basis{};

        std::uint32_t set{0};
        if (!Set(molecule.basis, set)) {
            error = "no basis set \"" + molecule.basis + "\" -- sto-3g, 6-31g, 6-31g*";
            return false;
        }

        for (std::uint32_t atom = 0; atom < molecule.atoms.size(); atom++) {
            std::uint32_t const z = molecule.atoms[atom].number;

            bool found = false;
            for (Ir77BasisShellRow const& row : BASIS_SHELLS) {
                if (row.set != set || row.z != z) continue;
                found = true;

                Ir77BasisShell shell{};
                shell.atom = atom;
                shell.l = row.l;
                shell.first_function = static_cast<std::uint32_t>(basis.functions.size());
                for (std::uint32_t k = 0; k < row.count; k++) {
                    shell.exponents.push_back(BASIS_PRIMITIVES[row.first + k][0]);
                    shell.contraction.push_back(BASIS_PRIMITIVES[row.first + k][1]);
                }

                // renormalize -- normalized primitives of one component overlap as (2 sqrt(a b) / (a + b))^(l + 3/2)
                double self = 0.0;
                for (std::size_t i = 0; i < shell.exponents.size(); i++)
                    for (std::size_t j = 0; j < shell.exponents.size(); j++) {
                        double const a = shell.exponents[i], b = shell.exponents[j];
                        self += shell.contraction[i] * shell.contraction[j] * pow(2.0 * sqrt(a * b) / (a + b), shell.l + 1.5);
                    }
                for (double& d : shell.contraction) d /= sqrt(self);

                for (std::array<std::int32_t, 3> const& powers : Components(shell.l)) {
                    Ir77BasisFunction f{};
                    f.powers = powers;
                    f.atom = atom;
                    f.shell = static_cast<std::uint32_t>(basis.shells.size());
                    f.exponents = shell.exponents;
                    for (std::size_t k = 0; k < shell.exponents.size(); k++)
                        f.coefficients.push_back(shell.contraction[k] * PrimitiveNorm(shell.exponents[k], powers[0], powers[1], powers[2]));
                    basis.functions.push_back(std::move(f));
                }

                basis.shells.push_back(std::move(shell));
            }

            if (!found) {
                error = std::string{"no "} + (z <= MAX_ELEMENT ? ELEMENTS[z] : "element") + " (Z " + std::to_string(z) + ") in " + molecule.basis +
                        " -- the Gaussian sets go to Ar; one atom to Kr: the slater-atom program";
                return false;
            }
        }

        return true;
    }

    // a shell's Cartesian components, in the layout's order
    static std::vector<std::array<std::int32_t, 3>> Components(std::uint32_t const l) {
        if (l == 0) return {{0, 0, 0}};
        if (l == 1) return {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
        return {{2, 0, 0}, {1, 1, 0}, {1, 0, 1}, {0, 2, 0}, {0, 1, 1}, {0, 0, 2}};
    }

   private:
    static constexpr double IR77_PI_D{3.14159265358979323846};
};

}  // namespace NSIr77Compute
