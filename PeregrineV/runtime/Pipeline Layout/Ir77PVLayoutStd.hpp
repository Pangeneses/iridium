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

#include "../../interface/IIr77PVLayout.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77PVLayoutStd : public Ir77Enlisted, public IIr77PVLayout, public std::enable_shared_from_this<Ir77PVLayoutStd> {
   public:
    Ir77PVLayoutStd() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    ~Ir77PVLayoutStd() { 
        VkDevice device;
        m_device->GetDevice(&device);
        
        vkDestroyPipelineLayout(device, m_pipeline_layout, nullptr); 
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberOfUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVLayoutStd>(uid);

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

        else if (iid == &GUIDIIr77PVLayout)
            obj = std::shared_ptr<IIr77PVLayout>(shared_from_this(), static_cast<IIr77PVLayout*>(this));

        else if (iid == &GUIDIr77PVLayoutStd)
            obj = std::shared_ptr<Ir77PVLayoutStd>(shared_from_this(), static_cast<Ir77PVLayoutStd*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) {
        m_device = device;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CreatePipelineLayout() {
        VkDevice device;
        m_device->GetDevice(&device);

        m_pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        m_pipeline_layout_info.setLayoutCount = 0;                      // Optional
        m_pipeline_layout_info.pSetLayouts = nullptr;                   // Optional
        m_pipeline_layout_info.pushConstantRangeCount = 0;     // Optional
        m_pipeline_layout_info.pPushConstantRanges = nullptr;  // Optional

        if (vkCreatePipelineLayout(device, &m_pipeline_layout_info, nullptr, &m_pipeline_layout) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: VkPipelineLayout failed.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetPipelineLayout(VkPipelineLayout* pipeline_layout) {

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    std::shared_ptr<IIr77PVDevice> m_device;

    std::uint64_t m_device_id{UINT64_MAX};
    
    VkPipelineLayoutCreateInfo m_pipeline_layout_info{};

    VkPipelineLayout m_pipeline_layout;
};
}  // namespace NSIr77PeregrineV