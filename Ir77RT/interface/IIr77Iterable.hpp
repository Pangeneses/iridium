#pragma once

#include <memory>

#include "IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef enum Ir77TypeEnum {
    Ir77Type_Empty = 0,
    Ir77Type_UInt8 = 1,
    Ir77Type_Int16 = 2,
    Ir77Type_UInt16 = 3,
    Ir77Type_Int32 = 4,
    Ir77Type_UInt32 = 5,
    Ir77Type_Int64 = 6,
    Ir77Type_UInt64 = 7,
    Ir77Type_Single = 8,
    Ir77Type_Double = 9,
    Ir77Type_Char32 = 10,
    Ir77Type_Boolean = 11,
    Ir77Type_String = 12,
    Ir77Type_Enlisted = 13,
    Ir77Type_DateTime = 14,
    Ir77Type_TimeSpan = 15,
    Ir77Type_GUID = 16,
    Ir77Type_Point = 17,
    Ir77Type_Size = 18,
    Ir77Type_Rect = 19,
    Ir77Type_Other = 20,
    Ir77Type_UInt8Array = 101,
    Ir77Type_Int16Array = 102,
    Ir77Type_UInt16Array = 103,
    Ir77Type_Int32Array = 104,
    Ir77Type_UInt32Array = 105,
    Ir77Type_Int64Array = 106,
    Ir77Type_UInt64Array = 107,
    Ir77Type_SingleArray = 108,
    Ir77Type_DoubleArray = 109,
    Ir77Type_Char32Array = 110,
    Ir77Type_BooleanArray = 111,
    Ir77Type_StringArray = 112,
    Ir77Type_MPVMArray = 113,
    Ir77Type_DateTimeArray = 114,
    Ir77Type_TimeSpanArray = 115,
    Ir77Type_GUIDArray = 116,
    Ir77Type_PointArray = 117,
    Ir77Type_SizeArray = 118,
    Ir77Type_RectArray = 119,
    Ir77Type_OtherArray = 120
} Ir77TypeEnum;

typedef struct IIr77Iterable : virtual public IIr77Enlisted {
    IIr77Iterable() = default;

    virtual Ir77TypeEnum IsType() const = 0;

    virtual bool IsTypeOf(Ir77TypeEnum const& type) = 0;

    virtual bool IsBefore() const = 0;

    virtual bool Begin() = 0;

    virtual bool Next() = 0;

    virtual bool Previous() = 0;

    virtual bool End() = 0;

    virtual bool IsAfter() const = 0;

    virtual std::shared_ptr<IIr77Enlisted const> Current() const = 0;

    virtual std::shared_ptr<IIr77Return const> SetCurrent(std::shared_ptr<IIr77Enlisted const>& current) = 0;

    virtual ~IIr77Iterable() = default;

}* PIr77Iterable;
}  // namespace NSIr77RT