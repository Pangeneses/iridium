#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <map>
#include <memory>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

struct Ir77PeregrineV;
struct IIr77PVDevice;

typedef struct IIr77PVPaint : virtual public IIr77Enlisted {
    IIr77PVPaint() = default;

    virtual std::shared_ptr<IIr77Return const> SetPeregrineV(std::shared_ptr<Ir77PeregrineV>& context) = 0;

    virtual std::shared_ptr<IIr77Return const> CreateCommandBuffers() = 0;

    virtual std::shared_ptr<IIr77Return const> Next() = 0;

    virtual std::shared_ptr<IIr77Return const> ResetSwapchain(std::uint32_t const& index) = 0;

    virtual std::shared_ptr<IIr77Return const> Draw() = 0;

    virtual ~IIr77PVPaint() = default;
}* pIIr77PVPaint;

}  // namespace NSIr77PeregrineV