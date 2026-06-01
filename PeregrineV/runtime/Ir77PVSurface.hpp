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

    std::shared_ptr<IIr77Return const> InitSurfaceCapabilities() {
        VkPhysicalDevice phys_device = GetPhysicalDevice();
        SDL_Window* window = GetWindow();

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys_device, m_surface, &m_surface_capabilities);

        if (m_surface_capabilities.currentExtent.width != UINT32_MAX) {
            m_swapchain_extent = m_surface_capabilities.currentExtent;
        } else {
            int w, h;
            SDL_GetWindowSizeInPixels(window, &w, &h);
            m_swapchain_extent.width = static_cast<uint32_t>(w);
            m_swapchain_extent.height = static_cast<uint32_t>(h);
        }

        m_image_count = m_surface_capabilities.minImageCount + 1;
        if (m_surface_capabilities.maxImageCount > 0 && m_image_count > m_surface_capabilities.maxImageCount)
            m_image_count = m_surface_capabilities.maxImageCount;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> InitPresentMode() {
        VkPhysicalDevice phys_device = GetPhysicalDevice();

        m_present_mode_count = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(phys_device, m_surface, &m_present_mode_count, nullptr);

        m_present_modes.resize(m_present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(phys_device, m_surface, &m_present_mode_count, m_present_modes.data());

        m_present_mode = VK_PRESENT_MODE_FIFO_KHR;
        for (auto& m : m_present_modes) {
            if (m == VK_PRESENT_MODE_MAILBOX_KHR) {
                m_present_mode = m;
                break;
            }
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> InitSurfaceFormat() {
        VkPhysicalDevice phys_device = GetPhysicalDevice();

        m_format_count = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(phys_device, m_surface, &m_format_count, nullptr);

        m_surface_formats.resize(m_format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(phys_device, m_surface, &m_format_count, m_surface_formats.data());

        m_surface_format = m_surface_formats[0];
        for (auto& f : m_surface_formats) {
            if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                m_surface_format = f;
                break;
            }
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> InitSwapChainInfo() {
        std::uint32_t gfx_family = GetGraphicsFamily();
        std::uint32_t present_family = GetPresentFamily();

        m_swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        m_swapchain_info.surface = m_surface;
        m_swapchain_info.minImageCount = m_image_count;
        m_swapchain_info.imageFormat = m_surface_format.format;
        m_swapchain_info.imageColorSpace = m_surface_format.colorSpace;
        m_swapchain_info.imageExtent = m_swapchain_extent;
        m_swapchain_info.imageArrayLayers = 1;
        m_swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        m_swapchain_info.preTransform = m_surface_capabilities.currentTransform;
        m_swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        m_swapchain_info.presentMode = m_present_mode;
        m_swapchain_info.clipped = VK_TRUE;

        m_family_indices.push_back(gfx_family);
        m_family_indices.push_back(present_family);
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

    VkPhysicalDevice GetPhysicalDevice();

    VkDevice GetDevice();

    std::uint32_t GetGraphicsFamily();

    std::uint32_t GetPresentFamily();

   private:
    std::shared_ptr<IIr77Enlisted> m_context;

    VkSurfaceKHR m_surface{VK_NULL_HANDLE};

    VkSurfaceCapabilitiesKHR m_surface_capabilities{};

    std::uint32_t m_image_count = 0;

    VkExtent2D m_swapchain_extent{};

    std::uint32_t m_present_mode_count = 0;

    std::vector<VkPresentModeKHR> m_present_modes;

    VkPresentModeKHR m_present_mode{VK_PRESENT_MODE_FIFO_KHR};

    std::uint32_t m_format_count = 0;

    std::vector<VkSurfaceFormatKHR> m_surface_formats;

    VkSurfaceFormatKHR m_surface_format{};

    VkSwapchainCreateInfoKHR m_swapchain_info{};

    std::vector<std::uint32_t> m_family_indices;

    VkSwapchainKHR m_swapchain{VK_NULL_HANDLE};

    VkFormat m_swapchain_format{};

    std::uint32_t m_swapchain_image_count = 0;

    std::vector<VkImage> m_swapchain_images;

    VkImageViewCreateInfo m_image_view_create_info{};

    std::vector<VkImageView> m_swapchain_views;
};

}  // namespace NSIr77PeregrineV