#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fastgltf/core.hpp>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77Scene.hpp"
#include "../dictionary/IDIr77Scene.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77Camera.hpp"

using namespace NSIr77RT;

namespace NSIr77Scene {

// =============================================================================================================================================
// Ir77Camera -- glTF camera nodes. Stores the file's parameters, not a finished matrix, so the projection is rebuilt every frame
// with the window's real aspect ratio (vertical fov / ymag kept, width follows the window -- no stretching on resize).
// Output matches Ir77PVAsset::MakeCamera: right-handed, depth 0..1 (_ZO), Y flipped for Vulkan clip space.
// =============================================================================================================================================
class Ir77Camera : public Ir77Enlisted, public IIr77Camera, public std::enable_shared_from_this<Ir77Camera> {
   private:
    struct Ir77CameraNode {
        bool perspective{true};
        float yfov{0.8f};      // perspective: vertical field of view, radians
        float ymag{1.0f};      // orthographic: half height
        float znear{0.1f};
        float zfar{0.0f};      // 0 = the file gave none (glTF: infinite) -> caller's fallback far plane
        glm::mat4 world{1.0f};
    };

   public:
    Ir77Camera() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    ~Ir77Camera() {}

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77Camera>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77Camera>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77Camera)
            obj = std::shared_ptr<IIr77Camera>(shared_from_this(), static_cast<IIr77Camera*>(this));

        else if (iid == GUIDIr77Camera)
            obj = std::shared_ptr<Ir77Camera>(shared_from_this(), static_cast<Ir77Camera*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

   public:
    // One call per camera node. The file's aspectRatio is ignored on purpose -- the window decides the width.
    std::shared_ptr<IIr77Return const> AddCamera(fastgltf::Camera const& camera, glm::mat4 const& world_transform) {
        Ir77CameraNode node{};

        if (auto const* persp = std::get_if<fastgltf::Camera::Perspective>(&camera.camera)) {
            node.perspective = true;
            node.yfov = persp->yfov;
            node.znear = persp->znear;
            node.zfar = persp->zfar.value_or(0.0f);
        } else if (auto const* ortho = std::get_if<fastgltf::Camera::Orthographic>(&camera.camera)) {
            node.perspective = false;
            node.ymag = ortho->ymag;
            node.znear = ortho->znear;
            node.zfar = ortho->zfar;
        } else {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Camera: camera is neither perspective nor orthographic.");
        }

        node.world = world_transform;

        m_cameras.push_back(node);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetCameraCount(std::uint32_t& count) {
        count = static_cast<std::uint32_t>(m_cameras.size());

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Builds camera `index` for a viewport of `aspect` (width / height). fallback_far is used when the file has no zfar.
    std::shared_ptr<IIr77Return const> BuildCamera(std::uint32_t const& index, float const& aspect, float const& fallback_far, Ir77PVCamera& camera) {
        if (index >= m_cameras.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Camera: camera index out of range.");

        Ir77CameraNode const& node = m_cameras[index];

        float const zfar = node.zfar > node.znear ? node.zfar : std::max(fallback_far, node.znear * 2.0f);

        camera = Ir77PVCamera{};

        if (node.perspective) {
            camera.proj = glm::perspectiveRH_ZO(node.yfov, aspect, node.znear, zfar);
        } else {
            float const xmag = node.ymag * aspect;
            camera.proj = glm::orthoRH_ZO(-xmag, xmag, -node.ymag, node.ymag, node.znear, zfar);
        }

        camera.proj[1][1] *= -1.0f;

        // rigid placement -- strip any scale on the node (or its parents) so the view keeps unit axes
        glm::mat4 rigid = node.world;
        rigid[0] = glm::vec4(glm::normalize(glm::vec3(rigid[0])), 0.0f);
        rigid[1] = glm::vec4(glm::normalize(glm::vec3(rigid[1])), 0.0f);
        rigid[2] = glm::vec4(glm::normalize(glm::vec3(rigid[2])), 0.0f);

        camera.view = glm::inverse(rigid);
        camera.eye = glm::vec4(glm::vec3(rigid[3]), 1.0f);
        camera.view_proj = camera.proj * camera.view;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    std::vector<Ir77CameraNode> m_cameras{};
};
}  // namespace NSIr77Scene