#pragma once

#include <type_traits>

#include <compare>

#include <stdexcept>

export module CIr77TENSOR:CIr77ZPOSITIVE;

import :CIr77FORWARD;
import :CIr77RESOURCE;

import :CIr77MANTISSA;

export namespace CIr77TENSOR {
template <assert_unsigned_int T>
class CIr77ZPOSITIVE {
   public:
    CIr77ZPOSITIVE() = default;
};

template <>
class CIr77ZPOSITIVE<uint8_t> {
   public:
    CIr77ZPOSITIVE();

    CIr77ZPOSITIVE(CIr77ZPOSITIVE<uint8_t> const& x);

    CIr77ZPOSITIVE(uint8_t const& x);

   public:
    void operator=(CIr77ZPOSITIVE<uint8_t> const& x);

    void operator=(uint8_t const& x);

    uint8_t operator()() const;

    unsigned char& operator[](size_t const& i);

    CIr77ZPOSITIVE<uint8_t> operator++();

    CIr77ZPOSITIVE<uint8_t> operator++(int);

    CIr77ZPOSITIVE<uint8_t> operator--();

    CIr77ZPOSITIVE<uint8_t> operator--(int);

    CIr77ZPOSITIVE<uint8_t> operator+() const;

   private:
    CIr77ZPOSITIVE<uint8_t> operator-() const {}

   public:
    CIr77ZPOSITIVE<uint8_t> operator+(CIr77ZPOSITIVE<uint8_t> const& x) const;

    CIr77ZPOSITIVE<uint8_t> operator-(CIr77ZPOSITIVE<uint8_t> const& x) const;

    CIr77MANTISSA<double> operator/(CIr77MANTISSA<double> const& x) const;

    CIr77ZPOSITIVE<uint8_t> operator*(CIr77ZPOSITIVE<uint8_t> const& x) const;

    size_t operator%(CIr77ZPOSITIVE<uint8_t> const& x) const;

    CIr77ZPOSITIVE<uint8_t> operator~() const;

    CIr77ZPOSITIVE<uint8_t> operator&(CIr77ZPOSITIVE<uint8_t> const& x) const;

    CIr77ZPOSITIVE<uint8_t> operator|(CIr77ZPOSITIVE<uint8_t> const x) const;

    CIr77ZPOSITIVE<uint8_t> operator^(CIr77ZPOSITIVE<uint8_t> const x) const;

    CIr77ZPOSITIVE<uint8_t> operator>>(CIr77ZPOSITIVE<uint8_t> const x) const;

    CIr77ZPOSITIVE<uint8_t> operator<<(CIr77ZPOSITIVE<uint8_t> const x) const;

    bool operator!() const;

    bool operator&&(CIr77ZPOSITIVE<uint8_t> const& x) const;

    bool operator||(CIr77ZPOSITIVE<uint8_t> const& x) const;

    bool operator==(CIr77ZPOSITIVE<uint8_t> const& x) const;

    bool operator!=(CIr77ZPOSITIVE<uint8_t> const& x) const;

    bool operator<(CIr77ZPOSITIVE<uint8_t> const& x) const;

    bool operator>(CIr77ZPOSITIVE<uint8_t> const& x) const;

    bool operator<=(CIr77ZPOSITIVE<uint8_t> const& x) const;

    bool operator>=(CIr77ZPOSITIVE<uint8_t> const& x) const;

    std::strong_ordering operator<=>(CIr77ZPOSITIVE<uint8_t> const& x) const;

    void operator+=(CIr77ZPOSITIVE<uint8_t> const& x);

    void operator-=(CIr77ZPOSITIVE<uint8_t> const& x);

    void operator*=(CIr77ZPOSITIVE<uint8_t> const& x);

   private:
    void operator/=(CIr77ZPOSITIVE<uint8_t> const&) {}

    void operator%=(CIr77ZPOSITIVE<uint8_t> const&) {}

   public:
    void operator&=(CIr77ZPOSITIVE<uint8_t> const& x);

    void operator|=(CIr77ZPOSITIVE<uint8_t> const& x);

    void operator^=(CIr77ZPOSITIVE<uint8_t> const& x);

    void operator>>=(CIr77ZPOSITIVE<uint8_t> const& x);

    void operator<<=(CIr77ZPOSITIVE<uint8_t> const& x);

   private:
    uint8_t Data{0};
};

typedef CIr77ZPOSITIVE<uint8_t> CIr77INTEGER8;

template <>
class CIr77ZPOSITIVE<uint16_t> {
   public:
    CIr77ZPOSITIVE();

    CIr77ZPOSITIVE(CIr77ZPOSITIVE<uint16_t> const& x);

    CIr77ZPOSITIVE(uint16_t const& x);

   public:
    void operator=(CIr77ZPOSITIVE<uint16_t> const& x);

    void operator=(uint16_t const& x);

    uint16_t operator()() const;

    unsigned char& operator[](size_t const& i);

    CIr77ZPOSITIVE<uint16_t> operator++();

    CIr77ZPOSITIVE<uint16_t> operator++(int);

    CIr77ZPOSITIVE<uint16_t> operator--();

    CIr77ZPOSITIVE<uint16_t> operator--(int);

    CIr77ZPOSITIVE<uint16_t> operator+() const;

   private:
    CIr77ZPOSITIVE<uint16_t> operator-() const {}

   public:
    CIr77ZPOSITIVE<uint16_t> operator+(CIr77ZPOSITIVE<uint16_t> const& x) const;

    CIr77ZPOSITIVE<uint16_t> operator-(CIr77ZPOSITIVE<uint16_t> const& x) const;

    CIr77MANTISSA<double> operator/(CIr77MANTISSA<double> const& x) const;

    CIr77ZPOSITIVE<uint16_t> operator*(CIr77ZPOSITIVE<uint16_t> const& x) const;

    size_t operator%(CIr77ZPOSITIVE<uint16_t> const& x) const;

    CIr77ZPOSITIVE<uint16_t> operator~() const;

    CIr77ZPOSITIVE<uint16_t> operator&(CIr77ZPOSITIVE<uint16_t> const& x) const;

    CIr77ZPOSITIVE<uint16_t> operator|(CIr77ZPOSITIVE<uint16_t> const x) const;

    CIr77ZPOSITIVE<uint16_t> operator^(CIr77ZPOSITIVE<uint16_t> const x) const;

    CIr77ZPOSITIVE<uint16_t> operator>>(CIr77ZPOSITIVE<uint16_t> const x) const;

    CIr77ZPOSITIVE<uint16_t> operator<<(CIr77ZPOSITIVE<uint16_t> const x) const;

    bool operator!() const;

    bool operator&&(CIr77ZPOSITIVE<uint16_t> const& x) const;

    bool operator||(CIr77ZPOSITIVE<uint16_t> const& x) const;

    bool operator==(CIr77ZPOSITIVE<uint16_t> const& x) const;

    bool operator!=(CIr77ZPOSITIVE<uint16_t> const& x) const;

    bool operator<(CIr77ZPOSITIVE<uint16_t> const& x) const;

    bool operator>(CIr77ZPOSITIVE<uint16_t> const& x) const;

    bool operator<=(CIr77ZPOSITIVE<uint16_t> const& x) const;

    bool operator>=(CIr77ZPOSITIVE<uint16_t> const& x) const;

    std::strong_ordering operator<=>(CIr77ZPOSITIVE<uint16_t> const& x) const;

    void operator+=(CIr77ZPOSITIVE<uint16_t> const& x);

    void operator-=(CIr77ZPOSITIVE<uint16_t> const& x);

    void operator*=(CIr77ZPOSITIVE<uint16_t> const& x);

   private:
    void operator/=(CIr77ZPOSITIVE<uint16_t> const&) {}

    void operator%=(CIr77ZPOSITIVE<uint16_t> const&) {}

   public:
    void operator&=(CIr77ZPOSITIVE<uint16_t> const& x);

    void operator|=(CIr77ZPOSITIVE<uint16_t> const& x);

    void operator^=(CIr77ZPOSITIVE<uint16_t> const& x);

    void operator>>=(CIr77ZPOSITIVE<uint16_t> const& x);

    void operator<<=(CIr77ZPOSITIVE<uint16_t> const& x);

   private:
    uint16_t Data{0};
};

typedef CIr77ZPOSITIVE<uint16_t> CIr77INTEGER16;

template <>
class CIr77ZPOSITIVE<uint32_t> {
   public:
    CIr77ZPOSITIVE();

    CIr77ZPOSITIVE(CIr77ZPOSITIVE<uint32_t> const& x);

    CIr77ZPOSITIVE(uint32_t const& x);

   public:
    void operator=(CIr77ZPOSITIVE<uint32_t> const& x);

    void operator=(uint32_t const& x);

    uint32_t operator()() const;

    unsigned char& operator[](size_t const& i);

    CIr77ZPOSITIVE<uint32_t> operator++();

    CIr77ZPOSITIVE<uint32_t> operator++(int);

    CIr77ZPOSITIVE<uint32_t> operator--();

    CIr77ZPOSITIVE<uint32_t> operator--(int);

    CIr77ZPOSITIVE<uint32_t> operator+() const;

   private:
    CIr77ZPOSITIVE<uint32_t> operator-() const {}

   public:
    CIr77ZPOSITIVE<uint32_t> operator+(CIr77ZPOSITIVE<uint32_t> const& x) const;

    CIr77ZPOSITIVE<uint32_t> operator-(CIr77ZPOSITIVE<uint32_t> const& x) const;

    CIr77MANTISSA<double> operator/(CIr77MANTISSA<double> const& x) const;

    CIr77ZPOSITIVE<uint32_t> operator*(CIr77ZPOSITIVE<uint32_t> const& x) const;

    size_t operator%(CIr77ZPOSITIVE<uint32_t> const& x) const;

    CIr77ZPOSITIVE<uint32_t> operator~() const;

    CIr77ZPOSITIVE<uint32_t> operator&(CIr77ZPOSITIVE<uint32_t> const& x) const;

    CIr77ZPOSITIVE<uint32_t> operator|(CIr77ZPOSITIVE<uint32_t> const x) const;

    CIr77ZPOSITIVE<uint32_t> operator^(CIr77ZPOSITIVE<uint32_t> const x) const;

    CIr77ZPOSITIVE<uint32_t> operator>>(CIr77ZPOSITIVE<uint32_t> const x) const;

    CIr77ZPOSITIVE<uint32_t> operator<<(CIr77ZPOSITIVE<uint32_t> const x) const;

    bool operator!() const;

    bool operator&&(CIr77ZPOSITIVE<uint32_t> const& x) const;

    bool operator||(CIr77ZPOSITIVE<uint32_t> const& x) const;

    bool operator==(CIr77ZPOSITIVE<uint32_t> const& x) const;

    bool operator!=(CIr77ZPOSITIVE<uint32_t> const& x) const;

    bool operator<(CIr77ZPOSITIVE<uint32_t> const& x) const;

    bool operator>(CIr77ZPOSITIVE<uint32_t> const& x) const;

    bool operator<=(CIr77ZPOSITIVE<uint32_t> const& x) const;

    bool operator>=(CIr77ZPOSITIVE<uint32_t> const& x) const;

    std::strong_ordering operator<=>(CIr77ZPOSITIVE<uint32_t> const& x) const;

    void operator+=(CIr77ZPOSITIVE<uint32_t> const& x);

    void operator-=(CIr77ZPOSITIVE<uint32_t> const& x);

    void operator*=(CIr77ZPOSITIVE<uint32_t> const& x);

   private:
    void operator/=(CIr77ZPOSITIVE<uint32_t> const&) {}

    void operator%=(CIr77ZPOSITIVE<uint32_t> const&) {}

   public:
    void operator&=(CIr77ZPOSITIVE<uint32_t> const& x);

    void operator|=(CIr77ZPOSITIVE<uint32_t> const& x);

    void operator^=(CIr77ZPOSITIVE<uint32_t> const& x);

    void operator>>=(CIr77ZPOSITIVE<uint32_t> const& x);

    void operator<<=(CIr77ZPOSITIVE<uint32_t> const& x);

   private:
    uint32_t Data{0};
};

typedef CIr77ZPOSITIVE<uint32_t> CIr77INTEGER32;

template <>
class CIr77ZPOSITIVE<uint64_t> {
   public:
    CIr77ZPOSITIVE();

    CIr77ZPOSITIVE(CIr77ZPOSITIVE<uint64_t> const& x);

    CIr77ZPOSITIVE(uint64_t const& x);

   public:
    void operator=(CIr77ZPOSITIVE<uint64_t> const& x);

    void operator=(uint64_t const& x);

    uint64_t operator()() const;

    unsigned char& operator[](size_t const& i);

    CIr77ZPOSITIVE<uint64_t> operator++();

    CIr77ZPOSITIVE<uint64_t> operator++(int);

    CIr77ZPOSITIVE<uint64_t> operator--();

    CIr77ZPOSITIVE<uint64_t> operator--(int);

    CIr77ZPOSITIVE<uint64_t> operator+() const;

   private:
    CIr77ZPOSITIVE<uint64_t> operator-() const {}

   public:
    CIr77ZPOSITIVE<uint64_t> operator+(CIr77ZPOSITIVE<uint64_t> const& x) const;

    CIr77ZPOSITIVE<uint64_t> operator-(CIr77ZPOSITIVE<uint64_t> const& x) const;

    CIr77MANTISSA<double> operator/(CIr77MANTISSA<double> const& x) const;

    CIr77ZPOSITIVE<uint64_t> operator*(CIr77ZPOSITIVE<uint64_t> const& x) const;

    size_t operator%(CIr77ZPOSITIVE<uint64_t> const& x) const;

    CIr77ZPOSITIVE<uint64_t> operator~() const;

    CIr77ZPOSITIVE<uint64_t> operator&(CIr77ZPOSITIVE<uint64_t> const& x) const;

    CIr77ZPOSITIVE<uint64_t> operator|(CIr77ZPOSITIVE<uint64_t> const x) const;

    CIr77ZPOSITIVE<uint64_t> operator^(CIr77ZPOSITIVE<uint64_t> const x) const;

    CIr77ZPOSITIVE<uint64_t> operator>>(CIr77ZPOSITIVE<uint64_t> const x) const;

    CIr77ZPOSITIVE<uint64_t> operator<<(CIr77ZPOSITIVE<uint64_t> const x) const;

    bool operator!() const;

    bool operator&&(CIr77ZPOSITIVE<uint64_t> const& x) const;

    bool operator||(CIr77ZPOSITIVE<uint64_t> const& x) const;

    bool operator==(CIr77ZPOSITIVE<uint64_t> const& x) const;

    bool operator!=(CIr77ZPOSITIVE<uint64_t> const& x) const;

    bool operator<(CIr77ZPOSITIVE<uint64_t> const& x) const;

    bool operator>(CIr77ZPOSITIVE<uint64_t> const& x) const;

    bool operator<=(CIr77ZPOSITIVE<uint64_t> const& x) const;

    bool operator>=(CIr77ZPOSITIVE<uint64_t> const& x) const;

    std::weak_ordering operator<=>(CIr77ZPOSITIVE<uint64_t> const& x) const;

    void operator+=(CIr77ZPOSITIVE<uint64_t> const& x);

    void operator-=(CIr77ZPOSITIVE<uint64_t> const& x);

    void operator*=(CIr77ZPOSITIVE<uint64_t> const& x);

   private:
    void operator/=(CIr77ZPOSITIVE<uint64_t> const&) {}

    void operator%=(CIr77ZPOSITIVE<uint64_t> const&) {}

   public:
    void operator&=(CIr77ZPOSITIVE<uint64_t> const& x);

    void operator|=(CIr77ZPOSITIVE<uint64_t> const& x);

    void operator^=(CIr77ZPOSITIVE<uint64_t> const& x);

    void operator>>=(CIr77ZPOSITIVE<uint64_t> const& x);

    void operator<<=(CIr77ZPOSITIVE<uint64_t> const& x);

   private:
    uint64_t Data{0};
};

typedef CIr77ZPOSITIVE<uint64_t> CIr77INTEGER64;

template <typename T>
concept ASSERT_CIr77ZPOSITIVE = std::is_same<T, CIr77ZPOSITIVE<uint8_t>>::value || std::is_same<T, CIr77ZPOSITIVE<uint16_t>>::value ||
                                std::is_same<T, CIr77ZPOSITIVE<uint32_t>>::value || std::is_same<T, CIr77ZPOSITIVE<uint64_t>>::value;

}  // namespace CIr77TENSOR