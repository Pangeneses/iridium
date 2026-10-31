#pragma once

#include "../../Ir77RT/dictionary/IDIr77RET.hpp"

#include "../../Ir77RT/interface/IIr77Return.hpp"

namespace NSIr77RT {
// Ir77Enlisted: keeps the step's result in `result`; true when the step did not succeed
inline bool Failed(std::shared_ptr<IIr77Return const> const& step, std::shared_ptr<IIr77Return const>& result) {
    result = step;
    return step->ID() != GUIDIr77OperationSucceeded;
}

// what an Ir77Json value is
enum class Ir77JsonKind : std::uint8_t { Null, Bool, Number, String, Array, Object, Raw };

}  // namespace NSIr77RT
