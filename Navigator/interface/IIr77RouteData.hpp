#pragma once

#include <cstdint>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77RouteData : virtual public IIr77Enlisted {
    IIr77RouteData() = default;

    virtual ~IIr77RouteData() = default;

}* PIr77RouteData;
}  // namespace NSIr77RT