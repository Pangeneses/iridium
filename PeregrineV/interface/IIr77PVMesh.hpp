#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <cstring>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

enum class Ir77PVIndexType : uint32_t {
    Uint16 = 0,
    Uint32 = 1,
};

struct IIr77PVBuffer;
struct IIr77PVCmdBuffer;

typedef struct IIr77PVMesh : virtual public IIr77Enlisted {
    IIr77PVMesh() = default;

    virtual std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context) = 0;

    virtual ~IIr77PVMesh() = default;
}* PIr77PVMesh;
}  // namespace NSIr77PeregrineV
