#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <cstring>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

enum class Ir77PVImageType : uint32_t {
    Texture = 0,       // sampled texture
    RenderTarget = 1,  // color attachment
    Depth = 2,         // depth/stencil attachment
    Storage = 3,       // compute read/write
};

struct IIr77PVMemory;
struct IIr77PVAllocation;

typedef struct IIr77PVImage : virtual public IIr77Enlisted {
    IIr77PVImage() = default;

    virtual std::shared_ptr<IIr77Return const> SetType(Ir77PVImageType const& type) = 0;

    virtual std::shared_ptr<IIr77Return const> GetType(Ir77PVImageType& type) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetFormat(VkFormat const& format) = 0;

    virtual std::shared_ptr<IIr77Return const> GetFormat(VkFormat& format) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetExtent(VkExtent3D const& extent) = 0;

    virtual std::shared_ptr<IIr77Return const> GetExtent(VkExtent3D& extent) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetMipLevels(uint32_t const& levels) = 0;

    virtual std::shared_ptr<IIr77Return const> GetMipLevels(uint32_t& levels) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetArrayLayers(uint32_t const& layers) = 0;

    virtual std::shared_ptr<IIr77Return const> GetArrayLayers(uint32_t& layers) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetUsage(VkImageUsageFlags const& usage) = 0;

    virtual std::shared_ptr<IIr77Return const> GetUsage(VkImageUsageFlags& usage) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetLayout(VkImageLayout const& layout) = 0;

    virtual std::shared_ptr<IIr77Return const> GetLayout(VkImageLayout& layout) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetMemory(std::shared_ptr<IIr77PVMemory const>& memory) = 0;

    virtual std::shared_ptr<IIr77Return const> GetMemory(std::shared_ptr<IIr77PVMemory const>& memory) const = 0;

    virtual std::shared_ptr<IIr77Return const> GetAllocation(std::shared_ptr<IIr77PVAllocation const>& alloc) const = 0;

    virtual std::shared_ptr<IIr77Return const> Build() = 0;

    virtual ~IIr77PVImage() = default;
}* PIr77PVImage;
}  // namespace NSIr77REDOS