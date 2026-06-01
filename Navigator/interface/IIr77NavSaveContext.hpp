#pragma once

#include <cstdint>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77NavSaveContext : virtual public IIr77Enlisted {
    IIr77NavSaveContext() = default;

    virtual ~IIr77NavSaveContext() = default;

}* PIr77NavSaveContext;
}  // namespace NSIr77RT