#!/usr/bin/env python3
# Test data for Compute/programs/orbitals.glsl's layout (version 1) -- writes field.data args ({"values": [...]}) and checks them by integrating on a grid.
#   h2.sto3g.json   H2 at 1.4 bohr, STO-3G, closed-shell Hartree-Fock solved here (E = -1.11671 hartree, the textbook value)
#   p.sto3g.json    one carbon 2p shell, each orbital a single p function -- an evaluator test, not a Hartree-Fock result
# Usage: orbitals.data.py [out dir]
import json, sys
from math import erf, pi, sqrt
import numpy as np

STO3G_H = ([3.42525091, 0.62391373, 0.16885540], [0.15432897, 0.53532814, 0.44463454])
STO3G_C_2P = ([2.9412494, 0.6834831, 0.2222899], [0.15591627, 0.60768372, 0.39195739])


def layout(atoms, shells, primitives, orbitals, energy):
    """atoms [(x, y, z, Z)], shells [(atom, l, first, count)], primitives [(alpha, d)], orbitals [(occ, eps, [c...])]"""
    functions = sum((l + 1) * (l + 2) // 2 for _, l, _, _ in shells)
    values = [1, len(atoms), len(shells), len(primitives), functions, len(orbitals), energy, 0]
    for a in atoms: values += list(a)
    for s in shells: values += list(s)
    for p in primitives: values += list(p)
    for occ, eps, c in orbitals:
        assert len(c) == functions
        values += [occ, eps] + list(c)
    return values


def h2():
    alpha, d = map(np.array, STO3G_H)
    R = 1.4
    centres = [np.array([0.0, -R / 2, 0.0]), np.array([0.0, R / 2, 0.0])]  # along world Y (up)
    N = lambda a: (2 * a / pi) ** 0.75
    F0 = lambda t: 1.0 if t < 1e-10 else 0.5 * sqrt(pi / t) * erf(sqrt(t))
    r2 = lambda A, B: float(np.dot(A - B, A - B))

    def pair(f):
        m = np.zeros((2, 2))
        for i in range(2):
            for j in range(2):
                m[i, j] = sum(da * db * N(a) * N(b) * f(a, centres[i], b, centres[j]) for a, da in zip(alpha, d) for b, db in zip(alpha, d))
        return m

    S = pair(lambda a, A, b, B: (pi / (a + b)) ** 1.5 * np.exp(-a * b / (a + b) * r2(A, B)))
    T = pair(lambda a, A, b, B: a * b / (a + b) * (3 - 2 * a * b / (a + b) * r2(A, B)) * (pi / (a + b)) ** 1.5 * np.exp(-a * b / (a + b) * r2(A, B)))
    V = pair(lambda a, A, b, B: sum(-2 * pi / (a + b) * np.exp(-a * b / (a + b) * r2(A, B)) * F0((a + b) * r2((a * A + b * B) / (a + b), C)) for C in centres))
    G = np.zeros((2, 2, 2, 2))
    for i in range(2):
        for j in range(2):
            for k in range(2):
                for l in range(2):
                    s = 0.0
                    for a, da in zip(alpha, d):
                        for b, db in zip(alpha, d):
                            for c, dc in zip(alpha, d):
                                for e, de in zip(alpha, d):
                                    p, q = a + b, c + e
                                    P, Q = (a * centres[i] + b * centres[j]) / p, (c * centres[k] + e * centres[l]) / q
                                    s += (da * db * dc * de * N(a) * N(b) * N(c) * N(e) * 2 * pi ** 2.5 / (p * q * sqrt(p + q)) *
                                          np.exp(-a * b / p * r2(centres[i], centres[j]) - c * e / q * r2(centres[k], centres[l])) * F0(p * q / (p + q) * r2(P, Q)))
                    G[i, j, k, l] = s
    H = T + V
    X = np.linalg.inv(np.linalg.cholesky(S)).T
    P = np.zeros((2, 2))
    for _ in range(100):
        F = H + np.einsum("kl,ijkl->ij", P, G) - 0.5 * np.einsum("kl,ikjl->ij", P, G)
        eps, Cp = np.linalg.eigh(X.T @ F @ X)
        C = X @ Cp
        Pn = 2 * np.outer(C[:, 0], C[:, 0])
        if np.abs(Pn - P).max() < 1e-12:
            break
        P = Pn
    E = 0.5 * np.sum(P * (H + F)) + 1 / R

    atoms = [(*c, 1.0) for c in centres]
    shells = [(0, 0, 0, 3), (1, 0, 3, 3)]
    primitives = [(a, dd) for a, dd in zip(alpha, d)] * 2
    orbitals = [(2.0, eps[0], list(C[:, 0])), (0.0, eps[1], list(C[:, 1]))]
    return layout(atoms, shells, primitives, orbitals, E), E, eps


def p_shell():
    alpha, d = STO3G_C_2P
    atoms = [(0.0, 0.0, 0.0, 6.0)]
    shells = [(0, 1, 0, 3)]
    primitives = list(zip(alpha, d))
    orbitals = [(2.0, 0.0, [1, 0, 0]), (2.0, 0.0, [0, 1, 0]), (2.0, 0.0, [0, 0, 1])]  # p along X, Y (up), Z
    return layout(atoms, shells, primitives, orbitals, 0.0)


# ---- the check: orbitals.glsl's evaluator, line for line, integrated on a grid ---------------------------------------------------------
def evaluate(v, orbital, p):
    A, S_, P_, Nf, M = (int(x) for x in v[1:6])
    atom_at, shell_at = 8, 8 + A * 4
    prim_at = shell_at + S_ * 4
    orb_at = prim_at + P_ * 2
    coef = orb_at + orbital * (2 + Nf) + 2
    mu, psi = 0, np.zeros(p.shape[:-1])
    for s in range(S_):
        atom, l, first, count = (int(x) for x in v[shell_at + s * 4: shell_at + s * 4 + 4])
        x = p - np.array(v[atom_at + atom * 4: atom_at + atom * 4 + 3])
        r2 = np.sum(x * x, -1)
        radial = sum(v[prim_at + (first + k) * 2 + 1] * (2 * v[prim_at + (first + k) * 2] / pi) ** 0.75 * (4 * v[prim_at + (first + k) * 2]) ** (l / 2) *
                     np.exp(-v[prim_at + (first + k) * 2] * r2) for k in range(count))
        if l == 0:
            psi += v[coef + mu] * radial
        else:
            psi += radial * sum(v[coef + mu + i] * x[..., i] for i in range(3))
        mu += 1 if l == 0 else 3
    return psi


def check(v, name):
    g = np.linspace(-6, 6, 121)
    h = g[1] - g[0]
    p = np.stack(np.meshgrid(g, g, g, indexing="ij"), -1)
    M = int(v[5])
    norms = [float(np.sum(evaluate(v, i, p) ** 2) * h ** 3) for i in range(M)]
    print(f"{name}: orbital norms {['%.4f' % n for n in norms]}")


if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else "."
    values, E, eps = h2()
    print(f"H2 STO-3G: E = {E:.6f} hartree, orbital energies {eps[0]:.6f} {eps[1]:.6f}")
    check(values, "H2")
    json.dump({"values": [float(x) for x in values]}, open(f"{out}/h2.sto3g.json", "w"))
    values = p_shell()
    check(values, "C 2p")
    json.dump({"values": [float(x) for x in values]}, open(f"{out}/p.sto3g.json", "w"))
