#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <memory>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../../dictionary/IDIIr77PeregrineV.hpp"
#include "../../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../../interface/IIr77PVRenderPass.hpp"
#include "../../interface/IIr77PVSwapchain.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77PVRenderPassColor : public Ir77Enlisted, public IIr77PVRenderPass, public std::enable_shared_from_this<Ir77PVRenderPassColor> {
   public:
    Ir77PVRenderPassColor() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    ~Ir77PVRenderPassColor() { vkDestroyRenderPass(GetDevice(), m_render_pass, nullptr); }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberOfUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVRenderPassColor>(uid);

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

        else if (iid == &GUIDIIr77PVRenderPass)
            obj = std::shared_ptr<IIr77PVRenderPass>(shared_from_this(), static_cast<IIr77PVRenderPass*>(this));

        else if (iid == &GUIDIr77PVRenderPassColor)
            obj = std::shared_ptr<Ir77PVRenderPassColor>(shared_from_this(), static_cast<Ir77PVRenderPassColor*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context) {
        m_context = context;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CreateRenderPass() {
        DefineColorAttachment();

        DefineColorAttachmentRef();

        DefineSubpass();

        DefineRenderPass();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineColorAttachment() {
        SwapchainSupportDetails swapchain = GetSwapchainSupportDetails();

        m_color_attachment.format = swapchain.format;
        m_color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        m_color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        m_color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        m_color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        m_color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        m_color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        m_color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineColorAttachmentRef() {
        m_color_attachment_ref.attachment = 0;
        m_color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineSubpass() {
        m_subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        m_subpass.colorAttachmentCount = 1;
        m_subpass.pColorAttachments = &m_color_attachment_ref;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineRenderPass() {
        VkDevice device = GetDevice();

        m_render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        m_render_pass_info.attachmentCount = 1;
        m_render_pass_info.pAttachments = &m_color_attachment;
        m_render_pass_info.subpassCount = 1;
        m_render_pass_info.pSubpasses = &m_subpass;

        if (vkCreateRenderPass(device, &m_render_pass_info, nullptr, &m_render_pass) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: VkRenderPass failed.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetRenderPass(VkRenderPass* render_pass) {
        *render_pass = m_render_pass;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    std::uint32_t CurrentDevice();

    VkDevice GetDevice();

    SwapchainSupportDetails GetSwapchainSupportDetails();

   private:
    std::shared_ptr<IIr77Enlisted> m_context;

    VkAttachmentDescription m_color_attachment{};

    VkAttachmentReference m_color_attachment_ref{};

    VkSubpassDescription m_subpass{};

    VkRenderPassCreateInfo m_render_pass_info{};

    VkRenderPass m_render_pass{VK_NULL_HANDLE};
};
}  // namespace NSIr77PeregrineV