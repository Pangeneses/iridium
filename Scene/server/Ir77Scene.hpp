#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fastgltf/core.hpp>

#include <memory>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77Scene.hpp"
#include "../dictionary/IDIr77Scene.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "IIr77Scene.hpp"

#include "../interface/IIr77Camera.hpp"
#include "../interface/IIr77Light.hpp"
#include "../interface/IIr77Mesh.hpp"
#include "../interface/IIr77Material.hpp"
#include "../interface/IIr77Texture.hpp"
#include "../interface/IIr77Animation.hpp"

#include "../runtime/Ir77Camera.hpp"
#include "../runtime/Ir77Light.hpp"
#include "../runtime/Ir77Mesh.hpp"
#include "../runtime/Ir77Material.hpp"
#include "../runtime/Ir77Texture.hpp"
#include "../runtime/Ir77Animation.hpp"

using namespace NSIr77RT;

namespace NSIr77Scene {

class Ir77Scene : public Ir77Enlisted, public IIr77Scene, public std::enable_shared_from_this<Ir77Scene> {
   public:
    Ir77Scene() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    ~Ir77Scene() {}

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77Scene>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77Scene>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    IIr77GUID* const QueryInterface(IIr77GUID const* iid, std::shared_ptr<void>& obj) {
        if (iid == &GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == &GUIDIIr77Scene)
            obj = std::shared_ptr<IIr77Scene>(shared_from_this(), static_cast<IIr77Scene*>(this));

        else if (iid == &GUIDIr77Scene)
            obj = std::shared_ptr<Ir77Scene>(shared_from_this(), static_cast<Ir77Scene*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> ParseAsset(const std::string& path) {
        fastgltf::Parser parser;
        auto data = fastgltf::GltfDataBuffer::FromPath(path);

        if (data.error() != fastgltf::Error::None) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: failed to load glTF file.");
        }

        auto asset = parser.loadGltf(data.get(), std::filesystem::path(path).parent_path(), fastgltf::Options::LoadExternalBuffers);

        if (asset.error() != fastgltf::Error::None) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: failed to parse glTF asset.");
        }

        m_asset = std::move(asset.get());

        std::size_t scene_index = m_asset.defaultScene.value_or(0);
        if (scene_index >= m_asset.scenes.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: no valid scene.");

        const auto& scene = m_asset.scenes.at(scene_index);

        std::vector<bool> mesh_used(m_asset.meshes.size(), false);

        m_meshes = std::make_shared<Ir77Mesh>();
        m_lights = std::make_shared<Ir77Light>();
        m_cameras = std::make_shared<Ir77Camera>();
        m_materials = std::make_shared<Ir77Material>();
        m_textures = std::make_shared<Ir77Texture>();
        m_animations = std::make_shared<Ir77Animation>();

        for (auto root_index : scene.nodeIndices) {
            WalkNode(root_index, glm::mat4(1.0f), mesh_used);
        }

        for (std::size_t i = 0; i < m_asset.meshes.size(); i++) {
            if (mesh_used[i]) m_meshes->AddMesh(m_asset, i);
        }

        m_materials->LoadMaterialsFromAsset(m_asset);
        m_textures->LoadTexturesFromAsset(m_asset);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    void WalkNode(std::size_t node_index, const glm::mat4& parent_transform, std::vector<bool>& mesh_used) {
        const auto& node = m_asset.nodes.at(node_index);

        glm::mat4 world_transform = parent_transform * ComputeLocalTransform(node);

        if (node.lightIndex.has_value()) {
            m_lights->AddLight(m_asset.lights.at(node.lightIndex.value()), world_transform);
        }

        if (node.cameraIndex.has_value()) {
            m_cameras->AddCamera(m_asset.cameras.at(node.cameraIndex.value()), world_transform);
        }

        if (node.meshIndex.has_value()) {
            mesh_used[node.meshIndex.value()] = true;
            m_meshes->AddMeshInstance(node.meshIndex.value(), world_transform);
        }

        for (auto child_index : node.children) {
            WalkNode(child_index, world_transform, mesh_used);
        }
    }

    glm::mat4 ComputeLocalTransform(const fastgltf::Node& node) {
        if (auto* trs = std::get_if<fastgltf::TRS>(&node.transform)) {
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(trs->translation[0], trs->translation[1], trs->translation[2]));

            glm::quat rotation(trs->rotation[3], trs->rotation[0], trs->rotation[1], trs->rotation[2]);

            glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(trs->scale[0], trs->scale[1], trs->scale[2]));

            return translation * glm::mat4_cast(rotation) * scale;
        }

        if (auto* mat = std::get_if<fastgltf::math::fmat4x4>(&node.transform)) {
            return glm::make_mat4(mat->data());
        }

        return glm::mat4(1.0f);
    }

   private:
    fastgltf::Asset m_asset;

    std::shared_ptr<IIr77Mesh> m_meshes{nullptr};

    std::shared_ptr<IIr77Camera> m_cameras{nullptr};

    std::shared_ptr<IIr77Light> m_lights{nullptr};

    std::shared_ptr<IIr77Material> m_materials{nullptr};

    std::shared_ptr<IIr77Texture> m_textures{nullptr};

    std::shared_ptr<IIr77Animation> m_animations{nullptr};
};
}  // namespace NSIr77Scene
