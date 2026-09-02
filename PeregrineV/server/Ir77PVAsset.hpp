#pragma once

#include <SDL3/SDL_video.h>

#include <map>
#include <memory>

#include "../runtime/Ir77PVTypes.hpp"

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "IDIr77PVContext.hpp"
#include "IIr77PVAsset.hpp"

#include "IIr77PVShader.hpp"
#include "Ir77PeregrineV.hpp"

#include "../runtime/Buffer/Ir77PVBufferVertex.hpp"
#include "../runtime/Buffer/Ir77PVBufferUBO.hpp"
#include "../runtime/Assets/Ir77PVShader.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77PVAsset : public Ir77Enlisted, public IIr77PVAsset, public std::enable_shared_from_this<Ir77PVAsset> {
   public:
    Ir77PVAsset() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVAsset>(uid);

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

        else if (iid == &GUIDIIr77PVAsset)
            obj = std::shared_ptr<IIr77PVAsset>(shared_from_this(), static_cast<IIr77PVAsset*>(this));

        else if (iid == &GUIDIr77PVAsset)
            obj = std::shared_ptr<Ir77PVAsset>(shared_from_this(), static_cast<Ir77PVAsset*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> SetPeregrineV(std::shared_ptr<Ir77PeregrineV>& context) {
        m_context = context;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> UploadVertexBuffer(std::vector<Ir77PVInputBuffer> buffers) {
        std::vector<SDL_Window*> windows = m_context->m_windows.at(m_context->m_current_device);

        if (windows.size() > 8) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: too many windows.");

        std::vector<std::shared_ptr<Ir77PVBufferVertex>> vertex_buffers;
        for (int i = 0; i < windows.size(); i++) {
            auto vertex_buffer = std::make_shared<Ir77PVBufferVertex>();

            vertex_buffer->SetDevice(m_context->m_devices.at(m_context->m_current_device));

            vertex_buffer->SetAllocator(m_context->m_allocators.at(m_context->m_current_device));

            vertex_buffer->UploadVertices(buffers.at(i).vertex_data.data(), static_cast<VkDeviceSize>(buffers.at(i).vertex_data.size() * sizeof(Ir77PVVertex)));

            vertex_buffer->UploadIndices(buffers.at(i).index_data.data(), static_cast<VkDeviceSize>(buffers.at(i).index_data.size() * sizeof(std::uint32_t)),
                                         buffers.at(i).index_data.size(), VK_INDEX_TYPE_UINT32);

            vertex_buffers.push_back(vertex_buffer);
        }

        m_context->m_buffer_vertex.emplace(m_context->m_current_device, vertex_buffers);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> UpdateBuffersUBO(std::vector<void*> data, std::vector<VkDeviceSize> size) {
        for (int i = 0; i < m_context->m_buffer_ubo.at(m_context->m_current_device).size(); i++) {
            uint32_t frame_index = m_context->m_current_frames.at(m_context->m_current_device).at(i);

            m_context->m_buffer_ubo.at(m_context->m_current_device).at(i)->UpdateUBO(frame_index, data.at(i), size.at(i));
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CreateBuffersUBO() {
        std::vector<SDL_Window*> windows = m_context->m_windows.at(m_context->m_current_device);

        if (windows.size() > 8) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: too many windows.");

        std::vector<std::shared_ptr<Ir77PVBufferUBO>> ubo_buffers;
        for (int i = 0; i < windows.size(); i++) {
            auto ubo_buffer = std::make_shared<Ir77PVBufferUBO>();

            ubo_buffer->SetDevice(m_context->m_devices.at(m_context->m_current_device));

            ubo_buffer->SetAllocator(m_context->m_allocators.at(m_context->m_current_device));

            ubo_buffer->SetLayoutUBO(m_context->m_layouts_ubo.at(m_context->m_current_device));

            ubo_buffer->CreateResources(sizeof(Ir77PVCameraUBO), MAX_FRAMES_IN_FLIGHT);

            ubo_buffers.push_back(ubo_buffer);
        }

        m_context->m_buffer_ubo.emplace(m_context->m_current_device, ubo_buffers);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CreateShaders() {
        auto shader = std::static_pointer_cast<IIr77PVShader>(std::make_shared<Ir77PVShader>());

        shader->SetDevice(m_context->m_devices.at(m_context->m_current_device));

        shader->LoadShader("/home/alpha/workspace/iridium/Shader/vert.spv", ID_SHADER_VERT, Ir77PVShaderStage::Vertex);

        shader->LoadShader("/home/alpha/workspace/iridium/Shader/frag.spv", ID_SHADER_FRAG, Ir77PVShaderStage::Fragment);

        shader->LoadShader("/home/alpha/workspace/iridium/Shader/cef.vert.spv", ID_SHADER_CEF_VERT, Ir77PVShaderStage::Vertex);

        shader->LoadShader("/home/alpha/workspace/iridium/Shader/cef.frag.spv", ID_SHADER_CEF_FRAG, Ir77PVShaderStage::Fragment);

        m_context->m_shaders.emplace(m_context->m_current_device, shader);

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Shaders.");
    }

   private:
    std::shared_ptr<Ir77PeregrineV> m_context{nullptr};
};
}  // namespace NSIr77PeregrineV