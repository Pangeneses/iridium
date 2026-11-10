#pragma once

#include <math.h>

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <vector>

namespace NSIr77Compute {

// =============================================================================================================================================
// Ir77Linear -- the small dense linear algebra the SCF needs: symmetric eigenproblems (cyclic Jacobi -- exact to machine precision, fine to a
// few hundred functions), products, a small solve for DIIS. Row-major n x n unless said.
// =============================================================================================================================================
class Ir77Linear {
   public:
    // a symmetric a (n x n) -> values ascending, vectors as columns (row-major n x n, column k = vector k)
    static void Eigen(std::vector<double> a, std::size_t const n, std::vector<double>& values, std::vector<double>& vectors) {
        std::vector<double> v(n * n, 0.0);
        for (std::size_t i = 0; i < n; i++) v[i * n + i] = 1.0;

        for (int sweep = 0; sweep < 100; sweep++) {
            double off = 0.0;
            for (std::size_t i = 0; i < n; i++)
                for (std::size_t j = i + 1; j < n; j++) off += a[i * n + j] * a[i * n + j];
            if (off < 1e-30) break;

            for (std::size_t p = 0; p < n; p++)
                for (std::size_t q = p + 1; q < n; q++) {
                    double const apq = a[p * n + q];
                    if (fabs(apq) < 1e-300) continue;

                    double const theta = (a[q * n + q] - a[p * n + p]) / (2.0 * apq);
                    double const t = (theta >= 0.0 ? 1.0 : -1.0) / (fabs(theta) + sqrt(theta * theta + 1.0));
                    double const c = 1.0 / sqrt(t * t + 1.0);
                    double const s = t * c;

                    for (std::size_t k = 0; k < n; k++) {
                        double const akp = a[k * n + p], akq = a[k * n + q];
                        a[k * n + p] = c * akp - s * akq;
                        a[k * n + q] = s * akp + c * akq;
                    }
                    for (std::size_t k = 0; k < n; k++) {
                        double const apk = a[p * n + k], aqk = a[q * n + k];
                        a[p * n + k] = c * apk - s * aqk;
                        a[q * n + k] = s * apk + c * aqk;
                    }
                    for (std::size_t k = 0; k < n; k++) {
                        double const vkp = v[k * n + p], vkq = v[k * n + q];
                        v[k * n + p] = c * vkp - s * vkq;
                        v[k * n + q] = s * vkp + c * vkq;
                    }
                }
        }

        std::vector<std::size_t> order(n);
        std::iota(order.begin(), order.end(), 0);
        std::sort(order.begin(), order.end(), [&](std::size_t x, std::size_t y) { return a[x * n + x] < a[y * n + y]; });

        values.resize(n);
        vectors.assign(n * n, 0.0);
        for (std::size_t k = 0; k < n; k++) {
            values[k] = a[order[k] * n + order[k]];
            for (std::size_t i = 0; i < n; i++) vectors[i * n + k] = v[i * n + order[k]];
        }
    }

    // c (r x m) = a (r x k) b (k x m)
    static std::vector<double> Multiply(std::vector<double> const& a, std::vector<double> const& b, std::size_t r, std::size_t k, std::size_t m) {
        std::vector<double> c(r * m, 0.0);
        for (std::size_t i = 0; i < r; i++)
            for (std::size_t x = 0; x < k; x++) {
                double const aix = a[i * k + x];
                if (aix == 0.0) continue;
                for (std::size_t j = 0; j < m; j++) c[i * m + j] += aix * b[x * m + j];
            }
        return c;
    }

    // a^T (r x c -> c x r)
    static std::vector<double> Transpose(std::vector<double> const& a, std::size_t r, std::size_t c) {
        std::vector<double> t(r * c);
        for (std::size_t i = 0; i < r; i++)
            for (std::size_t j = 0; j < c; j++) t[j * r + i] = a[i * c + j];
        return t;
    }

    // a x = b, a (n x n) -- Gaussian elimination, partial pivoting; false: singular
    static bool Solve(std::vector<double> a, std::vector<double> b, std::size_t const n, std::vector<double>& x) {
        for (std::size_t col = 0; col < n; col++) {
            std::size_t pivot = col;
            for (std::size_t r = col + 1; r < n; r++)
                if (fabs(a[r * n + col]) > fabs(a[pivot * n + col])) pivot = r;
            if (fabs(a[pivot * n + col]) < 1e-14) return false;

            if (pivot != col) {
                for (std::size_t k = 0; k < n; k++) std::swap(a[col * n + k], a[pivot * n + k]);
                std::swap(b[col], b[pivot]);
            }

            for (std::size_t r = col + 1; r < n; r++) {
                double const f = a[r * n + col] / a[col * n + col];
                for (std::size_t k = col; k < n; k++) a[r * n + k] -= f * a[col * n + k];
                b[r] -= f * b[col];
            }
        }

        x.assign(n, 0.0);
        for (std::size_t i = n; i-- > 0;) {
            double s = b[i];
            for (std::size_t k = i + 1; k < n; k++) s -= a[i * n + k] * x[k];
            x[i] = s / a[i * n + i];
        }
        return true;
    }
};

}  // namespace NSIr77Compute
