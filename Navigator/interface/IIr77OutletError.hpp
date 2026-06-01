#pragma once

#include <cstdint>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77OutletError : virtual public IIr77Enlisted {
    IIr77OutletError() = default;

    virtual ~IIr77OutletError() = default;

}* PIr77OutletError;
}  // namespace NSIr77RT