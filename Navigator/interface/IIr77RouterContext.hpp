#pragma once

#include <cstdint>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77RouterContext : virtual public IIr77Enlisted {
    IIr77RouterContext() = default;

    virtual ~IIr77RouterContext() = default;

}* PIr77RouterContext;
}  // namespace NSIr77RT