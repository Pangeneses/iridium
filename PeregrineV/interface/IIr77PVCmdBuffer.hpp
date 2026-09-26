#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVDevice.hpp"
#include "IIr77PVSwapchain.hpp"
#include "IIr77PVRenderPass.hpp"
#include "IIr77PVPipeline.hpp"
#include "IIr77PVLayout.hpp"
#include "IIr77PVBuffer.hpp"
#include "IIr77PVDescriptorSet.hpp"
#include "IIr77PVOverlay.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// Which render pass a draw list is recorded into.
//   Shadow      -> depth-only shadow pass (Shadow / ShadowSkinned pipelines)
//   Opaque      -> main pass, first (Static / Skinned pipelines)
//   Transparent -> main pass, after opaque; caller sorts back to front (Transparent pipeline)
enum class Ir77PVPass : std::uint8_t { Shadow, Opaque, Transparent };

// One draw. Sets 0 (global) and 1 (pass) are bound by the command buffer; the item carries only
// what changes per draw. Leave indirect empty for a direct indexed draw.
struct Ir77PVDrawItem {
    std::shared_ptr<IIr77PVPipeline> pipeline{};

    std::shared_ptr<IIr77PVLayout> layout{};

    std::shared_ptr<IIr77PVDescriptorSet> material{};  // set 2 -- Static / Skinned / Transparent

    std::shared_ptr<IIr77PVDescriptorSet> bones{};  // set 3 Skinned, set 1 ShadowSkinned

    std::shared_ptr<IIr77PVBuffer> vertex{};

    std::shared_ptr<IIr77PVBuffer> index{};

    VkIndexType index_type{VK_INDEX_TYPE_UINT32};

    std::uint32_t index_count{0};

    std::uint32_t first_index{0};

    std::int32_t vertex_offset{0};

    std::uint32_t instance_count{1};

    std::uint32_t first_instance{0};

    std::shared_ptr<IIr77PVBuffer> indirect{};

    VkDeviceSize indirect_offset{0};

    std::uint32_t indirect_count{0};

    std::array<float, 16> model{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};

    std::uint32_t light_index{0};  // shadow kinds only
};

typedef struct IIr77PVCmdBuffer : virtual public IIr77Enlisted {
    IIr77PVCmdBuffer() = default;

    // wiring
    virtual std::shared_ptr<IIr77Return const> SetInstance(std::shared_ptr<IIr77PVInstance> instance) = 0;

    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    virtual std::shared_ptr<IIr77Return const> SetSwapchain(std::shared_ptr<IIr77PVSwapchain> swapchain) = 0;

    virtual std::shared_ptr<IIr77Return const> SetRenderPass(std::shared_ptr<IIr77PVRenderPass> render_pass) = 0;

    virtual std::shared_ptr<IIr77Return const> SetShadowTarget(std::shared_ptr<IIr77PVRenderPass> render_pass, VkFramebuffer const& framebuffer,
                                                               VkExtent2D const& extent) = 0;

    virtual std::shared_ptr<IIr77Return const> SetGlobalSet(std::shared_ptr<IIr77PVDescriptorSet> global_set) = 0;

    virtual std::shared_ptr<IIr77Return const> SetPassSet(std::shared_ptr<IIr77PVDescriptorSet> pass_set) = 0;

    virtual std::shared_ptr<IIr77Return const> SetOverlay(std::shared_ptr<IIr77PVOverlay> overlay, std::shared_ptr<IIr77PVPipeline> pipeline,
                                                          std::shared_ptr<IIr77PVLayout> layout) = 0;

    virtual std::shared_ptr<IIr77Return const> SetClearColor(VkClearColorValue const& clear_color) = 0;

    // per frame
    virtual std::shared_ptr<IIr77Return const> SetCurrentFrame(std::uint32_t const& current_frame) = 0;

    virtual std::shared_ptr<IIr77Return const> SetDraws(Ir77PVPass const& pass, std::vector<Ir77PVDrawItem> const& draws) = 0;

    virtual std::shared_ptr<IIr77Return const> ClearDraws() = 0;

    // setup
    virtual std::shared_ptr<IIr77Return const> DefineSyncObjects() = 0;

    virtual std::shared_ptr<IIr77Return const> DefineCommandPool() = 0;

    virtual std::shared_ptr<IIr77Return const> AllocateCommandBuffer() = 0;

    // frame loop
    virtual std::shared_ptr<IIr77Return const> WaitForFence() = 0;

    virtual std::shared_ptr<IIr77Return const> AcquireNextImage(VkResult* acquire_next_result) = 0;

    virtual std::shared_ptr<IIr77Return const> ResetFence() = 0;

    virtual std::shared_ptr<IIr77Return const> RecordCommandBuffer() = 0;

    virtual std::shared_ptr<IIr77Return const> SubmitFrame() = 0;

    virtual std::shared_ptr<IIr77Return const> PresentFrame(VkResult* queue_present_result) = 0;

    // queries
    virtual std::shared_ptr<IIr77Return const> GetCurrentFrame(std::uint32_t* current_frame) = 0;

    virtual std::shared_ptr<IIr77Return const> GetImageIndex(std::uint32_t* image_index) = 0;

    virtual std::shared_ptr<IIr77Return const> GetCommandPool(VkCommandPool* pool) = 0;

    virtual std::shared_ptr<IIr77Return const> GetQueue(VkQueue* queue) = 0;

    virtual ~IIr77PVCmdBuffer() = default;
}* pIIr77PVCmdBuffer;
}  // namespace NSIr77PeregrineV