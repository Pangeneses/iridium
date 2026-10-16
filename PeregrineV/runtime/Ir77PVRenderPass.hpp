#pragma once

#include <vulkan/vulkan.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <memory>

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
// Ir77PVRenderPass -- what one kind of pass renders into, for dynamic rendering (no VkRenderPass, no framebuffers):
//   Main       color (swapchain) + depth, presents
//   Post       color (swapchain), no depth, presents
//   Offscreen  color (HDR) + depth, color sampled afterwards
//   Shadow     depth only, sampled afterwards
//
//   SetDevice -> SetKind (resets the formats) -> [SetColorFormat / SetDepthFormat] -> CreateRenderPass
// Pipelines declare these formats (VkPipelineRenderingCreateInfo); the swapchain sizes its depth image with GetDepthFormat; Ir77PVCmdBuffer
// begins the pass with vkCmdBeginRendering on the matching images.
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

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Render Pass.");
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // queries
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> GetKind(Ir77PVRenderPassKind* kind) {
        *kind = m_kind;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetColorFormat(VkFormat* format) {
        *format = HasColorAttachment() ? m_color_format : VK_FORMAT_UNDEFINED;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // valid after CreateRenderPass -- the swapchain / shadow target size their depth image with this
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
        *count = (HasColorAttachment() ? 1u : 0u) + (HasDepthAttachment() ? 1u : 0u);

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

    // -------------------------------------------------------------------------------------------------------------------------------------
    // builders -- values only, no state
    // -------------------------------------------------------------------------------------------------------------------------------------
    bool HasColorAttachment() const { return m_kind != Ir77PVRenderPassKind::Shadow; }

    bool HasDepthAttachment() const { return m_kind != Ir77PVRenderPassKind::Post; }

   private:
    std::shared_ptr<IIr77PVDevice> m_device{};

    Ir77PVRenderPassKind m_kind{Ir77PVRenderPassKind::Main};

    VkFormat m_color_format{VK_FORMAT_B8G8R8A8_SRGB};

    VkFormat m_depth_format{VK_FORMAT_UNDEFINED};

};
}  // namespace NSIr77PeregrineV