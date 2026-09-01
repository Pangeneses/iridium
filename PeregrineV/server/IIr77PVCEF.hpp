#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <memory>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

struct Ir77PeregrineV;
struct IIr77PVDevice;
struct Ir77PVBufferCEF;

typedef struct IIr77PVCEF : virtual public IIr77Enlisted {
    IIr77PVCEF() = default;

    virtual std::shared_ptr<IIr77Return const> SetPeregrineV(std::shared_ptr<Ir77PeregrineV>& context) = 0;

    virtual std::shared_ptr<IIr77Return const> CreateLayoutCEF() = 0;

    virtual std::shared_ptr<IIr77Return const> CreateBufferCEF() = 0;

    virtual std::shared_ptr<IIr77Return const> GetBufferCEF(std::uint32_t window_index, std::shared_ptr<Ir77PVBufferCEF>& buffer) = 0;

    virtual ~IIr77PVCEF() = default;
}* pIIr77PVCEF;

}  // namespace NSIr77PeregrineV
