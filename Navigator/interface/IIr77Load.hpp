#pragma once

#include <cstdint>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77Load : virtual public IIr77Enlisted {
    IIr77Load() = default;

    virtual ~IIr77Load() = default;

}* PIr77Load;
}  // namespace NSIr77RT