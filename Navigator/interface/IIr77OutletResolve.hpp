#pragma once

#include <cstdint>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77OutletResolve : virtual public IIr77Enlisted {
    IIr77OutletResolve() = default;

    virtual ~IIr77OutletResolve() = default;

}* PIr77OutletResolve;
}  // namespace NSIr77RT