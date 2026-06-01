#pragma once

#include <cstdint>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77NavTree : virtual public IIr77Enlisted {
    IIr77NavTree() = default;

    virtual ~IIr77NavTree() = default;

}* PIr77NavTree;
}  // namespace NSIr77RT