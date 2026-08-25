#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <cstring>
#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

struct IIr77PVPipeline;
struct IIr77PVDescriptorSet;
struct IIr77PVSampler;
struct IIr77PVImageView;
struct IIr77PVCmdBuffer;

typedef struct IIr77PVMaterial : virtual public IIr77Enlisted {
    IIr77PVMaterial() = default;

    virtual std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context) = 0;

    virtual ~IIr77PVMaterial() = default;
}* pIIr77PVMaterial;
}  // namespace NSIr77PeregrineV