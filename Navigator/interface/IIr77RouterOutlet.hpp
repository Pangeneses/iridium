#pragma once

#include <cstdint>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77RouterOutlet : virtual public IIr77Enlisted {
    IIr77RouterOutlet() = default;

    virtual ~IIr77RouterOutlet() = default;

}* PIr77RouterOutlet;
}  // namespace NSIr77RT