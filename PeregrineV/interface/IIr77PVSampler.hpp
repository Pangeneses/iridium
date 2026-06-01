#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <cstring>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

typedef struct IIr77PVSampler : virtual public IIr77Enlisted {
    IIr77PVSampler() = default;

    virtual std::shared_ptr<IIr77Return const> SetFilter(VkFilter const& min, VkFilter const& mag) = 0;

    virtual std::shared_ptr<IIr77Return const> GetFilter(VkFilter& min, VkFilter& mag) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetMipmapMode(VkSamplerMipmapMode const& mode) = 0;

    virtual std::shared_ptr<IIr77Return const> GetMipmapMode(VkSamplerMipmapMode& mode) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetAddressMode(VkSamplerAddressMode const& u, VkSamplerAddressMode const& v, VkSamplerAddressMode const& w) = 0;

    virtual std::shared_ptr<IIr77Return const> GetAddressMode(VkSamplerAddressMode& u, VkSamplerAddressMode& v, VkSamplerAddressMode& w) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetAnisotropy(float max_anisotropy) = 0;

    virtual std::shared_ptr<IIr77Return const> GetAnisotropy(float& max_anisotropy) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetLodRange(float min_lod, float max_lod) = 0;

    virtual std::shared_ptr<IIr77Return const> GetLodRange(float& min_lod, float& max_lod) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetBorderColor(VkBorderColor const& color) = 0;

    virtual std::shared_ptr<IIr77Return const> GetBorderColor(VkBorderColor& color) const = 0;

    virtual std::shared_ptr<IIr77Return const> Build() = 0;

    virtual ~IIr77PVSampler() = default;
}* PIr77PVSampler;
}  // namespace NSIr77REDOS

/*
Internal — VkSampler, compare op (depth sampling), unnormalized coordinates flag. Those are edge cases — leave internal, expose via property page if needed.
Build fires vkCreateSampler.
*/