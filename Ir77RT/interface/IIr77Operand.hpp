#pragma once

#include <cstdint>
#include <memory>

#include "IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Feedback;
struct IIr77Return;

typedef struct IIr77Operand : virtual public IIr77Enlisted {
    IIr77Operand() = default;

    virtual std::shared_ptr<IIr77Return const> SetInstanceID(std::uint64_t const& id) = 0;

    virtual std::shared_ptr<IIr77Return const> SetFeedbackInterface(std::shared_ptr<IIr77Feedback const>& feedback) = 0;

    virtual std::shared_ptr<IIr77Return const> Resize(std::uint32_t const& sz) = 0;

    virtual std::shared_ptr<IIr77Return const> Size(std::uint32_t& sz) const = 0;

    virtual std::shared_ptr<IIr77Return const> IsEmpty() const = 0;

    virtual std::shared_ptr<IIr77Return const> SealOperand() = 0;

    virtual std::shared_ptr<IIr77Return const> IsSealed() const = 0;

    virtual std::shared_ptr<IIr77Return const> SetOpcode(std::shared_ptr<IIr77GUID const> obj) = 0;

    virtual std::shared_ptr<IIr77Return const> GetOpcode(std::shared_ptr<IIr77GUID const> obj) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetIndexed(std::uint32_t const& at, std::shared_ptr<IIr77Enlisted const> obj) = 0;

    virtual std::shared_ptr<IIr77Return const> GetIndexed(std::uint32_t const& at, std::shared_ptr<IIr77Enlisted const>& obj) const = 0;

    virtual ~IIr77Operand() = default;
}* PIr77Operand;
}  // namespace NSIr77RT