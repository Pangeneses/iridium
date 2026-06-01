#pragma once

#include <capnp/ez-rpc.h>

#include <memory>

#include "IIr77Enlisted.hpp"
#include "IIr77Stack.hpp"
#include "IIr77Operand.hpp"

namespace NSIr77RT {
struct IIr77REDOS;
struct IIr77Return;

struct Ir77Outgoing {
    uint64_t id;
    std::string restful;
    uint64_t schema;
    kj::Array<kj::byte> blob;
    std::shared_ptr<IIr77Feedback> feedback;
    std::shared_ptr<IIr77Return const> ret;
};

typedef struct IIr77Feedback : virtual public IIr77Enlisted {
    IIr77Feedback() = default;

    virtual std::shared_ptr<IIr77Return const> SetState(OperandState const& state) = 0;

    virtual std::shared_ptr<IIr77Return const> GetState(OperandState& state) = 0;

    virtual std::shared_ptr<IIr77Return const> Gate(std::shared_ptr<IIr77Operand const> op) = 0;

    virtual std::shared_ptr<IIr77Return const> Complete(Ir77Outgoing&& completion) = 0;

    virtual ~IIr77Feedback() = default;
}* PIr77Feedback;
}  // namespace NSIr77RT