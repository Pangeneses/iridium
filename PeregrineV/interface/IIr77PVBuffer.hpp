#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <memory>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVDevice.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// Dynamic: host-mapped, one copy per frame in flight, written with Update() every frame
// Static:  device-local, single copy, written once with Upload() through a staging buffer
enum class Ir77PVBufferMode : std::uint8_t { Dynamic, Static };

typedef struct IIr77PVBuffer : virtual public IIr77Enlisted {
    IIr77PVBuffer() = default;

    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    virtual std::shared_ptr<IIr77Return const> SetAllocator(VmaAllocator allocator) = 0;

    virtual std::shared_ptr<IIr77Return const> SetUsage(VkBufferUsageFlags const& usage) = 0;

    virtual std::shared_ptr<IIr77Return const> SetMode(Ir77PVBufferMode const& mode) = 0;

    virtual std::shared_ptr<IIr77Return const> CreateResources(VkDeviceSize const& size, std::uint32_t const& copies) = 0;

    virtual std::shared_ptr<IIr77Return const> Update(std::uint32_t const& copy, void const* data, VkDeviceSize const& size, VkDeviceSize const& offset = 0) = 0;

    virtual std::shared_ptr<IIr77Return const> Upload(VkCommandPool const& pool, VkQueue const& queue, void const* data, VkDeviceSize const& size) = 0;

    virtual std::shared_ptr<IIr77Return const> GetBuffer(std::uint32_t const& copy, VkBuffer* buffer) = 0;

    virtual std::shared_ptr<IIr77Return const> GetBufferInfo(std::uint32_t const& copy, VkDescriptorBufferInfo* info) = 0;

    virtual std::shared_ptr<IIr77Return const> GetSize(VkDeviceSize* size) = 0;

    virtual std::shared_ptr<IIr77Return const> GetCopies(std::uint32_t* copies) = 0;

    virtual std::shared_ptr<IIr77Return const> GetMode(Ir77PVBufferMode* mode) = 0;

    virtual ~IIr77PVBuffer() = default;
}* pIIr77PVBuffer;
}  // namespace NSIr77PeregrineV