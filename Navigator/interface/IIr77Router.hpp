#pragma once

#include <cstdint>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77Router : virtual public IIr77Enlisted {
    IIr77Router() = default;

    virtual ~IIr77Router() = default;

}* PIr77Router;
}  // namespace NSIr77RT