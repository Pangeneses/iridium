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

// The scene module's work -- owns and wires Pose, View, Lighting and Field; hands the renderer the file as one asset and each frame as one
// frame.
//   SetRenderer / SetViewSize / SetPipeline -> CreateScene -> per frame: Update(dt) -> CreateFrame(seconds)
typedef struct IIr77Animate : virtual public IIr77Enlisted {
    IIr77Animate() = default;

    // The renderer's server -- where the scene's asset goes (CreateScene) and each frame goes (CreateFrame). Field gets it too (its
    // program, buffers and draw items). Its forwarding methods are the only way into PeregrineV.
    virtual std::shared_ptr<IIr77Return const> SetRenderer(std::shared_ptr<NSIr77PeregrineV::Ir77Server> const& renderer) = 0;

    // The windows CreateFrame fills, and their size -- the cameras' aspect. Any time; the next frame uses it.
    virtual std::shared_ptr<IIr77Return const> SetViewSize(std::size_t const& windows, int const& width, int const& height) = 0;

    // What the static (non-skinned, opaque) geometry draws with -- a manifest pipeline or role. Unset -> the "static" role. Taken by the next
    // CreateScene.
    virtual std::shared_ptr<IIr77Return const> SetPipeline(Ir77PVPipelineHandle const& pipeline) = 0;

    // Parses the glTF and hands it to the renderer as one asset bundle (LoadAsset), replacing the scene loaded before; then the lights, the
    // cameras and the clips. use_mesh -> mesh shading for the static geometry when the device has it.
    // No geometry is fine: an empty path, a file with nothing drawable, or a file that fails to load all leave a running scene with no
    // geometry. A file that fails to load is logged (and listed in the stats warnings), not a failure.
    virtual std::shared_ptr<IIr77Return const> CreateScene(std::string const& path, bool const& use_mesh) = 0;

    // Once per frame, before CreateFrame: animation, world matrices and joint palettes (Pose), then the held camera keys (View)
    virtual std::shared_ptr<IIr77Return const> Update(float const& dt) = 0;

    // Every frame, after Update: the frame -- every window's camera, the asset's transforms and bones, the lights, the field's dispatches and
    // draws -- to the renderer in one SubmitFrame.
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
