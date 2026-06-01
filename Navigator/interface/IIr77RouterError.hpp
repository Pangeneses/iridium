#pragma once

#include <cstdint>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77RouterError : virtual public IIr77Enlisted {
    IIr77RouterError() = default;

    virtual ~IIr77RouterError() = default;

}* PIr77RouterError;
}  // namespace NSIr77RT