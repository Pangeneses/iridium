#pragma once

#include <cstdint>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77OutletContext : virtual public IIr77Enlisted {
    IIr77OutletContext() = default;

    virtual ~IIr77OutletContext() = default;

}* PIr77OutletContext;
}  // namespace NSIr77RT