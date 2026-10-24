#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <cstdint>
#include <memory>
#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVDevice.hpp"
#include "IIr77PVSwapchain.hpp"
#include "IIr77PVDescriptorSet.hpp"
#include "IIr77PVOverlay.hpp"

#include "../types/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// The CEF overlays of one device -- one per window, each with a texture slot in the device's bindless table. CEF paints straight into them.
//   Ir77PVInit wires it: SetDevice / SetAllocator / SetSwapchains / SetDescriptorSet -> CreateOverlays
typedef struct IIr77PVCEF : virtual public IIr77Enlisted {
    IIr77PVCEF() = default;

    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    virtual std::shared_ptr<IIr77Return const> SetAllocator(VmaAllocator allocator) = 0;

    // One per window, in window order -- one overlay is made for each
    virtual std::shared_ptr<IIr77Return const> SetSwapchains(std::vector<std::shared_ptr<IIr77PVSwapchain>> const& swapchains) = 0;

    // The device's bindless table -- each overlay image gets a texture slot in it
    virtual std::shared_ptr<IIr77Return const> SetDescriptorSet(std::shared_ptr<IIr77PVDescriptorSet> descriptor_set) = 0;

    // One overlay per window. Before Paint's command buffers, so Paint can wire them in.
    virtual std::shared_ptr<IIr77Return const> CreateOverlays() = 0;

    // Window `window_index`'s overlay
    virtual std::shared_ptr<IIr77Return const> GetOverlay(std::uint32_t const& window_index, std::shared_ptr<IIr77PVOverlay>& overlay) = 0;

    // Every overlay, in window order -- for Paint
    virtual std::shared_ptr<IIr77Return const> GetOverlays(std::vector<std::shared_ptr<IIr77PVOverlay>>& overlays) = 0;

    virtual ~IIr77PVCEF() = default;
}* pIIr77PVCEF;
}  // namespace NSIr77PeregrineV
