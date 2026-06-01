#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <cstring>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

struct IIr77PVDescriptorLayout;
struct IIr77PVBuffer;
struct IIr77PVImageView;
struct IIr77PVSampler;

typedef struct IIr77PVDescriptorSet : virtual public IIr77Enlisted {
    IIr77PVDescriptorSet() = default;

    virtual std::shared_ptr<IIr77Return const> SetLayout(std::shared_ptr<IIr77PVDescriptorLayout const>& layout) = 0;

    virtual std::shared_ptr<IIr77Return const> GetLayout(std::shared_ptr<IIr77PVDescriptorLayout const>& layout) const = 0;

    virtual std::shared_ptr<IIr77Return const> BindBuffer(uint32_t binding, std::shared_ptr<IIr77PVBuffer const>& buffer, VkDeviceSize const& offset,
                                                          VkDeviceSize const& range) = 0;

    virtual std::shared_ptr<IIr77Return const> BindImage(uint32_t binding, std::shared_ptr<IIr77PVImageView const>& view, VkImageLayout const& layout) = 0;

    virtual std::shared_ptr<IIr77Return const> BindSampler(uint32_t binding, std::shared_ptr<IIr77PVImageView const>& view,
                                                           std::shared_ptr<IIr77PVSampler const>& sampler, VkImageLayout layout) = 0;

    virtual std::shared_ptr<IIr77Return const> Build() = 0;

    virtual std::shared_ptr<IIr77Return const> Update() = 0;

    virtual ~IIr77PVDescriptorSet() = default;
}* PIr77PVDescriptorSet;
}  // namespace NSIr77PeregrineV

/*
Internal — VkDescriptorSet, VkDescriptorPool reference, write descriptors accumulated from Bind* calls.
Build allocates from the pool via vkAllocateDescriptorSets then writes all accumulated bindings via vkUpdateDescriptorSets. Update re-writes bindings after
initial build — for dynamic resources that change per frame without reallocating the set.
*/