#pragma once

#include <cstdint>
#include <memory>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77TBASIC : virtual public IIr77Enlisted {
    IIr77TBASIC() = default;

    virtual std::shared_ptr<IIr77Return const> Resize(std::uint32_t const& sz) = 0;

    virtual ~IIr77TBASIC() = default;

}* PIr77TBASIC;
}  // namespace NSIr77RT