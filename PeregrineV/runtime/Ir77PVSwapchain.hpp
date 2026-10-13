#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>

#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77PVSwapchain.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// =============================================================================================================================================
// Ir77PVSwapchain -- one window's surface, swapchain, image views, shared depth image and Main-pass framebuffers.
//
//   create:    SetInstance / SetDevice / SetAllocator / SetRenderPass -> CreateSurface
//   build:     DefineSwapchainSupport -> DefineSwapSurfaceFormat -> DefinePresentMode -> DefineSurfaceCapabilities -> DefineSwapchain
//              -> DefineSwapchainImages -> DefineImageView -> DefineFramebuffers
//   rebuild:   the same sequence, with CleanupSwapchain after DefineSwapchain (it releases what the new swapchain replaced)
// =============================================================================================================================================
class Ir77PVSwapchain : public Ir77Enlisted, public IIr77PVSwapchain, public std::enable_shared_from_this<Ir77PVSwapchain> {
   public:
    Ir77PVSwapchain() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();

        // UINT32_MAX = "the window decides the extent" until the surface is queried
        m_capabilities.currentExtent.width = UINT32_MAX;
        m_capabilities.currentExtent.height = UINT32_MAX;
    }

    ~Ir77PVSwapchain() {
        if (!m_device) return;

        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        DestroyFramebuffers(device);

        DestroyDepth(device);

        DestroyViews(device);

        if (m_old_swapchain != VK_NULL_HANDLE) vkDestroySwapchainKHR(device, m_old_swapchain, nullptr);
        if (m_swapchain != VK_NULL_HANDLE) vkDestroySwapchainKHR(device, m_swapchain, nullptr);

        if (m_instance && m_surface != VK_NULL_HANDLE) {
            VkInstance instance{VK_NULL_HANDLE};
            m_instance->GetInstance(&instance);

            vkDestroySurfaceKHR(instance, m_surface, nullptr);
        }
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

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77PVSwapchain)
            obj = std::shared_ptr<IIr77PVSwapchain>(shared_from_this(), static_cast<IIr77PVSwapchain*>(this));

        else if (iid == GUIDIr77PVSwapchain)
            obj = std::shared_ptr<Ir77PVSwapchain>(shared_from_this(), static_cast<Ir77PVSwapchain*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // configuration
    // -------------------------------------------------------------------------------------------------------------------------------------
   public:
    std::shared_ptr<IIr77Return const> SetInstance(std::shared_ptr<IIr77PVInstance> instance) {
        m_instance = instance;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) {
        m_device = device;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetAllocator(VmaAllocator allocator) {
        m_allocator = allocator;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetRenderPass(std::shared_ptr<IIr77PVRenderPass> render_pass) {
        m_render_pass = render_pass;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CreateSurface(SDL_Window* window) {
        m_window = window;

        VkInstance instance{VK_NULL_HANDLE};
        m_instance->GetInstance(&instance);

        if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &m_surface)) {
            std::string const error{SDL_GetError()};
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: SDL_Vulkan_CreateSurface failed: " + error);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // build / rebuild
    // -------------------------------------------------------------------------------------------------------------------------------------
    // first build -- support, format, present mode, extent, swapchain, images, views, framebuffers
    std::shared_ptr<IIr77Return const> CreateSwapchain() { return Build(false); }

    // after OUT_OF_DATE / SUBOPTIMAL -- the same sequence, releasing what the new swapchain replaced
    std::shared_ptr<IIr77Return const> RebuildSwapchain() { return Build(true); }

    // surface format only -- for render passes created before the swapchain
    std::shared_ptr<IIr77Return const> QuerySurfaceFormat(VkSurfaceFormatKHR& surface_format) {
        auto result = DefineSwapchainSupport();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        result = DefineSwapSurfaceFormat();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        return GetSurfaceFormat(surface_format);
    }

   private:
    std::shared_ptr<IIr77Return const> Build(bool const& rebuild) {
        auto result = DefineSwapchainSupport();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        result = DefineSwapSurfaceFormat();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        result = DefinePresentMode();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        result = DefineSurfaceCapabilities();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        result = DefineSwapchain();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        if (rebuild) CleanupSwapchain();

        result = DefineSwapchainImages();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        result = DefineImageView();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        result = DefineFramebuffers();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        return Ir77RETURN<Ir77OperationSucceeded>(this, rebuild ? "Success: Rebuild Swapchain." : "Success: Create Swapchain.");
    }

    // Releases the framebuffers, depth image and views of the previous build, and the swapchain DefineSwapchain replaced
    std::shared_ptr<IIr77Return const> CleanupSwapchain() {
        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        DestroyFramebuffers(device);

        DestroyDepth(device);

        DestroyViews(device);

        if (m_old_swapchain != VK_NULL_HANDLE) vkDestroySwapchainKHR(device, m_old_swapchain, nullptr);
        m_old_swapchain = VK_NULL_HANDLE;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineSwapchainSupport() {
        VkPhysicalDevice phys_device{VK_NULL_HANDLE};
        m_device->GetPhysicalDevice(&phys_device);

        if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys_device, m_surface, &m_capabilities) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: vkGetPhysicalDeviceDefineSurfaceCapabilitiesKHR failed.");

        std::uint32_t format_count{0};
        if (vkGetPhysicalDeviceSurfaceFormatsKHR(phys_device, m_surface, &format_count, nullptr) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: vkGetPhysicalDeviceSurfaceFormatsKHR failed.");

        m_formats.resize(format_count);
        if (format_count > 0 && vkGetPhysicalDeviceSurfaceFormatsKHR(phys_device, m_surface, &format_count, m_formats.data()) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: vkGetPhysicalDeviceSurfaceFormatsKHR failed.");

        std::uint32_t present_mode_count{0};
        if (vkGetPhysicalDeviceSurfacePresentModesKHR(phys_device, m_surface, &present_mode_count, nullptr) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: vkGetPhysicalDeviceSurfacePresentModesKHR failed.");

        m_present_modes.resize(present_mode_count);
        if (present_mode_count > 0 &&
            vkGetPhysicalDeviceSurfacePresentModesKHR(phys_device, m_surface, &present_mode_count, m_present_modes.data()) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: vkGetPhysicalDeviceSurfacePresentModesKHR failed.");

        if (m_formats.empty() || m_present_modes.empty())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: surface has no formats or present modes.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // B8G8R8A8_SRGB + SRGB_NONLINEAR when offered, otherwise the surface's first format
    std::shared_ptr<IIr77Return const> DefineSwapSurfaceFormat() {
        if (m_formats.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: no surface formats -- call DefineSwapchainSupport first.");

        m_format = m_formats[0].format;
        m_color_space = m_formats[0].colorSpace;

        for (auto const& format : m_formats) {
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                m_format = format.format;
                m_color_space = format.colorSpace;
                break;
            }
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // MAILBOX when offered, otherwise FIFO (always available)
    std::shared_ptr<IIr77Return const> DefinePresentMode() {
        m_present_mode = VK_PRESENT_MODE_FIFO_KHR;

        for (auto const& mode : m_present_modes) {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                m_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Extent and image count from the capabilities DefineSwapchainSupport read
    std::shared_ptr<IIr77Return const> DefineSurfaceCapabilities() {
        ChooseExtent();

        m_image_count = m_capabilities.minImageCount + 1;
        if (m_capabilities.maxImageCount > 0 && m_image_count > m_capabilities.maxImageCount) m_image_count = m_capabilities.maxImageCount;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineSwapchain() {
        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        VkPhysicalDevice phys_device{VK_NULL_HANDLE};
        m_device->GetPhysicalDevice(&phys_device);

        // the window can resize between DefineSwapchainSupport and here (live drag) -- take the extent as late as possible
        if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys_device, m_surface, &m_capabilities) == VK_SUCCESS) ChooseExtent();

        // minimized / zero-size window -- no swapchain can exist; the next present retries
        if (m_swapchain_extent.width == 0 || m_swapchain_extent.height == 0)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: zero-size surface, swapchain not created.");

        VkSwapchainCreateInfoKHR swapchain_info{};
        swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchain_info.surface = m_surface;
        swapchain_info.minImageCount = m_image_count;
        swapchain_info.imageFormat = m_format;
        swapchain_info.imageColorSpace = m_color_space;
        swapchain_info.imageExtent = m_swapchain_extent;
        swapchain_info.imageArrayLayers = 1;
        swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;  // CONCURRENT once graphics and present use different families
        swapchain_info.preTransform = m_capabilities.currentTransform;
        swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchain_info.presentMode = m_present_mode;
        swapchain_info.clipped = VK_TRUE;
        swapchain_info.oldSwapchain = m_swapchain;  // VK_NULL_HANDLE on the first build

        VkSwapchainKHR swapchain{VK_NULL_HANDLE};
        if (vkCreateSwapchainKHR(device, &swapchain_info, nullptr, &swapchain) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: vkCreateSwapchainKHR failed.");

        // the replaced swapchain is released by CleanupSwapchain
        m_old_swapchain = m_swapchain;
        m_swapchain = swapchain;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineSwapchainImages() {
        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        std::uint32_t image_count{0};
        if (vkGetSwapchainImagesKHR(device, m_swapchain, &image_count, nullptr) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: vkGetSwapchainImagesKHR failed.");

        m_swapchain_images.resize(image_count);
        if (vkGetSwapchainImagesKHR(device, m_swapchain, &image_count, m_swapchain_images.data()) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: vkGetSwapchainImagesKHR failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineImageView() {
        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        m_swapchain_views.assign(m_swapchain_images.size(), VK_NULL_HANDLE);

        for (std::size_t i = 0; i < m_swapchain_images.size(); i++) {
            VkImageViewCreateInfo view_info{};
            view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            view_info.image = m_swapchain_images[i];
            view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view_info.format = m_format;
            view_info.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
            view_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

            if (vkCreateImageView(device, &view_info, nullptr, &m_swapchain_views[i]) != VK_SUCCESS)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: vkCreateImageView failed at " + std::to_string(i) + ".");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Attachment order matches the Main render pass: [0] color (per swapchain image), [1] depth (shared)
    std::shared_ptr<IIr77Return const> DefineFramebuffers() {
        if (m_allocator == VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: SetAllocator not called -- the depth image needs it.");
        if (!m_render_pass) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: SetRenderPass not called.");

        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        VkRenderPass render_pass{VK_NULL_HANDLE};
        m_render_pass->GetRenderPass(&render_pass);

        if (!DefineDepth(device)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: depth image creation failed.");

        m_swapchain_framebuffers.assign(m_swapchain_views.size(), VK_NULL_HANDLE);

        for (std::size_t i = 0; i < m_swapchain_views.size(); i++) {
            std::array<VkImageView, 2> const attachments{m_swapchain_views[i], m_depth_view};

            VkFramebufferCreateInfo framebuffer_info{};
            framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_info.renderPass = render_pass;
            framebuffer_info.attachmentCount = static_cast<std::uint32_t>(attachments.size());
            framebuffer_info.pAttachments = attachments.data();
            framebuffer_info.width = m_swapchain_extent.width;
            framebuffer_info.height = m_swapchain_extent.height;
            framebuffer_info.layers = 1;

            if (vkCreateFramebuffer(device, &framebuffer_info, nullptr, &m_swapchain_framebuffers[i]) != VK_SUCCESS)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: vkCreateFramebuffer failed at " + std::to_string(i) + ".");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   public:
    // -------------------------------------------------------------------------------------------------------------------------------------
    // queries
    // -------------------------------------------------------------------------------------------------------------------------------------
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

    std::shared_ptr<IIr77Return const> GetSurfaceFormat(VkSurfaceFormatKHR& surface_format) {
        surface_format.format = m_format;
        surface_format.colorSpace = m_color_space;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // internals
    // -------------------------------------------------------------------------------------------------------------------------------------
   private:
    // currentExtent when the surface fixes it; otherwise the window's pixel size, clamped to what the surface allows
    void ChooseExtent() {
        if (m_capabilities.currentExtent.width != UINT32_MAX) {
            m_swapchain_extent = m_capabilities.currentExtent;
            return;
        }

        int width{0}, height{0};
        SDL_GetWindowSizeInPixels(m_window, &width, &height);

        m_swapchain_extent.width = std::clamp(static_cast<std::uint32_t>(width), m_capabilities.minImageExtent.width, m_capabilities.maxImageExtent.width);
        m_swapchain_extent.height = std::clamp(static_cast<std::uint32_t>(height), m_capabilities.minImageExtent.height, m_capabilities.maxImageExtent.height);
    }

    // One depth image per swapchain, sized to the current extent, shared by every framebuffer
    bool DefineDepth(VkDevice device) {
        DestroyDepth(device);

        VkFormat depth_format{VK_FORMAT_UNDEFINED};
        m_render_pass->GetDepthFormat(&depth_format);
        if (depth_format == VK_FORMAT_UNDEFINED) return false;

        VkImageCreateInfo image_info{};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.format = depth_format;
        image_info.extent = {m_swapchain_extent.width, m_swapchain_extent.height, 1};
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo alloc_info{};
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
        alloc_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

        if (vmaCreateImage(m_allocator, &image_info, &alloc_info, &m_depth_image, &m_depth_allocation, nullptr) != VK_SUCCESS) return false;

        VkImageViewCreateInfo view_info{};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = m_depth_image;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = depth_format;
        view_info.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};

        return vkCreateImageView(device, &view_info, nullptr, &m_depth_view) == VK_SUCCESS;
    }

    void DestroyDepth(VkDevice device) {
        if (m_depth_view != VK_NULL_HANDLE) vkDestroyImageView(device, m_depth_view, nullptr);
        if (m_depth_image != VK_NULL_HANDLE) vmaDestroyImage(m_allocator, m_depth_image, m_depth_allocation);

        m_depth_view = VK_NULL_HANDLE;
        m_depth_image = VK_NULL_HANDLE;
        m_depth_allocation = VK_NULL_HANDLE;
    }

    void DestroyFramebuffers(VkDevice device) {
        for (auto const framebuffer : m_swapchain_framebuffers)
            if (framebuffer != VK_NULL_HANDLE) vkDestroyFramebuffer(device, framebuffer, nullptr);

        m_swapchain_framebuffers.clear();
    }

    void DestroyViews(VkDevice device) {
        for (auto const view : m_swapchain_views)
            if (view != VK_NULL_HANDLE) vkDestroyImageView(device, view, nullptr);

        m_swapchain_views.clear();
    }

   private:
    std::shared_ptr<IIr77PVInstance> m_instance{};

    std::shared_ptr<IIr77PVDevice> m_device{};

    std::shared_ptr<IIr77PVRenderPass> m_render_pass{};

    VmaAllocator m_allocator{VK_NULL_HANDLE};

    SDL_Window* m_window{nullptr};

    VkSurfaceKHR m_surface{VK_NULL_HANDLE};

    VkSurfaceCapabilitiesKHR m_capabilities{};

    std::vector<VkSurfaceFormatKHR> m_formats{};

    VkFormat m_format{VK_FORMAT_B8G8R8A8_SRGB};

    VkColorSpaceKHR m_color_space{VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

    std::vector<VkPresentModeKHR> m_present_modes{};

    VkPresentModeKHR m_present_mode{VK_PRESENT_MODE_FIFO_KHR};

    VkExtent2D m_swapchain_extent{};

    std::uint32_t m_image_count{0};

    VkSwapchainKHR m_swapchain{VK_NULL_HANDLE};

    VkSwapchainKHR m_old_swapchain{VK_NULL_HANDLE};

    std::vector<VkImage> m_swapchain_images{};

    std::vector<VkImageView> m_swapchain_views{};

    VkImage m_depth_image{VK_NULL_HANDLE};

    VmaAllocation m_depth_allocation{VK_NULL_HANDLE};

    VkImageView m_depth_view{VK_NULL_HANDLE};

    std::vector<VkFramebuffer> m_swapchain_framebuffers{};
};

}  // namespace NSIr77PeregrineV