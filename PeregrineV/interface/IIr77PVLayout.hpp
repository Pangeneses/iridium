#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

typedef struct IIr77PVLayout : virtual public IIr77Enlisted {
    IIr77PVLayout() = default;

    virtual std::shared_ptr<IIr77Return const> CreatePipelineLayout() = 0;

    virtual std::shared_ptr<IIr77Return const> GetPipelineLayout(VkPipelineLayout* pipeline_layout) = 0;

    virtual ~IIr77PVLayout() = default;
}* PIr77PVDescriptorSet;
}  // namespace NSIr77PeregrineV
