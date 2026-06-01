#pragma once

#include <cstdint>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77NavEvent : virtual public IIr77Enlisted {
    IIr77NavEvent() = default;

    virtual ~IIr77NavEvent() = default;

}* PIr77NavEvent;
}  // namespace NSIr77RT