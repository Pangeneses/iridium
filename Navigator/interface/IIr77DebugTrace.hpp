#pragma once

#include <cstdint>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77DebugTrace : virtual public IIr77Enlisted {
    IIr77DebugTrace() = default;

    virtual ~IIr77DebugTrace() = default;

}* PIr77DebugTrace;
}  // namespace NSIr77RT