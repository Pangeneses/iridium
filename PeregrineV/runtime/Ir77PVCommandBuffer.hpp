#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <vector>
#include <cstring>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77PVCommandBuffer.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77PVCommandBuffer : public Ir77Enlisted, public IIr77PVCommandBuffer, public std::enable_shared_from_this<Ir77PVCommandBuffer> {
   public:
    Ir77PVCommandBuffer() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

   public:
    std::shared_ptr<IIr77Return const> MemberOfUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVCommandBuffer>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PeregrineV>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    IIr77GUID* const QueryInterface(IIr77GUID const* iid, std::shared_ptr<void>& obj) {
        if (iid == &GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == &GUIDIr77PVCommandBuffer)
            obj = std::shared_ptr<Ir77PVCommandBuffer>(shared_from_this(), static_cast<Ir77PVCommandBuffer*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context) {
        m_context = context;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SyncCommandBuffer() {
        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.queueFamilyIndex = m_graphics_family;
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        if (vkCreateCommandPool(m_device, &pool_info, nullptr, &m_cmd_pool) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: vkCreateCommandPool failed.");
        }

        VkCommandBufferAllocateInfo cmd_alloc_info{};
        cmd_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_alloc_info.commandPool = m_cmd_pool;
        cmd_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd_alloc_info.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(m_device, &cmd_alloc_info, &m_cmd_buf) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: vkAllocateCommandBuffers failed.");
        }
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SyncLevel(Ir77PVCommandBufferLevel& level) const { return Ir77RETURN<Ir77OperationSucceeded>(); }

    std::shared_ptr<IIr77Return const> SyncBegin() { return Ir77RETURN<Ir77OperationSucceeded>(); }

    std::shared_ptr<IIr77Return const> SyncEnd() { return Ir77RETURN<Ir77OperationSucceeded>(); }

    std::shared_ptr<IIr77Return const> SyncReset() { return Ir77RETURN<Ir77OperationSucceeded>(); }

    std::shared_ptr<IIr77Return const> SyncBeginRenderPass(std::shared_ptr<IIr77PVRenderPass const>& pass,
                                                           std::shared_ptr<IIr77PVFramebuffer const>& framebuffer, VkRect2D const& area,
                                                           std::vector<VkClearValue> const& clear_values) {
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SyncEndRenderPass() { return Ir77RETURN<Ir77OperationSucceeded>(); }

    std::shared_ptr<IIr77Return const> SyncPipeline(std::shared_ptr<IIr77PVPipeline const>& pipeline) { return Ir77RETURN<Ir77OperationSucceeded>(); }

    std::shared_ptr<IIr77Return const> SyncVertexBuffer(std::shared_ptr<IIr77PVBuffer const>& buffer, VkDeviceSize const& offset) {
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> BindIndexBuffer(std::shared_ptr<IIr77PVBuffer const>& buffer, VkDeviceSize const& offset,
                                                       VkIndexType const& index_type) {
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> BindDescriptorSet(std::shared_ptr<IIr77PVDescriptorSet const>& set, uint32_t binding) {
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetViewport(VkViewport const& viewport) { return Ir77RETURN<Ir77OperationSucceeded>(); }

    std::shared_ptr<IIr77Return const> SetScissor(VkRect2D const& scissor) { return Ir77RETURN<Ir77OperationSucceeded>(); }

    std::shared_ptr<IIr77Return const> Draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance) {
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DrawIndexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset,
                                                   uint32_t first_instance) {
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DrawIndirect(std::shared_ptr<IIr77PVBuffer const>& buffer, VkDeviceSize const& offset, uint32_t draw_count) {
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> Dispatch(uint32_t x, uint32_t y, uint32_t z) { return Ir77RETURN<Ir77OperationSucceeded>(); }

    std::shared_ptr<IIr77Return const> PipelineBarrier(std::shared_ptr<IIr77PVBarrier const>& barrier) { return Ir77RETURN<Ir77OperationSucceeded>(); }

    std::shared_ptr<IIr77Return const> Execute(std::shared_ptr<IIr77PVCommandBuffer const>& secondary) { return Ir77RETURN<Ir77OperationSucceeded>(); }

    std::shared_ptr<IIr77Return const> CopyBuffer(std::shared_ptr<IIr77PVBuffer const>& src, std::shared_ptr<IIr77PVBuffer const>& dst,
                                                  VkBufferCopy const& region) {
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CopyBufferToImage(std::shared_ptr<IIr77PVBuffer const>& src, std::shared_ptr<IIr77PVImage const>& dst,
                                                         VkBufferImageCopy const& region) {
        return Ir77RETURN<Ir77OperationSucceeded>();
    }
    
   private:
    std::shared_ptr<IIr77Enlisted> m_context;

    VkCommandPool m_cmd_pool{VK_NULL_HANDLE};

    VkCommandBuffer m_cmd_buf{VK_NULL_HANDLE};

    VkCommandPoolCreateInfo m_cmd_pool_create_info;

    VkCommandBufferAllocateInfo m_cmd_buffer_allocate_info;
};
}  // namespace NSIr77PeregrineV