#pragma once

#include <functional>
#include <memory>

#include "IIr77Enlisted.hpp"

namespace NSIr77RT {

struct IIr77Operand;
struct IIr77Stack;
struct IIr77Return;

typedef struct Ir77Operator {
    Ir77GUID operand;
    Ir77GUID opcode;
    bool operator<(Ir77Operator const& o) const {
        if (opcode != o.opcode) return opcode < o.opcode;
        return operand < o.operand;
    }
} Ir77Operator;

using Ir7Execute = std::function<std::shared_ptr<IIr77Return const>(std::shared_ptr<IIr77Operand const>&, std::shared_ptr<IIr77Operand const>&)>;

typedef struct IIr77Patch : virtual public IIr77Enlisted {
    IIr77Patch() = default;

    virtual std::shared_ptr<IIr77Return const> AddOperation(Ir77Operator op, Ir7Execute execute) = 0;

    virtual std::shared_ptr<IIr77Return const> Forward(std::shared_ptr<IIr77Stack const>& stack) = 0;

    virtual ~IIr77Patch() = default;

}* PIr77Patch;
}  // namespace NSIr77RT