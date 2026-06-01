#pragma once

#include <cstdint>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77NavCancel : virtual public IIr77Enlisted {
    IIr77NavCancel() = default;

    virtual ~IIr77NavCancel() = default;

}* PIr77NavCancel;
}  // namespace NSIr77RT