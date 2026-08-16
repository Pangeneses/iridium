#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

enum class Ir77PVCommandBufferLevel : uint32_t {
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

typedef struct IIr77PVCommandBuffer : virtual public IIr77Enlisted {
    IIr77PVCommandBuffer() = default;

    virtual std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context) = 0;

    virtual std::shared_ptr<IIr77Return const> Begin() = 0;

    virtual std::shared_ptr<IIr77Return const> End() = 0;

    virtual std::shared_ptr<IIr77Return const> Reset() = 0;

    virtual std::shared_ptr<IIr77Return const> BeginRenderPass() = 0;

    virtual std::shared_ptr<IIr77Return const> EndRenderPass() = 0;

    virtual std::shared_ptr<IIr77Return const> GetPipeline() = 0;

    virtual std::shared_ptr<IIr77Return const> GetVertexBuffer() = 0;

    virtual std::shared_ptr<IIr77Return const> GetIndexBuffer() = 0;

    virtual std::shared_ptr<IIr77Return const> GetDescriptorSet() = 0;

    virtual std::shared_ptr<IIr77Return const> Draw() = 0;

    virtual std::shared_ptr<IIr77Return const> GetIndexed() = 0;

    virtual std::shared_ptr<IIr77Return const> GetIndirect() = 0;

    virtual std::shared_ptr<IIr77Return const> GetDispatch() = 0;

    virtual std::shared_ptr<IIr77Return const> GetBarrier() = 0;

    virtual std::shared_ptr<IIr77Return const> GetExecute() = 0;

    virtual std::shared_ptr<IIr77Return const> GetBuffer() = 0;

    virtual std::shared_ptr<IIr77Return const> GetBufferToImage() = 0;

    virtual ~IIr77PVCommandBuffer() = default;

}* PIr77PVCommandBuffer;
}  // namespace NSIr77PeregrineV

/*
Internal — VkCommandBuffer, VkCommandPool reference, recording state tracking.
*/