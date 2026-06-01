#pragma once

#include <vector>

#include <stdexcept>

export module CIr77TENSOR:CIr77FIXEDBYTES;

import :CIr77FORWARD;
import :CIr77RESOURCE;

export namespace CIr77TENSOR {
template <size_t n>
class CIr77FIXEDBYTES {
   public:
    CIr77FIXEDBYTES() {}

    CIr77FIXEDBYTES(char const (&x)[n]) {
        size_t sz = std::size(x);

        if (sz != n) throw std::runtime_error{"Array overrun."};

        std::memcpy((void*)Data, (const void*)x, n);
    }

    CIr77FIXEDBYTES(int8_t const& x) {
        if (n != 1) throw std::runtime_error{"Array overrun."};
        std::memcpy((void*)Data, (const void*)&x, n);
    }
    CIr77FIXEDBYTES(int16_t const& x) {
        if (n != 2) throw std::runtime_error{"Array overrun."};
        std::memcpy((void*)Data, (const void*)&x, n);
    }
    CIr77FIXEDBYTES(int32_t const& x) {
        if (n != 4) throw std::runtime_error{"Array overrun."};
        std::memcpy((void*)Data, (const void*)&x, n);
    }
    CIr77FIXEDBYTES(int64_t const& x) {
        if (n != 8) throw std::runtime_error{"Array overrun."};
        std::memcpy((void*)Data, (const void*)&x, n);
    }

    CIr77FIXEDBYTES(uint8_t const& x) {
        if (n != 1) throw std::runtime_error{"Array overrun."};
        std::memcpy((void*)Data, (const void*)&x, n);
    }
    CIr77FIXEDBYTES(uint16_t const& x) {
        if (n != 2) throw std::runtime_error{"Array overrun."};
        std::memcpy((void*)Data, (const void*)&x, n);
    }
    CIr77FIXEDBYTES(uint32_t const& x) {
        if (n != 4) throw std::runtime_error{"Array overrun."};
        std::memcpy((void*)Data, (const void*)&x, n);
    }
    CIr77FIXEDBYTES(uint64_t const& x) {
        if (n != 8) throw std::runtime_error{"Array overrun."};
        std::memcpy((void*)Data, (const void*)&x, n);
    }

    CIr77FIXEDBYTES(float const& x) {
        if (n != 4) throw std::runtime_error{"Array overrun."};
        std::memcpy((void*)Data, (const void*)&x, n);
    }
    CIr77FIXEDBYTES(double const& x) {
        if (n != 8) throw std::runtime_error{"Array overrun."};
        std::memcpy((void*)Data, (const void*)&x, n);
    }

   public:
    void operator=(char const (&x)[n]) {
        size_t sz = std::size(x);

        if (sz != n) throw std::runtime_error{"Array overrun."};

        std::memcpy((void*)Data, (const void*)x, n);
    }

    void operator=(int8_t const& x) {
        if (n != 1) throw std::runtime_error{"Array overrun."};
        std::memcpy((void*)Data, (const void*)&x, n);
    }
    void operator=(int16_t const& x) {
        if (n != 2) throw std::runtime_error{"Array overrun."};
        std::memcpy((void*)Data, (const void*)&x, n);
    }
    void operator=(int32_t const& x) {
        if (n != 4) throw std::runtime_error{"Array overrun."};
        std::memcpy((void*)Data, (const void*)&x, n);
    }
    void operator=(int64_t const& x) {
        if (n != 8) throw std::runtime_error{"Array overrun."};
        std::memcpy((void*)Data, (const void*)&x, n);
    }

    void operator=(uint8_t const& x) {
        if (n != 1) throw std::runtime_error{"Array overrun."};
        std::memcpy((void*)Data, (const void*)&x, n);
    }
    void operator=(uint16_t const& x) {
        if (n != 2) throw std::runtime_error{"Array overrun."};
        std::memcpy((void*)Data, (const void*)&x, n);
    }
    void operator=(uint32_t const& x) {
        if (n != 4) throw std::runtime_error{"Array overrun."};
        std::memcpy((void*)Data, (const void*)&x, n);
    }
    void operator=(uint64_t const& x) {
        if (n != 8) throw std::runtime_error{"Array overrun."};
        std::memcpy((void*)Data, (const void*)&x, n);
    }

    void operator=(float const& x) {
        if (n != 4) throw std::runtime_error{"Array overrun."};
        std::memcpy((void*)Data, (const void*)&x, n);
    }
    void operator=(double const& x) {
        if (n != 8) throw std::runtime_error{"Array overrun."};
        std::memcpy((void*)Data, (const void*)&x, n);
    }

   public:
    unsigned char* operator()() const { return Data; }

   public:
    unsigned char& operator[](size_t const& i) { return Data[i]; }

   private:
    CIr77FIXEDBYTES operator++() { return CIr77FIXEDBYTES{}; }
    CIr77FIXEDBYTES operator--() { return CIr77FIXEDBYTES{}; }

   private:
    CIr77FIXEDBYTES operator+() { return CIr77FIXEDBYTES{}; }
    CIr77FIXEDBYTES operator-() { return CIr77FIXEDBYTES{}; }

   private:
    CIr77FIXEDBYTES operator+(CIr77FIXEDBYTES const& x) const { return CIr77FIXEDBYTES{}; }
    CIr77FIXEDBYTES operator-(CIr77FIXEDBYTES const& x) const { return CIr77FIXEDBYTES{}; }
    CIr77FIXEDBYTES operator/(CIr77FIXEDBYTES const& x) const { return CIr77FIXEDBYTES{}; }
    CIr77FIXEDBYTES operator*(CIr77FIXEDBYTES const& x) const { return CIr77FIXEDBYTES{}; }
    CIr77FIXEDBYTES operator%(CIr77FIXEDBYTES const& x) const { return CIr77FIXEDBYTES{}; }

   public:
    CIr77FIXEDBYTES<n> operator~() const {
        CIr77FIXEDBYTES<n> A;

        std::memcpy((void*)A.Data, (const void*)Data, n);

        for (size_t i; i < n; ++i) A.Data[i] = ~Data[i];

        return A;
    }
    CIr77FIXEDBYTES<n> operator&(CIr77FIXEDBYTES<n> const& x) const {
        CIr77FIXEDBYTES<n> A{*this}, B{x};

        for (size_t i; i < n; ++i) A.Data[i] = Data[i] & B.Data[i];

        return A;
    }
    CIr77FIXEDBYTES<n> operator|(CIr77FIXEDBYTES<n> const& x) const {
        CIr77FIXEDBYTES<n> A{*this}, B{x};

        for (size_t i; i < n; ++i) A.Data[i] = Data[i] | B.Data[i];

        return A;
    }
    CIr77FIXEDBYTES<n> operator^(CIr77FIXEDBYTES<n> const& x) const {
        CIr77FIXEDBYTES<n> A{*this}, B{x};

        for (size_t i; i < n; ++i) A.Data[i] = Data[i] ^ B.Data[i];

        return A;
    }
    CIr77FIXEDBYTES<n> operator>>(size_t const& shift) const {
        CIr77FIXEDBYTES<n> A{*this};

        unsigned char bit{0};

        for (size_t i; i < shift; ++i) {
            bit = Data[i] & 0b00000001;

            bit = bit << 8;

            if (i + 1 != shift) A.Data[i + 1] = A.Data[i + 1] & 0b10000000;

            A.Data[i] = Data[i] >> shift;
        }

        return A;
    }
    CIr77FIXEDBYTES<n> operator<<(size_t const& shift) const {
        CIr77FIXEDBYTES<n> A{*this};

        unsigned char bit{0};

        for (size_t i = shift; i != 0; --i) {
            bit = Data[i] & 0b100000000;

            bit = bit >> 8;

            if (i - 1 != 0) A.Data[i - 1] = A.Data[i - 1] & 0b00000001;

            A.Data[i] = Data[i] << shift;
        }

        return A;
    }

   private:
    bool operator&&(CIr77FIXEDBYTES<n> const& x) {}
    bool operator||(CIr77FIXEDBYTES<n> const& x) {}

   public:
    bool operator!() const {
        CIr77FIXEDBYTES<n> A;

        std::memcpy((void*)A.Data, (const void*)Data, n);

        for (size_t i = 0; i < n; ++i)
            if (Data[i] != 0) return false;

        return true;
    }
    bool operator==(CIr77FIXEDBYTES<n> const& x) const {
        CIr77FIXEDBYTES<n> A, B{x};

        std::memcpy((void*)A.Data, (const void*)Data, n);

        for (size_t i = 0; i < n; ++i)
            if (A.Data[i] != B.Data[i]) return false;

        return true;
    }
    bool operator!=(CIr77FIXEDBYTES<n> const& x) const {
        CIr77FIXEDBYTES<n> A{x};

        if (this->operator==(A)) return false;

        return true;
    }

   private:
    bool operator<(CIr77FIXEDBYTES<n> const& x) const {}
    bool operator>(CIr77FIXEDBYTES<n> const& x) const {}

   private:
    bool operator<=(CIr77FIXEDBYTES<n> const& x) {}
    bool operator>=(CIr77FIXEDBYTES<n> const& x) {}
    bool operator<=>(CIr77FIXEDBYTES<n> const& x) {}

   private:
    void operator+=(CIr77FIXEDBYTES<n> const& x) {}
    void operator-=(CIr77FIXEDBYTES<n> const& x) {}
    void operator*=(CIr77FIXEDBYTES<n> const& x) {}
    void operator/=(CIr77FIXEDBYTES<n> const& x) {}
    void operator%=(CIr77FIXEDBYTES<n> const& x) {}

   public:
    void operator&=(CIr77FIXEDBYTES<n> const& x) {
        CIr77FIXEDBYTES<n> A{x};

        for (size_t i = 0; i < n; ++i) Data[i] = Data[i] & A.Data[i];
    }
    void operator|=(CIr77FIXEDBYTES<n> const& x) {
        CIr77FIXEDBYTES<n> A{x};

        for (size_t i = 0; i < n; ++i) Data[i] = Data[i] | A.Data[i];
    }
    void operator^=(CIr77FIXEDBYTES<n> const& x) {
        CIr77FIXEDBYTES<n> A{x};

        for (size_t i = 0; i < n; ++i) Data[i] = Data[i] ^ A.Data[i];
    }
    void operator>>=(size_t const& shift) {
        for (size_t i = 0; i < n; ++i) Data[i] = Data[i] >> shift;
    }
    void operator<<=(size_t const& shift) {
        for (size_t i = 0; i < n; ++i) Data[i] = Data[i] << shift;
    }

   public:
    unsigned char Data[n];
};

}  // namespace CIr77TENSOR