#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>
#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVDevice.hpp"

#include "../types/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// Pipeline layout + descriptor set layouts + pool for the ray-tracing pipeline.
// Call order mirrors Ir77PVLayout: SetDevice -> (SetBindings / SetPushConstantSize) -> DefineDescriptorSetLayout ->
// DefineDescriptorPool -> DefinePipelineLayout.
typedef struct IIr77PVLayoutRay : virtual public IIr77Enlisted {
    IIr77PVLayoutRay() = default;

    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    // Replaces one set's bindings. Sets must be contiguous from 0. With no sets given, set 0 gets the defaults:
    //   0 TLAS, 1 storage image (raygen output), 2 camera UBO, 3 storage buffer (per-instance geometry / material table)
    virtual std::shared_ptr<IIr77Return const> SetBindings(std::uint32_t const& set, std::vector<VkDescriptorSetLayoutBinding> const& bindings) = 0;

    // 0 = no push constants. When > 0, visible to every ray-tracing stage.
    virtual std::shared_ptr<IIr77Return const> SetPushConstantSize(std::uint32_t const& size) = 0;

    virtual std::shared_ptr<IIr77Return const> DefineDescriptorSetLayout() = 0;

    // Room for `capacity` sets of every set layout
    virtual std::shared_ptr<IIr77Return const> DefineDescriptorPool(std::uint32_t const& capacity) = 0;

    virtual std::shared_ptr<IIr77Return const> DefinePipelineLayout() = 0;

    // `count` sets of one set layout from this layout's pool (one per frame in flight, typically)
    virtual std::shared_ptr<IIr77Return const> AllocateSets(std::uint32_t const& set, std::uint32_t const& count, std::vector<VkDescriptorSet>& sets) = 0;

    virtual std::shared_ptr<IIr77Return const> GetPipelineLayout(VkPipelineLayout* layout) = 0;

    virtual std::shared_ptr<IIr77Return const> GetDescriptorSetLayout(std::uint32_t const& set, VkDescriptorSetLayout* layout) = 0;

    virtual std::shared_ptr<IIr77Return const> GetDescriptorPool(VkDescriptorPool* pool) = 0;

    virtual std::shared_ptr<IIr77Return const> GetSetCount(std::uint32_t* count) = 0;

    virtual std::shared_ptr<IIr77Return const> GetPushConstantStages(VkShaderStageFlags* stages) = 0;

    virtual ~IIr77PVLayoutRay() = default;
}* pIIr77PVLayoutRay;
}  // namespace NSIr77PeregrineV