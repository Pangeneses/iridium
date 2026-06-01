#pragma once

#include <cstdint>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77LoadView : virtual public IIr77Enlisted {
    IIr77LoadView() = default;

    virtual ~IIr77LoadView() = default;

}* PIr77LoadView;
}  // namespace NSIr77RT