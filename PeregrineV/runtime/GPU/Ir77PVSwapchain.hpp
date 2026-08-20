#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <chrono>
#include <memory>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../../dictionary/IDIIr77PeregrineV.hpp"
#include "../../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"

#include "../../interface/IIr77PVSwapchain.hpp"
#include "../../interface/IIr77PVQueue.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

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
    }

    ~Ir77PVSwapchain() {
        VkDevice device = GetDevice();

        for (auto imageView : m_swapchain_views) {
            vkDestroyImageView(device, imageView, nullptr);
        }
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberOfUuid(std::shared_ptr<IIr77GUID const>& uid) const {
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
    std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context) {
        m_context = context;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CreateSurface() {
        SDL_Window* window = GetWindow();
        VkInstance instance = GetInstance();

        if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &m_surface)) {
            std::string str{SDL_GetError()};
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: SDL_Vulkan_CreateSurface failed: " + str);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> QuerySwapchainSupport(SwapchainSupportDetails& details) {
        std::int32_t index = CurrentDevice();
        VkPhysicalDevice phys_device = GetPhysicalDevice();
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

        if (details.formats.empty() || details.present_modes.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: Swap Chain not supported.");

        m_details.emplace(index, details);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SwapSurfaceFormat() {
        std::int32_t index = CurrentDevice();

        for (const auto& format : m_details[index].formats) {
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                m_details[index].format = VK_FORMAT_B8G8R8A8_SRGB;
                m_details[index].color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
                break;
            }
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> PresentMode(bool& available) {
        std::int32_t index = CurrentDevice();

        for (const auto& mode : m_details[index].present_modes) {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                m_details[index].present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SurfaceCapabilities() {
        std::int32_t index = CurrentDevice();

        SDL_Window* window = GetWindow();

        auto capabilities = m_details[index].capabilities;

        if (capabilities.currentExtent.width != UINT32_MAX) {
            m_details[index].swapchain_extent = capabilities.currentExtent;
        } else {
            int width, height;

            SDL_GetWindowSizeInPixels(window, &width, &height);

            VkExtent2D actual_extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

            m_details[index].swapchain_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            m_details[index].swapchain_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        }

        m_details[index].imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && m_details[index].imageCount > capabilities.maxImageCount)
            m_details[index].imageCount = capabilities.maxImageCount;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> InitSwapchainInfo() {
        std::int32_t index = CurrentDevice();

        std::vector<Ir77PVQueueFamily> queue_families = GetQueueFamilies();

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
        m_swapchain_info.oldSwapchain = VK_NULL_HANDLE;

        // add VK_SHARING_MODE_CONCURRENT when necessary
        m_swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CreateSwapchain() {
        VkDevice device = GetDevice();

        if (vkCreateSwapchainKHR(device, &m_swapchain_info, nullptr, &m_swapchain) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: vkCreateSwapchainKHR failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> InitSwapchainImages() {
        VkDevice device = GetDevice();

        std::uint32_t swapchain_image_count = 0;
        vkGetSwapchainImagesKHR(device, m_swapchain, &swapchain_image_count, nullptr);

        m_swapchain_images.resize(swapchain_image_count);
        vkGetSwapchainImagesKHR(device, m_swapchain, &swapchain_image_count, m_swapchain_images.data());

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CreateImageView() {
        std::int32_t index = CurrentDevice();
        VkDevice device = GetDevice();

        m_swapchain_views.resize(m_swapchain_images.size());

        for (uint32_t i = 0; i < m_swapchain_views.size(); ++i) {
            m_image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            m_image_view_create_info.image = m_swapchain_images[i];
            m_image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            m_image_view_create_info.format = m_details.at(index).format;
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

    std::shared_ptr<IIr77Return const> GetSurface(VkSurfaceKHR* surface) {
        *surface = m_surface;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetSwapchainSupportDetails(SwapchainSupportDetails& details) {
        std::int32_t index = CurrentDevice();

        details = m_details.at(index);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetImage(VkImage* image) {
        std::int32_t index = CurrentDevice();
        
        *image = m_swapchain_images.at(index);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    std::uint32_t CurrentDevice();

    SDL_Window* GetWindow();

    VkInstance GetInstance();

    VkPhysicalDevice GetPhysicalDevice();

    VkDevice GetDevice();

    std::vector<Ir77PVQueueFamily> GetQueueFamilies();

   private:
    std::shared_ptr<IIr77Enlisted> m_context;

    VkSurfaceKHR m_surface{VK_NULL_HANDLE};

    std::map<std::uint32_t, SwapchainSupportDetails> m_details;

    VkSwapchainCreateInfoKHR m_swapchain_info{};

    VkSwapchainKHR m_swapchain{VK_NULL_HANDLE};

    VkImageViewCreateInfo m_image_view_create_info{};

    std::vector<VkImageView> m_swapchain_views;

    std::vector<VkImage> m_swapchain_images;
};

}  // namespace NSIr77PeregrineV