#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <chrono>
#include <memory>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../../dictionary/IDIIr77PeregrineV.hpp"
#include "../../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"

#include "../../interface/IIr77PVSwapchain.hpp"
#include "../../interface/IIr77PeregrineV.hpp"

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
        std::vector<Ir77PVDeviceInfo> device_infos = GetDeviceInfos();

        for (int i = 0; i < m_swapchain.swapchain_views.size(); i++) {
            vkDestroyImageView(device_infos[i].device, m_swapchain.swapchain_views[i], nullptr);
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
    std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context, std::uint32_t const& device_index) {
        m_context = context;

        m_device_index = device_index;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> BindWindow() {
        CreateSurface();

        QuerySwapchainSupport();

        SwapSurfaceFormat();

        PresentMode();

        SurfaceCapabilities();

        InitSwapchainInfo();

        CreateSwapchain();

        InitSwapchainImages();

        CreateImageView();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CreateSurface() {
        SDL_Window* window = GetWindowInfos().at(m_device_index).window;
        VkInstance instance = GetInstance();

        if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &m_swapchain.surface)) {
            std::string str{SDL_GetError()};
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: SDL_Vulkan_CreateSurface failed: " + str);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> QuerySwapchainSupport() {
        SDL_Window* window = GetWindowInfos().at(m_device_index).window;
        VkPhysicalDevice phys_device = GetDeviceInfos().at(m_device_index).phys_device;
        VkSurfaceKHR surface = m_swapchain.surface;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys_device, surface, &m_swapchain.capabilities);

        uint32_t format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(phys_device, surface, &format_count, nullptr);

        if (format_count != 0) {
            m_swapchain.formats.resize(format_count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(phys_device, surface, &format_count, m_swapchain.formats.data());
        }

        uint32_t present_mode_count;
        vkGetPhysicalDeviceSurfacePresentModesKHR(phys_device, surface, &present_mode_count, nullptr);

        if (present_mode_count != 0) {
            m_swapchain.present_modes.resize(present_mode_count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(phys_device, surface, &present_mode_count, m_swapchain.present_modes.data());
        }

        if (m_swapchain.formats.empty() || m_swapchain.present_modes.empty())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: Swap Chain not supported.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SwapSurfaceFormat() {
        for (const auto& format : m_swapchain.formats) {
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                m_swapchain.format = VK_FORMAT_B8G8R8A8_SRGB;
                m_swapchain.color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
                break;
            }
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> PresentMode() {
        for (const auto& mode : m_swapchain.present_modes) {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                m_swapchain.present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SurfaceCapabilities() {
        SDL_Window* window = GetWindowInfos().at(m_device_index).window;

        auto capabilities = m_swapchain.capabilities;

        if (capabilities.currentExtent.width != UINT32_MAX) {
            m_swapchain.swapchain_extent = capabilities.currentExtent;
        } else {
            int width, height;

            SDL_GetWindowSizeInPixels(window, &width, &height);

            VkExtent2D actual_extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

            m_swapchain.swapchain_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            m_swapchain.swapchain_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        }

        m_swapchain.imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && m_swapchain.imageCount > capabilities.maxImageCount) m_swapchain.imageCount = capabilities.maxImageCount;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> InitSwapchainInfo() {
        Ir77PVQueueInfo queue_info = GetQueueInfos().at(m_device_index);

        m_swapchain.swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        m_swapchain.swapchain_info.surface = m_swapchain.surface;
        m_swapchain.swapchain_info.minImageCount = m_swapchain.imageCount;
        m_swapchain.swapchain_info.imageFormat = m_swapchain.format;
        m_swapchain.swapchain_info.imageColorSpace = m_swapchain.color_space;
        m_swapchain.swapchain_info.imageExtent = m_swapchain.swapchain_extent;
        m_swapchain.swapchain_info.imageArrayLayers = 1;
        m_swapchain.swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        m_swapchain.swapchain_info.preTransform = m_swapchain.capabilities.currentTransform;
        m_swapchain.swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        m_swapchain.swapchain_info.presentMode = m_swapchain.present_mode;
        m_swapchain.swapchain_info.clipped = VK_TRUE;
        m_swapchain.swapchain_info.oldSwapchain = VK_NULL_HANDLE;

        // add VK_SHARING_MODE_CONCURRENT when necessary
        m_swapchain.swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CreateSwapchain() {
        VkDevice device = GetDeviceInfos().at(m_device_index).device;

        if (vkCreateSwapchainKHR(device, &m_swapchain.swapchain_info, nullptr, &m_swapchain.swapchain) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: vkCreateSwapchainKHR failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> InitSwapchainImages() {
        VkDevice device = GetDeviceInfos().at(m_device_index).device;

        std::uint32_t swapchain_image_count = 0;
        vkGetSwapchainImagesKHR(device, m_swapchain.swapchain, &swapchain_image_count, nullptr);

        m_swapchain.swapchain_images.resize(swapchain_image_count);
        vkGetSwapchainImagesKHR(device, m_swapchain.swapchain, &swapchain_image_count, m_swapchain.swapchain_images.data());

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CreateImageView() {
        VkDevice device = GetDeviceInfos().at(m_device_index).device;

        m_swapchain.swapchain_views.resize(m_swapchain.swapchain_images.size());

        for (uint32_t i = 0; i < m_swapchain.swapchain_views.size(); ++i) {
            m_swapchain.image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            m_swapchain.image_view_create_info.image = m_swapchain.swapchain_images[i];
            m_swapchain.image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            m_swapchain.image_view_create_info.format = m_swapchain.format;
            m_swapchain.image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            m_swapchain.image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            m_swapchain.image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            m_swapchain.image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            m_swapchain.image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            m_swapchain.image_view_create_info.subresourceRange.baseMipLevel = 0;
            m_swapchain.image_view_create_info.subresourceRange.levelCount = 1;
            m_swapchain.image_view_create_info.subresourceRange.baseArrayLayer = 0;
            m_swapchain.image_view_create_info.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device, &m_swapchain.image_view_create_info, nullptr, &m_swapchain.swapchain_views[i]) != VK_SUCCESS)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: vkCreateImageView failed at " + std::to_string(i));
        }
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetSwapchainInfo(Ir77PVSwapchainInfo& swapchain_info) {
        swapchain_info = m_swapchain;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    std::uint32_t CurrentDevice();

    VkInstance GetInstance();

    std::vector<Ir77PVWindowInfo> GetWindowInfos();

    std::vector<Ir77PVDeviceInfo> GetDeviceInfos();

    std::vector<Ir77PVQueueInfo> GetQueueInfos();

   private:
    std::shared_ptr<IIr77Enlisted> m_context;

    std::uint32_t m_device_index{UINT32_MAX};

    Ir77PVSwapchainInfo m_swapchain;
};

}  // namespace NSIr77PeregrineV