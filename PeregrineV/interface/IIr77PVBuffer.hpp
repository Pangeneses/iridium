#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <cstring>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

enum class Ir77BufferType : uint32_t {
    Vertex = 0,
    Index = 1,
    Uniform = 2,
    Storage = 3,
    Staging = 4,
    Indirect = 5,
};

struct IIr77PVPregrineV;
struct IIr77PVMemory;
struct IIr77PVAllocation;

typedef struct IIr77PVBuffer : virtual public IIr77Enlisted {
    IIr77PVBuffer() = default;

    virtual std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context) = 0;

    virtual ~IIr77PVBuffer() = default;
}* PIr77Buffer;
}  // namespace NSIr77PeregrineV

/*
Internal — VkBuffer, memory properties flags driven by type. Build fires vkCreateBuffer + allocates through IIr77PVMemory + vkBindBufferMemory. Same pattern as
IIr77PVImage. Write is the CPU upload path — only valid on host-visible buffers (staging, uniform). Implementation calls through to the persistent mapped
pointer from the allocation. Caller never touches the raw pointer.
*/