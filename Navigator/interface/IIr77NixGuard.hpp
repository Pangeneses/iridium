#pragma once

#include <cstdint>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77NixGuard : virtual public IIr77Enlisted {
    IIr77NixGuard() = default;

    virtual ~IIr77NixGuard() = default;

}* PIr77NixGuard;
}  // namespace NSIr77RT