#pragma once

#include <SDL3/SDL_video.h>
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77PVDevice.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77PVDevice : public Ir77Enlisted, public IIr77PVDevice, public std::enable_shared_from_this<Ir77PVDevice> {
   public:
    Ir77PVDevice() {
        m_enlisted_uuid.Generate();

        m_enlisted = std::chrono::system_clock::now();
    }

    ~Ir77PVDevice() {
        if (m_upload_pool != VK_NULL_HANDLE) vkDestroyCommandPool(m_device, m_upload_pool, nullptr);
        if (m_transfer_pool != VK_NULL_HANDLE) vkDestroyCommandPool(m_device, m_transfer_pool, nullptr);
        if (m_device != VK_NULL_HANDLE) vkDestroyDevice(m_device, nullptr);
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVDevice>(uid);

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

        else if (iid == GUIDIIr77PVDevice)
            obj = std::shared_ptr<IIr77PVDevice>(shared_from_this(), static_cast<IIr77PVDevice*>(this));

        else if (iid == GUIDIr77PVDevice)
            obj = std::shared_ptr<Ir77PVDevice>(shared_from_this(), static_cast<Ir77PVDevice*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> SetInstance(std::shared_ptr<IIr77PVInstance> instance) {
        m_instance = instance;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> EnumeratePhysicalDevices(std::uint32_t const& device_index) {
        VkInstance instance;
        m_instance->GetInstance(&instance);

        std::uint32_t physical_device_count = 0;
        vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr);

        if (physical_device_count == 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDevice: no Vulkan devices found.");

        std::vector<VkPhysicalDevice> phys_devices(physical_device_count);
        vkEnumeratePhysicalDevices(instance, &physical_device_count, phys_devices.data());

        if (device_index >= physical_device_count) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDevice: device index out of range.");

        m_phys_device = phys_devices[device_index];

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    /************************************************************************************************************************************************************/

    std::shared_ptr<IIr77Return const> DefineQueueFamilyProps(SDL_Window* window) {
        VkInstance instance;
        m_instance->GetInstance(&instance);

        VkSurfaceKHR surface;
        if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface)) {
            std::string str{SDL_GetError()};
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDevice: SDL_Vulkan_CreateSurface failed: " + str);
        }

        std::uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_phys_device, &queue_family_count, nullptr);

        m_queue_families.resize(queue_family_count);

        std::vector<VkQueueFamilyProperties> fp(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(m_phys_device, &queue_family_count, fp.data());

        for (std::uint32_t i = 0; i < queue_family_count; ++i) {
            m_queue_families[i].family_properties = fp[i];

            if (fp[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) m_queue_families[i].type = IR77_GRAPHICS_BIT | m_queue_families[i].type;
            if (fp[i].queueFlags & VK_QUEUE_COMPUTE_BIT) m_queue_families[i].type = IR77_COMPUTE_BIT | m_queue_families[i].type;
            if (fp[i].queueFlags & VK_QUEUE_TRANSFER_BIT) m_queue_families[i].type = IR77_TRANSFER_BIT | m_queue_families[i].type;
            if (fp[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) m_queue_families[i].type = IR77_SPARSE_BIT | m_queue_families[i].type;
            if (fp[i].queueFlags & VK_QUEUE_PROTECTED_BIT) m_queue_families[i].type = IR77_PROTECTED_BIT | m_queue_families[i].type;
            if (fp[i].queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) m_queue_families[i].type = IR77_DECODE_BIT | m_queue_families[i].type;
            if (fp[i].queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR) m_queue_families[i].type = IR77_ENCODE_BIT | m_queue_families[i].type;

            VkBool32 present_support = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(m_phys_device, i, surface, &present_support);

            bool const graphics = (m_queue_families[i].type & IR77_GRAPHICS_BIT) == IR77_GRAPHICS_BIT;
            m_queue_families[i].presentation = (graphics && present_support == VK_TRUE) ? VK_TRUE : VK_FALSE;
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineQueueCreateInfos() {
        for (std::size_t i = 0; i < m_queue_families.size(); i++) {
            VkDeviceQueueCreateInfo queue_create_info{};
            queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.queueFamilyIndex = static_cast<std::uint32_t>(i);
            queue_create_info.queueCount = 1;
            queue_create_info.pQueuePriorities = &m_queue_priority;  // member -- must outlive vkCreateDevice
            m_queue_families[i].create_info = queue_create_info;
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    /************************************************************************************************************************************************************/

    // Builds the extension list from what the GPU actually exposes. Required ones fail the call; optional ones are added only when present.
    std::shared_ptr<IIr77Return const> QueryDeviceExtensionSupport() {
        std::uint32_t count = 0;
        vkEnumerateDeviceExtensionProperties(m_phys_device, nullptr, &count, nullptr);

        m_available_extensions.resize(count);
        vkEnumerateDeviceExtensionProperties(m_phys_device, nullptr, &count, m_available_extensions.data());

        if (!HasExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDevice: swapchain extension not supported.");

        m_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

        // optional -- kept only if DefineDeviceInfo also finds the features it needs
        if (HasExtension(VK_EXT_MESH_SHADER_EXTENSION_NAME)) m_extensions.push_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);

        // optional -- ray queries need all three (acceleration structures, the query itself, and the deferred host operations the first
        // depends on). Requested together or not at all.
        if (HasExtension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) && HasExtension(VK_KHR_RAY_QUERY_EXTENSION_NAME) &&
            HasExtension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME)) {
            for (char const* name : RAY_QUERY_EXTENSIONS) m_extensions.push_back(name);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // display suitability, before any logical device exists: VK_KHR_swapchain + a graphics family that can present to an SDL window.
    // rank 4 discrete, 3 integrated, 2 virtual, 1 other, 0 CPU; fails (rank -1) when the device can't drive a display
    std::shared_ptr<IIr77Return const> QueryDisplaySupport(int& rank, std::string& name) {
        rank = -1;
        name.clear();

        if (m_phys_device == VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDevice: no physical device.");

        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(m_phys_device, &props);
        name = props.deviceName;

        std::uint32_t count{0};
        vkEnumerateDeviceExtensionProperties(m_phys_device, nullptr, &count, nullptr);
        m_available_extensions.resize(count);
        vkEnumerateDeviceExtensionProperties(m_phys_device, nullptr, &count, m_available_extensions.data());

        if (!HasExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDevice: " + name + " has no swapchain support.");

        VkInstance instance{VK_NULL_HANDLE};
        m_instance->GetInstance(&instance);

        std::uint32_t family_count{0};
        vkGetPhysicalDeviceQueueFamilyProperties(m_phys_device, &family_count, nullptr);
        std::vector<VkQueueFamilyProperties> families(family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(m_phys_device, &family_count, families.data());

        bool display{false};
        for (std::uint32_t f = 0; f < family_count && !display; f++)
            display = (families[f].queueFlags & VK_QUEUE_GRAPHICS_BIT) && SDL_Vulkan_GetPresentationSupport(instance, m_phys_device, f);

        if (!display) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDevice: " + name + " cannot present to a window.");

        switch (props.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                rank = 4;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                rank = 3;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                rank = 2;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                rank = 1;
                break;
            default:
                rank = 0;
                break;  // CPU (llvmpipe, lavapipe)
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    /******************************************************************************************************************************************************************************/

    // Features go through VkPhysicalDeviceFeatures2 in pNext, so extension feature structs can be chained after it
    std::shared_ptr<IIr77Return const> DefineDeviceInfo() {
        if (m_extensions.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDevice: QueryDeviceExtensionSupport not called.");

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(m_phys_device, &properties);

        if (properties.apiVersion < VK_API_VERSION_1_3) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDevice: Vulkan 1.3 required (dynamic rendering).");

        // query: features2 -> mesh -> acceleration structure -> ray query -> vulkan11 -> vulkan12 -> vulkan13
        VkPhysicalDeviceVulkan13Features v13_supported{};
        v13_supported.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

        VkPhysicalDeviceVulkan12Features v12_supported{};
        v12_supported.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        v12_supported.pNext = &v13_supported;

        VkPhysicalDeviceVulkan11Features v11_supported{};
        v11_supported.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        v11_supported.pNext = &v12_supported;

        VkPhysicalDeviceRayQueryFeaturesKHR ray_query_supported{};
        ray_query_supported.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
        ray_query_supported.pNext = &v11_supported;

        VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_supported{};
        acceleration_supported.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        acceleration_supported.pNext = &ray_query_supported;

        VkPhysicalDeviceMeshShaderFeaturesEXT mesh_supported{};
        mesh_supported.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
        mesh_supported.pNext = &acceleration_supported;

        VkPhysicalDeviceFeatures2 supported{};
        supported.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        supported.pNext = &mesh_supported;
        vkGetPhysicalDeviceFeatures2(m_phys_device, &supported);

        if (!supported.features.multiDrawIndirect) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDevice: multiDrawIndirect not supported.");
        if (!supported.features.drawIndirectFirstInstance) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDevice: drawIndirectFirstInstance not supported.");

        // gl_DrawID -- the vertex shaders find each indirect command's material through it (per-draw material tables)
        if (!v11_supported.shaderDrawParameters) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDevice: shaderDrawParameters not supported.");

        // bindless -- one descriptor set of large, partially bound, update-after-bind arrays (Ir77PVLayout)
        bool const bindless = v12_supported.runtimeDescriptorArray && v12_supported.descriptorBindingPartiallyBound &&
                              v12_supported.descriptorBindingUpdateUnusedWhilePending && v12_supported.shaderSampledImageArrayNonUniformIndexing &&
                              v12_supported.shaderStorageBufferArrayNonUniformIndexing && v12_supported.shaderStorageImageArrayNonUniformIndexing &&
                              v12_supported.descriptorBindingSampledImageUpdateAfterBind && v12_supported.descriptorBindingStorageBufferUpdateAfterBind &&
                              v12_supported.descriptorBindingStorageImageUpdateAfterBind;

        if (!bindless) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDevice: descriptor indexing (bindless) not supported.");

        // dynamic rendering (no VkRenderPass / framebuffers) + vkCmdPipelineBarrier2
        if (!v13_supported.dynamicRendering || !v13_supported.synchronization2 || !v13_supported.maintenance4 || !v13_supported.shaderDemoteToHelperInvocation)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDevice: dynamicRendering / synchronization2 / maintenance4 / demote not supported.");

        m_phys_device_features = VkPhysicalDeviceFeatures{};
        m_phys_device_features.samplerAnisotropy = supported.features.samplerAnisotropy;
        m_phys_device_features.geometryShader = supported.features.geometryShader;
        m_phys_device_features.multiDrawIndirect = supported.features.multiDrawIndirect;
        m_phys_device_features.independentBlend = supported.features.independentBlend;
        m_phys_device_features.drawIndirectFirstInstance = supported.features.drawIndirectFirstInstance;

        // mesh shading -- extension present, both stages supported, and gl_DrawID available; otherwise the extension is dropped entirely
        auto const mesh_ext = FindRequested(VK_EXT_MESH_SHADER_EXTENSION_NAME);

        m_mesh_shading = mesh_ext != m_extensions.end() && mesh_supported.meshShader == VK_TRUE && mesh_supported.taskShader == VK_TRUE &&
                         v11_supported.shaderDrawParameters == VK_TRUE;

        if (!m_mesh_shading && mesh_ext != m_extensions.end()) m_extensions.erase(mesh_ext);

        m_mesh_features = VkPhysicalDeviceMeshShaderFeaturesEXT{};
        m_mesh_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
        m_mesh_features.taskShader = m_mesh_shading ? VK_TRUE : VK_FALSE;
        m_mesh_features.meshShader = m_mesh_shading ? VK_TRUE : VK_FALSE;
        m_mesh_features.pNext = nullptr;

        // buffer device address -- a buffer's GPU address, which acceleration-structure builds read geometry through. Enabled whenever
        // the GPU has it (core in 1.2); the allocator is told so it can hand out addressable memory.
        m_buffer_device_address = v12_supported.bufferDeviceAddress == VK_TRUE;

        // ray queries -- the three extensions requested, both features supported, and buffer device address; otherwise all three are dropped
        bool const ray_extensions = std::all_of(std::begin(RAY_QUERY_EXTENSIONS), std::end(RAY_QUERY_EXTENSIONS),
                                                [this](char const* name) { return FindRequested(name) != m_extensions.end(); });

        // descriptorBindingAccelerationStructureUpdateAfterBind -- the TLAS lives in the bindless set (binding 3), which is update-after-bind
        m_ray_query = ray_extensions && acceleration_supported.accelerationStructure == VK_TRUE && ray_query_supported.rayQuery == VK_TRUE &&
                      acceleration_supported.descriptorBindingAccelerationStructureUpdateAfterBind == VK_TRUE && m_buffer_device_address;

        if (!m_ray_query) {
            for (char const* name : RAY_QUERY_EXTENSIONS) {
                auto const found = FindRequested(name);
                if (found != m_extensions.end()) m_extensions.erase(found);
            }
        }

        m_acceleration_features = VkPhysicalDeviceAccelerationStructureFeaturesKHR{};
        m_acceleration_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        m_acceleration_features.accelerationStructure = m_ray_query ? VK_TRUE : VK_FALSE;
        m_acceleration_features.descriptorBindingAccelerationStructureUpdateAfterBind = m_ray_query ? VK_TRUE : VK_FALSE;

        m_ray_query_features = VkPhysicalDeviceRayQueryFeaturesKHR{};
        m_ray_query_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
        m_ray_query_features.rayQuery = m_ray_query ? VK_TRUE : VK_FALSE;

        // enable: create info -> features2 -> vulkan11 -> vulkan12 -> vulkan13 -> [mesh] -> [acceleration structure -> ray query]
        m_vulkan11 = VkPhysicalDeviceVulkan11Features{};
        m_vulkan11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        m_vulkan11.shaderDrawParameters = VK_TRUE;  // required above
        m_vulkan11.pNext = &m_vulkan12;

        m_vulkan12 = VkPhysicalDeviceVulkan12Features{};
        m_vulkan12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        m_vulkan12.runtimeDescriptorArray = VK_TRUE;
        m_vulkan12.descriptorBindingPartiallyBound = VK_TRUE;
        m_vulkan12.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
        m_vulkan12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
        m_vulkan12.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
        m_vulkan12.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;
        m_vulkan12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
        m_vulkan12.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
        m_vulkan12.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
        m_vulkan12.bufferDeviceAddress = m_buffer_device_address ? VK_TRUE : VK_FALSE;
        m_vulkan12.pNext = &m_vulkan13;

        m_vulkan13 = VkPhysicalDeviceVulkan13Features{};
        m_vulkan13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        m_vulkan13.dynamicRendering = VK_TRUE;
        m_vulkan13.synchronization2 = VK_TRUE;
        m_vulkan13.maintenance4 = VK_TRUE;                    // LocalSizeId -- task / mesh / compute built for vulkan1.3
        m_vulkan13.shaderDemoteToHelperInvocation = VK_TRUE;  // discard in vulkan1.3 SPIR-V
        m_vulkan13.pNext = nullptr;

        // the optional structs hang off the end of the chain, each only when it's on
        void** tail = &m_vulkan13.pNext;

        if (m_mesh_shading) {
            *tail = &m_mesh_features;
            tail = &m_mesh_features.pNext;
        }

        if (m_ray_query) {
            *tail = &m_acceleration_features;
            m_acceleration_features.pNext = &m_ray_query_features;
            tail = &m_ray_query_features.pNext;
        }

        *tail = nullptr;

        m_features2 = VkPhysicalDeviceFeatures2{};
        m_features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        m_features2.features = m_phys_device_features;
        m_features2.pNext = &m_vulkan11;

        m_create_info.clear();
        for (std::size_t i = 0; i < m_queue_families.size(); i++) m_create_info.push_back(m_queue_families[i].create_info);

        m_device_create_info = VkDeviceCreateInfo{};
        m_device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        m_device_create_info.pNext = &m_features2;
        m_device_create_info.pEnabledFeatures = nullptr;  // must be null when features come through VkPhysicalDeviceFeatures2
        m_device_create_info.queueCreateInfoCount = static_cast<std::uint32_t>(m_create_info.size());
        m_device_create_info.pQueueCreateInfos = m_create_info.data();
        m_device_create_info.enabledExtensionCount = static_cast<std::uint32_t>(m_extensions.size());
        m_device_create_info.ppEnabledExtensionNames = m_extensions.data();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineDevice() {
        if (m_device != VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDevice: DefineDevice called twice -- device already exists.");

        if (vkCreateDevice(m_phys_device, &m_device_create_info, nullptr, &m_device) != VK_SUCCESS) {
            m_device = VK_NULL_HANDLE;
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDevice: vkCreateDevice failed.");
        }

        for (std::size_t i = 0; i < m_queue_families.size(); i++) {
            vkGetDeviceQueue(m_device, m_queue_families[i].create_info.queueFamilyIndex, 0, &m_queue_families[i].queue);
        }

        // properties: vulkan12 -> [acceleration structure] -- the latter's alignments size the BLAS / TLAS builds
        m_acceleration_properties = VkPhysicalDeviceAccelerationStructurePropertiesKHR{};
        m_acceleration_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;

        m_vulkan12_properties = VkPhysicalDeviceVulkan12Properties{};
        m_vulkan12_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
        m_vulkan12_properties.pNext = m_ray_query ? &m_acceleration_properties : nullptr;

        VkPhysicalDeviceProperties2 properties2{};
        properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        properties2.pNext = &m_vulkan12_properties;

        vkGetPhysicalDeviceProperties2(m_phys_device, &properties2);

        m_phys_device_properties = properties2.properties;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    /************************************************************************************************************************************************************/

    std::shared_ptr<IIr77Return const> GetPhysicalDevice(VkPhysicalDevice* phys_device) {
        *phys_device = m_phys_device;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetDeviceProperties(VkPhysicalDeviceProperties& phys_device_props) {
        phys_device_props = m_phys_device_properties;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Core features that were enabled (not merely supported)
    std::shared_ptr<IIr77Return const> GetDeviceFeatures(VkPhysicalDeviceFeatures& phys_device_features) {
        phys_device_features = m_phys_device_features;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // true only when VK_EXT_mesh_shader is enabled with task + mesh stages
    std::shared_ptr<IIr77Return const> GetMeshShading(bool& enabled) {
        enabled = m_mesh_shading;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // true only when VK_KHR_ray_query and VK_KHR_acceleration_structure are enabled with their features (and buffer device address)
    std::shared_ptr<IIr77Return const> GetRayQuery(bool& enabled) {
        enabled = m_ray_query;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // true when buffers can be created with VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT (the allocator is created to match)
    std::shared_ptr<IIr77Return const> GetBufferDeviceAddress(bool& enabled) {
        enabled = m_buffer_device_address;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // acceleration-structure limits (scratch alignment, max geometry / instance counts) -- zeros when ray queries are off
    std::shared_ptr<IIr77Return const> GetAccelerationStructureProperties(VkPhysicalDeviceAccelerationStructurePropertiesKHR& properties) {
        properties = m_acceleration_properties;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // descriptor-indexing limits -- Ir77PVLayout sizes the bindless arrays from these
    std::shared_ptr<IIr77Return const> GetVulkan12Properties(VkPhysicalDeviceVulkan12Properties& properties) {
        properties = m_vulkan12_properties;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetDevice(VkDevice* device) {
        *device = m_device;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetQueueFamilies(std::vector<Ir77PVQueueFamily>& queue_family) {
        queue_family = m_queue_families;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // first graphics family that can present -- valid after DefineDevice
    std::shared_ptr<IIr77Return const> GetPresentQueue(VkQueue& queue, std::uint32_t& family) {
        for (std::size_t i = 0; i < m_queue_families.size(); i++) {
            if (m_queue_families[i].presentation == VK_TRUE) {
                queue = m_queue_families[i].queue;
                family = static_cast<std::uint32_t>(i);
                return Ir77RETURN<Ir77OperationSucceeded>();
            }
        }

        return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDevice: no presentation queue.");
    }

    // transient pool on the present family for one-shot uploads -- created on first use, destroyed with the device
    std::shared_ptr<IIr77Return const> GetUploadPool(VkCommandPool& pool, VkQueue& queue) {
        std::uint32_t family{0};

        auto const result = GetPresentQueue(queue, family);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        if (m_upload_pool == VK_NULL_HANDLE) {
            VkCommandPoolCreateInfo pool_info{};
            pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            pool_info.queueFamilyIndex = family;

            if (vkCreateCommandPool(m_device, &pool_info, nullptr, &m_upload_pool) != VK_SUCCESS)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDevice: upload pool creation failed.");
        }

        pool = m_upload_pool;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // first transfer-only family (the copy engine -- runs beside rendering) -- valid after DefineDevice. A GPU without one gets the present
    // queue: compare family with GetPresentQueue's to know whether a queue-family ownership transfer is needed.
    std::shared_ptr<IIr77Return const> GetTransferQueue(VkQueue& queue, std::uint32_t& family) {
        for (std::size_t i = 0; i < m_queue_families.size(); i++) {
            std::uint64_t const type = m_queue_families[i].type;

            bool const copy_only = (type & IR77_TRANSFER_BIT) && !(type & IR77_GRAPHICS_BIT) && !(type & IR77_COMPUTE_BIT);

            if (copy_only && m_queue_families[i].queue != VK_NULL_HANDLE) {
                queue = m_queue_families[i].queue;
                family = static_cast<std::uint32_t>(i);
                return Ir77RETURN<Ir77OperationSucceeded>();
            }
        }

        return GetPresentQueue(queue, family);
    }

    // transient pool on the transfer family, for streaming uploads -- its own pool even when it falls back to the present family, so it
    // never shares a pool with the upload path (pools aren't thread-safe). Created on first use, destroyed with the device.
    std::shared_ptr<IIr77Return const> GetTransferPool(VkCommandPool& pool, VkQueue& queue) {
        std::uint32_t family{0};

        auto const result = GetTransferQueue(queue, family);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        if (m_transfer_pool == VK_NULL_HANDLE) {
            VkCommandPoolCreateInfo pool_info{};
            pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            pool_info.queueFamilyIndex = family;

            if (vkCreateCommandPool(m_device, &pool_info, nullptr, &m_transfer_pool) != VK_SUCCESS)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDevice: transfer pool creation failed.");
        }

        pool = m_transfer_pool;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    bool HasExtension(char const* name) const {
        return std::any_of(m_available_extensions.begin(), m_available_extensions.end(),
                           [name](VkExtensionProperties const& e) { return std::strcmp(e.extensionName, name) == 0; });
    }

    std::vector<char const*>::iterator FindRequested(char const* name) {
        return std::find_if(m_extensions.begin(), m_extensions.end(), [name](char const* e) { return std::strcmp(e, name) == 0; });
    }

   private:
    std::shared_ptr<IIr77PVInstance> m_instance;

    VkPhysicalDevice m_phys_device{VK_NULL_HANDLE};

    VkPhysicalDeviceProperties m_phys_device_properties{};

    VkPhysicalDeviceFeatures m_phys_device_features{};

    std::vector<Ir77PVQueueFamily> m_queue_families;

    std::vector<VkDeviceQueueCreateInfo> m_create_info;

    float m_queue_priority{1.0f};

    VkDevice m_device{VK_NULL_HANDLE};

    VkCommandPool m_upload_pool{VK_NULL_HANDLE};

    VkCommandPool m_transfer_pool{VK_NULL_HANDLE};  // GetTransferPool -- streaming uploads, on the copy engine when there is one

    std::vector<VkExtensionProperties> m_available_extensions;

    // requested extensions -- pointers into this list are handed to vkCreateDevice, so it must outlive DefineDevice
    std::vector<char const*> m_extensions{};

    // feature chain -- members for the same reason
    VkPhysicalDeviceFeatures2 m_features2{};

    VkPhysicalDeviceMeshShaderFeaturesEXT m_mesh_features{};

    VkPhysicalDeviceVulkan11Features m_vulkan11{};

    VkPhysicalDeviceVulkan12Features m_vulkan12{};

    VkPhysicalDeviceVulkan13Features m_vulkan13{};

    VkPhysicalDeviceVulkan12Properties m_vulkan12_properties{};

    VkPhysicalDeviceAccelerationStructureFeaturesKHR m_acceleration_features{};

    VkPhysicalDeviceRayQueryFeaturesKHR m_ray_query_features{};

    VkPhysicalDeviceAccelerationStructurePropertiesKHR m_acceleration_properties{};

    bool m_mesh_shading{false};

    bool m_ray_query{false};  // acceleration structures + ray queries enabled

    bool m_buffer_device_address{false};

    // requested together or not at all -- acceleration structures need deferred host operations; ray queries need acceleration structures
    static constexpr std::array<char const*, 3> RAY_QUERY_EXTENSIONS{VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, VK_KHR_RAY_QUERY_EXTENSION_NAME,
                                                                     VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME};

    VkDeviceCreateInfo m_device_create_info{};
};
}  // namespace NSIr77PeregrineV