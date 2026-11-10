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

#include "IIr77Model.hpp"
#include "IIr77View.hpp"
#include "IIr77Lighting.hpp"

#include "../types/Ir77Stats.hpp"
#include "../types/Ir77Types.hpp"

#include "../../PeregrineV/types/Ir77Types.hpp"

namespace NSIr77PeregrineV {
class Ir77Server;
}

namespace NSIr77Fields {
class Ir77Server;
}

using namespace NSIr77RT;
using namespace NSIr77PeregrineV;

namespace NSIr77Scene {

// The scene module's work -- owns and wires the entities (one Model each), View and Lighting, and draws the fields module into its frame; hands the renderer each file as one asset
// and each frame as one frame.
//   SetRenderer / SetViewSize -> CreateScene -> [ImportEntity / RemoveEntity / SetEntityTransform, AddLight / SetLight /
//   RemoveLight] -> per frame: Update(dt) -> CreateFrame(seconds)
typedef struct IIr77World : virtual public IIr77Enlisted {
    IIr77World() = default;

    // The renderer's server -- where the scene's asset goes (CreateScene) and each frame goes (CreateFrame). Its forwarding methods are the
    // only way into PeregrineV.
    virtual std::shared_ptr<IIr77Return const> SetRenderer(std::shared_ptr<NSIr77PeregrineV::Ir77Server> const& renderer) = 0;

    // The fields module's server -- CreateFrame asks it for its dispatches, draws and box. Optional: without it the frame has no field.
    virtual std::shared_ptr<IIr77Return const> SetField(std::shared_ptr<NSIr77Fields::Ir77Server> const& fields) = 0;

    // The windows CreateFrame fills, and their size -- the cameras' aspect. Any time; the next frame uses it.
    virtual std::shared_ptr<IIr77Return const> SetViewSize(std::size_t const& windows, int const& width, int const& height) = 0;

    // The background every window clears to -- sRGB 0..1, as a colour picker gives it. Any time; the next frame uses it.
    virtual std::shared_ptr<IIr77Return const> SetBackground(glm::vec3 const& color) = 0;

    virtual std::shared_ptr<IIr77Return const> GetBackground(glm::vec3& color) const = 0;

    // The editor grid in every view -- the ground (XZ) in perspective, the plane facing the camera in orthographic. On by default; a manifest
    // without a "grid" pipeline leaves it out (one stats warning).
    virtual std::shared_ptr<IIr77Return const> SetGridVisible(bool const& visible) = 0;

    virtual std::shared_ptr<IIr77Return const> GetGridVisible(bool& visible) const = 0;

    // after RemoveEntity
    virtual std::shared_ptr<IIr77Return const> SetEntityVisible(std::uint32_t const& entity, bool const& visible) = 0;
    // after RemoveLight
    virtual std::shared_ptr<IIr77Return const> SetLightVisible(std::uint32_t const& id, bool const& visible) = 0;

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
    virtual std::shared_ptr<IIr77Return const> AddLight(Ir77PVLight const& light, glm::vec3 const& translation, glm::quat const& rotation,
                                                        std::uint32_t& id) = 0;

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

    // An entity's Model -- its clips and nodes
    virtual std::shared_ptr<IIr77Return const> GetModel(std::uint32_t const& entity, std::shared_ptr<IIr77Model>& model) = 0;

    // The model in edit mode -- it stops playing and rests in its rest pose, then its vertices and edges are drawn over it from the next
    // frame. EDIT_NO_ENTITY leaves edit mode, and the model plays what it played before, from the start.
    virtual std::shared_ptr<IIr77Return const> SetEditEntity(std::uint32_t const& entity) = 0;

    // The model drawn with the selection's outline (SELECT_OUTLINE_COLOR) from the next frame -- EDIT_NO_ENTITY: none
    virtual std::shared_ptr<IIr77Return const> SetOutlineEntity(std::uint32_t const& entity) = 0;

    // entity = the model in edit mode (EDIT_NO_ENTITY = none); points / edges = what its cage holds
    virtual std::shared_ptr<IIr77Return const> GetEditEntity(std::uint32_t& entity, std::uint32_t& points, std::uint32_t& edges) const = 0;

    // The clips the model in edit mode was playing (held until it leaves) -- empty when nothing is in edit mode
    virtual std::shared_ptr<IIr77Return const> GetEditClips(std::vector<Ir77EditClip>& clips) const = 0;

    // What a click in edit mode selects -- vertex / edge / face
    virtual std::shared_ptr<IIr77Return const> SetEditMode(Ir77EditMode const& mode) = 0;

    // A click in edit mode -- x, y fractions of window 0 from its top left; the nearest visible element under it, by the mode. extend: toggle
    // it in the selection; otherwise it replaces the selection, and a miss clears it.
    virtual std::shared_ptr<IIr77Return const> EditPick(float const& x, float const& y, bool const& extend, bool& hit) = 0;

    // A box drag in edit mode -- (x0, y0) to (x1, y1), fractions of window 0, through the view it started in; extend adds, otherwise replaces
    virtual std::shared_ptr<IIr77Return const> EditBoxSelect(float const& x0, float const& y0, float const& x1, float const& y1, bool const& extend,
                                                             std::uint32_t& selected) = 0;

    // Every vertex of the model in edit mode (true) or none
    virtual std::shared_ptr<IIr77Return const> EditSelectAll(bool const& select) = 0;

    // The edit mode's mode and how many vertices are selected
    virtual std::shared_ptr<IIr77Return const> GetEditSelection(Ir77EditMode& mode, std::uint32_t& selected) const = 0;


    // The cursor at x, y now -> the selection moved, free or along one world axis; delta = the move, world space
    virtual std::shared_ptr<IIr77Return const> EditGrabMove(float const& x, float const& y, Ir77EditAxis const& axis, glm::vec3& delta) = 0;

    // confirm: the move stays; otherwise it goes back
    virtual std::shared_ptr<IIr77Return const> EditGrabEnd(bool const& confirm) = 0;

    // What the gizmo does -- translate / rotate / scale; hot = the handle lit (hovered or held), GIZMO_NONE: none
    virtual std::shared_ptr<IIr77Return const> SetGizmoMode(Ir77GizmoMode const& mode) = 0;

    virtual std::shared_ptr<IIr77Return const> GetGizmo(Ir77GizmoMode& mode, std::uint32_t& hot) const = 0;

    // ---- object mode's gizmo -- the active entity's whole transform. World draws it and answers the drag; what moves is the caller's.

    // shown at world (the active entity's world matrix: its pivot, and its own axes for a scale) from the next frame; hidden: none
    virtual std::shared_ptr<IIr77Return const> SetObjectGizmo(bool const& shown, glm::mat4 const& world) = 0;

    // x, y fractions of window 0 -- the handle under the cursor lights up (GIZMO_NONE: none)
    virtual std::shared_ptr<IIr77Return const> ObjectGizmoHover(float const& x, float const& y, std::uint32_t& handle) = 0;

    // a press on a handle starts a drag -- GIZMO_NONE: none, nothing starts
    virtual std::shared_ptr<IIr77Return const> ObjectGizmoPress(float const& x, float const& y, std::uint32_t& handle) = 0;

    // the drag since the press, one world matrix about its pivot; changed = not the identity
    virtual std::shared_ptr<IIr77Return const> ObjectGizmoDrag(float const& x, float const& y, glm::mat4& world, bool& changed) = 0;

    virtual std::shared_ptr<IIr77Return const> ObjectGizmoEnd() = 0;

    // The cursor over the viewport -- the gizmo handle under it lights up; handle = it (GIZMO_NONE: none)
    virtual std::shared_ptr<IIr77Return const> EditGizmoHover(float const& x, float const& y, std::uint32_t& handle) = 0;

    // A press on a gizmo handle starts a transform of the selection (driven by EditGrabMove / EditGrabEnd); handle = GIZMO_NONE: not on one
    virtual std::shared_ptr<IIr77Return const> EditGizmoPress(float const& x, float const& y, std::uint32_t& handle) = 0;

    // grabbing = a grab is under way; modified = the model in edit mode has a confirmed edit
    virtual std::shared_ptr<IIr77Return const> GetEditGrab(bool& grabbing, bool& modified) const = 0;

    // the last confirmed edit of the model in edit mode back / again; done = false: nothing there
    virtual std::shared_ptr<IIr77Return const> EditUndo(bool& done) = 0;

    virtual std::shared_ptr<IIr77Return const> EditRedo(bool& done) = 0;

    // how many steps undo and redo can go
    virtual std::shared_ptr<IIr77Return const> GetEditHistory(std::uint32_t& undo, std::uint32_t& redo) const = 0;

    // Every model with an edit its file doesn't hold yet -> its .glb rewritten (other entities on the file take the edit too). written = the
    // files written; errors = one line each for those that weren't.
    virtual std::shared_ptr<IIr77Return const> CommitEdits(std::vector<std::string>& written, std::vector<std::string>& errors) = 0;

    // The model under a point of window 0 -- x, y fractions of the window from its top left, through the view there, as the last frame drew
    // it. Nothing hit -> EDIT_NO_ENTITY.
    virtual std::shared_ptr<IIr77Return const> PickEntity(float const& x, float const& y, std::uint32_t& entity) = 0;

    virtual std::shared_ptr<IIr77Return const> GetView(std::shared_ptr<IIr77View>& view) = 0;

    virtual std::shared_ptr<IIr77Return const> GetLighting(std::shared_ptr<IIr77Lighting>& lighting) = 0;

    // stats -- copies, safe to keep. The scene as it is now: every entity's counts added up, the lights, the warnings
    virtual std::shared_ptr<IIr77Return const> GetSceneStats(Ir77SceneStats& stats) const = 0;

    // The scene's part of the frame stats -- active camera, draw counts, playing clips. Timings stay with whoever runs the loop.
    virtual std::shared_ptr<IIr77Return const> WriteFrameStats(Ir77FrameStats& stats) const = 0;

    virtual ~IIr77World() = default;
}* pIIr77World;
}  // namespace NSIr77Scene
