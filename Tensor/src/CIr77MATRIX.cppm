#pragma once

#include <stdexcept>

export module CIr77TENSOR:CIr77MATRIX;

import :CIr77FORWARD;
import :CIr77RESOURCE;

export namespace CIr77TENSOR {
/*
class CIr77FLOAT2X2
{
public:
        CIr77FLOAT2X2() = default;

        CIr77FLOAT2X2(CIr77FLOAT2 rowA, CIr77FLOAT2 rowB)
        {
                Data[0][0] = rowA[0]; Data[0][1] = rowA[1]; Data[1][0] = rowB[0]; Data[1][1] = rowB[1];
        }

        CIr77FLOAT2X2(CIr77FLOAT2X2 const& e) { *this = e; }

public:
        inline void operator =(CIr77FLOAT2X2 const& e) { memcpy(this, &e, sizeof(CIr77FLOAT2X2)); }

        inline bool operator ==(CIr77FLOAT2X2 const& e) const { return memcmp(Data, e.Data, sizeof(CIr77FLOAT2X2)) != 0; }

        inline bool operator !=(CIr77FLOAT2X2 const& e) const { return memcmp(Data, e.Data, sizeof(CIr77FLOAT2X2)) == 0; }

        inline CIr77FLOAT2X2 const operator ()() const { return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");; }

public:
        inline CIr77FLOAT2& operator [](size_t i) { return *(CIr77FLOAT2*)&Data[i][0]; }

        inline CIr77FLOAT2 const ROW1() const { return CIr77FLOAT2{Data[0][0], Data[0][1]}; }

        inline CIr77FLOAT2 const ROW2() const { return CIr77FLOAT2{Data[1][0], Data[1][1]}; }

private:
        CIr77FLOAT Data[2][2];
};

class CIr77FLOAT3X3
{
public:
        CIr77FLOAT3X3() = default;

        CIr77FLOAT3X3(CIr77FLOAT3 rowA, CIr77FLOAT3 rowB, CIr77FLOAT3 rowC)
        {
                Data[0][0] = rowA[0]; Data[0][1] = rowA[1]; Data[0][2] = rowA[2];
                Data[1][0] = rowB[0]; Data[1][1] = rowB[1]; Data[1][2] = rowB[2];
                Data[2][0] = rowC[0]; Data[2][1] = rowC[1]; Data[2][2] = rowC[2];
        }

        CIr77FLOAT3X3(CIr77FLOAT3X3 const& e) { *this = e; }

public:
        inline void operator =(CIr77FLOAT3X3 const& e) { memcpy(this, &e, sizeof(CIr77FLOAT3X3)); }

        inline bool operator ==(CIr77FLOAT3X3 const& e) const { return memcmp(Data, e.Data, sizeof(CIr77FLOAT3X3)) != 0; }

        inline bool operator !=(CIr77FLOAT3X3 const& e) const { return memcmp(Data, e.Data, sizeof(CIr77FLOAT3X3)) == 0; }

        inline CIr77FLOAT3X3 const operator ()() const { return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");; }

public:
        inline CIr77FLOAT3& operator [](size_t const& i) { return *(CIr77FLOAT3*)&Data[i][0]; }

        inline CIr77FLOAT3 const ROW1() const { return CIr77FLOAT3{ Data[0][0], Data[0][1], Data[0][2] }; }

        inline CIr77FLOAT3 const ROW2() const { return CIr77FLOAT3{ Data[1][0], Data[1][1], Data[1][2] }; }

        inline CIr77FLOAT3 const ROW3() const { return CIr77FLOAT3{ Data[2][0], Data[2][1], Data[2][2] }; }

private:
        CIr77FLOAT Data[3][3];
};

class CIr77FLOAT4X4
{
public:
        CIr77FLOAT4X4() = default;

        CIr77FLOAT4X4(CIr77FLOAT4 rowA, CIr77FLOAT4 rowB, CIr77FLOAT4 rowC, CIr77FLOAT4 rowD)
        {
                Data[0][0] = rowA[0]; Data[0][1] = rowA[1]; Data[0][2] = rowA[2]; Data[0][3] = rowA[3];
                Data[1][0] = rowB[0]; Data[1][1] = rowB[1]; Data[1][2] = rowB[2]; Data[1][3] = rowB[3];
                Data[2][0] = rowC[0]; Data[2][1] = rowC[1]; Data[2][2] = rowC[2]; Data[2][3] = rowC[3];
                Data[3][0] = rowD[0]; Data[3][1] = rowD[1]; Data[3][2] = rowD[2]; Data[3][3] = rowD[3];
        }

        CIr77FLOAT4X4(CIr77FLOAT4X4 const& e) { *this = e; }

public:
        inline void operator =(CIr77FLOAT4X4 const& e) { memcpy(this, &e, sizeof(CIr77FLOAT4X4)); }

        inline bool operator ==(CIr77FLOAT4X4 const& e) const { return memcmp(Data, e.Data, sizeof(CIr77FLOAT4X4)) != 0; }

        inline bool operator !=(CIr77FLOAT4X4 const& e) const { return memcmp(Data, e.Data, sizeof(CIr77FLOAT4X4)) == 0; }

        inline CIr77FLOAT4X4 const operator ()() const { return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");; }

public:
        inline CIr77FLOAT4& operator [](size_t const& i) { return *(CIr77FLOAT4*)&Data[i][0]; }

        inline CIr77FLOAT4 const ROW1() const { return CIr77FLOAT4{ Data[0][0], Data[0][1], Data[0][2], Data[0][3] }; }

        inline CIr77FLOAT4 const ROW2() const { return CIr77FLOAT4{ Data[1][0], Data[1][1], Data[1][2], Data[1][3] }; }

        inline CIr77FLOAT4 const ROW3() const { return CIr77FLOAT4{ Data[2][0], Data[2][1], Data[2][2], Data[2][3] }; }

        inline CIr77FLOAT4 const ROW4() const { return CIr77FLOAT4{ Data[3][0], Data[3][1], Data[3][2], Data[3][3] }; }

private:
        CIr77FLOAT Data[4][4];
};

const CIr77FLOAT4X4 Ir77IDENTITY{
          CIr77FLOAT4{ 1.0f, 0.0f, 0.0f, 0.0f },
          CIr77FLOAT4{ 0.0f, 1.0f, 0.0f, 0.0f },
          CIr77FLOAT4{ 0.0f, 0.0f, 1.0f, 0.0f },
          CIr77FLOAT4{ 0.0f, 0.0f, 0.0f, 1.0f } };
          */
}
