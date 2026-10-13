#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVDevice.hpp"
#include "IIr77PVLayout.hpp"

#include "../types/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// THE descriptor set -- one per device: the bindless table every pipeline reads through IIr77PVLayout.
// Register* hands out slot indices; shaders receive them through push constants or other buffers.
typedef struct IIr77PVDescriptorSet : virtual public IIr77Enlisted {
    IIr77PVDescriptorSet() = default;

    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    virtual std::shared_ptr<IIr77Return const> SetLayout(std::shared_ptr<IIr77PVLayout> layout) = 0;

    // Pool + the one set. The layout must be created first.
    virtual std::shared_ptr<IIr77Return const> CreateDescriptorSet() = 0;

    // ---- register: a new slot, written immediately ----------------------------------------------------------------------------------------
    // The first texture registered gets slot BINDLESS_PLACEHOLDER_TEXTURE (0) -- register the placeholder first.
    virtual std::shared_ptr<IIr77Return const> RegisterTexture(VkImageView const& view, VkSampler const& sampler, std::uint32_t& index) = 0;

    virtual std::shared_ptr<IIr77Return const> RegisterBuffer(VkBuffer const& buffer, VkDeviceSize const& offset, VkDeviceSize const& range,
                                                              std::uint32_t& index) = 0;

    // Image must be in VK_IMAGE_LAYOUT_GENERAL when a shader uses it
    virtual std::shared_ptr<IIr77Return const> RegisterStorageImage(VkImageView const& view, std::uint32_t& index) = 0;

    // ---- update: rewrite an existing slot (resize, regrow). Only when no frame in flight reads the slot; otherwise register + Release ----
    virtual std::shared_ptr<IIr77Return const> UpdateTexture(std::uint32_t const& index, VkImageView const& view, VkSampler const& sampler) = 0;

    virtual std::shared_ptr<IIr77Return const> UpdateBuffer(std::uint32_t const& index, VkBuffer const& buffer, VkDeviceSize const& offset,
                                                            VkDeviceSize const& range) = 0;

    virtual std::shared_ptr<IIr77Return const> UpdateStorageImage(std::uint32_t const& index, VkImageView const& view) = 0;

    // ---- release / per frame -----------------------------------------------------------------------------------------------------------
    // binding = BINDLESS_BINDING_TEXTURES / _BUFFERS / _STORAGE_IMAGES. The slot is reused only after MAX_FRAMES_IN_FLIGHT Ticks.
    virtual std::shared_ptr<IIr77Return const> Release(std::uint32_t const& binding, std::uint32_t const& index) = 0;

    // Once per frame, before recording
    virtual std::shared_ptr<IIr77Return const> Tick() = 0;

    // Binds the set at set 0. Once per command buffer per bind point -- pipeline switches don't disturb it.
    virtual std::shared_ptr<IIr77Return const> Bind(VkCommandBuffer const& cmd, VkPipelineBindPoint const& bind_point) = 0;

    virtual std::shared_ptr<IIr77Return const> GetSet(VkDescriptorSet* set) = 0;

    virtual ~IIr77PVDescriptorSet() = default;
}* pIIr77PVDescriptorSet;
}  // namespace NSIr77PeregrineV