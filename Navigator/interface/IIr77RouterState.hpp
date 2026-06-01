#pragma once

#include <cstdint>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77RouterState : virtual public IIr77Enlisted {
    IIr77RouterState() = default;

    virtual ~IIr77RouterState() = default;

}* PIr77RouterState;
}  // namespace NSIr77RT