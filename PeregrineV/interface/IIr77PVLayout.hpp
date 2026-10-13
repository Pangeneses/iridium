#pragma once

#include <vulkan/vulkan.h>

#include <memory>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVDevice.hpp"

#include "../types/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// THE pipeline layout -- one per device, shared by every graphics, compute and ray pipeline.
// set 0: bindless textures / storage buffers / storage images. Push constants: Ir77PVPushConstants, every stage.
typedef struct IIr77PVLayout : virtual public IIr77Enlisted {
    IIr77PVLayout() = default;

    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    // Sizes the arrays from the device limits, then creates the set layout and the pipeline layout. Once per device.
    virtual std::shared_ptr<IIr77Return const> CreateLayout() = 0;

    virtual std::shared_ptr<IIr77Return const> GetPipelineLayout(VkPipelineLayout* pipeline_layout) = 0;

    virtual std::shared_ptr<IIr77Return const> GetDescriptorSetLayout(VkDescriptorSetLayout* set_layout) = 0;

    // Array sizes actually in use -- BINDLESS_* clamped to the device's update-after-bind limits. Valid after CreateLayout.
    virtual std::shared_ptr<IIr77Return const> GetCapacity(Ir77PVBindlessCapacity& capacity) = 0;

    virtual ~IIr77PVLayout() = default;
}* pIIr77PVLayout;
}  // namespace NSIr77PeregrineV