#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>

#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77Scene.hpp"
#include "../dictionary/IDIr77Scene.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77Mesh.hpp"

using namespace NSIr77RT;

namespace NSIr77Scene {

// =============================================================================================================================================
// Ir77Mesh -- every triangle primitive of every used glTF mesh, stacked into two streams:
//
//   static  -- position, normal, uv                     -> Ir77PVVertex         (vertex path or mesh shading)
//   skinned -- position, normal, uv, joints, weights    -> Ir77PVVertexSkinned  (vertex path, Skinned pipeline)
//
//   a primitive is skinned when it has JOINTS_0 and WEIGHTS_0
//   one range per primitive, in its own stream; Ir77MeshRange is laid out as a VkDrawIndexedIndirectCommand
//   ranges of the same mesh share its instances: one instance list for both streams, same first_instance / instance_count
//   indices stay local to their primitive (0-based); vertex_offset places them in their stream
//
//   ParseAsset order: AddMeshInstance (while walking nodes) -> AddMesh (per used mesh) -> PopulateMeshRanges
// =============================================================================================================================================
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
    // Appends every triangle primitive of mesh `index` -- one range each, into the static or skinned stream.
    // Points / lines / strips and primitives without POSITION are skipped.
    std::shared_ptr<IIr77Return const> AddMesh(fastgltf::Asset const& asset, std::uint32_t const& index) {
        if (index >= asset.meshes.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Mesh: mesh index out of range.");

        fastgltf::Mesh const& mesh = asset.meshes[index];

        for (fastgltf::Primitive const& primitive : mesh.primitives) {
            if (primitive.type != fastgltf::PrimitiveType::Triangles) continue;

            auto const* position_attr = primitive.findAttribute("POSITION");
            if (position_attr == primitive.attributes.end() || !primitive.indicesAccessor.has_value()) continue;  // indices: GenerateMeshIndices

            fastgltf::Accessor const& position_accessor = asset.accessors.at(position_attr->accessorIndex);
            std::size_t const vertex_count = position_accessor.count;

            // skinned when both skinning attributes are present and line up with the positions
            auto const* joints_attr = primitive.findAttribute("JOINTS_0");
            auto const* weights_attr = primitive.findAttribute("WEIGHTS_0");
            bool const skinned = joints_attr != primitive.attributes.end() && weights_attr != primitive.attributes.end() &&
                                 asset.accessors.at(joints_attr->accessorIndex).count == vertex_count &&
                                 asset.accessors.at(weights_attr->accessorIndex).count == vertex_count;

            Ir77MeshStream& stream = skinned ? m_skinned : m_static;

            Ir77MeshRange range{};
            range.vertex_offset = static_cast<std::int32_t>(stream.positions.size());
            range.index_offset = static_cast<std::uint32_t>(stream.indices.size());

            // positions
            stream.positions.reserve(stream.positions.size() + vertex_count);
            fastgltf::iterateAccessor<glm::vec3>(asset, position_accessor, [&](glm::vec3 p) { stream.positions.push_back(p); });

            // normals -- default up when missing or mismatched
            auto const* normal_attr = primitive.findAttribute("NORMAL");
            if (normal_attr != primitive.attributes.end() && asset.accessors.at(normal_attr->accessorIndex).count == vertex_count) {
                fastgltf::iterateAccessor<glm::vec3>(asset, asset.accessors.at(normal_attr->accessorIndex), [&](glm::vec3 n) { stream.normals.push_back(n); });
            } else {
                stream.normals.insert(stream.normals.end(), vertex_count, glm::vec3(0.0f, 1.0f, 0.0f));
            }

            // uvs -- zero when missing or mismatched
            auto const* uv_attr = primitive.findAttribute("TEXCOORD_0");
            if (uv_attr != primitive.attributes.end() && asset.accessors.at(uv_attr->accessorIndex).count == vertex_count) {
                fastgltf::iterateAccessor<glm::vec2>(asset, asset.accessors.at(uv_attr->accessorIndex), [&](glm::vec2 uv) { stream.uvs.push_back(uv); });
            } else {
                stream.uvs.insert(stream.uvs.end(), vertex_count, glm::vec2(0.0f));
            }

            // joints / weights -- joints are u8 or u16 in the file, widened to uint; weights may be normalized u8 / u16
            if (skinned) {
                fastgltf::iterateAccessor<glm::uvec4>(asset, asset.accessors.at(joints_attr->accessorIndex), [&](glm::uvec4 j) { stream.joints.push_back(j); });
                fastgltf::iterateAccessor<glm::vec4>(asset, asset.accessors.at(weights_attr->accessorIndex), [&](glm::vec4 w) { stream.weights.push_back(w); });
            }

            // indices -- local to this primitive
            fastgltf::Accessor const& index_accessor = asset.accessors.at(primitive.indicesAccessor.value());
            stream.indices.reserve(stream.indices.size() + index_accessor.count);
            fastgltf::iterateAccessor<std::uint32_t>(asset, index_accessor, [&](std::uint32_t i) { stream.indices.push_back(i); });
            range.index_count = static_cast<std::uint32_t>(index_accessor.count);

            stream.ranges.push_back(range);
            stream.range_vertex_counts.push_back(static_cast<std::uint32_t>(vertex_count));
            stream.range_materials.push_back(primitive.materialIndex.has_value() ? static_cast<std::uint32_t>(primitive.materialIndex.value()) : NO_MATERIAL);
            stream.ranges_of_mesh[index].push_back(stream.ranges.size() - 1);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // One call per node that uses the mesh -- the node's world transform becomes one instance.
    std::shared_ptr<IIr77Return const> AddMeshInstance(std::uint32_t const& mesh_index, glm::mat4 const& world_transform) {
        m_pending_instances[mesh_index].push_back(world_transform);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Lays out the instance array (each mesh's instances contiguous, shared by both streams), points every range of a mesh at
    // its instances, and computes the world-space bounds of everything drawn. Safe to call again.
    std::shared_ptr<IIr77Return const> PopulateMeshRanges() {
        m_transforms.clear();

        for (auto const& [mesh_index, transforms] : m_pending_instances) {
            auto const static_found = m_static.ranges_of_mesh.find(mesh_index);
            auto const skinned_found = m_skinned.ranges_of_mesh.find(mesh_index);

            bool const has_static = static_found != m_static.ranges_of_mesh.end() && !static_found->second.empty();
            bool const has_skinned = skinned_found != m_skinned.ranges_of_mesh.end() && !skinned_found->second.empty();
            if (!has_static && !has_skinned) continue;  // mesh had no drawable primitive

            std::uint32_t const first = static_cast<std::uint32_t>(m_transforms.size());
            std::uint32_t const count = static_cast<std::uint32_t>(transforms.size());

            if (has_static)
                for (std::size_t const r : static_found->second) {
                    m_static.ranges[r].first_instance = first;
                    m_static.ranges[r].instance_count = count;
                }

            if (has_skinned)
                for (std::size_t const r : skinned_found->second) {
                    m_skinned.ranges[r].first_instance = first;
                    m_skinned.ranges[r].instance_count = count;
                }

            m_transforms.insert(m_transforms.end(), transforms.begin(), transforms.end());
        }

        // bounds -- every range's vertices through every one of its instances (skinned in bind pose)
        m_bounds_min = glm::vec3(std::numeric_limits<float>::max());
        m_bounds_max = glm::vec3(std::numeric_limits<float>::lowest());

        AccumulateBounds(m_static);
        AccumulateBounds(m_skinned);

        if (m_transforms.empty()) {
            m_bounds_min = glm::vec3(-1.0f);
            m_bounds_max = glm::vec3(1.0f);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // ----- static stream -----------------------------------------------------------------------------------------------------------------

    std::shared_ptr<IIr77Return const> GetVertexBuffer(std::vector<Ir77PVVertex>& vertex_buffer) {
        vertex_buffer.clear();
        vertex_buffer.reserve(m_static.positions.size());

        for (std::size_t i = 0; i < m_static.positions.size(); i++) vertex_buffer.push_back({m_static.positions[i], m_static.normals[i], m_static.uvs[i]});

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetIndexBuffer(std::vector<std::uint32_t>& index_buffer) {
        index_buffer = m_static.indices;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // one per static primitive, ready to use as VkDrawIndexedIndirectCommand
    std::shared_ptr<IIr77Return const> GetMeshRanges(std::vector<Ir77MeshRange>& mesh_ranges) {
        mesh_ranges = m_static.ranges;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // parallel to GetMeshRanges -- glTF material index per range, NO_MATERIAL when the primitive has none
    std::shared_ptr<IIr77Return const> GetRangeMaterials(std::vector<std::uint32_t>& materials) {
        materials = m_static.range_materials;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // ----- skinned stream ----------------------------------------------------------------------------------------------------------------

    std::shared_ptr<IIr77Return const> GetSkinnedVertexBuffer(std::vector<Ir77PVVertexSkinned>& vertex_buffer) {
        vertex_buffer.clear();
        vertex_buffer.reserve(m_skinned.positions.size());

        for (std::size_t i = 0; i < m_skinned.positions.size(); i++)
            vertex_buffer.push_back({m_skinned.positions[i], m_skinned.normals[i], m_skinned.uvs[i], m_skinned.joints[i], m_skinned.weights[i]});

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetSkinnedIndexBuffer(std::vector<std::uint32_t>& index_buffer) {
        index_buffer = m_skinned.indices;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // one per skinned primitive; offsets are into the skinned stream
    std::shared_ptr<IIr77Return const> GetSkinnedMeshRanges(std::vector<Ir77MeshRange>& mesh_ranges) {
        mesh_ranges = m_skinned.ranges;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetSkinnedRangeMaterials(std::vector<std::uint32_t>& materials) {
        materials = m_skinned.range_materials;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // ----- shared ------------------------------------------------------------------------------------------------------------------------

    // models[] -- instance i's world transform; ranges in both streams point into it with first_instance
    std::shared_ptr<IIr77Return const> GetInstanceBuffer(std::vector<glm::mat4>& instance_buffer) {
        instance_buffer = m_transforms;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // First models[] slot of a mesh's instances -- every primitive of the mesh, in either stream, shares it
    std::shared_ptr<IIr77Return const> GetMeshFirstInstance(std::uint32_t const& mesh_index, std::uint32_t& first) {
        auto const static_found = m_static.ranges_of_mesh.find(mesh_index);
        if (static_found != m_static.ranges_of_mesh.end() && !static_found->second.empty()) {
            first = m_static.ranges[static_found->second.front()].first_instance;
            return Ir77RETURN<Ir77OperationSucceeded>();
        }

        auto const skinned_found = m_skinned.ranges_of_mesh.find(mesh_index);
        if (skinned_found != m_skinned.ranges_of_mesh.end() && !skinned_found->second.empty()) {
            first = m_skinned.ranges[skinned_found->second.front()].first_instance;
            return Ir77RETURN<Ir77OperationSucceeded>();
        }

        return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Mesh: mesh has no ranges.");
    }

    // world-space box around everything drawn -- for framing a fallback camera
    std::shared_ptr<IIr77Return const> GetBounds(glm::vec3& min, glm::vec3& max) {
        min = m_bounds_min;
        max = m_bounds_max;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   public:
    static constexpr std::uint32_t NO_MATERIAL = UINT32_MAX;  // primitive without a material -> the default material

   private:
    struct Ir77MeshStream {
        std::vector<glm::vec3> positions{};
        std::vector<glm::vec3> normals{};
        std::vector<glm::vec2> uvs{};
        std::vector<glm::uvec4> joints{};  // skinned stream only
        std::vector<glm::vec4> weights{};  // skinned stream only
        std::vector<std::uint32_t> indices{};

        std::vector<Ir77MeshRange> ranges{};  // one per primitive
        std::vector<std::uint32_t> range_materials{};
        std::vector<std::uint32_t> range_vertex_counts{};
        std::map<std::uint32_t, std::vector<std::size_t>> ranges_of_mesh{};  // glTF mesh -> its ranges in this stream
    };

   private:
    void AccumulateBounds(Ir77MeshStream const& stream) {
        for (std::size_t r = 0; r < stream.ranges.size(); r++) {
            Ir77MeshRange const& range = stream.ranges[r];

            for (std::uint32_t i = 0; i < range.instance_count; i++) {
                glm::mat4 const& model = m_transforms[range.first_instance + i];

                for (std::uint32_t v = 0; v < stream.range_vertex_counts[r]; v++) {
                    glm::vec3 const p = glm::vec3(model * glm::vec4(stream.positions[range.vertex_offset + v], 1.0f));
                    m_bounds_min = glm::min(m_bounds_min, p);
                    m_bounds_max = glm::max(m_bounds_max, p);
                }
            }
        }
    }

   private:
    Ir77MeshStream m_static{};

    Ir77MeshStream m_skinned{};

    // glTF mesh index -> its instances (std::map so the instance layout is deterministic)
    std::map<std::uint32_t, std::vector<glm::mat4>> m_pending_instances{};

    std::vector<glm::mat4> m_transforms{};

    glm::vec3 m_bounds_min{-1.0f};

    glm::vec3 m_bounds_max{1.0f};
};
}  // namespace NSIr77Scene