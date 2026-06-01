#pragma once

#include <cstdint>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77NavState : virtual public IIr77Enlisted {
    IIr77NavState() = default;

    virtual ~IIr77NavState() = default;

}* PIr77NavState;
}  // namespace NSIr77RT