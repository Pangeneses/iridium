#pragma once

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../PeregrineV/types/Ir77PVTypes.hpp"

using namespace NSIr77RT;
using namespace NSIr77PeregrineV;

namespace NSIr77Scene {

// The scene's lights and shadows -- unit conversion, the shadow casters over the scene's (and the field's) bounds. Fills the frame; never
// sees the renderer.
//   SetBounds -> CreateLights -> [SetFieldBounds] -> per frame: WriteFrame(frame)
typedef struct IIr77Lighting : virtual public IIr77Enlisted {
    IIr77Lighting() = default;

    // The scene's bounds -- the sun's shadow box covers them. Rebuilds the casters.
    virtual std::shared_ptr<IIr77Return const> SetBounds(glm::vec3 const& low, glm::vec3 const& high) = 0;

    // What else the shadow box must cover -- the field, while it is up (active = false drops it). Rebuilds the casters.
    virtual std::shared_ptr<IIr77Return const> SetFieldBounds(bool const& active, glm::vec3 const& low, glm::vec3 const& high) = 0;

    // The file's lights, once: glTF units -> engine units, a default sun when there are none, capped at the Lights capacity, then the shadow
    // casters for the current bounds.
    virtual std::shared_ptr<IIr77Return const> CreateLights(std::vector<Ir77PVLight> const& lights) = 0;

    // The lights after conversion, each with its shadow tile -- for the stats
    virtual std::shared_ptr<IIr77Return const> GetLights(std::vector<Ir77PVLight>& lights) = 0;

    // Shadow-casting lights = atlas tiles = shadow draws per stream
    virtual std::shared_ptr<IIr77Return const> GetShadowCount(std::uint32_t& count) = 0;

    // The frame's lights (each with its shadow tile) and shadow matrices (index = tile)
    virtual std::shared_ptr<IIr77Return const> WriteFrame(Ir77PVFrame& frame) = 0;

    virtual ~IIr77Lighting() = default;
}* pIIr77Lighting;
}  // namespace NSIr77Scene
