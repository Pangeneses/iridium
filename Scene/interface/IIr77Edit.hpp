#pragma once

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77Mesh.hpp"
#include "IIr77Material.hpp"
#include "IIr77Gizmo.hpp"

#include "../types/Ir77Types.hpp"

#include "../../PeregrineV/types/Ir77Types.hpp"

namespace NSIr77PeregrineV {
class Ir77Server;
}

using namespace NSIr77RT;
using namespace NSIr77PeregrineV;

namespace NSIr77Scene {

// Edit mode -- the model's edit mesh (vertices once, their edges and faces) built when it goes into edit mode, the selection on it, and its
// cage drawn over the model: selected faces tinted, edges, vertex points. The draws place it through the model's models[] slots, so it follows
// the model wherever it moves.
//   SetRenderer -> CreateCage(meshes, asset, instance_base, bones, bone_bases) -> [SetMode / Pick / SelectAll, BeginGrab -> MoveGrab ... ->
//   EndGrab, Undo / Redo] -> per frame: AppendDraws -> ReleaseCage
typedef struct IIr77Edit : virtual public IIr77Enlisted {
    IIr77Edit() = default;

    // PeregrineV's server -- the cage's storage buffers and draws go through it
    virtual std::shared_ptr<IIr77Return const> SetRenderer(std::shared_ptr<NSIr77PeregrineV::Ir77Server> const& renderer) = 0;

    // The transform gizmo -- Animate's, shared with object mode. Edit mode places it at the selection's median, world axes.
    virtual std::shared_ptr<IIr77Return const> SetGizmo(std::shared_ptr<IIr77Gizmo> const& gizmo) = 0;

    // The model's triangles -> the edit mesh, uploaded as the cage; nothing selected, vertex mode. asset = its renderer asset (what an edit
    // updates); instance_base = the asset's first models[] slot. Skinned geometry is posed by bones (the asset's joint palettes,
    // Pose::WriteBoneMatrices) from bone_bases (per instance) -- the rest pose, since a model in edit mode plays nothing. Replaces the cage
    // before.
    // materials = the model's -- its BLEND (alpha) faces don't hide the vertices behind them from a pick
    virtual std::shared_ptr<IIr77Return const> CreateCage(std::shared_ptr<IIr77Mesh> const& meshes, std::shared_ptr<IIr77Material> const& materials,
                                                          std::uint32_t const& asset, std::uint32_t const& instance_base, std::vector<glm::mat4> const& bones,
                                                          std::vector<std::uint32_t> const& bone_bases) = 0;

    // No cage, no edit mesh -- the buffers go back once the frames in flight are done with them
    virtual std::shared_ptr<IIr77Return const> ReleaseCage() = 0;

    // What a click selects -- the selection itself stays (it's vertices in every mode)
    virtual std::shared_ptr<IIr77Return const> SetMode(Ir77EditMode const& mode) = 0;

    // A click: the nearest visible vertex / edge / face under the cursor, by the mode. transforms = the model's instances this frame (its own
    // slot order). extend: toggle it in the selection; otherwise it replaces the selection, and a miss clears it. hit = something was there.
    virtual std::shared_ptr<IIr77Return const> Pick(Ir77EditView const& view, std::vector<glm::mat4> const& transforms, bool const& extend, bool& hit) = 0;

    // A box drag -- every visible element inside the box (pixels in the view, two opposite corners), by the mode: vertices inside it, edges
    // with both ends inside, faces whose centre is inside. extend: added; otherwise it replaces the selection. selected = the elements taken.
    virtual std::shared_ptr<IIr77Return const> BoxSelect(Ir77EditView const& view, glm::vec2 const& a, glm::vec2 const& b, std::vector<glm::mat4> const& transforms,
                                                         bool const& extend, std::uint32_t& selected) = 0;

    // Every vertex (true) or none
    virtual std::shared_ptr<IIr77Return const> SelectAll(bool const& select) = 0;

    // The mode and how many vertices are selected
    virtual std::shared_ptr<IIr77Return const> GetSelection(Ir77EditMode& mode, std::uint32_t& selected) const = 0;

    // A grab of the selection from the cursor's ray -- it slides on the plane facing the camera through the selection's centre. Nothing
    // selected: grabbing = false. transforms = the model's instances this frame.
    virtual std::shared_ptr<IIr77Return const> BeginGrab(Ir77EditView const& view, std::vector<glm::mat4> const& transforms, bool& grabbing) = 0;

    // The cursor's ray now (the grab's view) -> the selection moved, on the plane or along one world axis through its centre. delta = the
    // move, world space. The model, its normals around the move and the cage follow at once.
    virtual std::shared_ptr<IIr77Return const> MoveGrab(Ir77EditView const& view, Ir77EditAxis const& axis, glm::vec3& delta) = 0;

    // confirm: the move stays and goes into the model's mesh; otherwise everything goes back
    virtual std::shared_ptr<IIr77Return const> EndGrab(bool const& confirm) = 0;

    // grabbing = a grab is under way; modified = a confirmed edit since the cage was built
    virtual std::shared_ptr<IIr77Return const> GetGrab(bool& grabbing, bool& modified) const = 0;

    // The last confirmed edit back / again -- positions and normals as they were. done = false: nothing to undo / redo. Cancels a grab first.
    virtual std::shared_ptr<IIr77Return const> Undo(bool& done) = 0;

    virtual std::shared_ptr<IIr77Return const> Redo(bool& done) = 0;

    // how many steps undo and redo can go
    virtual std::shared_ptr<IIr77Return const> GetHistory(std::uint32_t& undo, std::uint32_t& redo) const = 0;

    // the model's file now holds every confirmed edit -- modified until the next one
    virtual std::shared_ptr<IIr77Return const> MarkSaved() = 0;

    // ---- gizmo -- translate / rotate / scale handles at the selection's median, while anything is selected (the shared Ir77Gizmo)

    // a transform under way is put back first -- it was the old mode's
    virtual std::shared_ptr<IIr77Return const> SetGizmoMode(Ir77GizmoMode const& mode) = 0;

    // mode; hot = the handle lit (hovered, or held through a transform), GIZMO_NONE: none
    virtual std::shared_ptr<IIr77Return const> GetGizmo(Ir77GizmoMode& mode, std::uint32_t& hot) const = 0;

    // the handle under the cursor lights up -- handle = it; transforms = the model's instances this frame
    virtual std::shared_ptr<IIr77Return const> HoverGizmo(Ir77EditView const& view, std::vector<glm::mat4> const& transforms, std::uint32_t& handle) = 0;

    virtual std::shared_ptr<IIr77Return const> ClearGizmoHot() = 0;

    // A press on a handle starts a transform of the selection -- a grab (MoveGrab / EndGrab) that moves along the axis, turns about it or sizes
    // along it (the centre: moves on the view plane / sizes evenly). handle = the one pressed; GIZMO_NONE: no handle there, nothing starts.
    virtual std::shared_ptr<IIr77Return const> PressGizmo(Ir77EditView const& view, std::vector<glm::mat4> const& transforms, std::uint32_t& handle) = 0;

    // The gizmo's draw for one window -- on top of everything; nothing selected, or no "edit.gizmo" pipeline: nothing
    virtual std::shared_ptr<IIr77Return const> AppendGizmo(std::size_t const& window, std::vector<glm::mat4> const& transforms,
                                                           std::vector<Ir77PVDrawItem>& items) = 0;

    // The cage's draws for one window -- selected faces, edges, then (vertex mode) points. No cage, or no edit pipelines in the manifest:
    // nothing.
    virtual std::shared_ptr<IIr77Return const> AppendDraws(std::size_t const& window, std::vector<Ir77PVDrawItem>& items) = 0;

    // What the edit mesh holds -- for the reply and the stats
    virtual std::shared_ptr<IIr77Return const> GetCageCounts(std::uint32_t& points, std::uint32_t& edges) const = 0;

    virtual ~IIr77Edit() = default;
}* pIIr77Edit;
}  // namespace NSIr77Scene
