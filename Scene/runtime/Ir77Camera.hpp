#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <memory>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77Scene.hpp"
#include "../dictionary/IDIr77Scene.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77Camera.hpp"

using namespace NSIr77RT;

namespace NSIr77Scene {

class Ir77Camera : public Ir77Enlisted, public IIr77Camera, public std::enable_shared_from_this<Ir77Camera> {
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
    std::shared_ptr<IIr77Return const> AddCamera(fastgltf::Camera const& camera, glm::mat4 const& world_transform) {
        Ir77PVCamera cam{};

        if (auto* persp = std::get_if<fastgltf::Camera::Perspective>(&camera.camera)) {
            float aspect = persp->aspectRatio.value_or(16.0f / 9.0f);
            float zfar = persp->zfar.value_or(1000.0f);

            cam.projection = glm::perspective(persp->yfov, aspect, persp->znear, zfar);
        } else if (auto* ortho = std::get_if<fastgltf::Camera::Orthographic>(&camera.camera)) {
            cam.projection = glm::ortho(-ortho->xmag, ortho->xmag, -ortho->ymag, ortho->ymag, ortho->znear, ortho->zfar);
        }

        cam.view = glm::inverse(world_transform);
        cam.model = world_transform;

        m_cameras.push_back(cam);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetCamerasBuffer(std::vector<Ir77PVCamera>& cameras) {
        cameras = m_cameras;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    std::vector<Ir77PVCamera> m_cameras;
};
}  // namespace NSIr77Scene
