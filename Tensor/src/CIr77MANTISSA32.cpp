#pragma once

#include <stdfloat>

#include <type_traits>

#include <optional>

#include <stdexcept>

module CIr77TENSOR:CIr77MANTISSA;

import :CIr77FORWARD;
import :CIr77RESOURCE;

namespace CIr77TENSOR {
CIr77MANTISSA<float>::CIr77MANTISSA(float const& x) {
    float A{x};

    unsigned char* Data = reinterpret_cast<unsigned char*>(&A);

    SignBit = ((Data[3] & 0b10000000) == 0b10000000) ? true : false;

    Shift = (Data[3] << 1) | (Data[2] >> 7);

    Significand = 0xFFFF8F00 & *reinterpret_cast<uint32_t*>(Data);
}

CIr77MANTISSA<float>::CIr77MANTISSA(bool sign, const uint32_t& mantissa, const uint8_t& exponent) {
    SignBit = sign;

    Significand = mantissa;

    Shift = exponent;
}

void CIr77MANTISSA<float>::operator=(float const& x) {
    float A{x};

    unsigned char* Data = reinterpret_cast<unsigned char*>(&A);

    SignBit = ((Data[3] & 0b10000000) == 0b10000000) ? true : false;

    Shift = (Data[3] << 1) | (Data[2] >> 7);

    Significand = 0xFFFF8F00 & *reinterpret_cast<uint32_t*>(Data);
}

float CIr77MANTISSA<float>::operator()() const {
    float expose{0};

    unsigned char* data = reinterpret_cast<unsigned char*>(&expose);

    *reinterpret_cast<uint32_t*>(data) = Significand;

    data[3] |= Shift >> 1;

    data[2] |= Shift << 7;

    expose = SignBit ? -1 * expose : expose;

    return expose;
}

float CIr77MANTISSA<float>::Absolute() const { return SignBit ? -1 * (this->operator()()) : this->operator()(); }

float CIr77MANTISSA<float>::Floor() const {
    if (IsZero()) return 0.0f;

    if (Shift == 126) return 0.0f;

    if (Shift >= 150) return this->operator()();

    uint32_t expose{Significand};

    if (Shift >= 127) {
        expose <<= 23 - (Shift - 127);

        expose <<= 23 - (Shift - 127);
    }

    unsigned char* expose_ptr = reinterpret_cast<unsigned char*>(&expose);

    expose_ptr[3] |= Shift >> 1;

    expose_ptr[2] |= Shift << 7;

    expose_ptr[3] = SignBit ? expose_ptr[3] | 0b10000000 : expose_ptr[3] & 0b01111111;

    return static_cast<float>(expose);
}

float CIr77MANTISSA<float>::Decimal() const { return this->operator()() - this->Floor(); }

bool CIr77MANTISSA<float>::IsPositive() const { return !SignBit ? true : false; }

bool CIr77MANTISSA<float>::IsNegative() const { return SignBit ? true : false; }

bool CIr77MANTISSA<float>::IsZero() const { return Significand == 0 && Shift == 0 ? true : false; }

bool CIr77MANTISSA<float>::IsNonNegative() const { return Significand == 0 && Shift == 0 || !SignBit ? true : false; }

bool CIr77MANTISSA<float>::IsNonPositive() const { return Significand == 0 && Shift == 0 || SignBit ? true : false; }

bool CIr77MANTISSA<float>::IsInteger() const {
    uint32_t mantissa = Significand;

    if (IsZero()) return true;

    if (Shift >= 127) {
        mantissa <<= 9 + Shift - 127;

        return mantissa > 0 ? false : true;
    }
    return false;
}

bool CIr77MANTISSA<float>::IsReal() const { return IsInteger() ? false : true; }

bool CIr77MANTISSA<float>::IsInfinite() const { return Shift == 0xFF && Significand == 0 ? true : false; }

bool CIr77MANTISSA<float>::IsMaximum() const {
    if (!SignBit && Shift == 0xFE && Significand == 8388607ui32)
        return true;

    else
        return false;
}

bool CIr77MANTISSA<float>::IsMinimum() const {
    if (SignBit && Shift == 0xFE && Significand == 8388607ui32)
        return true;

    else
        return false;
}

bool& CIr77MANTISSA<float>::Sign() { return SignBit; }

void CIr77MANTISSA<float>::Mantissa(uint32_t const& x) {
    if (x > 8388608ui32) throw std::overflow_error{"Mantissa out of range."};
}
uint32_t CIr77MANTISSA<float>::Mantissa() const { return Significand; }
void CIr77MANTISSA<float>::Exponent(uint8_t const& x) {
    if (x > 256ui8) throw std::overflow_error{"Exponent out of range."};
}
uint8_t CIr77MANTISSA<float>::Exponent() const { return Shift; }

}  // namespace CIr77TENSOR