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
struct IIr77PVCommandBuffer;

typedef struct IIr77PVMesh : virtual public IIr77Enlisted {
    IIr77PVMesh() = default;

    virtual std::shared_ptr<IIr77Return const> SetVertexBuffer(std::shared_ptr<IIr77PVBuffer const>& buffer) = 0;

    virtual std::shared_ptr<IIr77Return const> GetVertexBuffer(std::shared_ptr<IIr77PVBuffer const>& buffer) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetVertexCount(uint32_t count) = 0;

    virtual std::shared_ptr<IIr77Return const> GetVertexCount(uint32_t& count) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetVertexStride(uint32_t stride) = 0;

    virtual std::shared_ptr<IIr77Return const> GetVertexStride(uint32_t& stride) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetIndexBuffer(std::shared_ptr<IIr77PVBuffer const>& buffer) = 0;

    virtual std::shared_ptr<IIr77Return const> GetIndexBuffer(std::shared_ptr<IIr77PVBuffer const>& buffer) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetIndexCount(uint32_t count) = 0;

    virtual std::shared_ptr<IIr77Return const> GetIndexCount(uint32_t& count) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetIndexType(Ir77PVIndexType const& type) = 0;

    virtual std::shared_ptr<IIr77Return const> GetIndexType(Ir77PVIndexType& type) const = 0;

    virtual std::shared_ptr<IIr77Return const> Bind(std::shared_ptr<IIr77PVCommandBuffer const>& cmd) = 0;

    virtual std::shared_ptr<IIr77Return const> Draw(std::shared_ptr<IIr77PVCommandBuffer const>& cmd, uint32_t instance_count, uint32_t first_instance) = 0;

    virtual std::shared_ptr<IIr77Return const> Build() = 0;

    virtual ~IIr77PVMesh() = default;
}* PIr77PVMesh;
}  // namespace NSIr77PeregrineV

/*
Internal — vertex offset, index offset into buffers, topology (VkPrimitiveTopology) driven by property page.
Bind does vkCmdBindVertexBuffers + vkCmdBindIndexBuffer in one shot. Draw follows immediately with vkCmdDrawIndexed or vkCmdDraw depending on whether an index
buffer is set. Caller just calls Bind then Draw — no raw Vulkan touch.
*/