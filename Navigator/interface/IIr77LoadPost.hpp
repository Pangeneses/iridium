#pragma once

#include <cstdint>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77LoadPost : virtual public IIr77Enlisted {
    IIr77LoadPost() = default;

    virtual ~IIr77LoadPost() = default;

}* PIr77LoadPost;
}  // namespace NSIr77RT