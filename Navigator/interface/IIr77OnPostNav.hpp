#pragma once

#include <cstdint>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77OnPostNav : virtual public IIr77Enlisted {
    IIr77OnPostNav() = default;

    virtual ~IIr77OnPostNav() = default;

}* PIr77OnPostNav;
}  // namespace NSIr77RT