#pragma once

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
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

namespace NSIr77Scene {

// The transform gizmo -- translate arrows, rotate rings or scale boxes, drawn on top of everything at a fixed size on screen. It knows nothing
// of what it moves: a placement (a matrix -- its axes as unit columns, its pivot as the translation) comes in, a world matrix about the pivot
// comes out of a drag. Edit mode places it at the selection's median, object mode at the active entity.
//   SetRenderer -> [SetMode, Hover / Press -> Drag ... -> End] -> per frame: Append
typedef struct IIr77Gizmo : virtual public IIr77Enlisted {
    IIr77Gizmo() = default;

    // PeregrineV's server -- the draw goes through it
    virtual std::shared_ptr<IIr77Return const> SetRenderer(std::shared_ptr<NSIr77PeregrineV::Ir77Server> const& renderer) = 0;

    // translate / rotate / scale -- a drag under way is ended first (its owner puts back what it moved)
    virtual std::shared_ptr<IIr77Return const> SetMode(Ir77GizmoMode const& mode) = 0;

    // mode; hot = the handle lit (hovered, or held through a drag), GIZMO_NONE: none
    virtual std::shared_ptr<IIr77Return const> GetMode(Ir77GizmoMode& mode, std::uint32_t& hot) const = 0;

    // the handle whose segment passes nearest the cursor, within GIZMO_PICK_PIXELS -- GIZMO_NONE: none
    virtual std::shared_ptr<IIr77Return const> HandleAt(Ir77EditView const& view, glm::mat4 const& placement, std::uint32_t& handle) const = 0;

    // the handle under the cursor lights up -- handle = it
    virtual std::shared_ptr<IIr77Return const> Hover(Ir77EditView const& view, glm::mat4 const& placement, std::uint32_t& handle) = 0;

    virtual std::shared_ptr<IIr77Return const> ClearHot() = 0;

    // A press on a handle starts a drag -- handle = the one pressed; GIZMO_NONE: no handle there, nothing starts
    virtual std::shared_ptr<IIr77Return const> Press(Ir77EditView const& view, glm::mat4 const& placement, std::uint32_t& handle) = 0;

    // The cursor's ray now (the press's view) -> the drag as one world matrix about the press's pivot:
    //   translate: along the axis (the line through the pivot nearest the cursor's ray); centre: on the plane facing the camera
    //   rotate:    about the axis, by the angle the cursor has swept around the pivot on screen
    //   scale:     along the axis, by how far the cursor has gone along it on screen against where it started; centre: evenly
    // changed = it isn't the identity. No drag: the identity.
    virtual std::shared_ptr<IIr77Return const> Drag(Ir77EditView const& view, glm::mat4& world, bool& changed) = 0;

    // the drag is over -- what it moved is its owner's to keep or put back
    virtual std::shared_ptr<IIr77Return const> End() = 0;

    // dragging = a drag is under way; handle = its handle (GIZMO_NONE: none)
    virtual std::shared_ptr<IIr77Return const> GetDrag(bool& dragging, std::uint32_t& handle) const = 0;

    // The gizmo's draw for one window at a placement -- on top of everything; no "edit.gizmo" pipeline: nothing
    virtual std::shared_ptr<IIr77Return const> Append(std::size_t const& window, glm::mat4 const& placement, std::vector<Ir77PVDrawItem>& items) = 0;

    virtual ~IIr77Gizmo() = default;
}* pIIr77Gizmo;
}  // namespace NSIr77Scene
