#pragma once

#include <cstdint>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77OnInitNav : virtual public IIr77Enlisted {
    IIr77OnInitNav() = default;

    virtual ~IIr77OnInitNav() = default;

}* PIr77OnInitNav;
}  // namespace NSIr77RT