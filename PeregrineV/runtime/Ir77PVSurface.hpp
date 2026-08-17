#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <chrono>
#include <memory>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"

#include "../interface/IIr77PVSurface.hpp"
#include "../interface/IIr77PVSurface.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77PVSurface : public Ir77Enlisted, public IIr77PVSurface, public std::enable_shared_from_this<Ir77PVSurface> {
   public:
    Ir77PVSurface() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }
        m_enlisted = std::chrono::system_clock::now();
    }

   public:
    std::shared_ptr<IIr77Return const> MemberOfUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVSurface>(uid);

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

        else if (iid == &GUIDIr77PVSurface)
            obj = std::shared_ptr<Ir77PVSurface>(shared_from_this(), static_cast<Ir77PVSurface*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context) {
        m_context = context;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CreateSurface() {
        SDL_Window* window = GetWindow();
        VkInstance instance = GetInstance();

        if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &m_surface)) {
            std::string str{SDL_GetError()};
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSurface: SDL_Vulkan_CreateSurface failed: " + str);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> QuerySwapChainSupport(std::uint32_t index, SwapChainSupportDetails& details) {
        VkPhysicalDevice phys_device = GetPhysicalDevice(index);
        SDL_Window* window = GetWindow();

        vkGetPhysicalDeviceProperties(phys_device, &details.device_properties);

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys_device, m_surface, &details.capabilities);

        uint32_t format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(phys_device, m_surface, &format_count, nullptr);

        if (format_count != 0) {
            details.formats.resize(format_count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(phys_device, m_surface, &format_count, details.formats.data());
        }

        uint32_t present_mode_count;
        vkGetPhysicalDeviceSurfacePresentModesKHR(phys_device, m_surface, &present_mode_count, nullptr);

        if (present_mode_count != 0) {
            details.present_modes.resize(present_mode_count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(phys_device, m_surface, &present_mode_count, details.present_modes.data());
        }

        if (details.formats.empty() || details.present_modes.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapChain: Swap Chain not supported.");

        m_details.emplace(index, details);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SwapSurfaceFormat(std::uint32_t index) {
        for (const auto& format : m_details[index].formats) {
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                m_details[index].format = VK_FORMAT_B8G8R8A8_SRGB;
                m_details[index].color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
                break;
            }
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> PresentMode(std::uint32_t index, bool& available) {
        for (const auto& mode : m_details[index].present_modes) {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                m_details[index].present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SurfaceCapabilities(std::uint32_t index) {
        auto capabilities = m_details[index].capabilities;

        if (capabilities.currentExtent.width != UINT32_MAX) {
            m_details[index].swapchain_extent = capabilities.currentExtent;
        } else {
            int width, height;

            SDL_GetWindowSizeInPixels(m_window, &width, &height);

            VkExtent2D actual_extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

            m_details[index].swapchain_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            m_details[index].swapchain_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        }

        m_details[index].imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && m_details[index].imageCount > capabilities.maxImageCount)
            m_details[index].imageCount = capabilities.maxImageCount;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> InitSwapChainInfo(std::uint32_t index) {
        std::uint32_t gfx_family = GetGraphicsFamily();
        std::uint32_t present_family = GetPresentFamily();

        auto const& details = m_details[index];

        m_swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        m_swapchain_info.surface = m_surface;
        m_swapchain_info.minImageCount = details.imageCount;
        m_swapchain_info.imageFormat = details.format;
        m_swapchain_info.imageColorSpace = details.color_space;
        m_swapchain_info.imageExtent = details.swapchain_extent;
        m_swapchain_info.imageArrayLayers = 1;
        m_swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        m_swapchain_info.preTransform = details.capabilities.currentTransform;
        m_swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        m_swapchain_info.presentMode = details.present_mode;
        m_swapchain_info.clipped = VK_TRUE;

        if (gfx_family != present_family) {
            m_swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            m_swapchain_info.queueFamilyIndexCount = 2;
            m_swapchain_info.pQueueFamilyIndices = m_family_indices.data();
        } else {
            m_swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CreateSwapchain() {
        VkDevice device = GetDevice();

        if (vkCreateSwapchainKHR(device, &m_swapchain_info, nullptr, &m_swapchain) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSurface: vkCreateSwapchainKHR failed.");

        m_swapchain_format = m_surface_format.format;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> InitSwapchainImages() {
        VkDevice device = GetDevice();

        m_swapchain_image_count = 0;
        vkGetSwapchainImagesKHR(device, m_swapchain, &m_swapchain_image_count, nullptr);

        m_swapchain_images.resize(m_swapchain_image_count);
        vkGetSwapchainImagesKHR(device, m_swapchain, &m_swapchain_image_count, m_swapchain_images.data());

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CreateImageView() {
        VkDevice device = GetDevice();

        m_swapchain_views.resize(m_swapchain_image_count);

        for (uint32_t i = 0; i < m_image_count; ++i) {
            m_image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            m_image_view_create_info.image = m_swapchain_images[i];
            m_image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            m_image_view_create_info.format = m_swapchain_format;
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
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSurface: vkCreateImageView failed at " + std::to_string(i));
        }
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetSurface(VkSurfaceKHR* surface) {
        *surface = m_surface;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    SDL_Window* GetWindow();

    VkInstance GetInstance();

    VkPhysicalDevice GetPhysicalDevice(std::uint32_t index);

    VkDevice GetDevice(std::uint32_t index);

    std::uint32_t GetGraphicsFamily();

    std::uint32_t GetPresentFamily();

   private:
    std::shared_ptr<IIr77Enlisted> m_context;

    VkSurfaceKHR m_surface{VK_NULL_HANDLE};

    std::map<std::uint32_t, SwapChainSupportDetails> m_details;

    /*****************************************************/

    VkSwapchainCreateInfoKHR m_swapchain_info{};

    VkSwapchainKHR m_swapchain{VK_NULL_HANDLE};

    VkFormat m_swapchain_format{};

    std::uint32_t m_swapchain_image_count = 0;

    std::vector<VkImage> m_swapchain_images;

    VkImageViewCreateInfo m_image_view_create_info{};

    std::vector<VkImageView> m_swapchain_views;
};

}  // namespace NSIr77PeregrineV