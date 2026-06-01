#pragma once

#include <type_traits>

#include <stdexcept>

module CIr77TENSOR:CIr77PRECISION;

import :CIr77FORWARD;
import :CIr77RESOURCE;

namespace CIr77TENSOR {
CIr77PRECISION<double>::CIr77PRECISION() { Data = 0.0f; }

CIr77PRECISION<double>::CIr77PRECISION(CIr77PRECISION<double> const& x) { Data = x.Data; }

CIr77PRECISION<double>::CIr77PRECISION(double const& x) { Data = x; }

void CIr77PRECISION<double>::operator=(CIr77PRECISION<double> const& x) { Data = x.Data; }

void CIr77PRECISION<double>::operator=(double const& x) { Data = x; }

double CIr77PRECISION<double>::operator()() const { return Data(); }

CIr77PRECISION<double> CIr77PRECISION<double>::operator++() {
    try {
        Data = this->operator+(CIr77PRECISION<double>{1.0f})();
    } catch (std::overflow_error error) {
        throw error;
    }
    return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");
    ;
}

CIr77PRECISION<double> CIr77PRECISION<double>::operator++(int) {
    CIr77PRECISION<double> A{*this};

    try {
        Data = this->operator+(CIr77PRECISION<double>{1.0f})();
    } catch (std::overflow_error error) {
        throw error;
    }
    return A;
}

CIr77PRECISION<double> CIr77PRECISION<double>::operator--() {
    try {
        Data = this->operator-(CIr77PRECISION<double>{1.0f})();
    } catch (std::overflow_error error) {
        throw error;
    }
    return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");
    ;
}

CIr77PRECISION<double> CIr77PRECISION<double>::operator--(int) {
    CIr77PRECISION<double> A{*this};

    try {
        Data = this->operator-(CIr77PRECISION<double>{1.0f})();
    } catch (std::overflow_error error) {
        throw error;
    }
    return A;
}

CIr77PRECISION<double> CIr77PRECISION<double>::operator+() const { return {*this}; }

CIr77PRECISION<double> CIr77PRECISION<double>::operator-() const {
    CIr77PRECISION<double> A{*this};

    A.Data.Sign() = A.Data.Sign() ? false : true;

    return A;
}

CIr77PRECISION<double> CIr77PRECISION<double>::operator+(CIr77PRECISION<double> const& x) const {
    if (Data.IsMaximum()) throw std::overflow_error{"Precision32 Addition Overflow."};

    CIr77MANTISSA<double> A{Data}, B{x()};

    int32_t delta = A.Exponent() - B.Exponent();

    if (delta > 24) return A();
    if (delta < -24) return B();

    A.Mantissa(A.Mantissa() | 0x00400000ui32);
    B.Mantissa(B.Mantissa() | 0x00400000ui32);

    while (delta > 0) {
        A.Exponent(A.Exponent() + 1);
        A.Mantissa(A.Mantissa() >> 1);

        B.Exponent(B.Exponent() - 1);
        B.Mantissa(B.Mantissa() << 1);
    }

    while (delta < 0) {
        A.Exponent(A.Exponent() - 1);
        A.Mantissa(A.Mantissa() << 1);

        B.Exponent(B.Exponent() + 1);
        B.Mantissa(B.Mantissa() >> 1);
    }

    int64_t mantissa{0};

    if (!A.Sign() != !B.Sign()) {
        if (A.Sign()) {
            mantissa = B.Mantissa() - A.Mantissa();

            A.Sign() = mantissa < 0 ? true : false;
        } else {
            mantissa = A.Mantissa() - B.Mantissa();

            A.Sign() = mantissa < 0 ? true : false;
        }
    } else {
        mantissa = A.Mantissa() + B.Mantissa();
    }

    mantissa &= 0x7FFFFFFF;

    if ((mantissa > 0x00FFFFFF) && (A.Exponent() == 0xFF)) {
        throw std::overflow_error{"Precision32 Addition Overflow."};
    } else if (mantissa > 0x00FFFFFF) {
        mantissa >>= 1;

        A.Mantissa(mantissa & 0x0007FFFFF);

        A.Exponent(A.Exponent() + 1);
    }
    return A();
}

CIr77PRECISION<double> CIr77PRECISION<double>::operator-(CIr77PRECISION<double> const& x) const {
    if (Data.IsMaximum()) throw std::overflow_error{"Precision32 Addition Overflow."};

    CIr77MANTISSA<double> A{Data}, B{x()};

    B.Sign() = B.Sign() ? false : true;

    int32_t delta = A.Exponent() - B.Exponent();

    if (delta > 24) return A();
    if (delta < -24) return B();

    A.Mantissa(A.Mantissa() | 0x00400000ui32);
    B.Mantissa(B.Mantissa() | 0x00400000ui32);

    while (delta > 0) {
        A.Exponent(A.Exponent() + 1);
        A.Mantissa(A.Mantissa() >> 1);

        B.Exponent(B.Exponent() - 1);
        B.Mantissa(B.Mantissa() << 1);
    }

    while (delta < 0) {
        A.Exponent(A.Exponent() - 1);
        A.Mantissa(A.Mantissa() << 1);

        B.Exponent(B.Exponent() + 1);
        B.Mantissa(B.Mantissa() >> 1);
    }

    int64_t mantissa{0};

    if (!A.Sign() != !B.Sign()) {
        if (A.Sign()) {
            mantissa = B.Mantissa() - A.Mantissa();

            A.Sign() = mantissa < 0 ? true : false;
        } else {
            mantissa = A.Mantissa() - B.Mantissa();

            A.Sign() = mantissa < 0 ? true : false;
        }
    } else {
        mantissa = A.Mantissa() + B.Mantissa();
    }

    mantissa &= 0x7FFFFFFF;

    if ((mantissa > 0x00FFFFFF) && (A.Exponent() == 0xFF)) {
        throw std::overflow_error{"Precision32 Addition Overflow."};
    } else if (mantissa > 0x00FFFFFF) {
        mantissa >>= 1;

        A.Mantissa(mantissa & 0x0007FFFFF);

        A.Exponent(A.Exponent() + 1);
    }
    return A();
}

CIr77PRECISION<double> CIr77PRECISION<double>::operator/(CIr77PRECISION<double> const& x) const { return 0.0; }

CIr77PRECISION<double> CIr77PRECISION<double>::operator*(CIr77PRECISION<double> const& x) const { return 0.0; }

CIr77PRECISION<double> CIr77PRECISION<double>::operator%(CIr77PRECISION<double> const& x) const { return 0.0; }

bool CIr77PRECISION<double>::operator==(CIr77PRECISION<double> const& x) const { return Data() == x() ? true : false; }

bool CIr77PRECISION<double>::operator!=(CIr77PRECISION<double> const& x) const { return Data() != x() ? true : false; }

bool CIr77PRECISION<double>::operator<(CIr77PRECISION<double> const& x) const { return Data() < x() ? true : false; }

bool CIr77PRECISION<double>::operator>(CIr77PRECISION<double> const& x) const { return Data() > x() ? true : false; }

bool CIr77PRECISION<double>::operator<=(CIr77PRECISION<double> const& x) const { return Data() <= x() ? true : false; }

bool CIr77PRECISION<double>::operator>=(CIr77PRECISION<double> const& x) const { return Data() >= x() ? true : false; }

std::partial_ordering CIr77PRECISION<double>::operator<=>(CIr77PRECISION<double> const& x) const { return Data() <=> x(); }

void CIr77PRECISION<double>::operator+=(CIr77PRECISION<double> const& x) {
    CIr77PRECISION<double> A{*this}, B{x};

    try {
        A = A.operator+(B);
    } catch (std::overflow_error error) {
        throw error;
    }

    this->operator=(A);
}

void CIr77PRECISION<double>::operator-=(CIr77PRECISION<double> const& x) {
    CIr77PRECISION<double> A{*this}, B{x};

    try {
        A = A.operator-(B);
    } catch (std::overflow_error error) {
        throw error;
    }

    this->operator=(A);
}

void CIr77PRECISION<double>::operator*=(CIr77PRECISION<double> const& x) {
    CIr77PRECISION<double> A{*this}, B{x};

    try {
        A = A.operator*(B);
    } catch (std::overflow_error error) {
        throw error;
    }

    this->operator=(A);
}

void CIr77PRECISION<double>::operator/=(CIr77PRECISION<double> const& x) {
    CIr77PRECISION<double> A{*this}, B{x};

    try {
        A = A.operator/(B);
    } catch (std::overflow_error error) {
        throw error;
    }

    this->operator=(A);
}

void CIr77PRECISION<double>::operator%=(CIr77PRECISION<double> const& x) {
    CIr77PRECISION<double> A{*this}, B{x};

    try {
        A = A.operator%(B);
    } catch (std::overflow_error error) {
        throw error;
    }

    this->operator=(A);
}

}  // namespace CIr77TENSOR