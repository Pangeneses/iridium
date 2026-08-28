#pragma once

#include <cstdint>
#include <memory>

#include "IIr77Enlisted.hpp"
#include "IIr77GUID.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77Operand : virtual public IIr77Enlisted {
    IIr77Operand() = default;

    virtual std::shared_ptr<IIr77Return const> IsEmpty() const = 0;

    virtual std::shared_ptr<IIr77Return const> SealOperand() = 0;

    virtual std::shared_ptr<IIr77Return const> IsSealed() const = 0;

    virtual std::shared_ptr<IIr77Return const> SetIndexed(std::uint64_t const& at, std::shared_ptr<IIr77Enlisted const>& obj) = 0;

    virtual std::shared_ptr<IIr77Return const> GetIndexed(std::uint64_t const& at, std::shared_ptr<IIr77Enlisted const>& obj) const = 0;

    virtual ~IIr77Operand() = default;
}* PIr77Operand;
}  // namespace NSIr77RT