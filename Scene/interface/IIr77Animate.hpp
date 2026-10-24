#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77Pose.hpp"
#include "IIr77View.hpp"
#include "IIr77Lighting.hpp"
#include "IIr77Field.hpp"

#include "../types/Ir77Stats.hpp"

#include "../../PeregrineV/types/Ir77PVTypes.hpp"

namespace NSIr77PeregrineV {
class Ir77Server;
}

using namespace NSIr77RT;
using namespace NSIr77PeregrineV;

namespace NSIr77Scene {

// The scene module's work -- owns and wires Pose, View, Lighting and Field; loads the scene through the renderer and hands it each frame.
//   SetRenderer / SetViewSize / SetPipeline -> CreateScene -> per frame: Update(dt) -> CreateFrame(seconds)
typedef struct IIr77Animate : virtual public IIr77Enlisted {
    IIr77Animate() = default;

    // The renderer's server -- where the scene's textures, materials and geometry go (CreateScene), which pipelines its draw items use, and
    // where each frame goes (CreateFrame). Field and Lighting get it too. Its forwarding methods are the only way into PeregrineV.
    virtual std::shared_ptr<IIr77Return const> SetRenderer(std::shared_ptr<NSIr77PeregrineV::Ir77Server> const& renderer) = 0;

    // The windows CreateFrame fills, and their size -- the cameras' aspect. Any time; the next frame uses it.
    virtual std::shared_ptr<IIr77Return const> SetViewSize(std::size_t const& windows, int const& width, int const& height) = 0;

    // What the static (non-skinned, opaque) runs draw with -- a manifest pipeline or role. Unset -> the "static" role.
    virtual std::shared_ptr<IIr77Return const> SetPipeline(Ir77PVPipelineHandle const& pipeline) = 0;

    // Once, after CreatePipelines. Parses the glTF, stacks every primitive into one upload, sorts the commands into material runs, uploads
    // textures / materials / geometry through the Asset and packs the lights. use_mesh -> mesh shading for the static stream when the device has it.
    // No geometry is fine: an empty path, a file with nothing drawable, or a file that fails to load all leave a running scene with no
    // geometry. A file that fails to load is logged (and listed in the stats warnings), not a failure.
    virtual std::shared_ptr<IIr77Return const> CreateScene(std::string const& path, bool const& use_mesh) = 0;

    // Once per frame, before CreateFrame: animation, world matrices and joint palettes (Pose), then the held camera keys (View)
    virtual std::shared_ptr<IIr77Return const> Update(float const& dt) = 0;

    // Every frame, after Update: hands the frame's results to Paint -- camera, instance transforms, lights, bones, one draw item per material
    // run and the sorted transparent draws, for every window.
    virtual std::shared_ptr<IIr77Return const> CreateFrame(float const& seconds) = 0;

    virtual std::shared_ptr<IIr77Return const> GetPose(std::shared_ptr<IIr77Pose>& pose) = 0;

    virtual std::shared_ptr<IIr77Return const> GetView(std::shared_ptr<IIr77View>& view) = 0;

    virtual std::shared_ptr<IIr77Return const> GetField(std::shared_ptr<IIr77Field>& field) = 0;

    virtual std::shared_ptr<IIr77Return const> GetLighting(std::shared_ptr<IIr77Lighting>& lighting) = 0;

    // stats -- copies, safe to keep
    virtual std::shared_ptr<IIr77Return const> GetSceneStats(Ir77SceneStats& stats) const = 0;

    // The scene's part of the frame stats -- active camera, draw counts, playing clips. Timings stay with whoever runs the loop.
    virtual std::shared_ptr<IIr77Return const> WriteFrameStats(Ir77FrameStats& stats) const = 0;

    virtual ~IIr77Animate() = default;
}* pIIr77Animate;
}  // namespace NSIr77Scene
