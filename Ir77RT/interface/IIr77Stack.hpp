#pragma once

#include <memory>

#include "IIr77Enlisted.hpp"
#include "IIr77Patch.hpp"

namespace NSIr77RT {

struct IIr77Operand;
struct IIr77Iterator;
struct IIr77Return;

enum class IsLocking { WriteAlso, ReadOnly, Locked };

enum class OperandState { Run, Wait, Parked, Retired };

typedef struct Ir77Operation {
    Ir77Operator op;
    std::shared_ptr<IIr77Operand const> lhs;
    std::shared_ptr<IIr77Operand const> rhs;
    std::shared_ptr<IIr77Return const> ret;
} Ir77Operation;

typedef struct IIr77Stack : virtual public IIr77Enlisted {
    IIr77Stack() = default;

    virtual std::shared_ptr<IIr77Return const> IsEmpty() const = 0;

    virtual std::shared_ptr<IIr77Return const> PushOperation(Ir77Operation operation) = 0;

    virtual std::shared_ptr<IIr77Return const> SealStack() = 0;

    virtual std::shared_ptr<IIr77Return const> IsSealed() const = 0;

    virtual std::shared_ptr<IIr77Return const> ForwardIterator(std::shared_ptr<IIr77Iterator const>& itt) = 0;

    virtual std::shared_ptr<IIr77Return const> ReverseIterator(std::shared_ptr<IIr77Iterator const>& itt) = 0;

    virtual std::shared_ptr<IIr77Return const> SetAccess(IsLocking const& access);

    virtual std::shared_ptr<IIr77Return const> GetAccess(IsLocking& access);

    virtual std::shared_ptr<IIr77Return const> InvalidateStack(std::shared_ptr<IIr77Return const>& condition) = 0;

    virtual std::shared_ptr<IIr77Return const> IsInvalid(std::shared_ptr<IIr77Return const>& condition) = 0;

    virtual ~IIr77Stack() = default;

}* PIr77Stack;
}  // namespace NSIr77RT