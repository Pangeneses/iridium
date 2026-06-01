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

typedef struct IIr77PVDescriptorLayout : virtual public IIr77Enlisted {
    IIr77PVDescriptorLayout() = default;

    virtual std::shared_ptr<IIr77Return const> AddBinding(uint32_t binding, VkDescriptorType const& type, uint32_t count, VkShaderStageFlags const& stages) = 0;

    virtual std::shared_ptr<IIr77Return const> GetBindings(std::vector<VkDescriptorSetLayoutBinding>& bindings) const = 0;

    virtual std::shared_ptr<IIr77Return const> Build() = 0;

    virtual ~IIr77PVDescriptorLayout() = default;
}* PIr77PVDescriptorLayout;
}  // namespace NSIr77REDOS

/*
Internal — VkDescriptorSetLayout. Build fires vkCreateDescriptorSetLayout. Bindings are accumulated via AddBinding in declaration order — binding index,
descriptor type (uniform buffer, sampled image, storage buffer etc.), array count, and which shader stages can see it.
*/