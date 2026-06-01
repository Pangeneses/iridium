#pragma once

#include <compare>

#include <stdexcept>

export module CIr77TENSOR:CIr77PRECISION;

import :CIr77FORWARD;
import :CIr77RESOURCE;

import :CIr77MANTISSA;

export namespace CIr77TENSOR {
template <assert_precision T>
class CIr77PRECISION {
   public:
    CIr77PRECISION() = default;
};

template <>
class CIr77PRECISION<float> {
   public:
    CIr77PRECISION();

    CIr77PRECISION(CIr77PRECISION<float> const& x);

    CIr77PRECISION(float const& x);

   public:
    void operator=(CIr77PRECISION<float> const& x);

    void operator=(float const& x);

    float operator()() const;

   private:
    CIr77PRECISION<float> operator[](int) const {}

   public:
    CIr77PRECISION<float> operator++();

    CIr77PRECISION<float> operator++(int);

    CIr77PRECISION<float> operator--();

    CIr77PRECISION<float> operator--(int);

   public:
    CIr77PRECISION<float> operator+() const;

    CIr77PRECISION<float> operator-() const;

    CIr77PRECISION<float> operator+(CIr77PRECISION<float> const& x) const;

    CIr77PRECISION<float> operator-(CIr77PRECISION<float> const& x) const;

    CIr77PRECISION<float> operator/(CIr77PRECISION<float> const& x) const;

    CIr77PRECISION<float> operator*(CIr77PRECISION<float> const& x) const;

    CIr77PRECISION<float> operator%(CIr77PRECISION<float> const& x) const;

   private:
    CIr77PRECISION<float> operator~() const { return {}; }

    CIr77PRECISION<float> operator&(CIr77PRECISION<float> const&) const { return {}; }

    CIr77PRECISION<float> operator|(CIr77PRECISION<float> const&) const { return {}; }

    CIr77PRECISION<float> operator^(CIr77PRECISION<float> const&) const { return {}; }

    CIr77PRECISION<float> operator>>(CIr77PRECISION<float> const&) const { return {}; }

    CIr77PRECISION<float> operator<<(CIr77PRECISION<float> const&) const { return {}; }

   private:
    bool operator!() const { return {}; }

    bool operator&&(CIr77PRECISION<float> const&) const { return {}; }

    bool operator||(CIr77PRECISION<float> const&) const { return {}; }

   public:
    bool operator==(CIr77PRECISION<float> const& x) const;

    bool operator!=(CIr77PRECISION<float> const& x) const;

    bool operator<(CIr77PRECISION<float> const& x) const;

    bool operator>(CIr77PRECISION<float> const& x) const;

    bool operator<=(CIr77PRECISION<float> const& x) const;

    bool operator>=(CIr77PRECISION<float> const& x) const;

    std::partial_ordering operator<=>(CIr77PRECISION<float> const& x) const;

   public:
    void operator+=(CIr77PRECISION<float> const& x);

    void operator-=(CIr77PRECISION<float> const& x);

    void operator*=(CIr77PRECISION<float> const& x);

    void operator/=(CIr77PRECISION<float> const&) {}

    void operator%=(CIr77PRECISION<float> const& x);

   private:
    void operator&=(CIr77PRECISION<float> const&) {}

    void operator|=(CIr77PRECISION<float> const&) {}

    void operator^=(CIr77PRECISION<float> const&) {}

    void operator>>=(CIr77PRECISION<float> const&) {}

    void operator<<=(CIr77PRECISION<float> const&) {}

   public:
    CIr77MANTISSA<float> Radix() const { return Data; }

    CIr77PRECISION<float> Abs() const { return Data.IsNegative() ? -Data() : Data(); }

   private:
    CIr77MANTISSA<float> Data;
};

template <>
class CIr77PRECISION<double> {
   public:
    CIr77PRECISION();

    CIr77PRECISION(CIr77PRECISION<double> const& x);

    CIr77PRECISION(double const& x);

   public:
    void operator=(CIr77PRECISION<double> const& x);

    void operator=(double const& x);

    double operator()() const;

   private:
    CIr77PRECISION<double> operator[](int) const {};

   public:
    CIr77PRECISION<double> operator++();

    CIr77PRECISION<double> operator++(int);

    CIr77PRECISION<double> operator--();

    CIr77PRECISION<double> operator--(int);

   public:
    CIr77PRECISION<double> operator+() const;

    CIr77PRECISION<double> operator-() const;

    CIr77PRECISION<double> operator+(CIr77PRECISION<double> const& x) const;

    CIr77PRECISION<double> operator-(CIr77PRECISION<double> const& x) const;

    CIr77PRECISION<double> operator/(CIr77PRECISION<double> const& x) const;

    CIr77PRECISION<double> operator*(CIr77PRECISION<double> const& x) const;

    CIr77PRECISION<double> operator%(CIr77PRECISION<double> const& x) const;

   private:
    CIr77PRECISION<double> operator~() const { return {}; }

    CIr77PRECISION<double> operator&(CIr77PRECISION<double> const&) const { return {}; }

    CIr77PRECISION<double> operator|(CIr77PRECISION<double> const&) const { return {}; }

    CIr77PRECISION<double> operator^(CIr77PRECISION<double> const&) const { return {}; }

    CIr77PRECISION<double> operator>>(CIr77PRECISION<double> const&) const { return {}; }

    CIr77PRECISION<double> operator<<(CIr77PRECISION<double> const&) const { return {}; }

   private:
    bool operator!() const { return {}; }

    bool operator&&(CIr77PRECISION<double> const&) const { return {}; }

    bool operator||(CIr77PRECISION<double> const&) const { return {}; }

   public:
    bool operator==(CIr77PRECISION<double> const& x) const;

    bool operator!=(CIr77PRECISION<double> const& x) const;

    bool operator<(CIr77PRECISION<double> const& x) const;

    bool operator>(CIr77PRECISION<double> const& x) const;

    bool operator<=(CIr77PRECISION<double> const& x) const;

    bool operator>=(CIr77PRECISION<double> const& x) const;

    std::partial_ordering operator<=>(CIr77PRECISION<double> const& x) const;

   public:
    void operator+=(CIr77PRECISION<double> const& x);

    void operator-=(CIr77PRECISION<double> const& x);

    void operator*=(CIr77PRECISION<double> const& x);

    void operator/=(CIr77PRECISION<double> const&) {}

    void operator%=(CIr77PRECISION<double> const& x);

   private:
    void operator&=(CIr77PRECISION<double> const&) {}

    void operator|=(CIr77PRECISION<double> const&) {}

    void operator^=(CIr77PRECISION<double> const&) {}

    void operator>>=(CIr77PRECISION<double> const&) {}

    void operator<<=(CIr77PRECISION<double> const&) {}

   public:
    CIr77MANTISSA<double> const& Radix() const { return Data; }

    CIr77PRECISION<double> Abs() const { return Data.IsNegative() ? -Data() : Data(); }

   private:
    CIr77MANTISSA<double> Data;
};

template <typename T>
concept ASSERT_CIr77PRECISION = std::is_same<T, CIr77PRECISION<float>>::value || std::is_same<T, CIr77PRECISION<double>>::value;

}  // namespace CIr77TENSOR