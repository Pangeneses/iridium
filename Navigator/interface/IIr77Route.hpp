#pragma once

#include <cstdint>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77Route : virtual public IIr77Enlisted {
    IIr77Route() = default;

    virtual ~IIr77Route() = default;

}* PIr77Route;
}  // namespace NSIr77RT