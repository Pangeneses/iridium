#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"
#include "../../Ir77RT/dictionary/IDIr77RET.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77PVDevice.hpp"
#include "../interface/IIr77PVLayout.hpp"
#include "../interface/IIr77PVDescriptorSet.hpp"

#include "Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77PVDescriptorSet : public Ir77Enlisted, public IIr77PVDescriptorSet, public std::enable_shared_from_this<Ir77PVDescriptorSet> {
   public:
    Ir77PVDescriptorSet() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    ~Ir77PVDescriptorSet() = default;

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVDescriptorSet>(uid);

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

        else if (iid == GUIDIIr77PVDescriptorSet)
            obj = std::shared_ptr<IIr77PVDescriptorSet>(shared_from_this(), static_cast<IIr77PVDescriptorSet*>(this));

        else if (iid == GUIDIr77PVDescriptorSet)
            obj = std::shared_ptr<Ir77PVDescriptorSet>(shared_from_this(), static_cast<Ir77PVDescriptorSet*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) {
        m_device = device;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetLayout(std::shared_ptr<IIr77PVLayout> layout, std::uint32_t const& set_index) {
        m_layout = layout;
        m_set_index = set_index;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // copies = frames in flight for per-frame sets (global, pass, bones); 1 for static sets (material)
    std::shared_ptr<IIr77Return const> CreateSets(std::uint32_t const& copies) {
        if (!m_layout) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDescriptorSet: layout not set.");
        if (copies == 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDescriptorSet: copy count is zero.");

        m_sets.assign(copies, VK_NULL_HANDLE);

        for (std::uint32_t i = 0; i < copies; i++) {
            if (m_layout->AllocateSet(m_set_index, &m_sets[i])->ID() != GUIDIr77OperationSucceeded) {
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDescriptorSet: set allocation failed.");
            }
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // UNIFORM_BUFFER / STORAGE_BUFFER -- resolves to the buffer's copy matching each set copy
    std::shared_ptr<IIr77Return const> BindBuffer(std::uint32_t const& binding, VkDescriptorType const& type, std::shared_ptr<IIr77PVBuffer> buffer) {
        if (!buffer) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDescriptorSet: buffer is empty.");

        Ir77PVDescriptorBinding entry{};
        entry.binding = binding;
        entry.type = type;
        entry.buffer = buffer;
        entry.is_image = false;

        m_bindings[binding] = entry;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // COMBINED_IMAGE_SAMPLER -- same image for every copy; fill from the texture class's GetImageInfo
    std::shared_ptr<IIr77Return const> BindImage(std::uint32_t const& binding, VkDescriptorType const& type, VkDescriptorImageInfo const& image) {
        Ir77PVDescriptorBinding entry{};
        entry.binding = binding;
        entry.type = type;
        entry.image = image;
        entry.is_image = true;

        m_bindings[binding] = entry;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Writes every binding into every copy. Call after all Bind* calls, and again after a rebind.
    // Don't call while any copy may still be in use by the GPU -- use WriteCopy for the current frame instead.
    std::shared_ptr<IIr77Return const> Write() {
        for (std::uint32_t i = 0; i < m_sets.size(); i++) {
            auto result = WriteCopy(i);
            if (result->ID() != GUIDIr77OperationSucceeded) return result;
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Writes every binding into one copy -- safe for the frame whose fence has signalled
    std::shared_ptr<IIr77Return const> WriteCopy(std::uint32_t const& copy) {
        if (m_sets.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDescriptorSet: CreateSets not called.");
        if (copy >= m_sets.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDescriptorSet: copy index out of range.");
        if (m_bindings.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDescriptorSet: no bindings.");

        VkDevice device;
        m_device->GetDevice(&device);

        // reserved up front so the pointers in writes stay valid until vkUpdateDescriptorSets
        std::vector<VkDescriptorBufferInfo> buffer_infos{};
        std::vector<VkDescriptorImageInfo> image_infos{};
        std::vector<VkWriteDescriptorSet> writes{};

        buffer_infos.reserve(m_bindings.size());
        image_infos.reserve(m_bindings.size());
        writes.reserve(m_bindings.size());

        for (auto const& [binding, entry] : m_bindings) {
            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = m_sets[copy];
            write.dstBinding = binding;
            write.dstArrayElement = 0;
            write.descriptorType = entry.type;
            write.descriptorCount = 1;

            if (entry.is_image) {
                image_infos.push_back(entry.image);
                write.pImageInfo = &image_infos.back();
            } else {
                VkDescriptorBufferInfo info{};
                if (entry.buffer->GetBufferInfo(copy, &info)->ID() != GUIDIr77OperationSucceeded) {
                    return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDescriptorSet: buffer info failed.");
                }

                buffer_infos.push_back(info);
                write.pBufferInfo = &buffer_infos.back();
            }

            writes.push_back(write);
        }

        vkUpdateDescriptorSets(device, static_cast<std::uint32_t>(writes.size()), writes.data(), 0, nullptr);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Single-copy sets ignore the index so callers can always pass the frame index
    std::shared_ptr<IIr77Return const> GetSet(std::uint32_t const& copy, VkDescriptorSet* set) {
        std::uint32_t const index = (m_sets.size() == 1) ? 0 : copy;
        if (index >= m_sets.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVDescriptorSet: copy index out of range.");

        *set = m_sets[index];

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetSetIndex(std::uint32_t* set_index) {
        *set_index = m_set_index;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetCopies(std::uint32_t* copies) {
        *copies = static_cast<std::uint32_t>(m_sets.size());

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    std::shared_ptr<IIr77PVDevice> m_device;

    std::shared_ptr<IIr77PVLayout> m_layout;

    std::uint32_t m_set_index{0};

    std::map<std::uint32_t, Ir77PVDescriptorBinding> m_bindings{};

    std::vector<VkDescriptorSet> m_sets{};
};
}  // namespace NSIr77PeregrineV