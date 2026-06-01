#pragma once

#include <vector>

#include <compare>

#include <optional>

#include <stdexcept>

export module CIr77TENSOR:CIr77ARITHMETIC;

import :CIr77FORWARD;
import :CIr77RESOURCE;

import :CIr77MANTISSA;
import :CIr77ZNEGATIVE;
import :CIr77ZNONPOS;
import :CIr77ZINTEGER;
import :CIr77ZNONNEG;
import :CIr77ZPOSITIVE;
import :CIr77PRECISION;

export namespace CIr77TENSOR {
template <ASSERT_CIr77ZPOSITIVE T = CIr77ZPOSITIVE<uint64_t>>
T ZPositivePower(T const& base, T const& pow) {
    T A{base}, B{pow}, Q{1ui8};

    while (B != 0ui8) {
        if ((B & 1ui8) == 1ui8) {
            try {
                Q *= A;
            } catch (std::overflow_error error) {
                throw error;
            }
        }

        B >>= 1ui8;

        if (B != 0ui8) {
            try {
                A *= A;
            } catch (std::overflow_error error) {
                throw error;
            }
        }
    }
    return Q;
}

template <ASSERT_CIr77ZPOSITIVE T = CIr77ZPOSITIVE<uint64_t>>
std::optional<T> ZPositiveCyclic(T const& z, T const& n) {
    if (z < n) return {};

    T A{z}, B{n}, Q{0};

    size_t bitWidth{63i8};

    while ((B & 0x3000000000000000ui64) != 0x3000000000000000ui64) {
        B <<= 1i8;

        --bitWidth;
    }

    B <<= 63i8 - bitWidth;

    for (size_t i = 63i8; i > bitWidth; --i) {
        if (A > B) {
            A -= B;

            Q <<= 1ui8;

            Q |= 1ui8;
        }

        B >>= 1i8;
    }

    if (Q != 0 && A == 0)
        return Q;

    else
        return {};
};
/*
template<ASSERT_CIr77ZPOSITIVE T = CIr77ZPOSITIVE<uint64_t>>
std::optional<T> ZPositiveWholeRoot(T const& b, T const& n)
{
        if (n == 1ui64) return b;

        CIr77ZPOSITIVE<uint64_t> N{ n }, B{ b };

        CIr77PRECISION <double> P{ 1.0 }, Q{ 1.0 }, D{ 0.0 };

        if (b <= 0xFFFFFFFF) Q = 0xFFFF;

        else if (b <= 0xFFFFFFFFFFFF) Q = 0xFFFFFF;

        else if (b <= 0xFFFFFFFFFFFFFFFF) Q = 0xFFFFFFFF;

        while (D < 0.000 && D.Abs() > 0.9999990)
        {
                CIr77ZPOSITIVE<uint64_t> T1{ 0 };

                //P = (1 / N) * ((N - 1) * Q + B / Q ^ (N - 1));

                try
                {
                        T1 = T{ 1 } / N;
                }
                catch (std::overflow_error error)
                {
                        throw error;
                }

                CIr77ZPOSITIVE<uint64_t> T2{ 0 };

                try
                {
                        T2 = (N - T{ 1 }) * Q;
                }
                catch (std::overflow_error error)
                {
                        throw error;
                }

                CIr77ZPOSITIVE<uint64_t> T3{ 0 };

                try
                {
                        T3 = Q ^ (N - T{ 1 });
                }
                catch (std::overflow_error error)
                {
                        throw error;
                }

                CIr77ZPOSITIVE<uint64_t> T4{ 0 };

                try
                {
                        T4 = B / T3;
                }
                catch (std::overflow_error error)
                {
                        throw error;
                }

                D = P - Q;

                D = D.Abs();

                Q = P;
        }

        if ()
}

template<ASSERT_CIr77ZINTEGER T = CIr77ZINTEGER<size_t>>
void AsFraction(CIr77PRECISION < double > const& precision, T& a, T& b)
{
        CIr77MANTISSA < double > P{ precision };

        CIr77ZINTEGER<size_t> W = P.Floor();

        CIr77MANTISSA < double > D = P() - P.Floor();

        CIr77ZINTEGER<size_t> A{ D.Mantissa() | 0x0008000000000000 }, B{ 0x000FFFFFFFFFFFFF };

        while (B != static_cast<CIr77ZINTEGER<size_t>>(0))
        {
                CIr77ZINTEGER<size_t> Q;

                try
                {
                        Q = A % B;
                }
                catch (std::runtime_error error)
                {
                        throw error;
                }

                A = B;

                B = Q;
        }

        a = (D.Mantissa() | 0x0008000000000000) / A();

        b = 0x000FFFFFFFFFFFFF / A();
}

template<ASSERT_CIr77PRECISION T = CIr77PRECISION<double>>
T PrecisionPower(T const& a, T const& b)
{

}

template<ASSERT_CIr77PRECISION T>
T Ln(T const& x, size_t iterations = 20)
{
}

template<>
CIr77PRECISION <float> Ln <CIr77PRECISION <float>>(CIr77PRECISION <float> const& x, size_t iterations)
{
        if (x <= 0.0f) return -std::numeric_limits<float>::infinity();

        CIr77PRECISION <float> inPlace = x() - 1.0f;

        //std::cout << "var: " << inPlace() << std::endl;

        for (size_t k = 1; k < iterations; k++)
        {
                CIr77ZINTEGER<uint64_t> p2k = PositivePower(CIr77ZINTEGER<size_t>{ 2 }, CIr77ZINTEGER<size_t>{ k });

                CIr77PRECISION <float> root = static_cast<float>(IsCyclic(CIr77ZINTEGER<size_t>{ x() }, CIr77ZINTEGER<size_t>{ p2k })());

                inPlace *= (2.0f / (1.0f + root()));

                //std::cout << "var: " << inPlace() << " n-th root: " << p2k << " root: " << "\n";
        }
        return inPlace;
}

template<>
CIr77PRECISION <double> Ln <CIr77PRECISION <double>>(CIr77PRECISION <double> const& x, size_t iterations)
{
        if (x <= 0.0f) return -std::numeric_limits<double>::infinity();

        CIr77PRECISION <double> inPlace = x() - 1.0f;

        //std::cout << "var: " << inPlace() << std::endl;

        for (size_t k = 1; k < iterations; k++)
        {
                CIr77ZINTEGER<size_t> p2k = PositivePower(CIr77ZINTEGER<size_t>{ 2 }, CIr77ZINTEGER<size_t>{ k });

                CIr77PRECISION <double> root = static_cast<double>(IsCyclic(CIr77ZINTEGER<size_t>{ x() }, CIr77ZINTEGER<size_t>{ p2k })());

                inPlace *= (2.0f / (1.0f + root()));

                //std::cout << "var: " << inPlace() << " n-th root: " << p2k << " root: " << "\n";
        }
        return inPlace;
}

template<ASSERT_CIr77PRECISION T>
T Log(T const& x, size_t iterations = 20)
{
}

template<>
CIr77PRECISION <float> Log <CIr77PRECISION <float>>(CIr77PRECISION <float> const& x, size_t iterations)
{
        if (x <= 0.0f || b <= 0.0f) return -std::numeric_limits<float>::infinity();

        CIr77PRECISION <float> Algebra{ b }, var{ x };

        CIr77PRECISION <float> inPlace = Ln(Algebra, iterations)() / Ln(var, iterations)();

        return inPlace;
}

template<>
CIr77PRECISION <double> Log <CIr77PRECISION <double>>(CIr77PRECISION <double> const& x, size_t iterations)
{
        if (x <= 0.0f || b <= 0.0f) return -std::numeric_limits<double>::infinity();

        CIr77PRECISION <double> Algebra{ b }, var{ x };

        CIr77PRECISION <double> inPlace = Ln(Algebra, iterations)() / Ln(var, iterations)();

        return inPlace;
}

CIr77PRECISION <double> const Gamma(CIr77PRECISION <double> x, size_t iterations = 50)
{
        return x * static_cast<double>(iterations);
}

CIr77PRECISION <double> Exp(CIr77PRECISION <double> x, size_t iterations = 10)
{
        return x * static_cast<double>(iterations);
}
*/
}  // namespace CIr77TENSOR
