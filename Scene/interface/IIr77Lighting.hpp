#pragma once

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../PeregrineV/types/Ir77PVTypes.hpp"

namespace NSIr77PeregrineV {
class Ir77Server;
}

using namespace NSIr77RT;
using namespace NSIr77PeregrineV;

namespace NSIr77Scene {

// The scene's lights and shadows -- unit conversion, the shadow casters over the scene's (and the field's) bounds, the packed lights buffer.
//   SetRenderer -> SetBounds -> CreateLights -> [SetFieldBounds] -> per frame: WriteFrameData(window)
typedef struct IIr77Lighting : virtual public IIr77Enlisted {
    IIr77Lighting() = default;

    // PeregrineV's server -- WriteFrameData hands it the lights and shadow matrices
    virtual std::shared_ptr<IIr77Return const> SetRenderer(std::shared_ptr<NSIr77PeregrineV::Ir77Server> const& renderer) = 0;

    // The scene's bounds -- the sun's shadow box covers them. Rebuilds the casters.
    virtual std::shared_ptr<IIr77Return const> SetBounds(glm::vec3 const& low, glm::vec3 const& high) = 0;

    // What else the shadow box must cover -- the field, while it is up (active = false drops it). Rebuilds the casters.
    virtual std::shared_ptr<IIr77Return const> SetFieldBounds(bool const& active, glm::vec3 const& low, glm::vec3 const& high) = 0;

    // The file's lights, packed once: glTF units -> engine units, a default sun when there are none, capped at the Lights capacity, then
    // the shadow casters for the current bounds.
    virtual std::shared_ptr<IIr77Return const> CreateLights(std::vector<Ir77PVLight> const& lights) = 0;

    // The lights after conversion, each with its shadow tile -- for the stats
    virtual std::shared_ptr<IIr77Return const> GetLights(std::vector<Ir77PVLight>& lights) = 0;

    // Shadow-casting lights = atlas tiles = shadow draws per stream
    virtual std::shared_ptr<IIr77Return const> GetShadowCount(std::uint32_t& count) = 0;

    // One window's Lights (header + Ir77PVLight[], set 0 binding 1) and ShadowMatrices frame data
    virtual std::shared_ptr<IIr77Return const> WriteFrameData(std::size_t const& window) = 0;

    virtual ~IIr77Lighting() = default;
}* pIIr77Lighting;
}  // namespace NSIr77Scene
