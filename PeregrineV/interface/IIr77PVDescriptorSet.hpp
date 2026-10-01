#pragma once
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <memory>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVDevice.hpp"
#include "IIr77PVLayout.hpp"
#include "IIr77PVBuffer.hpp"

#include "../runtime/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

typedef struct IIr77PVDescriptorSet : virtual public IIr77Enlisted {
    IIr77PVDescriptorSet() = default;

    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    virtual std::shared_ptr<IIr77Return const> SetLayout(std::shared_ptr<IIr77PVLayout> layout, std::uint32_t const& set_index) = 0;

    virtual std::shared_ptr<IIr77Return const> CreateSets(std::uint32_t const& copies) = 0;

    virtual std::shared_ptr<IIr77Return const> BindBuffer(std::uint32_t const& binding, VkDescriptorType const& type, std::shared_ptr<IIr77PVBuffer> buffer) = 0;

    virtual std::shared_ptr<IIr77Return const> BindImage(std::uint32_t const& binding, VkDescriptorType const& type, VkDescriptorImageInfo const& image) = 0;

    virtual std::shared_ptr<IIr77Return const> Write() = 0;

    virtual std::shared_ptr<IIr77Return const> WriteCopy(std::uint32_t const& copy) = 0;

    virtual std::shared_ptr<IIr77Return const> GetSet(std::uint32_t const& copy, VkDescriptorSet* set) = 0;

    virtual std::shared_ptr<IIr77Return const> GetSetIndex(std::uint32_t* set_index) = 0;

    virtual std::shared_ptr<IIr77Return const> GetCopies(std::uint32_t* copies) = 0;

    virtual ~IIr77PVDescriptorSet() = default;
}* pIIr77PVDescriptorSet;
}  // namespace NSIr77PeregrineV