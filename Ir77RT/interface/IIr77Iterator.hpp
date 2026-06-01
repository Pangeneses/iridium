#pragma once

#include <cstdint>
#include <memory>

#include "IIr77Enlisted.hpp"
#include "IIr77Patch.hpp"
#include "IIr77Stack.hpp"

namespace NSIr77RT {
struct IIr77Operand;
struct IIr77Return;

typedef struct IIr77Iterator : virtual public IIr77Enlisted {
    IIr77Iterator() = default;

    virtual std::shared_ptr<IIr77Return const> InvalidateIterator(std::shared_ptr<IIr77Return const>& condition) = 0;

    virtual std::shared_ptr<IIr77Return const> IsInvalid(std::shared_ptr<IIr77Return const>& condition) const = 0;

    virtual std::shared_ptr<IIr77Return const> IttBegin() = 0;

    virtual std::shared_ptr<IIr77Return const> IttMove(std::int32_t& mov) = 0;

    virtual std::shared_ptr<IIr77Return const> IttAt(std::uint32_t const& at) = 0;

    virtual std::shared_ptr<IIr77Return const> IttCurrentOperation(Ir77Operation& current) = 0;

    virtual std::shared_ptr<IIr77Return const> IttEnd() = 0;

    virtual std::shared_ptr<IIr77Return const> IttEndOfStack() = 0;

    virtual ~IIr77Iterator() = default;

}* PIr77Iterator;
}  // namespace NSIr77RT