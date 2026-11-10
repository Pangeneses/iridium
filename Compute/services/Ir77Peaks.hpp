#pragma once

#include <math.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

#include "../types/Ir77Types.hpp"

namespace NSIr77Compute {

// =============================================================================================================================================
// Ir77Peaks -- each orbital's largest |psi| and (one atom) its size, read straight from a layout (the same arithmetic Shaders/fields/orbitals.glsl does), so the
// reader can show an orbital relative to its own peak: one isosurface level then draws any orbital's lobes, a tight 1s or a Rydberg 4f a
// million times fainter.
//   sampled on rays out of every atom -- RADII radii (0.01 to REACH bohr, geometric) along DIRECTIONS directions (a Fibonacci sphere)
//   size: one atom only -- the radius holding 90% of the orbital's probability (of the density's electrons for the density), so the reader
//   can frame each orbital in its box: a core 1s a hundredth of a bohr across, a Rydberg 5f fifty
// =============================================================================================================================================
class Ir77Peaks {
   public:
    static constexpr std::uint32_t RADII{128};
    static constexpr std::uint32_t DIRECTIONS{146};
    static constexpr double REACH{80.0};
    static constexpr double HOLDS{0.9};  // the share of the probability a size holds

    struct Ir77Found {
        std::vector<double> peak{};    // per row, max |psi|
        std::vector<double> extent{};  // per row, bohr (one atom; 0 otherwise)
        double density_extent{0.0};    // the density's, bohr (one atom; 0 otherwise)
    };

    // values: a layout whose rows are (occupation, energy, N coefficients) -- rows = the orbital rows
    static Ir77Found Find(std::vector<float> const& values, std::size_t const rows) {
        Ir77Found found{};
        found.peak.assign(rows, 0.0);
        found.extent.assign(rows, 0.0);
        std::vector<double>& peak = found.peak;
        if (values.size() < ORBITALS_HEADER) return found;

        auto const whole = [&](std::size_t i) { return static_cast<std::size_t>(std::max(values[i], 0.0f) + 0.5f); };
        std::size_t const header = whole(1), atoms = whole(2), shells = whole(3), primitives = whole(4), functions = whole(5);
        std::size_t const atom_stride = whole(8), shell_stride = whole(9), primitive_stride = whole(10), orbital_stride = whole(11);
        bool const slater = whole(12) == ORBITALS_FUNCTION_SLATER;

        std::size_t const atom_at = header;
        std::size_t const shell_at = atom_at + atoms * atom_stride;
        std::size_t const primitive_at = shell_at + shells * shell_stride;
        std::size_t const orbital_at = primitive_at + primitives * primitive_stride;
        if (orbital_at + rows * orbital_stride > values.size()) return found;

        // one atom: per radius, the angular mean of psi^2 (each row) and of the density -- for the sizes
        bool const sized = atoms == 1;
        std::vector<double> shell_mean(sized ? rows * RADII : 0, 0.0);
        std::vector<double> density_mean(sized ? RADII : 0, 0.0);

        // the directions -- a Fibonacci sphere
        std::vector<std::array<double, 3>> directions(DIRECTIONS);
        for (std::uint32_t i = 0; i < DIRECTIONS; i++) {
            double const z = 1.0 - 2.0 * (i + 0.5) / DIRECTIONS;
            double const r = sqrt(std::max(1.0 - z * z, 0.0));
            double const phi = i * 2.39996322972865332;
            directions[i] = {r * cos(phi), r * sin(phi), z};
        }

        std::vector<double> phi(functions, 0.0);
        for (std::size_t a = 0; a < atoms; a++) {
            double const ax = values[atom_at + a * atom_stride], ay = values[atom_at + a * atom_stride + 1], az = values[atom_at + a * atom_stride + 2];

            for (std::uint32_t ri = 0; ri < RADII; ri++) {
                double const radius = Radius(ri);
                for (auto const& d : directions) {
                    double const px = ax + radius * d[0], py = ay + radius * d[1], pz = az + radius * d[2];

                    // every basis function's value here
                    std::size_t mu = 0;
                    for (std::size_t s = 0; s < shells; s++) {
                        std::size_t const at = shell_at + s * shell_stride;
                        std::size_t const atom = whole(at), l = whole(at + 1), first = whole(at + 2), count = whole(at + 3);
                        double const x = px - values[atom_at + atom * atom_stride], y = py - values[atom_at + atom * atom_stride + 1],
                                     z = pz - values[atom_at + atom * atom_stride + 2];
                        double const r2 = x * x + y * y + z * z;

                        double radial = 0.0;
                        for (std::size_t k = 0; k < count; k++) {
                            double const alpha = values[primitive_at + (first + k) * primitive_stride];
                            double const coefficient = values[primitive_at + (first + k) * primitive_stride + 1];
                            radial += coefficient * Norm(alpha, l, slater) * (slater ? exp(-alpha * sqrt(r2)) : exp(-alpha * r2));
                        }

                        for (int pa = static_cast<int>(l); pa >= 0; pa--)
                            for (int pb = static_cast<int>(l) - pa; pb >= 0; pb--) {
                                int const pc = static_cast<int>(l) - pa - pb;
                                if (mu < functions) phi[mu] = radial * pow(x, pa) * pow(y, pb) * pow(z, pc) / sqrt(DoubleFactorial(2 * pa - 1) * DoubleFactorial(2 * pb - 1) * DoubleFactorial(2 * pc - 1));
                                mu++;
                            }
                    }

                    for (std::size_t row = 0; row < rows; row++) {
                        std::size_t const c = orbital_at + row * orbital_stride + 2;
                        double psi = 0.0;
                        for (std::size_t i = 0; i < functions; i++) psi += values[c + i] * phi[i];
                        peak[row] = std::max(peak[row], fabs(psi));
                        if (sized) {
                            shell_mean[row * RADII + ri] += psi * psi / DIRECTIONS;
                            density_mean[ri] += values[c - 2] * psi * psi / DIRECTIONS;
                        }
                    }
                }
            }
        }

        // the sizes -- cumulative 4 pi r^2 <psi^2> over the geometric radii, the radius where it reaches HOLDS of the whole
        if (sized) {
            auto const size = [&](auto const& mean) {
                std::vector<double> cumulative(RADII, 0.0);
                for (std::uint32_t ri = 1; ri < RADII; ri++) {
                    double const r0 = Radius(ri - 1), r1 = Radius(ri);
                    cumulative[ri] = cumulative[ri - 1] + 0.5 * (r1 - r0) * (r0 * r0 * mean(ri - 1) + r1 * r1 * mean(ri));
                }
                double const whole = cumulative[RADII - 1];
                if (whole <= 0.0) return 0.0;
                for (std::uint32_t ri = 1; ri < RADII; ri++)
                    if (cumulative[ri] >= HOLDS * whole) {
                        double const t = (HOLDS * whole - cumulative[ri - 1]) / std::max(cumulative[ri] - cumulative[ri - 1], 1e-300);
                        return Radius(ri - 1) + t * (Radius(ri) - Radius(ri - 1));
                    }
                return REACH;
            };
            for (std::size_t row = 0; row < rows; row++) found.extent[row] = size([&](std::uint32_t ri) { return shell_mean[row * RADII + ri]; });
            found.density_extent = size([&](std::uint32_t ri) { return density_mean[ri]; });
        }
        return found;
    }

    static double Radius(std::uint32_t const ri) { return 0.01 * pow(REACH / 0.01, ri / (RADII - 1.0)); }

   private:
    static double DoubleFactorial(int const n) {
        double r = 1.0;
        for (int i = n; i > 1; i -= 2) r *= i;
        return r;
    }

    // the primitive's normalization as the reader applies it -- Gaussian (2a/pi)^(3/4) (4a)^(l/2); Slater sqrt((2l+1)!! (2a)^(2l+3) / (4 pi (2l+2)!))
    static double Norm(double const alpha, std::size_t const l, bool const slater) {
        if (!slater) return pow(2.0 * alpha / M_PI, 0.75) * pow(4.0 * alpha, 0.5 * static_cast<double>(l));
        double fact = 1.0;
        for (std::size_t i = 2; i <= 2 * l + 2; i++) fact *= static_cast<double>(i);
        return sqrt(DoubleFactorial(static_cast<int>(2 * l + 1)) * pow(2.0 * alpha, static_cast<double>(2 * l + 3)) / (4.0 * M_PI * fact));
    }
};

}  // namespace NSIr77Compute
