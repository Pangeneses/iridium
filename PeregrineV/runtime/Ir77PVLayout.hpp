#pragma once

#include <vulkan/vulkan.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

#include "../types/Ir77PVTypes.hpp"

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77PVLayout.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// =============================================================================================================================================
// Ir77PVLayout -- THE pipeline layout. One per device, shared by every graphics, compute and ray pipeline.
//
//   set 0, one bindless set:
//     binding 0  sampler2D  textures[]        -- every sampled texture (material maps, env, shadow map, CEF overlay, plot colour maps)
//     binding 1  buffer     buffers[]         -- every storage buffer (frame globals, instances, materials, bones, vertices, meshlets...)
//     binding 2  image2D    storage_images[]  -- compute / ray outputs
//     binding 3  accelerationStructureEXT[]   -- the TLASes ray queries trace (ray-query devices only -- the binding doesn't exist otherwise)
//   push constants: Ir77PVPushConstants (128 bytes, every stage) -- indices into the arrays above, per draw / dispatch
//
//   create: CreateLayout -> DefineCapacity -> DefineSetLayout -> DefinePipelineLayout
//
// Shaders see the same thing through Shader/include/bindless.glsl. A new shader needs no new layout -- it reads different indices.
// =============================================================================================================================================
class Ir77PVLayout : public Ir77Enlisted, public IIr77PVLayout, public std::enable_shared_from_this<Ir77PVLayout> {
   public:
    Ir77PVLayout() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    // the owner waits for the device to go idle before releasing layouts
    ~Ir77PVLayout() {
        if (!m_device) return;

        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        if (m_pipeline_layout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, m_pipeline_layout, nullptr);
        if (m_set_layout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, m_set_layout, nullptr);
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVLayout>(uid);

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

        else if (iid == GUIDIIr77PVLayout)
            obj = std::shared_ptr<IIr77PVLayout>(shared_from_this(), static_cast<IIr77PVLayout*>(this));

        else if (iid == GUIDIr77PVLayout)
            obj = std::shared_ptr<Ir77PVLayout>(shared_from_this(), static_cast<Ir77PVLayout*>(this));

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

    // -------------------------------------------------------------------------------------------------------------------------------------
    // create
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> CreateLayout() {
        if (!m_device) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayout: SetDevice not called.");
        if (m_pipeline_layout != VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayout: layout already created.");

        auto result = DefineCapacity();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        result = DefineSetLayout();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        result = DefinePipelineLayout();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Bindless Layout.");
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // queries
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> GetPipelineLayout(VkPipelineLayout* pipeline_layout) {
        *pipeline_layout = m_pipeline_layout;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetDescriptorSetLayout(VkDescriptorSetLayout* set_layout) {
        *set_layout = m_set_layout;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Array sizes actually used -- the requested BINDLESS_* sizes clamped to what the device allows
    std::shared_ptr<IIr77Return const> GetCapacity(Ir77PVBindlessCapacity& capacity) {
        capacity = m_capacity;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // steps
    // -------------------------------------------------------------------------------------------------------------------------------------
   private:
    // Clamp each array to the device's update-after-bind limits, per stage and per set
    std::shared_ptr<IIr77Return const> DefineCapacity() {
        VkPhysicalDeviceVulkan12Properties limits{};
        m_device->GetVulkan12Properties(limits);

        m_capacity.textures = std::min({BINDLESS_TEXTURES, limits.maxPerStageDescriptorUpdateAfterBindSampledImages,
                                        limits.maxDescriptorSetUpdateAfterBindSampledImages});

        m_capacity.buffers = std::min({BINDLESS_BUFFERS, limits.maxPerStageDescriptorUpdateAfterBindStorageBuffers,
                                       limits.maxDescriptorSetUpdateAfterBindStorageBuffers});

        m_capacity.storage_images = std::min({BINDLESS_STORAGE_IMAGES, limits.maxPerStageDescriptorUpdateAfterBindStorageImages,
                                              limits.maxDescriptorSetUpdateAfterBindStorageImages});

        if (m_capacity.textures == 0 || m_capacity.buffers == 0 || m_capacity.storage_images == 0)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayout: device reports no update-after-bind descriptor capacity.");

        // acceleration structures -- ray-query devices only; 0 leaves binding 3 out of the layout
        bool ray_query{false};
        m_device->GetRayQuery(ray_query);

        m_capacity.acceleration_structures = 0;

        if (ray_query) {
            VkPhysicalDeviceAccelerationStructurePropertiesKHR acceleration{};
            m_device->GetAccelerationStructureProperties(acceleration);

            m_capacity.acceleration_structures = std::min({BINDLESS_ACCELERATION_STRUCTURES, acceleration.maxPerStageDescriptorUpdateAfterBindAccelerationStructures,
                                                           acceleration.maxDescriptorSetUpdateAfterBindAccelerationStructures});
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Partially bound: unwritten slots are legal as long as no shader reads them.
    // Update after bind + unused while pending: slots can be written while frames using other slots are in flight.
    std::shared_ptr<IIr77Return const> DefineSetLayout() {
        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        std::vector<VkDescriptorSetLayoutBinding> bindings{};

        bindings.push_back({BINDLESS_BINDING_TEXTURES, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_capacity.textures, VK_SHADER_STAGE_ALL, nullptr});

        bindings.push_back({BINDLESS_BINDING_BUFFERS, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_capacity.buffers, VK_SHADER_STAGE_ALL, nullptr});

        bindings.push_back({BINDLESS_BINDING_STORAGE_IMAGES, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, m_capacity.storage_images, VK_SHADER_STAGE_ALL, nullptr});

        if (m_capacity.acceleration_structures > 0)
            bindings.push_back({BINDLESS_BINDING_ACCELERATION, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, m_capacity.acceleration_structures,
                                VK_SHADER_STAGE_ALL, nullptr});

        VkDescriptorBindingFlags const binding_flag{VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
                                                    VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT};

        std::vector<VkDescriptorBindingFlags> binding_flags(bindings.size(), binding_flag);

        VkDescriptorSetLayoutBindingFlagsCreateInfo flags_info{};
        flags_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        flags_info.bindingCount = static_cast<std::uint32_t>(binding_flags.size());
        flags_info.pBindingFlags = binding_flags.data();

        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.pNext = &flags_info;
        layout_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
        layout_info.bindingCount = static_cast<std::uint32_t>(bindings.size());
        layout_info.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &m_set_layout) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayout: vkCreateDescriptorSetLayout failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // One set, one push-constant range visible to every stage
    std::shared_ptr<IIr77Return const> DefinePipelineLayout() {
        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        VkPushConstantRange push_range{};
        push_range.stageFlags = VK_SHADER_STAGE_ALL;
        push_range.offset = 0;
        push_range.size = sizeof(Ir77PVPushConstants);

        VkPipelineLayoutCreateInfo pipeline_layout_info{};
        pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_info.setLayoutCount = 1;
        pipeline_layout_info.pSetLayouts = &m_set_layout;
        pipeline_layout_info.pushConstantRangeCount = 1;
        pipeline_layout_info.pPushConstantRanges = &push_range;

        if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &m_pipeline_layout) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVLayout: vkCreatePipelineLayout failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    std::shared_ptr<IIr77PVDevice> m_device{};

    Ir77PVBindlessCapacity m_capacity{};

    VkDescriptorSetLayout m_set_layout{VK_NULL_HANDLE};

    VkPipelineLayout m_pipeline_layout{VK_NULL_HANDLE};
};

}  // namespace NSIr77PeregrineV