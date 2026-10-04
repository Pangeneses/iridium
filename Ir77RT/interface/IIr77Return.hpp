#pragma once

#include <memory>

#include "IIr77GUID.hpp"

#include "IIr77Enlisted.hpp"

namespace NSIr77RT {
typedef struct IIr77Return : virtual public IIr77Enlisted {
    IIr77Return() = default;

    virtual Ir77GUID ID() const = 0;

    virtual Ir77GUID GID() const = 0;

    virtual std::shared_ptr<IIr77Return const> InvalidateReturn(std::shared_ptr<IIr77Return const>& condition) = 0;

    virtual std::shared_ptr<IIr77Return const> IsInvalid() const = 0;

    virtual std::shared_ptr<IIr77Return const> IsInvalid(std::shared_ptr<IIr77Return const>& condition) = 0;

    virtual ~IIr77Return() = default;

}* PIr77Return;
}  // namespace NSIr77RT