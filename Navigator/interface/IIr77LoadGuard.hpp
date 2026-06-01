#pragma once

#include <cstdint>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77LoadGuard : virtual public IIr77Enlisted {
    IIr77LoadGuard() = default;

    virtual ~IIr77LoadGuard() = default;

}* PIr77LoadGuard;
}  // namespace NSIr77RT