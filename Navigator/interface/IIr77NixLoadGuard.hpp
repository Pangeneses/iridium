#pragma once

#include <cstdint>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77NixLoadGuard : virtual public IIr77Enlisted {
    IIr77NixLoadGuard() = default;

    virtual ~IIr77NixLoadGuard() = default;

}* PIr77NixLoadGuard;
}  // namespace NSIr77RT