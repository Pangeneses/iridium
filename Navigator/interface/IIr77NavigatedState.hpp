#pragma once

#include <cstdint>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77NavigatedState : virtual public IIr77Enlisted {
    IIr77NavigatedState() = default;

    virtual ~IIr77NavigatedState() = default;

}* PIr77NavigatedState;
}  // namespace NSIr77RT