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

// THE descriptor set -- one per device: the bindless table every pipeline reads through IIr77PVLayout. Register* hands out slots, each marked
// with what it holds; shaders get the numbers through push constants or other buffers.
//   SetDevice / SetLayout -> CreateDescriptorSet -> Register* ... Update* / Release; per command buffer Bind
typedef struct IIr77PVDescriptorSet : virtual public IIr77Enlisted {
    IIr77PVDescriptorSet() = default;

    // ---- wiring -- Ir77PVInit ------------------------------------------------------------------------------------------------------------------

    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    virtual std::shared_ptr<IIr77Return const> SetLayout(std::shared_ptr<IIr77PVLayout> layout) = 0;

    // The pool, the one set, and a slot allocator per binding sized from the layout's capacity
    virtual std::shared_ptr<IIr77Return const> CreateDescriptorSet() = 0;

    // ---- register -- a new slot, written now ---------------------------------------------------------------------------------------------------

    // The first texture registered gets slot BINDLESS_PLACEHOLDER_TEXTURE (0) -- Init registers the white placeholder first.
    // layout = the layout the image is in whenever a shader samples it (a depth texture: DEPTH_STENCIL_READ_ONLY_OPTIMAL)
    virtual std::shared_ptr<IIr77Return const> RegisterTexture(VkImageView const& view, VkSampler const& sampler, Ir77PVSlotKind const& kind,
                                                               std::uint32_t& index, VkImageLayout const& layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) = 0;

    virtual std::shared_ptr<IIr77Return const> RegisterBuffer(VkBuffer const& buffer, VkDeviceSize const& offset, VkDeviceSize const& range,
                                                              Ir77PVSlotKind const& kind, std::uint32_t& index) = 0;

    // The image must be in VK_IMAGE_LAYOUT_GENERAL whenever a shader uses it
    virtual std::shared_ptr<IIr77Return const> RegisterStorageImage(VkImageView const& view, Ir77PVSlotKind const& kind, std::uint32_t& index) = 0;

    // Binding 3 -- a TLAS, kind TopLevel. Ray-query devices only (fails otherwise: the binding has no room).
    virtual std::shared_ptr<IIr77Return const> RegisterAccelerationStructure(VkAccelerationStructureKHR const& structure, std::uint32_t& index) = 0;

    // ---- update -- rewrite a registered slot in place, only while no frame in flight reads it ----------------------------------------------
    // When frames might read it, register a new slot and Bury the old one instead.

    virtual std::shared_ptr<IIr77Return const> UpdateTexture(std::uint32_t const& index, VkImageView const& view, VkSampler const& sampler,
                                                             VkImageLayout const& layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) = 0;

    virtual std::shared_ptr<IIr77Return const> UpdateAccelerationStructure(std::uint32_t const& index, VkAccelerationStructureKHR const& structure) = 0;

    // ---- release -----------------------------------------------------------------------------------------------------------------------------

    // binding = BINDLESS_BINDING_TEXTURES / _BUFFERS / _STORAGE_IMAGES / _ACCELERATION. Free now -- only for a slot no frame in flight can read:
    // at runtime Transfer's graveyard calls this when a grave expires; teardown calls it after the device idles.
    virtual std::shared_ptr<IIr77Return const> Release(std::uint32_t const& binding, std::uint32_t const& index) = 0;

    // ---- record / query ----------------------------------------------------------------------------------------------------------------------

    // Binds the set at set 0. Once per command buffer per bind point -- pipeline switches don't disturb it.
    virtual std::shared_ptr<IIr77Return const> Bind(VkCommandBuffer const& cmd, VkPipelineBindPoint const& bind_point) = 0;

    // What a slot holds -- None when it was never registered, has been released, or is out of range
    virtual std::shared_ptr<IIr77Return const> GetSlotKind(std::uint32_t const& binding, std::uint32_t const& index, Ir77PVSlotKind& kind) const = 0;

    virtual ~IIr77PVDescriptorSet() = default;
}* pIIr77PVDescriptorSet;
}  // namespace NSIr77PeregrineV