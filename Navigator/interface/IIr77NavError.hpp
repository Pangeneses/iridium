#pragma once

#include <cstdint>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77NavError : virtual public IIr77Enlisted {
    IIr77NavError() = default;

    virtual ~IIr77NavError() = default;

}* PIr77NavError;
}  // namespace NSIr77RT