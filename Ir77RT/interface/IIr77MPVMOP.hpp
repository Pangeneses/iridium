#pragma once

#include "IIr77Enlisted.hpp"

namespace NSIr77RT {

typedef enum class Ir77MPVMOPType : unsigned int {
    Ir77Object,
    Ir77UUID = 1,
    Ir77Boolean = 2,
    Ir77Char = 3,
    Ir77WChar = 4,
    Ir77String = 5,
    Ir77WString = 6,
    Ir77UInt8 = 7,
    Ir77UInt16 = 8,
    Ir77UInt32 = 9,
    Ir77UInt64 = 10,
    Ir77Int16 = 11,
    Ir77Int32 = 12,
    Ir77Int64 = 13,
    Ir77F32 = 14,
    Ir77F64 = 15,
    Ir77F128 = 16,
    Ir77Data = 17,
    Ir77TimePoint = 18,
    Ir77System = 19,
    Ir77Tag = 20,
    Ir77Item = 21,
    Ir77Collect = 22,
    Ir77RetVar = 23,
} Ir77MPVMOPType;

template <typename T>
struct IIr77MPVMOP : public IIr77Enlisted {
    IIr77MPVMOP() = default;

    virtual void Set(T::type in) = 0;

    virtual T::type Get() = 0;

    virtual Ir77MPVMOPType RAWType() = 0;

    virtual ~IIr77MPVMOP() = default;
};

template <typename T>
using PIr77MPVMOP = IIr77MPVMOP<T>*;

}  // namespace NSIr77RT