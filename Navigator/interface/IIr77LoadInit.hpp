#pragma once

#include <cstdint>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77LoadInit : virtual public IIr77Enlisted {
    IIr77LoadInit() = default;

    virtual ~IIr77LoadInit() = default;

}* PIr77LoadInit;
}  // namespace NSIr77RT