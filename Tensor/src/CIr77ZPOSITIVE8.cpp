#pragma once

#include <type_traits>

#include <compare>

#include <stdexcept>

module CIr77TENSOR:CIr77ZPOSITIVE;

import :CIr77FORWARD;
import :CIr77RESOURCE;

import :CIr77MANTISSA;

namespace CIr77TENSOR {
CIr77ZPOSITIVE<uint8_t>::CIr77ZPOSITIVE() { Data = 0i8; }

CIr77ZPOSITIVE<uint8_t>::CIr77ZPOSITIVE(CIr77ZPOSITIVE<uint8_t> const& x) { Data = x(); }

CIr77ZPOSITIVE<uint8_t>::CIr77ZPOSITIVE(uint8_t const& x) { Data = x; }

void CIr77ZPOSITIVE<uint8_t>::operator=(CIr77ZPOSITIVE<uint8_t> const& x) { Data = x(); }

void CIr77ZPOSITIVE<uint8_t>::operator=(uint8_t const& x) { Data = x; }

uint8_t CIr77ZPOSITIVE<uint8_t>::operator()() const { return Data; }

unsigned char& CIr77ZPOSITIVE<uint8_t>::operator[](size_t const& i) {
    if (i > 0) throw std::overflow_error{"Index out of range."};

    return reinterpret_cast<unsigned char*>(&Data)[0];
}

CIr77ZPOSITIVE<uint8_t> CIr77ZPOSITIVE<uint8_t>::operator++() {
    if (Data == std::numeric_limits<uint8_t>::max()) throw std::overflow_error{"ZPositive Increment Overflow."};

    Data += 1i8;

    return Data;
}

CIr77ZPOSITIVE<uint8_t> CIr77ZPOSITIVE<uint8_t>::operator++(int) {
    if (Data == std::numeric_limits<uint8_t>::max()) throw std::overflow_error{"ZPositive Increment Overflow."};

    uint8_t Q = Data;

    Data += 1i8;

    return Q;
}

CIr77ZPOSITIVE<uint8_t> CIr77ZPOSITIVE<uint8_t>::operator--() {
    if (Data == std::numeric_limits<uint8_t>::min() + 1ui8) throw std::overflow_error{"ZPositive Decrement Overflow."};

    Data -= 1i8;

    return Data;
}

CIr77ZPOSITIVE<uint8_t> CIr77ZPOSITIVE<uint8_t>::operator--(int) {
    if (Data == std::numeric_limits<uint8_t>::min() + 1ui8) throw std::overflow_error{"ZPositive Decrement Overflow."};

    uint8_t Q = Data;

    Data -= 1i8;

    return Q;
}

CIr77ZPOSITIVE<uint8_t> CIr77ZPOSITIVE<uint8_t>::operator+() const { return uint8_t{Data}; }

CIr77ZPOSITIVE<uint8_t> CIr77ZPOSITIVE<uint8_t>::operator+(CIr77ZPOSITIVE<uint8_t> const& x) const {
    int16_t A{Data}, B{x()};

    A += B;

    if (A < std::numeric_limits<uint8_t>::min() + 1ui8 || A > std::numeric_limits<uint8_t>::max()) throw std::overflow_error{"ZPositive Addition Overflow."};

    return static_cast<uint8_t>(A);
}

CIr77ZPOSITIVE<uint8_t> CIr77ZPOSITIVE<uint8_t>::operator-(CIr77ZPOSITIVE<uint8_t> const& x) const {
    int16_t A{Data}, B{x()};

    A -= B;

    if (A < std::numeric_limits<uint8_t>::min() + 1ui8 || A > std::numeric_limits<uint8_t>::max()) throw std::overflow_error{"ZPositive Subtraction Overflow."};

    return static_cast<uint8_t>(A);
}

CIr77MANTISSA<double> CIr77ZPOSITIVE<uint8_t>::operator/(CIr77MANTISSA<double> const& x) const {
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

CIr77ZPOSITIVE<uint8_t> CIr77ZPOSITIVE<uint8_t>::operator*(CIr77ZPOSITIVE<uint8_t> const& x) const {
    int16_t A{Data}, B{x()};

    A *= B;

    if (A < std::numeric_limits<uint8_t>::min() + 1ui8 || A > std::numeric_limits<uint8_t>::max())
        throw std::overflow_error{"ZPositive Multiplication Overflow."};

    return static_cast<uint8_t>(A);
}

size_t CIr77ZPOSITIVE<uint8_t>::operator%(CIr77ZPOSITIVE<uint8_t> const& x) const {
    if (Data < x) return static_cast<size_t>(Data & 0x7FFFFFFFFFFFFFFF);

    if (x() == 0i8) throw std::runtime_error{"Division by Zero."};

    if (Data == 0i8) return 0i8;

    uint64_t A{Data}, B{x()};

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

CIr77ZPOSITIVE<uint8_t> CIr77ZPOSITIVE<uint8_t>::operator~() const { return ~Data; }

CIr77ZPOSITIVE<uint8_t> CIr77ZPOSITIVE<uint8_t>::operator&(CIr77ZPOSITIVE<uint8_t> const& x) const { return Data & x(); }

CIr77ZPOSITIVE<uint8_t> CIr77ZPOSITIVE<uint8_t>::operator|(CIr77ZPOSITIVE<uint8_t> const x) const { return Data | x(); }

CIr77ZPOSITIVE<uint8_t> CIr77ZPOSITIVE<uint8_t>::operator^(CIr77ZPOSITIVE<uint8_t> const x) const { return Data ^ x(); }

CIr77ZPOSITIVE<uint8_t> CIr77ZPOSITIVE<uint8_t>::operator>>(CIr77ZPOSITIVE<uint8_t> const x) const { return Data >> x(); }

CIr77ZPOSITIVE<uint8_t> CIr77ZPOSITIVE<uint8_t>::operator<<(CIr77ZPOSITIVE<uint8_t> const x) const { return Data << x(); }

bool CIr77ZPOSITIVE<uint8_t>::operator!() const { return !Data; }

bool CIr77ZPOSITIVE<uint8_t>::operator&&(CIr77ZPOSITIVE<uint8_t> const& x) const { return Data && x(); }

bool CIr77ZPOSITIVE<uint8_t>::operator||(CIr77ZPOSITIVE<uint8_t> const& x) const { return Data || x(); }

bool CIr77ZPOSITIVE<uint8_t>::operator==(CIr77ZPOSITIVE<uint8_t> const& x) const { return Data == x(); }

bool CIr77ZPOSITIVE<uint8_t>::operator!=(CIr77ZPOSITIVE<uint8_t> const& x) const { return Data != x(); }

bool CIr77ZPOSITIVE<uint8_t>::operator<(CIr77ZPOSITIVE<uint8_t> const& x) const { return Data < x(); }

bool CIr77ZPOSITIVE<uint8_t>::operator>(CIr77ZPOSITIVE<uint8_t> const& x) const { return Data > x(); }

bool CIr77ZPOSITIVE<uint8_t>::operator<=(CIr77ZPOSITIVE<uint8_t> const& x) const { return Data <= x(); }

bool CIr77ZPOSITIVE<uint8_t>::operator>=(CIr77ZPOSITIVE<uint8_t> const& x) const { return Data >= x(); }

std::strong_ordering CIr77ZPOSITIVE<uint8_t>::operator<=>(CIr77ZPOSITIVE<uint8_t> const& x) const { return Data <=> x(); }

void CIr77ZPOSITIVE<uint8_t>::operator+=(CIr77ZPOSITIVE<uint8_t> const& x) {
    CIr77ZPOSITIVE A{*this}, B{x};

    try {
        A = A.operator+(B);
    } catch (std::overflow_error error) {
        throw error;
    }

    *this = A;
}

void CIr77ZPOSITIVE<uint8_t>::operator-=(CIr77ZPOSITIVE<uint8_t> const& x) {
    CIr77ZPOSITIVE A{*this}, B{x};

    try {
        A = A.operator-(B);
    } catch (std::overflow_error error) {
        throw error;
    }

    *this = A;
}

void CIr77ZPOSITIVE<uint8_t>::operator*=(CIr77ZPOSITIVE<uint8_t> const& x) {
    CIr77ZPOSITIVE A{*this}, B{x};

    try {
        A = A.operator*(B);
    } catch (std::overflow_error error) {
        throw error;
    }

    *this = A;
}

void CIr77ZPOSITIVE<uint8_t>::operator&=(CIr77ZPOSITIVE<uint8_t> const& x) {
    CIr77ZPOSITIVE A{*this}, B{x};

    try {
        A = A.operator&(B);
    } catch (std::overflow_error error) {
        throw error;
    }

    *this = A;
}

void CIr77ZPOSITIVE<uint8_t>::operator|=(CIr77ZPOSITIVE<uint8_t> const& x) {
    CIr77ZPOSITIVE A{*this}, B{x};

    try {
        A = A.operator|(B);
    } catch (std::overflow_error error) {
        throw error;
    }

    *this = A;
}

void CIr77ZPOSITIVE<uint8_t>::operator^=(CIr77ZPOSITIVE<uint8_t> const& x) {
    CIr77ZPOSITIVE A{*this}, B{x};

    try {
        A = A.operator^(B);
    } catch (std::overflow_error error) {
        throw error;
    }

    *this = A;
}

void CIr77ZPOSITIVE<uint8_t>::operator>>=(CIr77ZPOSITIVE<uint8_t> const& x) {
    CIr77ZPOSITIVE A{*this}, B{x};

    try {
        A = A.operator>>(B);
    } catch (std::overflow_error error) {
        throw error;
    }

    *this = A;
}

void CIr77ZPOSITIVE<uint8_t>::operator<<=(CIr77ZPOSITIVE<uint8_t> const& x) {
    CIr77ZPOSITIVE A{*this}, B{x};

    try {
        A = A.operator<<(B);
    } catch (std::overflow_error error) {
        throw error;
    }

    *this = A;
}
}  // namespace CIr77TENSOR