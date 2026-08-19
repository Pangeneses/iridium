#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

enum class Ir77PVCmdBufferLevel : uint32_t {
    Primary = 0,    // submitted directly to queue
    Secondary = 1,  // executed from primary via vkCmdExecuteCommands
};

struct IIr77PVPeregrineV;
struct IIr77PVRenderPass;
struct IIr77PVFramebuffer;
struct IIr77PVPipeline;
struct IIr77PVBuffer;
struct IIr77PVDescriptorSet;
struct IIr77PVBarrier;
struct IIr77PVSemaphore;
struct IIr77PVImage;

typedef struct IIr77PVCmdBuffer : virtual public IIr77Enlisted {
    IIr77PVCmdBuffer() = default;

    virtual std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context) = 0;

    virtual ~IIr77PVCmdBuffer() = default;

}* PIr77PVCmdBuffer;
}  // namespace NSIr77PeregrineV

/*
Internal — VkCommandBuffer, VkCommandPool reference, recording state tracking.
*/