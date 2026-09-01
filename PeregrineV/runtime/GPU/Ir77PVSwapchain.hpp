#pragma once

#include <SDL3/SDL_video.h>
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../../dictionary/IDIIr77PeregrineV.hpp"
#include "../../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../../interface/IIr77PVSwapchain.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77PVSwapchain : public Ir77Enlisted, public IIr77PVSwapchain, public std::enable_shared_from_this<Ir77PVSwapchain> {
   public:
    Ir77PVSwapchain() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }
        m_enlisted = std::chrono::system_clock::now();

        m_capabilities.currentExtent.width = UINT32_MAX;
        m_capabilities.currentExtent.height = UINT32_MAX;
    }

    ~Ir77PVSwapchain() {
        VkDevice device;
        m_device->GetDevice(&device);

        for (int i = 0; i < m_swapchain_framebuffers.size(); i++) {
            vkDestroyFramebuffer(device, m_swapchain_framebuffers[i], nullptr);
        }

        for (int i = 0; i < m_swapchain_views.size(); i++) {
            vkDestroyImageView(device, m_swapchain_views[i], nullptr);
        }

        vkDestroySwapchainKHR(device, m_swapchain, nullptr);

        VkInstance instance;
        m_instance->GetInstance(&instance);
        vkDestroySurfaceKHR(instance, m_surface, nullptr);
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVSwapchain>(uid);

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

        else if (iid == &GUIDIIr77PVSwapchain)
            obj = std::shared_ptr<IIr77PVSwapchain>(shared_from_this(), static_cast<IIr77PVSwapchain*>(this));

        else if (iid == &GUIDIr77PVSwapchain)
            obj = std::shared_ptr<Ir77PVSwapchain>(shared_from_this(), static_cast<Ir77PVSwapchain*>(this));

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

    std::shared_ptr<IIr77Return const> SetRenderPass(std::shared_ptr<IIr77PVRenderPass> render_pass) {
        m_render_pass = render_pass;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CreateSurface(SDL_Window* window) {
        m_window = window;

        VkInstance instance;
        m_instance->GetInstance(&instance);

        if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &m_surface)) {
            std::string str{SDL_GetError()};
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: SDL_Vulkan_CreateSurface failed: " + str);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CleanupSwapchain() {
        VkDevice device;
        m_device->GetDevice(&device);

        for (auto framebuffer : m_swapchain_framebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        for (auto image_view : m_swapchain_views) {
            vkDestroyImageView(device, image_view, nullptr);
        }

        vkDestroySwapchainKHR(device, m_old_swapchain, nullptr);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> QuerySwapchainSupport() {
        VkPhysicalDevice phys_device;
        m_device->GetPhysicalDevice(&phys_device);

        if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys_device, m_surface, &m_capabilities) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed.");

        uint32_t format_count;
        if (vkGetPhysicalDeviceSurfaceFormatsKHR(phys_device, m_surface, &format_count, nullptr) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: vkGetPhysicalDeviceSurfaceFormatsKHR failed.");

        if (format_count != 0) {
            m_formats.resize(format_count);
            if (vkGetPhysicalDeviceSurfaceFormatsKHR(phys_device, m_surface, &format_count, m_formats.data()) != VK_SUCCESS)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: vkGetPhysicalDeviceSurfaceFormatsKHR failed.");
        }

        uint32_t present_mode_count;
        if (vkGetPhysicalDeviceSurfacePresentModesKHR(phys_device, m_surface, &present_mode_count, nullptr) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: vkGetPhysicalDeviceSurfacePresentModesKHR failed.");

        if (present_mode_count != 0) {
            m_present_modes.resize(present_mode_count);
            if (vkGetPhysicalDeviceSurfacePresentModesKHR(phys_device, m_surface, &present_mode_count, m_present_modes.data()) != VK_SUCCESS)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: vkGetPhysicalDeviceSurfacePresentModesKHR failed.");
        }

        if (m_formats.empty() || m_present_modes.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: Swap Chain not supported.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SwapSurfaceFormat() {
        for (const auto& format : m_formats) {
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                m_format = VK_FORMAT_B8G8R8A8_SRGB;
                m_color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
                break;
            }
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> PresentMode() {
        for (const auto& mode : m_present_modes) {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                m_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SurfaceCapabilities() {
        if (m_capabilities.currentExtent.width != UINT32_MAX) {
            m_swapchain_extent = m_capabilities.currentExtent;
        } else {
            int width, height;

            SDL_GetWindowSizeInPixels(m_window, &width, &height);

            VkExtent2D actual_extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

            m_swapchain_extent.width = std::clamp(actual_extent.width, m_capabilities.minImageExtent.width, m_capabilities.maxImageExtent.width);
            m_swapchain_extent.height = std::clamp(actual_extent.height, m_capabilities.minImageExtent.height, m_capabilities.maxImageExtent.height);
        }

        m_image_count = m_capabilities.minImageCount + 1;
        if (m_capabilities.maxImageCount > 0 && m_image_count > m_capabilities.maxImageCount) m_image_count = m_capabilities.maxImageCount;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineSwapchain() {
        VkDevice device;
        m_device->GetDevice(&device);

        m_old_swapchain = m_swapchain;

        m_swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        m_swapchain_info.surface = m_surface;
        m_swapchain_info.minImageCount = m_image_count;
        m_swapchain_info.imageFormat = m_format;
        m_swapchain_info.imageColorSpace = m_color_space;
        m_swapchain_info.imageExtent = m_swapchain_extent;
        m_swapchain_info.imageArrayLayers = 1;
        m_swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        m_swapchain_info.preTransform = m_capabilities.currentTransform;
        m_swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        m_swapchain_info.presentMode = m_present_mode;
        m_swapchain_info.clipped = VK_TRUE;
        if (m_swapchain == VK_NULL_HANDLE) {
            m_swapchain_info.oldSwapchain = VK_NULL_HANDLE;
        } else {
            m_swapchain_info.oldSwapchain = m_swapchain;
        }

        // add VK_SHARING_MODE_CONCURRENT when necessary
        m_swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateSwapchainKHR(device, &m_swapchain_info, nullptr, &m_swapchain) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: vkCreateSwapchainKHR failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> InitSwapchainImages() {
        VkDevice device;
        m_device->GetDevice(&device);

        std::uint32_t swapchain_image_count = 0;
        if (vkGetSwapchainImagesKHR(device, m_swapchain, &swapchain_image_count, nullptr) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: vkGetSwapchainImagesKHR failed.");

        m_swapchain_images.resize(swapchain_image_count);
        if (vkGetSwapchainImagesKHR(device, m_swapchain, &swapchain_image_count, m_swapchain_images.data()) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: vkGetSwapchainImagesKHR failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineImageView() {
        VkDevice device;
        m_device->GetDevice(&device);

        m_swapchain_views.resize(m_swapchain_images.size());

        for (uint32_t i = 0; i < m_swapchain_views.size(); ++i) {
            m_image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            m_image_view_create_info.image = m_swapchain_images[i];
            m_image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            m_image_view_create_info.format = m_format;
            m_image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            m_image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            m_image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            m_image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            m_image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            m_image_view_create_info.subresourceRange.baseMipLevel = 0;
            m_image_view_create_info.subresourceRange.levelCount = 1;
            m_image_view_create_info.subresourceRange.baseArrayLayer = 0;
            m_image_view_create_info.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device, &m_image_view_create_info, nullptr, &m_swapchain_views[i]) != VK_SUCCESS)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: vkCreateImageView failed at " + std::to_string(i));
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineFramebuffers() {
        VkDevice device;
        m_device->GetDevice(&device);

        VkRenderPass render_pass;
        m_render_pass->GetRenderPass(&render_pass);

        m_swapchain_framebuffers.resize(m_swapchain_views.size());

        for (size_t i = 0; i < m_swapchain_views.size(); i++) {
            VkImageView attachments[] = {m_swapchain_views[i]};

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = render_pass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = m_swapchain_extent.width;
            framebufferInfo.height = m_swapchain_extent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &m_swapchain_framebuffers[i]) != VK_SUCCESS) {
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: vkCreateFramebuffer");
            }
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetSwapchain(VkSwapchainKHR* swapchain) {
        *swapchain = m_swapchain;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetSwapchainExtents(VkExtent2D& swapchain_extent) {
        swapchain_extent = m_swapchain_extent;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetSwapchainViews(std::vector<VkImageView>& swapchain_views) {
        swapchain_views = m_swapchain_views;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetSwapchainFramebuffers(std::vector<VkFramebuffer>& swapchain_framebuffers) {
        swapchain_framebuffers = m_swapchain_framebuffers;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    std::shared_ptr<IIr77PVInstance> m_instance;

    std::shared_ptr<IIr77PVDevice> m_device;

    std::shared_ptr<IIr77PVRenderPass> m_render_pass;

    SDL_Window* m_window{nullptr};

    VkSurfaceKHR m_surface{VK_NULL_HANDLE};

    VkSurfaceCapabilitiesKHR m_capabilities{};

    std::vector<VkSurfaceFormatKHR> m_formats;

    VkFormat m_format{VK_FORMAT_B8G8R8A8_SRGB};

    VkColorSpaceKHR m_color_space{VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

    std::vector<VkPresentModeKHR> m_present_modes{};

    VkPresentModeKHR m_present_mode{VK_PRESENT_MODE_FIFO_KHR};

    VkExtent2D m_swapchain_extent{};

    VkSwapchainCreateInfoKHR m_swapchain_info{};

    VkSwapchainKHR m_swapchain{VK_NULL_HANDLE};

    VkSwapchainKHR m_old_swapchain{VK_NULL_HANDLE};

    VkImageViewCreateInfo m_image_view_create_info{};

    std::uint32_t m_image_count{0};

    std::vector<VkImageView> m_swapchain_views;

    std::vector<VkImage> m_swapchain_images;

    std::vector<VkFramebuffer> m_swapchain_framebuffers;
};

}  // namespace NSIr77PeregrineV