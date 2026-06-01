#pragma once

#include <cstdint>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77OutletRedirect : virtual public IIr77Enlisted {
    IIr77OutletRedirect() = default;

    virtual ~IIr77OutletRedirect() = default;

}* PIr77OutletRedirect;
}  // namespace NSIr77RT