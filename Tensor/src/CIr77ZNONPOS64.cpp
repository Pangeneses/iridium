#pragma once

#include <type_traits>

#include <compare>

#include <stdexcept>

module CIr77TENSOR:CIr77ZNONPOS;

import :CIr77FORWARD;
import :CIr77RESOURCE;

namespace CIr77TENSOR {
CIr77ZNONPOS<int64_t>::CIr77ZNONPOS() { Data = 0i64; }

CIr77ZNONPOS<int64_t>::CIr77ZNONPOS(CIr77ZNONPOS<int64_t> const& x) { Data = x(); }

CIr77ZNONPOS<int64_t>::CIr77ZNONPOS(int64_t const& x) { Data = x; }

void CIr77ZNONPOS<int64_t>::operator=(CIr77ZNONPOS<int64_t> const& x) { Data = x(); }

void CIr77ZNONPOS<int64_t>::operator=(int64_t const& x) { Data = x; }

int64_t CIr77ZNONPOS<int64_t>::operator()() const { return Data; }

unsigned char& CIr77ZNONPOS<int64_t>::operator[](size_t const& i) {
    if (i > 7i8) throw std::runtime_error{"Index Out of Range."};

    return reinterpret_cast<unsigned char*>(&Data)[7i8 - i];
}

CIr77ZNONPOS<int64_t> CIr77ZNONPOS<int64_t>::operator++() {
    if (Data == 0i64) throw std::overflow_error{"ZNonPos Increment Overflow."};

    Data += 1;

    return Data;
}

CIr77ZNONPOS<int64_t> CIr77ZNONPOS<int64_t>::operator++(int) {
    if (Data == 0i64) throw std::overflow_error{"ZNonPos Increment Overflow."};

    int64_t Q = Data;

    Data += 1;

    return Q;
}

CIr77ZNONPOS<int64_t> CIr77ZNONPOS<int64_t>::operator--() {
    if (Data == std::numeric_limits<int64_t>::min()) throw std::overflow_error{"ZNonPos Decrement Overflow."};

    Data -= 1;

    return Data;
}

CIr77ZNONPOS<int64_t> CIr77ZNONPOS<int64_t>::operator--(int) {
    if (Data == std::numeric_limits<int64_t>::min()) throw std::overflow_error{"ZNonPos Decrement Overflow."};

    int64_t Q = Data;

    Data -= 1;

    return Q;
}

CIr77ZNONPOS<int64_t> CIr77ZNONPOS<int64_t>::operator+() const { return int64_t{Data}; }

CIr77ZNONPOS<int64_t> CIr77ZNONPOS<int64_t>::operator-() const { return int64_t{-1 * Data}; }

CIr77ZNONPOS<int64_t> CIr77ZNONPOS<int64_t>::operator+(CIr77ZNONPOS<int64_t> const& x) const {
    int64_t A{Data}, B{x()};

    if (A <= (std::numeric_limits<int64_t>::min() - B)) throw std::overflow_error{"ZNonPos Addition Overflow."};

    A += B;

    return A;
}

CIr77ZNONPOS<int64_t> CIr77ZNONPOS<int64_t>::operator-(CIr77ZNONPOS<int64_t> const& x) const {
    int64_t A{Data}, B{x()};

    A -= B;

    if (A < std::numeric_limits<int64_t>::min() || A > 0i64) throw std::overflow_error{"ZNonPos Addition Overflow."};

    return A;
}

CIr77MANTISSA<double> CIr77ZNONPOS<int64_t>::operator/(CIr77MANTISSA<double> const& x) const {
    if (x.IsZero()) throw std::runtime_error{"Division by Zero."};

    if (Data == 0) return 0.0;

    CIr77MANTISSA<double> A{static_cast<double>(Data)}, B{x}, Q{0};

    uint64_t dividend{A.Mantissa()}, divisor{B.Mantissa()};

    dividend |= 0x0008000000000000ui64;

    divisor |= 0x0008000000000000ui64;

    for (size_t i = 0; i < 52; ++i) {
        try {
            Q.Mantissa(Q.Mantissa() << 1);
        }

        catch (std::overflow_error error) {
            throw error;
        }

        if (dividend >= divisor) {
            dividend -= divisor;

            try {
                Q.Mantissa(Q.Mantissa() | 0x0000000000000001ui64);
            }

            catch (std::overflow_error error) {
                throw error;
            }
        }

        if (dividend < divisor) dividend <<= 1;
    }

    try {
        Q.Mantissa(dividend & 0x00007FFFFFFFFFFFF);
    }

    catch (std::overflow_error error) {
        throw error;
    }

    try {
        Q.Exponent(A.Exponent() - B.Exponent() + 127i8);
    }

    catch (std::overflow_error error) {
        throw error;
    }

    Q.Sign() = (!A.Sign() != !B.Sign()) ? true : false;

    return Q;
}

CIr77ZNONPOS<int64_t> CIr77ZNONPOS<int64_t>::operator*(CIr77ZNONPOS<int64_t> const& x) const {
    int64_t A{Data}, B{x()}, Q{0};

    bool sign = !(A < 0) != !(B < 0) ? true : false;

    for (size_t i = 63; i > 0; --i) {
        if (B & 0x0000000000000001ui64) {
            Q += A;
        }

        B >>= 1;

        if (B != 0 && A >= 0x8000000000000000ui64) {
            throw std::overflow_error{"ZNonPos Multiplication Overflow."};
        }

        A <<= 1;
    }

    Q = sign ? (-1 * Q) : Q;

    if (Q > 0i64) throw std::overflow_error{"ZNonPos Multiplication Overflow."};

    return Q;
}

size_t CIr77ZNONPOS<int64_t>::operator%(CIr77ZNONPOS<int64_t> const& x) const {
    if (Data < x) return static_cast<size_t>(Data & 0x7FFFFFFFFFFFFFFF);

    if (x() == 0i8) throw std::runtime_error{"Division by Zero."};

    if (Data == 0i8) return 0i8;

    uint64_t A{static_cast<uint64_t>(Data & 0x7FFFFFFFFFFFFFFF)}, B{static_cast<uint64_t>(x() & 0x7FFFFFFFFFFFFFFF)};

    size_t bitWidth{63i8};

    while ((B & 0x3000000000000000ui64) != 0x3000000000000000ui64) {
        B <<= 1i8;

        --bitWidth;

        if (bitWidth == 0) throw std::runtime_error{"Division by Zero."};
    }

    B <<= 63i8 - bitWidth;

    for (size_t i = 63i8; i > bitWidth; --i) {
        if (A > B) {
            A -= B;
        }

        B >>= 1i8;
    }
    return A;
}

CIr77ZNONPOS<int64_t> CIr77ZNONPOS<int64_t>::operator~() const { return ~Data; }

CIr77ZNONPOS<int64_t> CIr77ZNONPOS<int64_t>::operator&(CIr77ZNONPOS<int64_t> const& x) const { return Data & x(); }

CIr77ZNONPOS<int64_t> CIr77ZNONPOS<int64_t>::operator|(CIr77ZNONPOS const x) const { return Data | x(); }

CIr77ZNONPOS<int64_t> CIr77ZNONPOS<int64_t>::operator^(CIr77ZNONPOS const x) const { return Data ^ x(); }

CIr77ZNONPOS<int64_t> CIr77ZNONPOS<int64_t>::operator>>(CIr77ZNONPOS<int64_t> const x) const { return Data >> x(); }

CIr77ZNONPOS<int64_t> CIr77ZNONPOS<int64_t>::operator<<(CIr77ZNONPOS const x) const { return Data << x(); }

bool CIr77ZNONPOS<int64_t>::operator!() const { return !Data; }

bool CIr77ZNONPOS<int64_t>::operator&&(CIr77ZNONPOS<int64_t> const& x) const { return Data && x(); }

bool CIr77ZNONPOS<int64_t>::operator||(CIr77ZNONPOS<int64_t> const& x) const { return Data || x(); }

bool CIr77ZNONPOS<int64_t>::operator==(CIr77ZNONPOS<int64_t> const& x) const { return Data == x(); }

bool CIr77ZNONPOS<int64_t>::operator!=(CIr77ZNONPOS<int64_t> const& x) const { return Data != x(); }

bool CIr77ZNONPOS<int64_t>::operator<(CIr77ZNONPOS<int64_t> const& x) const { return Data < x(); }

bool CIr77ZNONPOS<int64_t>::operator>(CIr77ZNONPOS<int64_t> const& x) const { return Data > x(); }

bool CIr77ZNONPOS<int64_t>::operator<=(CIr77ZNONPOS<int64_t> const& x) const { return Data <= x(); }

bool CIr77ZNONPOS<int64_t>::operator>=(CIr77ZNONPOS<int64_t> const& x) const { return Data >= x(); }

std::weak_ordering CIr77ZNONPOS<int64_t>::operator<=>(CIr77ZNONPOS<int64_t> const& x) const { return Data <=> x(); }

void CIr77ZNONPOS<int64_t>::operator+=(CIr77ZNONPOS<int64_t> const& x) {
    CIr77ZNONPOS A{*this}, B{x};

    try {
        A = A.operator+(B);
    } catch (std::overflow_error error) {
        throw error;
    }

    *this = A;
}

void CIr77ZNONPOS<int64_t>::operator-=(CIr77ZNONPOS<int64_t> const& x) {
    CIr77ZNONPOS A{*this}, B{x};

    try {
        A = A.operator-(B);
    } catch (std::overflow_error error) {
        throw error;
    }

    *this = A;
}

void CIr77ZNONPOS<int64_t>::operator*=(CIr77ZNONPOS<int64_t> const& x) {
    CIr77ZNONPOS A{*this}, B{x};

    try {
        A = A.operator*(B);
    } catch (std::overflow_error error) {
        throw error;
    }

    *this = A;
}

void CIr77ZNONPOS<int64_t>::operator&=(CIr77ZNONPOS<int64_t> const& x) {
    CIr77ZNONPOS A{*this}, B{x};

    try {
        A = A.operator&(B);
    } catch (std::overflow_error error) {
        throw error;
    }

    *this = A;
}

void CIr77ZNONPOS<int64_t>::operator|=(CIr77ZNONPOS<int64_t> const& x) {
    CIr77ZNONPOS A{*this}, B{x};

    try {
        A = A.operator|(B);
    } catch (std::overflow_error error) {
        throw error;
    }

    *this = A;
}

void CIr77ZNONPOS<int64_t>::operator^=(CIr77ZNONPOS<int64_t> const& x) {
    CIr77ZNONPOS A{*this}, B{x};

    try {
        A = A.operator^(B);
    } catch (std::overflow_error error) {
        throw error;
    }

    *this = A;
}

void CIr77ZNONPOS<int64_t>::operator>>=(CIr77ZNONPOS<int64_t> const& x) {
    CIr77ZNONPOS A{*this}, B{x};

    try {
        A = A.operator>>(B);
    } catch (std::overflow_error error) {
        throw error;
    }

    *this = A;
}

void CIr77ZNONPOS<int64_t>::operator<<=(CIr77ZNONPOS<int64_t> const& x) {
    CIr77ZNONPOS A{*this}, B{x};

    try {
        A = A.operator<<(B);
    } catch (std::overflow_error error) {
        throw error;
    }

    *this = A;
}
}  // namespace CIr77TENSOR