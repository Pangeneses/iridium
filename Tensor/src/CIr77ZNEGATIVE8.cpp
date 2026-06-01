#pragma once

#include <type_traits>

#include <compare>

#include <stdexcept>

module CIr77TENSOR:CIr77ZNEGATIVE;

import :CIr77FORWARD;
import :CIr77RESOURCE;

import :CIr77MANTISSA;

namespace CIr77TENSOR {
CIr77ZNEGATIVE<int8_t>::CIr77ZNEGATIVE() { Data = -1i8; }

CIr77ZNEGATIVE<int8_t>::CIr77ZNEGATIVE(CIr77ZNEGATIVE<int8_t> const& x) { Data = x(); }

CIr77ZNEGATIVE<int8_t>::CIr77ZNEGATIVE(int8_t const& x) { Data = x; }

void CIr77ZNEGATIVE<int8_t>::operator=(CIr77ZNEGATIVE<int8_t> const& x) { Data = x(); }

void CIr77ZNEGATIVE<int8_t>::operator=(int8_t const& x) { Data = x; }

int8_t CIr77ZNEGATIVE<int8_t>::operator()() const { return Data; }

unsigned char& CIr77ZNEGATIVE<int8_t>::operator[](size_t const& i) {
    if (i > 0) throw std::overflow_error{"Index out of range."};

    return reinterpret_cast<unsigned char*>(&Data)[0];
}

CIr77ZNEGATIVE<int8_t> CIr77ZNEGATIVE<int8_t>::operator++() {
    if (Data == -1i8) throw std::overflow_error{"ZNegative Increment Overflow."};

    Data += 1i8;

    return Data;
}

CIr77ZNEGATIVE<int8_t> CIr77ZNEGATIVE<int8_t>::operator++(int) {
    if (Data == -1i8) throw std::overflow_error{"ZNegative Increment Overflow."};

    int8_t Q = Data;

    Data += 1i8;

    return Q;
}

CIr77ZNEGATIVE<int8_t> CIr77ZNEGATIVE<int8_t>::operator--() {
    if (Data == std::numeric_limits<int8_t>::min()) throw std::overflow_error{"ZNegative Decrement Overflow."};

    Data -= 1i8;

    return Data;
}

CIr77ZNEGATIVE<int8_t> CIr77ZNEGATIVE<int8_t>::operator--(int) {
    if (Data == std::numeric_limits<int8_t>::min()) throw std::overflow_error{"ZNegative Decrement Overflow."};

    int8_t Q = Data;

    Data -= 1i8;

    return Q;
}

CIr77ZNEGATIVE<int8_t> CIr77ZNEGATIVE<int8_t>::operator+() const { return int8_t{Data}; }

CIr77ZNEGATIVE<int8_t> CIr77ZNEGATIVE<int8_t>::operator-() const { return int8_t{-1 * Data}; }

CIr77ZNEGATIVE<int8_t> CIr77ZNEGATIVE<int8_t>::operator+(CIr77ZNEGATIVE<int8_t> const& x) const {
    int16_t A{Data}, B{x()};

    A += B;

    if (A < std::numeric_limits<int8_t>::min() || A > -1i8) throw std::overflow_error{"ZNegative Addition Overflow."};

    return static_cast<int8_t>(A);
}

CIr77ZNEGATIVE<int8_t> CIr77ZNEGATIVE<int8_t>::operator-(CIr77ZNEGATIVE<int8_t> const& x) const {
    int16_t A{Data}, B{x()};

    A -= B;

    if (A < std::numeric_limits<int8_t>::min() || A > -1i8) throw std::overflow_error{"ZNegative Subtraction Overflow."};

    return static_cast<int8_t>(A);
}

CIr77MANTISSA<double> CIr77ZNEGATIVE<int8_t>::operator/(CIr77MANTISSA<double> const& x) const {
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

CIr77ZNEGATIVE<int8_t> CIr77ZNEGATIVE<int8_t>::operator*(CIr77ZNEGATIVE<int8_t> const& x) const {
    int16_t A{Data}, B{x()};

    A *= B;

    if (A < std::numeric_limits<int8_t>::min() || A > -1i8) throw std::overflow_error{"ZNegative Multiplication Overflow."};

    return static_cast<int8_t>(A);
}

size_t CIr77ZNEGATIVE<int8_t>::operator%(CIr77ZNEGATIVE<int8_t> const& x) const {
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

CIr77ZNEGATIVE<int8_t> CIr77ZNEGATIVE<int8_t>::operator~() const { return ~Data; }

CIr77ZNEGATIVE<int8_t> CIr77ZNEGATIVE<int8_t>::operator&(CIr77ZNEGATIVE<int8_t> const& x) const { return Data & x(); }

CIr77ZNEGATIVE<int8_t> CIr77ZNEGATIVE<int8_t>::operator|(CIr77ZNEGATIVE<int8_t> const x) const { return Data | x(); }

CIr77ZNEGATIVE<int8_t> CIr77ZNEGATIVE<int8_t>::operator^(CIr77ZNEGATIVE<int8_t> const x) const { return Data ^ x(); }

CIr77ZNEGATIVE<int8_t> CIr77ZNEGATIVE<int8_t>::operator>>(CIr77ZNEGATIVE<int8_t> const x) const { return Data >> x(); }

CIr77ZNEGATIVE<int8_t> CIr77ZNEGATIVE<int8_t>::operator<<(CIr77ZNEGATIVE<int8_t> const x) const { return Data << x(); }

bool CIr77ZNEGATIVE<int8_t>::operator!() const { return !Data; }

bool CIr77ZNEGATIVE<int8_t>::operator&&(CIr77ZNEGATIVE<int8_t> const& x) const { return Data && x(); }

bool CIr77ZNEGATIVE<int8_t>::operator||(CIr77ZNEGATIVE<int8_t> const& x) const { return Data || x(); }

bool CIr77ZNEGATIVE<int8_t>::operator==(CIr77ZNEGATIVE<int8_t> const& x) const { return Data == x(); }

bool CIr77ZNEGATIVE<int8_t>::operator!=(CIr77ZNEGATIVE<int8_t> const& x) const { return Data != x(); }

bool CIr77ZNEGATIVE<int8_t>::operator<(CIr77ZNEGATIVE<int8_t> const& x) const { return Data < x(); }

bool CIr77ZNEGATIVE<int8_t>::operator>(CIr77ZNEGATIVE<int8_t> const& x) const { return Data > x(); }

bool CIr77ZNEGATIVE<int8_t>::operator<=(CIr77ZNEGATIVE<int8_t> const& x) const { return Data <= x(); }

bool CIr77ZNEGATIVE<int8_t>::operator>=(CIr77ZNEGATIVE<int8_t> const& x) const { return Data >= x(); }

std::strong_ordering CIr77ZNEGATIVE<int8_t>::operator<=>(CIr77ZNEGATIVE<int8_t> const& x) const { return Data <=> x(); }

void CIr77ZNEGATIVE<int8_t>::operator+=(CIr77ZNEGATIVE<int8_t> const& x) {
    CIr77ZNEGATIVE A{*this}, B{x};

    try {
        A = A.operator+(B);
    } catch (std::overflow_error error) {
        throw error;
    }

    *this = A;
}

void CIr77ZNEGATIVE<int8_t>::operator-=(CIr77ZNEGATIVE<int8_t> const& x) {
    CIr77ZNEGATIVE A{*this}, B{x};

    try {
        A = A.operator-(B);
    } catch (std::overflow_error error) {
        throw error;
    }

    *this = A;
}

void CIr77ZNEGATIVE<int8_t>::operator*=(CIr77ZNEGATIVE<int8_t> const& x) {
    CIr77ZNEGATIVE A{*this}, B{x};

    try {
        A = A.operator*(B);
    } catch (std::overflow_error error) {
        throw error;
    }

    *this = A;
}

void CIr77ZNEGATIVE<int8_t>::operator&=(CIr77ZNEGATIVE<int8_t> const& x) {
    CIr77ZNEGATIVE A{*this}, B{x};

    try {
        A = A.operator&(B);
    } catch (std::overflow_error error) {
        throw error;
    }

    *this = A;
}

void CIr77ZNEGATIVE<int8_t>::operator|=(CIr77ZNEGATIVE<int8_t> const& x) {
    CIr77ZNEGATIVE A{*this}, B{x};

    try {
        A = A.operator|(B);
    } catch (std::overflow_error error) {
        throw error;
    }

    *this = A;
}

void CIr77ZNEGATIVE<int8_t>::operator^=(CIr77ZNEGATIVE<int8_t> const& x) {
    CIr77ZNEGATIVE A{*this}, B{x};

    try {
        A = A.operator^(B);
    } catch (std::overflow_error error) {
        throw error;
    }

    *this = A;
}

void CIr77ZNEGATIVE<int8_t>::operator>>=(CIr77ZNEGATIVE<int8_t> const& x) {
    CIr77ZNEGATIVE A{*this}, B{x};

    try {
        A = A.operator>>(B);
    } catch (std::overflow_error error) {
        throw error;
    }

    *this = A;
}

void CIr77ZNEGATIVE<int8_t>::operator<<=(CIr77ZNEGATIVE<int8_t> const& x) {
    CIr77ZNEGATIVE A{*this}, B{x};

    try {
        A = A.operator<<(B);
    } catch (std::overflow_error error) {
        throw error;
    }

    *this = A;
}
}  // namespace CIr77TENSOR