#pragma once

#include <stdfloat>

#include <type_traits>

#include <stdexcept>

module CIr77TENSOR:CIr77MANTISSA;

import :CIr77FORWARD;
import :CIr77RESOURCE;

namespace CIr77TENSOR {
CIr77MANTISSA<double>::CIr77MANTISSA(double const& x) {
    double A{x};

    unsigned char* Data = reinterpret_cast<unsigned char*>(&A);

    SignBit = ((Data[7] & 0b10000000) == 0b10000000) ? true : false;

    Shift = (Data[7] << 1) | (Data[6] >> 5);

    Significand = 0xFFFFFFFFFFFF8F00 & *reinterpret_cast<uint64_t*>(Data);
}

CIr77MANTISSA<double>::CIr77MANTISSA(bool sign, const uint64_t& mantissa, const uint16_t& exponent) {
    SignBit = sign;

    Significand = mantissa;

    Shift = exponent;
}

void CIr77MANTISSA<double>::operator=(double const& x) {
    double A{x};

    unsigned char* Data = reinterpret_cast<unsigned char*>(&A);

    SignBit = ((Data[7] & 0b10000000) == 0b10000000) ? true : false;

    Shift = (Data[7] << 1) | (Data[6] >> 5);

    Significand = 0xFFFFFFFFFFFF8F00 & *reinterpret_cast<uint64_t*>(Data);
}

double CIr77MANTISSA<double>::operator()() const {
    double expose{0};

    unsigned char* data = reinterpret_cast<unsigned char*>(&expose);

    *reinterpret_cast<uint64_t*>(data) = Significand;

    data[7] |= Shift >> 1;

    data[6] |= Shift << 5;

    expose = SignBit ? -1 * expose : expose;

    return expose;
}

double CIr77MANTISSA<double>::Absolute() const { return SignBit ? -1 * (this->operator()()) : this->operator()(); }

double CIr77MANTISSA<double>::Floor() const {
    if (IsZero()) return 0.0f;

    if (Shift == 1023) return 0.0f;

    if (Shift >= 1073) return this->operator()();

    uint64_t expose{Significand};

    if (Shift >= 1073) {
        expose <<= 52 - (Shift - 1073);

        expose <<= 52 - (Shift - 1073);
    }

    unsigned char* expose_ptr = reinterpret_cast<unsigned char*>(&expose);

    expose_ptr[7] |= Shift >> 1;

    expose_ptr[6] |= Shift << 5;

    expose_ptr[7] = SignBit ? expose_ptr[7] | 0b10000000 : expose_ptr[7] & 0b01111111;

    return static_cast<double>(expose);
}

double CIr77MANTISSA<double>::Decimal() const { return this->operator()() - this->Floor(); }

bool CIr77MANTISSA<double>::IsPositive() const { return !SignBit ? true : false; }

bool CIr77MANTISSA<double>::IsNegative() const { return SignBit ? true : false; }

bool CIr77MANTISSA<double>::IsZero() const { return Significand == 0 && Shift == 0 ? true : false; }

bool CIr77MANTISSA<double>::IsNonNegative() const { return Significand == 0 && Shift == 0 || !SignBit ? true : false; }

bool CIr77MANTISSA<double>::IsNonPositive() const { return Significand == 0 && Shift == 0 || SignBit ? true : false; }

bool CIr77MANTISSA<double>::IsInteger() const {
    uint64_t mantissa = Significand;

    if (IsZero()) return true;

    if (Shift >= 1023) {
        mantissa <<= 12 + Shift - 1023;

        return mantissa > 0 ? false : true;
    }
    return false;
}

bool CIr77MANTISSA<double>::IsReal() const { return IsInteger() ? false : true; }

bool CIr77MANTISSA<double>::IsInfinite() const { return Shift == 0x08FF && Significand == 0 ? true : false; }

bool CIr77MANTISSA<double>::IsMaximum() const {
    if (!SignBit && Shift == 0x07FE && Significand == 4503599627370495ui64)
        return true;

    else
        return false;
}

bool CIr77MANTISSA<double>::IsMinimum() const {
    if (SignBit && Shift == 0x07FE && Significand == 4503599627370495ui64)
        return true;

    else
        return false;
}

bool& CIr77MANTISSA<double>::Sign() { return SignBit; }

void CIr77MANTISSA<double>::Mantissa(uint64_t const& x) {
    if (x > 4503599627370496ui64) throw std::overflow_error{"Mantissa out of range."};
}
uint64_t CIr77MANTISSA<double>::Mantissa() const { return Significand; }
void CIr77MANTISSA<double>::Exponent(uint16_t const& x) {
    if (x > 2048ui16) throw std::overflow_error{"Exponent out of range."};
}
uint16_t CIr77MANTISSA<double>::Exponent() const { return Shift; }

}  // namespace CIr77TENSOR