#pragma once

#include <glm/glm.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../types/Ir77Types.hpp"

#include "../../PeregrineV/types/Ir77Types.hpp"

namespace NSIr77PeregrineV {
class Ir77Server;
}

using namespace NSIr77RT;
using namespace NSIr77PeregrineV;

namespace NSIr77Fields {

// One compute-driven visual -- a surface or particle cloud, its program compiled from source at runtime (include/field.glsl is the contract).
//   SetRenderer -> CompileField -> per frame: AppendDispatches(dispatches, dt) -> AppendDraws(window, pass, shadows, items); GetBounds for
//   whoever frames the shadows (the scene's Lighting)
typedef struct IIr77Field : virtual public IIr77Enlisted {
    IIr77Field() = default;

    // PeregrineV's server -- compiles, pipelines, storage buffers, the canvas, dispatches and draws all go through it
    virtual std::shared_ptr<IIr77Return const> SetRenderer(std::shared_ptr<NSIr77PeregrineV::Ir77Server> const& renderer) = 0;

    // errors = the compiler's messages, lines numbered from the program's first line (empty on success)
    virtual std::shared_ptr<IIr77Return const> CompileField(Ir77FieldSettings const& settings, std::string& errors) = 0;

    // The 8 live values every program reads as params[] -- sliders. Applied from the next frame, no recompile.
    virtual std::shared_ptr<IIr77Return const> SetFieldParams(std::array<float, 8> const& params) = 0;

    // The program's data -- floats it reads with Ir77FieldData(i), replacing what was there; empty: none. Kept across recompiles, released
    // with the field.
    virtual std::shared_ptr<IIr77Return const> SetFieldData(std::vector<float> const& values) = 0;

    // Blanks the canvas on the next frame
    virtual std::shared_ptr<IIr77Return const> ClearFieldCanvas() = 0;

    // Runs the program from its first frame again (f.first), with a blank canvas and the pens back at t = 0
    virtual std::shared_ptr<IIr77Return const> ResetField() = 0;

    // Hidden: no compute, no draws -- the state stays where it was
    virtual std::shared_ptr<IIr77Return const> ShowField(bool const& visible) = 0;

    // The program out: no compute, no draws, the point buffers and the ray surface freed. The canvas stays (made once). field.compile brings
    // one back.
    virtual std::shared_ptr<IIr77Return const> ReleaseField() = 0;

    // The field's box -- grid x / z on the ground plane, the value range up. active = compiled and shown (false: there's nothing to cover).
    virtual std::shared_ptr<IIr77Return const> GetBounds(bool& active, glm::vec3& low, glm::vec3& high) = 0;

    // The field's compute for this frame, appended to the frame's dispatch list (recorded once a frame, so the simulation steps once however
    // many windows draw it): Field() over every element, then (with a Pen) the canvas decay / clear and the pens between last frame's t and
    // this one's. dt = seconds since the last frame (clamped to 0..0.1 here). Hidden or not compiled: nothing.
    virtual std::shared_ptr<IIr77Return const> AppendDispatches(std::vector<Ir77PVDispatch>& dispatches, float const& dt) = 0;

    // The field's draws for one pass list: surface -> Opaque + one Shadow draw per casting light (shadows = the frame's shadow tiles);
    // particles -> Transparent (additive). A pipeline that can't be built yet just leaves the field out of this frame.
    virtual std::shared_ptr<IIr77Return const> AppendDraws(std::size_t const& window, Ir77PVPass const& pass, std::uint32_t const& shadows,
                                                           std::vector<Ir77PVDrawItem>& items) = 0;

    virtual ~IIr77Field() = default;
}* pIIr77Field;
}  // namespace NSIr77Fields
