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

struct IIr77PVPipeline;
struct IIr77PVDescriptorSet;
struct IIr77PVSampler;
struct IIr77PVImageView;
struct IIr77PVCommandBuffer;

typedef struct IIr77PVMaterial : virtual public IIr77Enlisted {
    IIr77PVMaterial() = default;

    virtual std::shared_ptr<IIr77Return const> SetPipeline(std::shared_ptr<IIr77PVPipeline const>& pipeline) = 0;

    virtual std::shared_ptr<IIr77Return const> GetPipeline(std::shared_ptr<IIr77PVPipeline const>& pipeline) const = 0;

    virtual std::shared_ptr<IIr77Return const> AddDescriptorSet(std::shared_ptr<IIr77PVDescriptorSet const>& set) = 0;

    virtual std::shared_ptr<IIr77Return const> GetDescriptorSets(std::vector<std::shared_ptr<IIr77PVDescriptorSet const>>& sets) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetTexture(uint32_t slot, std::shared_ptr<IIr77PVImageView const>& view,
                                                          std::shared_ptr<IIr77PVSampler const>& sampler) = 0;

    virtual std::shared_ptr<IIr77Return const> GetTexture(uint32_t slot, std::shared_ptr<IIr77PVImageView const>& view,
                                                          std::shared_ptr<IIr77PVSampler const>& sampler) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetPushConstant(void const* data, uint32_t size, uint32_t offset) = 0;

    virtual std::shared_ptr<IIr77Return const> Bind(std::shared_ptr<IIr77PVCommandBuffer const>& cmd) = 0;

    virtual std::shared_ptr<IIr77Return const> Build() = 0;

    virtual ~IIr77PVMaterial() = default;
}* PIr77PVMaterial;
}  // namespace NSIr77REDOS