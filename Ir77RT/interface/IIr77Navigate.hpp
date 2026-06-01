#pragma once

#include <functional>
#include <memory>

#include "IIr77GUID.hpp"

#include "IIr77Enlisted.hpp"

namespace NSIr77RT {
struct IIr77Return;

inline std::function<std::shared_ptr<IIr77Return const>(std::shared_ptr<IIr77GUID const>& appID)> IIr77Navigate;

}  // namespace NSIr77RT