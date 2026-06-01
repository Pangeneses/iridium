#pragma once

#include <cstdint>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77NavActive : virtual public IIr77Enlisted {
    IIr77NavActive() = default;

    virtual ~IIr77NavActive() = default;

}* PIr77NavActive;
}  // namespace NSIr77RT