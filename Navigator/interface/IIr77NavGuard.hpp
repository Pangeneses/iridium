#pragma once

#include <cstdint>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77NavGuard : virtual public IIr77Enlisted {
    IIr77NavGuard() = default;

    virtual ~IIr77NavGuard() = default;

}* PIr77NavGuard;
}  // namespace NSIr77RT