#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77Scene.hpp"
#include "../dictionary/IDIr77Scene.hpp"

#include "../../Ir77RT/types/Ir77Types.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"
#include "../../Ir77RT/runtime/Ir77Return.hpp"

#include "../interface/IIr77Lighting.hpp"

// the direct link -- Scene -> PeregrineV's server, one way
#include "../../PeregrineV/server/Ir77Server.hpp"

using namespace NSIr77RT;
using namespace NSIr77PeregrineV;

namespace NSIr77Scene {

// =============================================================================================================================================
// Ir77Lighting -- the scene's lights and their shadows. Converts the file's lights to engine units (a default sun when there are none), picks
// the shadow casters and builds their matrices over the scene's bounds (plus the field's while it is up), and packs the lights buffer.
// Owned by Ir77Animate; Ir77Field reports its box here.
//
//   scene:     SetRenderer -> SetBounds -> CreateLights
//   field:     SetFieldBounds -- the sun's box grows / shrinks with the field
//   per frame: WriteFrameData(window) -- Lights + ShadowMatrices; GetShadowCount for the shadow draws
// =============================================================================================================================================
class Ir77Lighting : public Ir77Enlisted, public IIr77Lighting, public std::enable_shared_from_this<Ir77Lighting> {
   public:
    Ir77Lighting() {
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
        seat_shared_uuid<&GUIDIr77Lighting>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77Scene>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77Lighting)
            obj = std::shared_ptr<IIr77Lighting>(shared_from_this(), static_cast<IIr77Lighting*>(this));

        else if (iid == GUIDIr77Lighting)
            obj = std::shared_ptr<Ir77Lighting>(shared_from_this(), static_cast<Ir77Lighting*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

   public:
    // PeregrineV's server -- WriteFrameData hands it the lights and shadow matrices
    std::shared_ptr<IIr77Return const> SetRenderer(std::shared_ptr<NSIr77PeregrineV::Ir77Server> const& renderer) {
        m_renderer = renderer;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The scene's bounds -- the sun's shadow box covers them. Rebuilds the casters.
    std::shared_ptr<IIr77Return const> SetBounds(glm::vec3 const& low, glm::vec3 const& high) {
        m_bounds_min = low;
        m_bounds_max = high;

        DefineLights();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // What else the shadow box must cover -- the field, while it is up (active = false drops it). Rebuilds the casters.
    std::shared_ptr<IIr77Return const> SetFieldBounds(bool const& active, glm::vec3 const& low, glm::vec3 const& high) {
        m_field_active = active;
        m_field_min = low;
        m_field_max = high;

        DefineLights();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The file's lights, packed once: glTF units -> engine units, a default sun when there are none, capped at the Lights capacity, then
    // the shadow casters for the current bounds.
    std::shared_ptr<IIr77Return const> CreateLights(std::vector<Ir77PVLight> const& lights) {
        std::vector<Ir77PVLight> light_list = lights;

        // glTF light units carry 683 lm/W -- sun in lux, point / spot in candela. Divided out, every light is in the same radiometric units
        // frag.frag works in (sun W/m2, point / spot W/sr with inverse square).
        for (Ir77PVLight& light : light_list) light.intensity /= 683.0f;

        // a default sun when the file has none -- already in engine units, so after the conversion
        if (light_list.empty()) {
            Ir77PVLight sun{};
            sun.direction = glm::normalize(glm::vec3(-0.4f, -1.0f, -0.3f));
            sun.color = glm::vec3(1.0f);
            sun.intensity = 1.0f;
            sun.type = 0;  // directional
            light_list.push_back(sun);
        }

        if (light_list.size() > 64) light_list.resize(64);  // Ir77PVFrameSizes::lights capacity

        m_light_list = light_list;
        DefineLights();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The lights after conversion, each with its shadow tile -- for the stats
    std::shared_ptr<IIr77Return const> GetLights(std::vector<Ir77PVLight>& lights) {
        lights = m_light_list;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Shadow-casting lights = atlas tiles = shadow draws per stream
    std::shared_ptr<IIr77Return const> GetShadowCount(std::uint32_t& count) {
        count = static_cast<std::uint32_t>(m_shadow_matrices.size());

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // One window's Lights (header + Ir77PVLight[], set 0 binding 1) and ShadowMatrices frame data
    std::shared_ptr<IIr77Return const> WriteFrameData(std::size_t const& window) {
        if (!m_renderer) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Lighting: SetRenderer not called.");

        std::shared_ptr<IIr77Return const> result{};

        if (Failed(m_renderer->SetFrameData(window, Ir77PVBufferSlot::Lights, m_lights_packed.data(), m_lights_packed.size()), result)) return result;

        if (!m_shadow_matrices.empty())
            return m_renderer->SetFrameData(window, Ir77PVBufferSlot::ShadowMatrices, m_shadow_matrices.data(), m_shadow_matrices.size() * sizeof(glm::mat4));

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    // Picks the shadow casters and builds their matrices -- the first directional light (orthographic box around the scene) and the spots
    // after it (perspective over the cone), up to MAX_SHADOW_LIGHTS. Each caster's tile goes into its Ir77PVLight::shadow; the rest keep -1.
    // Lights don't move yet (packed once), so this runs when the bounds change. Point lights need cube maps -- not yet.
    void DefineShadowCasters(std::vector<Ir77PVLight>& lights) {
        m_shadow_matrices.clear();

        // the scene, and the field when it is up
        glm::vec3 low = m_bounds_min;
        glm::vec3 high = m_bounds_max;

        if (m_field_active) {
            low = glm::min(low, m_field_min);
            high = glm::max(high, m_field_max);
        }

        glm::vec3 const center = (low + high) * 0.5f;
        float const radius = std::max(glm::length(high - low) * 0.5f, 0.01f);

        bool sun{false};

        for (Ir77PVLight& light : lights) {
            light.shadow = -1;

            if (m_shadow_matrices.size() >= MAX_SHADOW_LIGHTS) continue;
            if (light.type == 0 && sun) continue;  // one sun casts
            if (light.type == 1) continue;         // point -- cube maps later

            glm::vec3 const direction = glm::normalize(light.direction);
            glm::vec3 const up = std::abs(direction.y) > 0.99f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);

            glm::mat4 view{1.0f};
            glm::mat4 proj{1.0f};

            if (light.type == 0) {
                // orthographic box around the scene's bounding sphere, looking down the sun
                view = glm::lookAtRH(center - direction * radius * 2.0f, center, up);
                proj = glm::orthoRH_ZO(-radius, radius, -radius, radius, radius, radius * 3.0f);
                sun = true;
            } else {
                // perspective over the outer cone, out to the range -- or to the far side of the scene when the range is infinite
                float const far = light.range > 0.0f ? light.range : glm::length(light.position - center) + radius;
                float const near = std::max(far * 0.001f, 0.01f);
                float const fov = std::min(light.outerConeAngle * 2.0f + 0.05f, glm::radians(170.0f));  // a little past the cone for the PCF taps

                view = glm::lookAtRH(light.position, light.position + direction, up);
                proj = glm::perspectiveRH_ZO(fov, 1.0f, near, far);
            }

            proj[1][1] *= -1.0f;  // Y flipped like the cameras

            light.shadow = static_cast<std::int32_t>(m_shadow_matrices.size());
            m_shadow_matrices.push_back(proj * view);
        }
    }

    // Shadow casters for the current bounds, then the lights buffer: header + Ir77PVLight[] (set 0 binding 1). Runs at load, and again
    // whenever the bounds change.
    void DefineLights() {
        DefineShadowCasters(m_light_list);

        Ir77PVLightsHeader header{};
        header.count = static_cast<std::uint32_t>(m_light_list.size());

        m_lights_packed.resize(sizeof(header) + m_light_list.size() * sizeof(Ir77PVLight));
        std::memcpy(m_lights_packed.data(), &header, sizeof(header));
        std::memcpy(m_lights_packed.data() + sizeof(header), m_light_list.data(), m_light_list.size() * sizeof(Ir77PVLight));
    }

   private:
    std::shared_ptr<NSIr77PeregrineV::Ir77Server> m_renderer{};

    glm::vec3 m_bounds_min{-1.0f};

    glm::vec3 m_bounds_max{1.0f};

    bool m_field_active{false};  // the field is up -- its box joins the scene's for the sun

    glm::vec3 m_field_min{0.0f};

    glm::vec3 m_field_max{0.0f};

    // lights after unit conversion, each with its shadow tile (DefineShadowCasters) -- packed into m_lights_packed by DefineLights
    std::vector<Ir77PVLight> m_light_list{};

    std::vector<std::uint8_t> m_lights_packed{};  // Ir77PVLightsHeader + Ir77PVLight[], as set 0 binding 1 expects

    // one view-projection per shadow-casting light, index = its atlas tile (Ir77PVLight::shadow) -> ShadowMatrices
    std::vector<glm::mat4> m_shadow_matrices{};
};
}  // namespace NSIr77Scene
