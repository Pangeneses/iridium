#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

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

// What an import brings besides geometry when the op doesn't say -- a file's lights and cameras. Imported for now; strip (false) once the UI
// places lights and cameras itself.
inline constexpr bool SCENE_IMPORT_LIGHTS{true};

inline constexpr bool SCENE_IMPORT_CAMERAS{true};

// The scene module's work -- owns and wires the entities (one Pose each), View, Lighting and Field; hands the renderer each file as one asset
// and each frame as one frame.
//   SetRenderer / SetViewSize / SetPipeline -> CreateScene -> [ImportEntity / RemoveEntity / SetEntityTransform, AddLight / SetLight /
//   RemoveLight] -> per frame: Update(dt) -> CreateFrame(seconds)
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

    // The scene: every entity and light goes, then the glTF becomes entity 0 (one asset bundle in the renderer) -- its clips with it, its
    // lights as world lights (lights), its cameras (cameras). use_mesh -> mesh shading for the static geometry when the device has it.
    // No geometry is fine: an empty path, a file with nothing drawable, or a file that fails to load all leave a running scene with no
    // geometry. A file that fails to load is logged (and listed in the stats warnings), not a failure.
    virtual std::shared_ptr<IIr77Return const> CreateScene(std::string const& path, bool const& use_mesh, bool const& lights, bool const& cameras) = 0;

    // One more file in the scene, at the origin, its clips playing -- entity is its id. A file that won't load adds nothing. lights -> its
    // lights become world lights; the cameras stay as they are.
    virtual std::shared_ptr<IIr77Return const> ImportEntity(std::string const& path, bool const& use_mesh, bool const& lights, std::uint32_t& entity) = 0;

    // Its asset leaves the renderer. Lights it brought stay -- they're world lights.
    virtual std::shared_ptr<IIr77Return const> RemoveEntity(std::uint32_t const& entity) = 0;

    // Where the whole entity sits in the world -- everything it draws moves with it from the next frame, the shadow casters' box with it
    virtual std::shared_ptr<IIr77Return const> SetEntityTransform(std::uint32_t const& entity, glm::vec3 const& translation, glm::quat const& rotation,
                                                                  glm::vec3 const& scale) = 0;

    // The entities in id order -- each one's id, file and placement
    virtual std::shared_ptr<IIr77Return const> GetEntities(std::vector<std::uint32_t>& ids, std::vector<std::string>& paths,
                                                           std::vector<glm::vec3>& translations, std::vector<glm::quat>& rotations,
                                                           std::vector<glm::vec3>& scales) = 0;

    // World lights -- the scene graph's. light: type, color, intensity (glTF units -- lux for the sun, candela for point / spot), range,
    // cones; the placement says where it is (it shines down its local -Z). Each change rebuilds the scene's lights.
    virtual std::shared_ptr<IIr77Return const> AddLight(Ir77PVLight const& light, glm::vec3 const& translation, glm::quat const& rotation, std::uint32_t& id) = 0;

    virtual std::shared_ptr<IIr77Return const> SetLight(std::uint32_t const& id, Ir77PVLight const& light, glm::vec3 const& translation,
                                                        glm::quat const& rotation) = 0;

    virtual std::shared_ptr<IIr77Return const> RemoveLight(std::uint32_t const& id) = 0;

    // entity -- the one it was imported with (UINT32_MAX = placed by the UI)
    virtual std::shared_ptr<IIr77Return const> GetLight(std::uint32_t const& id, Ir77PVLight& light, glm::vec3& translation, glm::quat& rotation,
                                                        std::uint32_t& entity) = 0;

    virtual std::shared_ptr<IIr77Return const> GetLightIds(std::vector<std::uint32_t>& ids) = 0;

    // Once per frame, before CreateFrame: every entity's animation, world matrices and joint palettes, then the held camera keys (View)
    virtual std::shared_ptr<IIr77Return const> Update(float const& dt) = 0;

    // Every frame, after Update: the frame -- every window's cameras, every entity's transforms and bones, the lights, the field's dispatches
    // and draws -- to the renderer in one SubmitFrame.
    virtual std::shared_ptr<IIr77Return const> CreateFrame(float const& seconds) = 0;

    // An entity's Pose -- its clips and nodes
    virtual std::shared_ptr<IIr77Return const> GetPose(std::uint32_t const& entity, std::shared_ptr<IIr77Pose>& pose) = 0;

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
