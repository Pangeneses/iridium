#pragma once

#include <cstdint>
#include <memory>

#include "IIr77GUID.hpp"

#include "IIr77Enlisted.hpp"

namespace NSIr77RT {

struct IIr77Stack;
struct IIr77Return;

struct IIr77REDOS;

const std::uint64_t CREATE_NEW = 0;

typedef struct IIr77Dispatch : virtual public IIr77Enlisted {
    IIr77Dispatch() = default;

    virtual std::shared_ptr<IIr77Return const> Dispatch(std::shared_ptr<IIr77Stack const>& stack) = 0;

    virtual std::shared_ptr<IIr77Return const> Factory(std::shared_ptr<IIr77GUID const>& uid, std::shared_ptr<IIr77Dispatch>& obj, std::uint64_t& id) = 0;

    virtual ~IIr77Dispatch() = default;

}* PIr77Dispatch;
}  // namespace NSIr77RT