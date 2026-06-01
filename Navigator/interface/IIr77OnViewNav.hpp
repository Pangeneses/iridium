#pragma once

#include <cstdint>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77OnViewNav : virtual public IIr77Enlisted {
    IIr77OnViewNav() = default;

    virtual ~IIr77OnViewNav() = default;

}* PIr77OnViewNav;
}  // namespace NSIr77RT