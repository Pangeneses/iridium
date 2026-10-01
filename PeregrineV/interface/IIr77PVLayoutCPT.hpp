#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <memory>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVDevice.hpp"

#include "../runtime/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

typedef struct IIr77PVLayoutCPT : virtual public IIr77Enlisted {
    IIr77PVLayoutCPT() = default;

    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    virtual std::shared_ptr<IIr77Return const> SetKind(Ir77PVComputeKind const& kind) = 0;

    virtual std::shared_ptr<IIr77Return const> DefineDescriptorSetLayout() = 0;

    virtual std::shared_ptr<IIr77Return const> DefineDescriptorPool(std::uint32_t const& max_count) = 0;

    virtual std::shared_ptr<IIr77Return const> DefinePipelineLayout() = 0;

    virtual std::shared_ptr<IIr77Return const> AllocateSet(std::uint32_t const& set_index, VkDescriptorSet* set) = 0;

    virtual std::shared_ptr<IIr77Return const> GetKind(Ir77PVComputeKind* kind) = 0;

    virtual std::shared_ptr<IIr77Return const> GetSetCount(std::uint32_t* count) = 0;

    virtual std::shared_ptr<IIr77Return const> GetDescriptorSetLayout(std::uint32_t const& set_index, VkDescriptorSetLayout* layout) = 0;

    virtual std::shared_ptr<IIr77Return const> GetDescriptorPool(VkDescriptorPool* pool) = 0;

    virtual std::shared_ptr<IIr77Return const> GetPipelineLayout(VkPipelineLayout* pipeline_layout) = 0;

    virtual ~IIr77PVLayoutCPT() = default;
}* pIIr77PVLayoutCPT;
}  // namespace NSIr77PeregrineV