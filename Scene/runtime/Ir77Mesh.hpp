#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>

#include <memory>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77Scene.hpp"
#include "../dictionary/IDIr77Scene.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77Mesh.hpp"

using namespace NSIr77RT;

namespace NSIr77Scene {

class Ir77Mesh : public Ir77Enlisted, public IIr77Mesh, public std::enable_shared_from_this<Ir77Mesh> {
   public:
    Ir77Mesh() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    ~Ir77Mesh() {}

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77Mesh>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77Mesh>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77Mesh)
            obj = std::shared_ptr<IIr77Mesh>(shared_from_this(), static_cast<IIr77Mesh*>(this));

        else if (iid == GUIDIr77Mesh)
            obj = std::shared_ptr<Ir77Mesh>(shared_from_this(), static_cast<Ir77Mesh*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> AddMesh(fastgltf::Asset const& asset, std::uint32_t const& index) {
        const auto& mesh = asset.meshes.at(index);
        const auto& primitive = mesh.primitives[0];

        if (m_mesh_ranges.size() <= index) m_mesh_ranges.resize(index + 1);

        Ir77MeshRange& range = m_mesh_ranges[index];
        range.vertex_offset = static_cast<int32_t>(m_positions.size());
        range.index_offset = static_cast<uint32_t>(m_indices.size());

        auto* position_attr = primitive.findAttribute("POSITION");
        const auto& position_accessor = asset.accessors.at(position_attr->accessorIndex);
        std::size_t vertex_count = position_accessor.count;

        fastgltf::iterateAccessor<fastgltf::math::fvec3>(asset, position_accessor,
                                                         [&](fastgltf::math::fvec3 p) { m_positions.push_back(glm::vec3(p.x(), p.y(), p.z())); });

        auto* normal_attr = primitive.findAttribute("NORMAL");
        if (normal_attr != primitive.attributes.end()) {
            const auto& normal_accessor = asset.accessors.at(normal_attr->accessorIndex);
            fastgltf::iterateAccessor<fastgltf::math::fvec3>(asset, normal_accessor,
                                                             [&](fastgltf::math::fvec3 n) { m_normals.push_back(glm::vec3(n.x(), n.y(), n.z())); });
        } else {
            m_normals.insert(m_normals.end(), vertex_count, glm::vec3(0.0f, 1.0f, 0.0f));  // default: up
        }

        auto* uv_attr = primitive.findAttribute("TEXCOORD_0");
        if (uv_attr != primitive.attributes.end()) {
            const auto& uv_accessor = asset.accessors.at(uv_attr->accessorIndex);
            fastgltf::iterateAccessor<fastgltf::math::fvec2>(asset, uv_accessor, [&](fastgltf::math::fvec2 uv) { m_uvs.push_back(glm::vec2(uv.x(), uv.y())); });
        } else {
            m_uvs.insert(m_uvs.end(), vertex_count, glm::vec2(0.0f, 0.0f));
        }

        const auto& index_accessor = asset.accessors.at(primitive.indicesAccessor.value());
        fastgltf::iterateAccessor<uint32_t>(asset, index_accessor, [&](uint32_t i) { m_indices.push_back(i); });
        range.index_count = static_cast<uint32_t>(index_accessor.count);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> AddMeshInstance(std::uint32_t const& mesh_instance, glm::mat4 const& world_transform) {
        m_pending_instances[mesh_instance].push_back(world_transform);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> PopulateMeshRanges() {
        for (auto& [mesh_index, transforms] : m_pending_instances) {
            Ir77MeshRange& range = m_mesh_ranges.at(mesh_index);
            range.first_instance = static_cast<uint32_t>(m_transforms.size());
            range.instance_count = static_cast<uint32_t>(transforms.size());

            m_transforms.insert(m_transforms.end(), transforms.begin(), transforms.end());
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetVertexBuffer(std::vector<Ir77PVVertex>& vertex_buffer) {
        vertex_buffer.clear();
        for (std::size_t i = 0; i < m_positions.size(); i++) {
            vertex_buffer.push_back({m_positions.at(i), m_normals.at(i), m_uvs.at(i)});
        }
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetIndexBuffer(std::vector<uint32_t>& index_buffer) {
        index_buffer = m_indices;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetInstanceBuffer(std::vector<glm::mat4>& instance_buffer) {
        instance_buffer = m_transforms;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetMeshRanges(std::vector<Ir77MeshRange>& mesh_ranges) {
        mesh_ranges = m_mesh_ranges;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    std::vector<glm::vec3> m_positions;

    std::vector<glm::vec3> m_normals;

    std::vector<glm::vec2> m_uvs;

    std::vector<uint32_t> m_indices;

    std::vector<Ir77MeshRange> m_mesh_ranges;

    std::unordered_map<uint32_t, std::vector<glm::mat4>> m_pending_instances;

    std::vector<glm::mat4> m_transforms;
};
}  // namespace NSIr77Scene
