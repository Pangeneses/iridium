#pragma once

#include <vulkan/vulkan.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77PVRenderPass.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// =============================================================================================================================================
// Ir77PVRenderPass -- one render pass of a kind:
//   Main       color (swapchain) + depth, presents
//   Post       color (swapchain), no depth, presents
//   Offscreen  color (HDR) + depth, color sampled afterwards
//   Shadow     depth only, sampled afterwards
//
//   SetDevice -> SetKind (resets the depth format) -> [SetColorFormat / SetDepthFormat] -> CreateRenderPass
// The depth format is decided here; framebuffers read it back with GetDepthFormat so they always match.
// =============================================================================================================================================
class Ir77PVRenderPass : public Ir77Enlisted, public IIr77PVRenderPass, public std::enable_shared_from_this<Ir77PVRenderPass> {
   public:
    Ir77PVRenderPass() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    ~Ir77PVRenderPass() {
        if (!m_device) return;

        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        if (m_render_pass != VK_NULL_HANDLE) vkDestroyRenderPass(device, m_render_pass, nullptr);
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVRenderPass>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PeregrineV>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77PVRenderPass)
            obj = std::shared_ptr<IIr77PVRenderPass>(shared_from_this(), static_cast<IIr77PVRenderPass*>(this));

        else if (iid == GUIDIr77PVRenderPass)
            obj = std::shared_ptr<Ir77PVRenderPass>(shared_from_this(), static_cast<Ir77PVRenderPass*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // configuration
    // -------------------------------------------------------------------------------------------------------------------------------------
   public:
    std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) {
        m_device = device;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Resets the formats to the kind's defaults -- call SetColorFormat / SetDepthFormat after this, not before
    std::shared_ptr<IIr77Return const> SetKind(Ir77PVRenderPassKind const& kind) {
        m_kind = kind;
        m_depth_format = VK_FORMAT_UNDEFINED;

        switch (m_kind) {
            case Ir77PVRenderPassKind::Main:
            case Ir77PVRenderPassKind::Post:
                m_color_format = VK_FORMAT_B8G8R8A8_SRGB;  // placeholder -- SetColorFormat to the swapchain's format
                break;

            case Ir77PVRenderPassKind::Offscreen:
                m_color_format = VK_FORMAT_R16G16B16A16_SFLOAT;
                break;

            case Ir77PVRenderPassKind::Shadow:
                m_color_format = VK_FORMAT_UNDEFINED;
                break;

            default:
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVRenderPass: unknown kind.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetColorFormat(VkFormat const& format) {
        m_color_format = format;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Optional -- left UNDEFINED, CreateRenderPass picks the first supported depth format
    std::shared_ptr<IIr77Return const> SetDepthFormat(VkFormat const& format) {
        m_depth_format = format;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // create
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> CreateRenderPass() {
        if (!m_device) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVRenderPass: device not set.");

        bool const color = HasColorAttachment();
        bool const depth = HasDepthAttachment();

        if (color && m_color_format == VK_FORMAT_UNDEFINED) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVRenderPass: color format not set.");

        if (depth) {
            auto const result = DefineDepthFormat();
            if (result->ID() != GUIDIr77OperationSucceeded) return result;
        }

        return DefineRenderPass(color, depth);
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // queries
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> GetRenderPass(VkRenderPass* render_pass) {
        *render_pass = m_render_pass;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetKind(Ir77PVRenderPassKind* kind) {
        *kind = m_kind;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetColorFormat(VkFormat* format) {
        *format = HasColorAttachment() ? m_color_format : VK_FORMAT_UNDEFINED;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // valid after CreateRenderPass -- framebuffers size their depth image with this
    std::shared_ptr<IIr77Return const> GetDepthFormat(VkFormat* format) {
        *format = HasDepthAttachment() ? m_depth_format : VK_FORMAT_UNDEFINED;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> HasColor(bool* has_color) {
        *has_color = HasColorAttachment();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> HasDepth(bool* has_depth) {
        *has_depth = HasDepthAttachment();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetAttachmentCount(std::uint32_t* count) {
        *count = m_attachment_count;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // define
    // -------------------------------------------------------------------------------------------------------------------------------------
   private:
    // Keeps a format given through SetDepthFormat; otherwise the first that works as a depth attachment (and as a sampled image for Shadow)
    std::shared_ptr<IIr77Return const> DefineDepthFormat() {
        if (m_depth_format != VK_FORMAT_UNDEFINED) return Ir77RETURN<Ir77OperationSucceeded>();

        VkPhysicalDevice physical{VK_NULL_HANDLE};
        m_device->GetPhysicalDevice(&physical);

        VkFormatFeatureFlags required{VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT};
        if (m_kind == Ir77PVRenderPassKind::Shadow) required |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;

        std::array<VkFormat, 4> const candidates{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM};

        for (auto const format : candidates) {
            VkFormatProperties props{};
            vkGetPhysicalDeviceFormatProperties(physical, format, &props);

            if ((props.optimalTilingFeatures & required) == required) {
                m_depth_format = format;
                return Ir77RETURN<Ir77OperationSucceeded>();
            }
        }

        return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVRenderPass: no supported depth format.");
    }

    // One subpass: [color], [depth] in that order -- framebuffers attach in the same order
    std::shared_ptr<IIr77Return const> DefineRenderPass(bool const& color, bool const& depth) {
        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        if (m_render_pass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(device, m_render_pass, nullptr);
            m_render_pass = VK_NULL_HANDLE;
        }

        std::vector<VkAttachmentDescription> attachments{};

        VkAttachmentReference color_ref{};
        VkAttachmentReference depth_ref{};

        if (color) {
            color_ref.attachment = static_cast<std::uint32_t>(attachments.size());
            color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            attachments.push_back(ColorAttachment());
        }

        if (depth) {
            depth_ref.attachment = static_cast<std::uint32_t>(attachments.size());
            depth_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            attachments.push_back(DepthAttachment());
        }

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = color ? 1 : 0;
        subpass.pColorAttachments = color ? &color_ref : nullptr;
        subpass.pDepthStencilAttachment = depth ? &depth_ref : nullptr;

        std::vector<VkSubpassDependency> const dependencies = Dependencies();

        VkRenderPassCreateInfo render_pass_info{};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_info.attachmentCount = static_cast<std::uint32_t>(attachments.size());
        render_pass_info.pAttachments = attachments.data();
        render_pass_info.subpassCount = 1;
        render_pass_info.pSubpasses = &subpass;
        render_pass_info.dependencyCount = static_cast<std::uint32_t>(dependencies.size());
        render_pass_info.pDependencies = dependencies.data();

        if (vkCreateRenderPass(device, &render_pass_info, nullptr, &m_render_pass) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVRenderPass: vkCreateRenderPass failed.");

        m_attachment_count = static_cast<std::uint32_t>(attachments.size());

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Render Pass.");
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // builders -- values only, no state
    // -------------------------------------------------------------------------------------------------------------------------------------
    bool HasColorAttachment() const { return m_kind != Ir77PVRenderPassKind::Shadow; }

    bool HasDepthAttachment() const { return m_kind != Ir77PVRenderPassKind::Post; }

    bool PresentsToSwapchain() const { return m_kind == Ir77PVRenderPassKind::Main || m_kind == Ir77PVRenderPassKind::Post; }

    // Swapchain passes end in PRESENT_SRC; Offscreen ends ready to be sampled
    VkAttachmentDescription ColorAttachment() const {
        VkAttachmentDescription attachment{};
        attachment.format = m_color_format;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = PresentsToSwapchain() ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        return attachment;
    }

    // Shadow keeps its depth (sampled later); Main / Offscreen discard it after the pass
    VkAttachmentDescription DepthAttachment() const {
        bool const sampled = m_kind == Ir77PVRenderPassKind::Shadow;

        VkAttachmentDescription attachment{};
        attachment.format = m_depth_format;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp = sampled ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = sampled ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        return attachment;
    }

    // External dependencies -- not by-region: the passes on the other side read whole images, not the same pixel
    //   in  -- prior use of the attachments (previous frame's writes / sampling) finishes before this pass writes them
    //   out -- for passes sampled afterwards (Shadow / Offscreen), the writes are visible to later fragment shaders
    std::vector<VkSubpassDependency> Dependencies() const {
        std::vector<VkSubpassDependency> dependencies{};

        VkSubpassDependency in{};
        in.srcSubpass = VK_SUBPASS_EXTERNAL;
        in.dstSubpass = 0;

        VkSubpassDependency out{};
        out.srcSubpass = 0;
        out.dstSubpass = VK_SUBPASS_EXTERNAL;

        switch (m_kind) {
            case Ir77PVRenderPassKind::Main:
            case Ir77PVRenderPassKind::Offscreen:
                in.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                in.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                in.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                in.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                dependencies.push_back(in);

                if (m_kind == Ir77PVRenderPassKind::Offscreen) {
                    out.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                    out.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                    out.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    out.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                    dependencies.push_back(out);
                }
                break;

            case Ir77PVRenderPassKind::Post:
                in.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                in.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                in.srcAccessMask = 0;
                in.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dependencies.push_back(in);
                break;

            case Ir77PVRenderPassKind::Shadow:
                in.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                in.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                in.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                in.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                dependencies.push_back(in);

                out.srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                out.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                out.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                out.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                dependencies.push_back(out);
                break;

            default:
                break;
        }

        return dependencies;
    }

   private:
    std::shared_ptr<IIr77PVDevice> m_device{};

    Ir77PVRenderPassKind m_kind{Ir77PVRenderPassKind::Main};

    VkFormat m_color_format{VK_FORMAT_B8G8R8A8_SRGB};

    VkFormat m_depth_format{VK_FORMAT_UNDEFINED};

    std::uint32_t m_attachment_count{0};

    VkRenderPass m_render_pass{VK_NULL_HANDLE};
};
}  // namespace NSIr77PeregrineV