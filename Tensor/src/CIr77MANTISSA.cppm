#pragma once

#include <stdfloat>

#include <type_traits>

#include <optional>

#include <stdexcept>

export module CIr77TENSOR:CIr77MANTISSA;

import :CIr77FORWARD;
import :CIr77RESOURCE;

export namespace CIr77TENSOR {
template <assert_precision T>
class CIr77MANTISSA {
   public:
    CIr77MANTISSA(T const&) {}
};

template <>
class CIr77MANTISSA<float> {
   public:
    CIr77MANTISSA() = default;

    CIr77MANTISSA(float const& x);

    CIr77MANTISSA(bool sign, const uint32_t& mantissa, const uint8_t& exponent);

   public:
    void operator=(float const& x);

    float operator()() const;

    float Absolute() const;

    float Floor() const;

    float Decimal() const;

    bool IsPositive() const;

    bool IsNegative() const;

    bool IsZero() const;

    bool IsNonNegative() const;

    bool IsNonPositive() const;

    bool IsInteger() const;

    bool IsReal() const;

    bool IsInfinite() const;

    bool IsMaximum() const;

    bool IsMinimum() const;

    bool& Sign();

    void Mantissa(uint32_t const& x);

    uint32_t Mantissa() const;

    void Exponent(uint8_t const& x);

    uint8_t Exponent() const;

   private:
    bool SignBit{false};

    uint32_t Significand;

    uint8_t Shift;
};

template <>
class CIr77MANTISSA<double> {
   public:
    CIr77MANTISSA() = default;

    CIr77MANTISSA(double const& x);

    CIr77MANTISSA(bool sign, const uint64_t& mantissa, const uint16_t& exponent);

   public:
    void operator=(double const& x);

    double operator()() const;

    double Absolute() const;

    double Floor() const;

    double Decimal() const;

    bool IsPositive() const;

    bool IsNegative() const;

    bool IsZero() const;

    bool IsNonNegative() const;

    bool IsNonPositive() const;

    bool IsInteger() const;

    bool IsReal() const;

    bool IsInfinite() const;

    bool IsMaximum() const;

    bool IsMinimum() const;

    bool& Sign();

    void Mantissa(uint64_t const& x);

    uint64_t Mantissa() const;

    void Exponent(uint16_t const& x);

    uint16_t Exponent() const;

   private:
    bool SignBit{false};

    uint64_t Significand;

    uint16_t Shift;
};

template <>
class CIr77MANTISSA<long double> {
   public:
    CIr77MANTISSA() = default;

    CIr77MANTISSA(long double const& x);

    CIr77MANTISSA(bool sign, const uint64_t& mantissa, const uint16_t& exponent);

   public:
    void operator=(long double const& x);

    long double operator()() const;

    long double Absolute() const;

    long double Floor() const;

    long double Decimal() const;

    bool IsPositive() const;

    bool IsNegative() const;

    bool IsZero() const;

    bool IsNonNegative() const;

    bool IsNonPositive() const;

    bool IsInteger() const;

    bool IsReal() const;

    bool IsInfinite() const;

    bool IsMaximum() const;

    bool IsMinimum() const;

    bool& Sign();

    void Mantissa(uint64_t const& x);

    uint64_t Mantissa() const;

    void Exponent(uint16_t const& x);

    uint16_t Exponent() const;

   private:
    bool SignBit{false};

    uint64_t Significand;

    uint16_t Shift;
};
}  // namespace CIr77TENSOR