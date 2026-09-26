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

#include "../interface/IIr77Light.hpp"

using namespace NSIr77RT;

namespace NSIr77Scene {

class Ir77Light : public Ir77Enlisted, public IIr77Light, public std::enable_shared_from_this<Ir77Light> {
   public:
    Ir77Light() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    ~Ir77Light() {}

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77Light>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77Light>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    IIr77GUID* const QueryInterface(IIr77GUID const* iid, std::shared_ptr<void>& obj) {
        if (iid == &GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == &GUIDIIr77Light)
            obj = std::shared_ptr<IIr77Light>(shared_from_this(), static_cast<IIr77Light*>(this));

        else if (iid == &GUIDIr77Light)
            obj = std::shared_ptr<Ir77Light>(shared_from_this(), static_cast<Ir77Light*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> AddLight(fastgltf::Light const& light, glm::mat4 const& world_transform) {
        Ir77PVLight pv_light{};

        pv_light.position = glm::vec3(world_transform[3]);
        pv_light.direction = glm::normalize(glm::vec3(world_transform * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));

        pv_light.color = glm::vec3(light.color.x(), light.color.y(), light.color.z());
        pv_light.intensity = light.intensity;

        pv_light.range = light.range.value_or(0.0f); 
        pv_light.innerConeAngle = light.innerConeAngle.value_or(0.0f);
        pv_light.outerConeAngle = light.outerConeAngle.value_or(0.0f);

        switch (light.type) {
            case fastgltf::LightType::Directional:
                pv_light.type = 0;
                break;
            case fastgltf::LightType::Point:
                pv_light.type = 1;
                break;
            case fastgltf::LightType::Spot:
                pv_light.type = 2;
                break;
        }

        m_lights.push_back(pv_light);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }
    
    std::shared_ptr<IIr77Return const> GetLightsBuffer(std::vector<Ir77PVLight>& lights) {
        lights = m_lights;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    std::vector<Ir77PVLight> m_lights;
};
}  // namespace NSIr77Scene
