#pragma once

#include <type_traits>

#include <compare>

#include <stdexcept>

export module CIr77TENSOR:CIr77ZNEGATIVE;

import :CIr77FORWARD;
import :CIr77RESOURCE;

import :CIr77MANTISSA;

export namespace CIr77TENSOR {
template <assert_signed_int T>
class CIr77ZNEGATIVE {
   public:
    CIr77ZNEGATIVE() = default;
};

template <>
class CIr77ZNEGATIVE<int8_t> {
   public:
    CIr77ZNEGATIVE();

    CIr77ZNEGATIVE(CIr77ZNEGATIVE<int8_t> const& x);

    CIr77ZNEGATIVE(int8_t const& x);

   public:
    void operator=(CIr77ZNEGATIVE<int8_t> const& x);

    void operator=(int8_t const& x);

    int8_t operator()() const;

    unsigned char& operator[](size_t const& i);

    CIr77ZNEGATIVE<int8_t> operator++();

    CIr77ZNEGATIVE<int8_t> operator++(int);

    CIr77ZNEGATIVE<int8_t> operator--();

    CIr77ZNEGATIVE<int8_t> operator--(int);

    CIr77ZNEGATIVE<int8_t> operator+() const;

    CIr77ZNEGATIVE<int8_t> operator-() const;

    CIr77ZNEGATIVE<int8_t> operator+(CIr77ZNEGATIVE<int8_t> const& x) const;

    CIr77ZNEGATIVE<int8_t> operator-(CIr77ZNEGATIVE<int8_t> const& x) const;

    CIr77MANTISSA<double> operator/(CIr77MANTISSA<double> const& x) const;

    CIr77ZNEGATIVE<int8_t> operator*(CIr77ZNEGATIVE<int8_t> const& x) const;

    size_t operator%(CIr77ZNEGATIVE<int8_t> const& x) const;

    CIr77ZNEGATIVE<int8_t> operator~() const;

    CIr77ZNEGATIVE<int8_t> operator&(CIr77ZNEGATIVE<int8_t> const& x) const;

    CIr77ZNEGATIVE<int8_t> operator|(CIr77ZNEGATIVE<int8_t> const x) const;

    CIr77ZNEGATIVE<int8_t> operator^(CIr77ZNEGATIVE<int8_t> const x) const;

    CIr77ZNEGATIVE<int8_t> operator>>(CIr77ZNEGATIVE<int8_t> const x) const;

    CIr77ZNEGATIVE<int8_t> operator<<(CIr77ZNEGATIVE<int8_t> const x) const;

    bool operator!() const;

    bool operator&&(CIr77ZNEGATIVE<int8_t> const& x) const;

    bool operator||(CIr77ZNEGATIVE<int8_t> const& x) const;

    bool operator==(CIr77ZNEGATIVE<int8_t> const& x) const;

    bool operator!=(CIr77ZNEGATIVE<int8_t> const& x) const;

    bool operator<(CIr77ZNEGATIVE<int8_t> const& x) const;

    bool operator>(CIr77ZNEGATIVE<int8_t> const& x) const;

    bool operator<=(CIr77ZNEGATIVE<int8_t> const& x) const;

    bool operator>=(CIr77ZNEGATIVE<int8_t> const& x) const;

    std::strong_ordering operator<=>(CIr77ZNEGATIVE<int8_t> const& x) const;

    void operator+=(CIr77ZNEGATIVE<int8_t> const& x);

    void operator-=(CIr77ZNEGATIVE<int8_t> const& x);

    void operator*=(CIr77ZNEGATIVE<int8_t> const& x);

   private:
    void operator/=(CIr77ZNEGATIVE<int8_t> const&) {}

    void operator%=(CIr77ZNEGATIVE<int8_t> const&) {}

   public:
    void operator&=(CIr77ZNEGATIVE<int8_t> const& x);

    void operator|=(CIr77ZNEGATIVE<int8_t> const& x);

    void operator^=(CIr77ZNEGATIVE<int8_t> const& x);

    void operator>>=(CIr77ZNEGATIVE<int8_t> const& x);

    void operator<<=(CIr77ZNEGATIVE<int8_t> const& x);

   public:
    bool Sign() { return (Data & 0x80) == 0x80 ? true : false; }

    void Sign(bool const& set) { set ? Data |= 0x80 : Data &= 0x7F; }

   private:
    int8_t Data{0};
};

typedef CIr77ZNEGATIVE<int8_t> ZNeg8;

template <>
class CIr77ZNEGATIVE<int16_t> {
   public:
    CIr77ZNEGATIVE();

    CIr77ZNEGATIVE(CIr77ZNEGATIVE<int16_t> const& x);

    CIr77ZNEGATIVE(int16_t const& x);

   public:
    void operator=(CIr77ZNEGATIVE<int16_t> const& x);

    void operator=(int16_t const& x);

    int16_t operator()() const;

    unsigned char& operator[](size_t const& i);

    CIr77ZNEGATIVE<int16_t> operator++();

    CIr77ZNEGATIVE<int16_t> operator++(int);

    CIr77ZNEGATIVE<int16_t> operator--();

    CIr77ZNEGATIVE<int16_t> operator--(int);

    CIr77ZNEGATIVE<int16_t> operator+() const;

    CIr77ZNEGATIVE<int16_t> operator-() const;

    CIr77ZNEGATIVE<int16_t> operator+(CIr77ZNEGATIVE<int16_t> const& x) const;

    CIr77ZNEGATIVE<int16_t> operator-(CIr77ZNEGATIVE<int16_t> const& x) const;

    CIr77MANTISSA<double> operator/(CIr77MANTISSA<double> const& x) const;

    CIr77ZNEGATIVE<int16_t> operator*(CIr77ZNEGATIVE<int16_t> const& x) const;

    size_t operator%(CIr77ZNEGATIVE<int16_t> const& x) const;

    CIr77ZNEGATIVE<int16_t> operator~() const;

    CIr77ZNEGATIVE<int16_t> operator&(CIr77ZNEGATIVE<int16_t> const& x) const;

    CIr77ZNEGATIVE<int16_t> operator|(CIr77ZNEGATIVE<int16_t> const x) const;

    CIr77ZNEGATIVE<int16_t> operator^(CIr77ZNEGATIVE<int16_t> const x) const;

    CIr77ZNEGATIVE<int16_t> operator>>(CIr77ZNEGATIVE<int16_t> const x) const;

    CIr77ZNEGATIVE<int16_t> operator<<(CIr77ZNEGATIVE<int16_t> const x) const;

    bool operator!() const;

    bool operator&&(CIr77ZNEGATIVE<int16_t> const& x) const;

    bool operator||(CIr77ZNEGATIVE<int16_t> const& x) const;

    bool operator==(CIr77ZNEGATIVE<int16_t> const& x) const;

    bool operator!=(CIr77ZNEGATIVE<int16_t> const& x) const;

    bool operator<(CIr77ZNEGATIVE<int16_t> const& x) const;

    bool operator>(CIr77ZNEGATIVE<int16_t> const& x) const;

    bool operator<=(CIr77ZNEGATIVE<int16_t> const& x) const;

    bool operator>=(CIr77ZNEGATIVE<int16_t> const& x) const;

    std::strong_ordering operator<=>(CIr77ZNEGATIVE<int16_t> const& x) const;

    void operator+=(CIr77ZNEGATIVE<int16_t> const& x);

    void operator-=(CIr77ZNEGATIVE<int16_t> const& x);

    void operator*=(CIr77ZNEGATIVE<int16_t> const& x);

   private:
    void operator/=(CIr77ZNEGATIVE<int16_t> const&) {}

    void operator%=(CIr77ZNEGATIVE<int16_t> const&) {}

   public:
    void operator&=(CIr77ZNEGATIVE<int16_t> const& x);

    void operator|=(CIr77ZNEGATIVE<int16_t> const& x);

    void operator^=(CIr77ZNEGATIVE<int16_t> const& x);

    void operator>>=(CIr77ZNEGATIVE<int16_t> const& x);

    void operator<<=(CIr77ZNEGATIVE<int16_t> const& x);

   public:
    bool Sign() { return (Data & 0x8000) == 0x8000 ? true : false; }

    void Sign(bool const& set) { set ? Data |= 0x8000 : Data &= 0x7FFF; }

   private:
    int16_t Data{0};
};

typedef CIr77ZNEGATIVE<int16_t> ZNeg16;

template <>
class CIr77ZNEGATIVE<int32_t> {
   public:
    CIr77ZNEGATIVE();

    CIr77ZNEGATIVE(CIr77ZNEGATIVE<int32_t> const& x);

    CIr77ZNEGATIVE(int32_t const& x);

   public:
    void operator=(CIr77ZNEGATIVE<int32_t> const& x);

    void operator=(int32_t const& x);

    int32_t operator()() const;

    unsigned char& operator[](size_t const& i);

    CIr77ZNEGATIVE<int32_t> operator++();

    CIr77ZNEGATIVE<int32_t> operator++(int);

    CIr77ZNEGATIVE<int32_t> operator--();

    CIr77ZNEGATIVE<int32_t> operator--(int);

    CIr77ZNEGATIVE<int32_t> operator+() const;

    CIr77ZNEGATIVE<int32_t> operator-() const;

    CIr77ZNEGATIVE<int32_t> operator+(CIr77ZNEGATIVE<int32_t> const& x) const;

    CIr77ZNEGATIVE<int32_t> operator-(CIr77ZNEGATIVE<int32_t> const& x) const;

    CIr77MANTISSA<double> operator/(CIr77MANTISSA<double> const& x) const;

    CIr77ZNEGATIVE<int32_t> operator*(CIr77ZNEGATIVE<int32_t> const& x) const;

    size_t operator%(CIr77ZNEGATIVE<int32_t> const& x) const;

    CIr77ZNEGATIVE<int32_t> operator~() const;

    CIr77ZNEGATIVE<int32_t> operator&(CIr77ZNEGATIVE<int32_t> const& x) const;

    CIr77ZNEGATIVE<int32_t> operator|(CIr77ZNEGATIVE<int32_t> const x) const;

    CIr77ZNEGATIVE<int32_t> operator^(CIr77ZNEGATIVE<int32_t> const x) const;

    CIr77ZNEGATIVE<int32_t> operator>>(CIr77ZNEGATIVE<int32_t> const x) const;

    CIr77ZNEGATIVE<int32_t> operator<<(CIr77ZNEGATIVE<int32_t> const x) const;

    bool operator!() const;

    bool operator&&(CIr77ZNEGATIVE<int32_t> const& x) const;

    bool operator||(CIr77ZNEGATIVE<int32_t> const& x) const;

    bool operator==(CIr77ZNEGATIVE<int32_t> const& x) const;

    bool operator!=(CIr77ZNEGATIVE<int32_t> const& x) const;

    bool operator<(CIr77ZNEGATIVE<int32_t> const& x) const;

    bool operator>(CIr77ZNEGATIVE<int32_t> const& x) const;

    bool operator<=(CIr77ZNEGATIVE<int32_t> const& x) const;

    bool operator>=(CIr77ZNEGATIVE<int32_t> const& x) const;

    std::strong_ordering operator<=>(CIr77ZNEGATIVE<int32_t> const& x) const;

    void operator+=(CIr77ZNEGATIVE<int32_t> const& x);

    void operator-=(CIr77ZNEGATIVE<int32_t> const& x);

    void operator*=(CIr77ZNEGATIVE<int32_t> const& x);

   private:
    void operator/=(CIr77ZNEGATIVE<int32_t> const&) {}

    void operator%=(CIr77ZNEGATIVE<int32_t> const&) {}

   public:
    void operator&=(CIr77ZNEGATIVE<int32_t> const& x);

    void operator|=(CIr77ZNEGATIVE<int32_t> const& x);

    void operator^=(CIr77ZNEGATIVE<int32_t> const& x);

    void operator>>=(CIr77ZNEGATIVE<int32_t> const& x);

    void operator<<=(CIr77ZNEGATIVE<int32_t> const& x);

   public:
    bool Sign() { return (Data & 0x80000000) == 0x80000000 ? true : false; }

    void Sign(bool const& set) { set ? Data |= 0x80000000 : Data &= 0x7FFFFFFF; }

   private:
    int32_t Data{0};
};

typedef CIr77ZNEGATIVE<int32_t> ZNeg32;

template <>
class CIr77ZNEGATIVE<int64_t> {
   public:
    CIr77ZNEGATIVE();

    CIr77ZNEGATIVE(CIr77ZNEGATIVE<int64_t> const& x);

    CIr77ZNEGATIVE(int64_t const& x);

   public:
    void operator=(CIr77ZNEGATIVE<int64_t> const& x);

    void operator=(int64_t const& x);

    int64_t operator()() const;

    unsigned char& operator[](size_t const& i);

    CIr77ZNEGATIVE<int64_t> operator++();

    CIr77ZNEGATIVE<int64_t> operator++(int);

    CIr77ZNEGATIVE<int64_t> operator--();

    CIr77ZNEGATIVE<int64_t> operator--(int);

    CIr77ZNEGATIVE<int64_t> operator+() const;

    CIr77ZNEGATIVE<int64_t> operator-() const;

    CIr77ZNEGATIVE<int64_t> operator+(CIr77ZNEGATIVE<int64_t> const& x) const;

    CIr77ZNEGATIVE<int64_t> operator-(CIr77ZNEGATIVE<int64_t> const& x) const;

    CIr77MANTISSA<double> operator/(CIr77MANTISSA<double> const& x) const;

    CIr77ZNEGATIVE<int64_t> operator*(CIr77ZNEGATIVE<int64_t> const& x) const;

    size_t operator%(CIr77ZNEGATIVE<int64_t> const& x) const;

    CIr77ZNEGATIVE<int64_t> operator~() const;

    CIr77ZNEGATIVE<int64_t> operator&(CIr77ZNEGATIVE<int64_t> const& x) const;

    CIr77ZNEGATIVE<int64_t> operator|(CIr77ZNEGATIVE<int64_t> const x) const;

    CIr77ZNEGATIVE<int64_t> operator^(CIr77ZNEGATIVE<int64_t> const x) const;

    CIr77ZNEGATIVE<int64_t> operator>>(CIr77ZNEGATIVE<int64_t> const x) const;

    CIr77ZNEGATIVE<int64_t> operator<<(CIr77ZNEGATIVE<int64_t> const x) const;

    bool operator!() const;

    bool operator&&(CIr77ZNEGATIVE<int64_t> const& x) const;

    bool operator||(CIr77ZNEGATIVE<int64_t> const& x) const;

    bool operator==(CIr77ZNEGATIVE<int64_t> const& x) const;

    bool operator!=(CIr77ZNEGATIVE<int64_t> const& x) const;

    bool operator<(CIr77ZNEGATIVE<int64_t> const& x) const;

    bool operator>(CIr77ZNEGATIVE<int64_t> const& x) const;

    bool operator<=(CIr77ZNEGATIVE<int64_t> const& x) const;

    bool operator>=(CIr77ZNEGATIVE<int64_t> const& x) const;

    std::weak_ordering operator<=>(CIr77ZNEGATIVE<int64_t> const& x) const;

    void operator+=(CIr77ZNEGATIVE<int64_t> const& x);

    void operator-=(CIr77ZNEGATIVE<int64_t> const& x);

    void operator*=(CIr77ZNEGATIVE<int64_t> const& x);

   private:
    void operator/=(CIr77ZNEGATIVE<int64_t> const&) {}

    void operator%=(CIr77ZNEGATIVE<int64_t> const&) {}

   public:
    void operator&=(CIr77ZNEGATIVE<int64_t> const& x);

    void operator|=(CIr77ZNEGATIVE<int64_t> const& x);

    void operator^=(CIr77ZNEGATIVE<int64_t> const& x);

    void operator>>=(CIr77ZNEGATIVE<int64_t> const& x);

    void operator<<=(CIr77ZNEGATIVE<int64_t> const& x);

   public:
    bool Sign() { return (Data & 0x8000000000000000) == 0x8000000000000000 ? true : false; }

    void Sign(bool const& set) { set ? Data |= 0x8000000000000000 : Data &= 0x7FFFFFFFFFFFFFFF; }

   private:
    int64_t Data{0};
};

typedef CIr77ZNEGATIVE<int64_t> ZNeg64;

template <typename T>
concept ASSERT_CIr77ZNEGATIVE = std::is_same<T, CIr77ZNEGATIVE<int8_t>>::value || std::is_same<T, CIr77ZNEGATIVE<int16_t>>::value ||
                                std::is_same<T, CIr77ZNEGATIVE<int32_t>>::value || std::is_same<T, CIr77ZNEGATIVE<int64_t>>::value;

}  // namespace CIr77TENSOR