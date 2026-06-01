#pragma once

#include <cstdint>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77BindingContext : virtual public IIr77Enlisted {
    IIr77BindingContext() = default;

    virtual ~IIr77BindingContext() = default;

}* PIr77BindingContext;
}  // namespace NSIr77RT