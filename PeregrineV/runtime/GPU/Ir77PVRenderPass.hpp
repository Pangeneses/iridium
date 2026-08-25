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

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

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
        VkDevice device;
        m_device->GetDevice(&device);

        vkDestroyRenderPass(device, m_render_pass, nullptr);
    }

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
            obj = std::shared_ptr<Ir77PVRenderPass>(shared_from_this(), static_cast<Ir77PVRenderPass*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> SetInstance(std::shared_ptr<IIr77PVInstance> instance) {
        m_instance = instance;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) {
        m_device = device;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineColorAttachment(SDL_Window* window) {
        VkInstance instance;
        m_instance->GetInstance(&instance);

        VkPhysicalDevice phys_device;
        m_device->GetPhysicalDevice(&phys_device);

        VkSurfaceKHR surface;
        if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface)) {
            std::string str{SDL_GetError()};
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDevice: SDL_Vulkan_CreateSurface failed: " + str);
        }

        uint32_t format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(phys_device, surface, &format_count, nullptr);

        std::vector<VkSurfaceFormatKHR> formats;
        if (format_count != 0) {
            formats.resize(format_count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(phys_device, surface, &format_count, formats.data());
        }

        for (int i = 0; i < formats.size(); i++) {
            if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB) {
                m_color_attachment.format = VK_FORMAT_B8G8R8A8_SRGB;
            }
        }

        if (m_color_attachment.format != VK_FORMAT_B8G8R8A8_SRGB) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVRPColor: format failed: ");
        }

        m_color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        m_color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        m_color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        m_color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        m_color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        m_color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        m_color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        vkDestroySurfaceKHR(instance, surface, nullptr);

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
        VkDevice device;
        m_device->GetDevice(&device);


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
    std::shared_ptr<IIr77PVInstance> m_instance;

    std::shared_ptr<IIr77PVDevice> m_device;

    std::uint64_t m_device_id{UINT64_MAX};

    VkAttachmentDescription m_color_attachment{};

    VkAttachmentReference m_color_attachment_ref{};

    VkSubpassDescription m_subpass{};

    VkRenderPassCreateInfo m_render_pass_info{};

    VkRenderPass m_render_pass{VK_NULL_HANDLE};
};
}  // namespace NSIr77PeregrineV