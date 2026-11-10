#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77Scene.hpp"
#include "../dictionary/IDIr77Scene.hpp"

#include "../../Ir77RT/types/Ir77Types.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"
#include "../../Ir77RT/runtime/Ir77Return.hpp"

#include "../interface/IIr77Gizmo.hpp"

#include "../types/Ir77Types.hpp"

// the direct link -- Scene -> PeregrineV's server, one way
#include "../../PeregrineV/server/Ir77Server.hpp"

using namespace NSIr77RT;
using namespace NSIr77PeregrineV;

namespace NSIr77Scene {

// =============================================================================================================================================
// Ir77Gizmo -- the transform gizmo (gizmo.vert / edit.frag through the "edit.gizmo" pipeline). Owned by Ir77World and shared with Ir77Edit:
// edit mode drives it at the selection's median, object mode at the active entity. One mode for both.
//
// A placement is a matrix: its pivot is the translation, its axes the (unit) columns -- the identity's for world axes, an object's own
// rotation for a scale along its local axes. The handles, the hit test and the drag all follow it; the centre square always faces the camera.
// A drag answers one world matrix about the pivot it was pressed at, from the press to now -- what it moves is its owner's business.
// =============================================================================================================================================
class Ir77Gizmo : public Ir77Enlisted, public IIr77Gizmo, public std::enable_shared_from_this<Ir77Gizmo> {
   public:
    Ir77Gizmo() {
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
        seat_shared_uuid<&GUIDIr77Gizmo>(uid);

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

        else if (iid == GUIDIIr77Gizmo)
            obj = std::shared_ptr<IIr77Gizmo>(shared_from_this(), static_cast<IIr77Gizmo*>(this));

        else if (iid == GUIDIr77Gizmo)
            obj = std::shared_ptr<Ir77Gizmo>(shared_from_this(), static_cast<Ir77Gizmo*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> SetRenderer(std::shared_ptr<NSIr77PeregrineV::Ir77Server> const& renderer) {
        m_renderer = renderer;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetMode(Ir77GizmoMode const& mode) {
        m_drag = Ir77GizmoDrag{};
        m_mode = mode;
        m_hot = GIZMO_NONE;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetMode(Ir77GizmoMode& mode, std::uint32_t& hot) const {
        mode = m_mode;
        hot = m_drag.active ? m_drag.handle : m_hot;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> HandleAt(Ir77EditView const& view, glm::mat4 const& placement, std::uint32_t& handle) const {
        handle = Nearest(view, placement);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> Hover(Ir77EditView const& view, glm::mat4 const& placement, std::uint32_t& handle) {
        handle = m_drag.active ? m_drag.handle : Nearest(view, placement);
        m_hot = handle;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> ClearHot() {
        m_hot = GIZMO_NONE;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> Press(Ir77EditView const& view, glm::mat4 const& placement, std::uint32_t& handle) {
        handle = Nearest(view, placement);
        if (handle == GIZMO_NONE) return Ir77RETURN<Ir77OperationSucceeded>();

        Ir77GizmoDrag drag{};
        drag.active = true;
        drag.handle = handle;
        drag.mode = m_mode;
        drag.pivot = glm::vec3(placement[3]);
        drag.basis = Basis(placement);
        drag.origin = view.origin;
        drag.ray = view.ray;
        drag.cursor_pixel = view.cursor;
        Project(view, drag.pivot, drag.centre_pixel);

        // the axis on screen -- what sizing along it measures against
        if (handle != GIZMO_CENTRE) {
            glm::vec2 tip{0.0f};
            if (Project(view, drag.pivot + Axis(drag.basis, handle) * Size(view, drag.pivot), tip)) {
                glm::vec2 const along = tip - drag.centre_pixel;
                if (glm::dot(along, along) > 1e-6f) drag.axis_pixel = glm::normalize(along);
            }
        }

        m_drag = drag;
        m_hot = handle;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> Drag(Ir77EditView const& view, glm::mat4& world, bool& changed) {
        world = glm::mat4(1.0f);
        changed = false;
        if (!m_drag.active) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Gizmo: no drag.");

        glm::vec3 const c = m_drag.pivot;

        if (m_drag.mode == Ir77GizmoMode::Translate) {
            glm::vec3 delta = m_drag.delta;

            if (m_drag.handle == GIZMO_CENTRE) {
                glm::vec3 start{0.0f};
                glm::vec3 now{0.0f};
                if (OnPlane(m_drag.origin, m_drag.ray, start) && OnPlane(view.origin, view.ray, now)) delta = now - start;
            } else {
                glm::vec3 const line = Axis(m_drag.basis, m_drag.handle);
                float start{0.0f};
                float now{0.0f};
                if (OnLine(m_drag.origin, m_drag.ray, line, start) && OnLine(view.origin, view.ray, line, now)) delta = line * (now - start);
            }

            m_drag.delta = delta;
            world = glm::translate(glm::mat4(1.0f), delta);
        } else if (m_drag.mode == Ir77GizmoMode::Rotate) {
            glm::vec2 const from = m_drag.cursor_pixel - m_drag.centre_pixel;
            glm::vec2 const to = view.cursor - m_drag.centre_pixel;
            if (glm::dot(from, from) < 1.0f || glm::dot(to, to) < 1.0f) return Ir77RETURN<Ir77OperationSucceeded>();  // on the centre -- no angle

            // screen y runs down: a clockwise sweep has a positive cross. A turn about an axis pointing at the eye looks counter-clockwise.
            float const swept = std::atan2(from.x * to.y - from.y * to.x, glm::dot(from, to));
            glm::vec3 const axis = Axis(m_drag.basis, m_drag.handle);
            float const facing = glm::dot(axis, -m_drag.ray) >= 0.0f ? 1.0f : -1.0f;

            world = glm::translate(glm::mat4(1.0f), c) * glm::rotate(glm::mat4(1.0f), -swept * facing, axis) * glm::translate(glm::mat4(1.0f), -c);
        } else {
            glm::vec3 factor{1.0f};

            if (m_drag.handle == GIZMO_CENTRE) {
                float const from = glm::length(m_drag.cursor_pixel - m_drag.centre_pixel);
                float const to = glm::length(view.cursor - m_drag.centre_pixel);
                factor = glm::vec3(from > 4.0f ? to / from : 1.0f);
            } else {
                float const from = glm::dot(m_drag.cursor_pixel - m_drag.centre_pixel, m_drag.axis_pixel);
                float const to = glm::dot(view.cursor - m_drag.centre_pixel, m_drag.axis_pixel);
                factor[static_cast<int>(m_drag.handle)] = std::abs(from) > 4.0f ? to / from : 1.0f + (to - from) / 100.0f;
            }

            // along the placement's axes -- B S B^T about the pivot
            glm::mat4 const basis{m_drag.basis};
            world = glm::translate(glm::mat4(1.0f), c) * basis * glm::scale(glm::mat4(1.0f), factor) * glm::transpose(basis) *
                    glm::translate(glm::mat4(1.0f), -c);
        }

        changed = world != glm::mat4(1.0f);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> End() {
        m_drag = Ir77GizmoDrag{};

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetDrag(bool& dragging, std::uint32_t& handle) const {
        dragging = m_drag.active;
        handle = m_drag.active ? m_drag.handle : GIZMO_NONE;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // pc.model = the placement (its axes and pivot), pc.user[4] = the mode, pc.user[5] = the lit handle
    std::shared_ptr<IIr77Return const> Append(std::size_t const& window, glm::mat4 const& placement, std::vector<Ir77PVDrawItem>& items) {
        if (!m_renderer || m_missing) return Ir77RETURN<Ir77OperationSucceeded>();

        if (m_pipeline == IR77PV_NO_PIPELINE && m_renderer->FindPipeline(std::string{"edit.gizmo"}, m_pipeline)->ID() != GUIDIr77OperationSucceeded) {
            m_pipeline = IR77PV_NO_PIPELINE;
            m_missing = true;
            return Ir77RETURN<Ir77NotConfigured>(this, "[WARN] Ir77Gizmo: no \"edit.gizmo\" pipeline in the manifest -- no gizmo.");
        }

        std::uint32_t const segments = static_cast<std::uint32_t>(Segments(m_mode, glm::vec3(1, 0, 0), glm::vec3(0, 1, 0)).size());

        Ir77PVDrawItem item{};
        if (m_renderer->MakeProceduralDrawItem(window, m_pipeline, segments * EDIT_VERTICES_PER_ELEMENT, 1, item)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77OperationSucceeded>();

        glm::mat4 at{Basis(placement)};
        at[3] = glm::vec4(glm::vec3(placement[3]), 1.0f);
        std::memcpy(item.model.data(), &at[0][0], sizeof(float) * 16);

        Ir77GizmoMode mode{};
        std::uint32_t hot{GIZMO_NONE};
        GetMode(mode, hot);

        item.user[0] = static_cast<std::uint32_t>(m_mode);  // -> pc.user[4]
        item.user[1] = hot;                                 // -> pc.user[5]
        items.push_back(item);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    // a placement's axes, unit length and square -- the columns, normalized and made orthogonal (Gram-Schmidt); degenerate: the world's
    static glm::mat3 Basis(glm::mat4 const& placement) {
        glm::vec3 x = glm::vec3(placement[0]);
        glm::vec3 y = glm::vec3(placement[1]);
        if (glm::dot(x, x) < 1e-12f || glm::dot(y, y) < 1e-12f) return glm::mat3(1.0f);

        x = glm::normalize(x);
        y = y - x * glm::dot(x, y);
        if (glm::dot(y, y) < 1e-12f) return glm::mat3(1.0f);
        y = glm::normalize(y);

        glm::vec3 z = glm::cross(x, y);
        if (glm::dot(z, glm::vec3(placement[2])) < 0.0f) z = -z;  // a mirrored placement keeps its z side

        return glm::mat3(x, y, z);
    }

    static glm::vec3 Axis(glm::mat3 const& basis, std::uint32_t const handle) {
        return basis[handle == GIZMO_X ? 0 : handle == GIZMO_Y ? 1 : 2];
    }

    // the gizmo's world size at centre -- GIZMO_FRACTION of half the view's height there (perspective: grows with depth; ortho: fixed)
    static float Size(Ir77EditView const& view, glm::vec3 const& centre) {
        float const w = (view.view_proj * glm::vec4(centre, 1.0f)).w;
        return GIZMO_FRACTION * std::max(w, 1e-6f) / std::max(std::abs(view.proj[1][1]), 1e-6f);
    }

    // a world point -> pixels in the view (false: behind the eye)
    static bool Project(Ir77EditView const& view, glm::vec3 const& point, glm::vec2& pixel) {
        glm::vec4 const clip = view.view_proj * glm::vec4(point, 1.0f);
        if (clip.w <= 1e-6f) return false;

        pixel = (glm::vec2(clip) / clip.w * 0.5f + 0.5f) * view.size;
        return true;
    }

    // The gizmo's segments in its own units, world axes -- right / up = the camera's (the centre square faces it). Must match Segment() in
    // gizmo.vert, which turns every segment but the centre square's by the placement's axes, as Nearest does.
    //   translate: per axis a shaft and an arrowhead (3), then the centre square (4)
    //   rotate:    per axis a ring of GIZMO_RING_SEGMENTS
    //   scale:     per axis a shaft and a box at its tip (5), then the centre square (4)
    static std::vector<Ir77GizmoSegment> Segments(Ir77GizmoMode const mode, glm::vec3 const& right, glm::vec3 const& up) {
        std::vector<Ir77GizmoSegment> out{};

        auto const sides = [](std::uint32_t const a, glm::vec3& p, glm::vec3& q) {
            p = a == GIZMO_X ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
            q = a == GIZMO_Z ? glm::vec3(0, 1, 0) : glm::vec3(0, 0, 1);
        };

        auto const square = [&](glm::vec3 const& c, glm::vec3 const& p, glm::vec3 const& q, float const half, std::uint32_t const handle) {
            glm::vec3 const corners[4]{c + (-p - q) * half, c + (p - q) * half, c + (p + q) * half, c + (-p + q) * half};
            for (int k = 0; k < 4; k++) out.push_back({corners[k], corners[(k + 1) % 4], handle});
        };

        for (std::uint32_t a = 0; a < 3; a++) {
            glm::vec3 const axis = Axis(glm::mat3(1.0f), a);
            glm::vec3 p{0.0f};
            glm::vec3 q{0.0f};
            sides(a, p, q);

            if (mode == Ir77GizmoMode::Rotate) {
                for (std::uint32_t s = 0; s < GIZMO_RING_SEGMENTS; s++) {
                    float const t0 = glm::two_pi<float>() * static_cast<float>(s) / static_cast<float>(GIZMO_RING_SEGMENTS);
                    float const t1 = glm::two_pi<float>() * static_cast<float>(s + 1) / static_cast<float>(GIZMO_RING_SEGMENTS);
                    out.push_back({p * std::cos(t0) + q * std::sin(t0), p * std::cos(t1) + q * std::sin(t1), a});
                }
                continue;
            }

            out.push_back({glm::vec3(0.0f), axis, a});

            if (mode == Ir77GizmoMode::Translate) {
                glm::vec3 const base = axis * (1.0f - GIZMO_HEAD_LENGTH);
                out.push_back({axis, base + p * GIZMO_HEAD_WIDTH, a});
                out.push_back({axis, base - p * GIZMO_HEAD_WIDTH, a});
            } else {
                square(axis, p, q, GIZMO_BOX, a);
            }
        }

        if (mode != Ir77GizmoMode::Rotate) square(glm::vec3(0.0f), right, up, GIZMO_SQUARE, GIZMO_CENTRE);

        return out;
    }

    // the handle whose segment passes nearest the cursor, within GIZMO_PICK_PIXELS -- GIZMO_NONE: none
    std::uint32_t Nearest(Ir77EditView const& view, glm::mat4 const& placement) const {
        glm::vec3 const centre{placement[3]};
        glm::mat3 const basis = Basis(placement);

        float const size = Size(view, centre);
        glm::vec3 const right{view.view[0][0], view.view[1][0], view.view[2][0]};
        glm::vec3 const up{view.view[0][1], view.view[1][1], view.view[2][1]};

        std::uint32_t best = GIZMO_NONE;
        float nearest = GIZMO_PICK_PIXELS;

        for (Ir77GizmoSegment const& segment : Segments(m_mode, right, up)) {
            glm::vec3 const a = segment.handle == GIZMO_CENTRE ? segment.a : basis * segment.a;
            glm::vec3 const b = segment.handle == GIZMO_CENTRE ? segment.b : basis * segment.b;

            glm::vec2 pa{0.0f};
            glm::vec2 pb{0.0f};
            if (!Project(view, centre + a * size, pa) || !Project(view, centre + b * size, pb)) continue;

            glm::vec2 const along = pb - pa;
            float const length2 = glm::dot(along, along);
            float const s = length2 > 1e-12f ? std::clamp(glm::dot(view.cursor - pa, along) / length2, 0.0f, 1.0f) : 0.0f;

            float const pixels = glm::length(pa + along * s - view.cursor);
            if (pixels < nearest) {
                nearest = pixels;
                best = segment.handle;
            }
        }

        // inside the centre square counts as the centre
        if (best == GIZMO_NONE && m_mode != Ir77GizmoMode::Rotate) {
            glm::vec2 pc{0.0f};
            glm::vec2 corner{0.0f};
            if (Project(view, centre, pc) && Project(view, centre + (right + up) * (GIZMO_SQUARE * size), corner)) {
                glm::vec2 const half = glm::abs(corner - pc);
                glm::vec2 const off = glm::abs(view.cursor - pc);
                if (off.x <= half.x && off.y <= half.y) best = GIZMO_CENTRE;
            }
        }

        return best;
    }

    // where a ray meets the plane through the pivot facing the press's ray -- false when it runs along it or away from it
    bool OnPlane(glm::vec3 const& origin, glm::vec3 const& ray, glm::vec3& hit) const {
        float const facing = glm::dot(ray, m_drag.ray);
        if (std::abs(facing) < 1e-6f) return false;

        float const t = glm::dot(m_drag.pivot - origin, m_drag.ray) / facing;
        if (t <= 0.0f) return false;

        hit = origin + ray * t;
        return true;
    }

    // the point of the line through the pivot along axis nearest a ray, as its distance s along the axis -- false when they run parallel
    bool OnLine(glm::vec3 const& origin, glm::vec3 const& ray, glm::vec3 const& axis, float& s) const {
        glm::vec3 const w = m_drag.pivot - origin;
        float const b = glm::dot(axis, ray);
        float const denominator = 1.0f - b * b;  // both unit length
        if (denominator < 1e-6f) return false;

        s = (b * glm::dot(ray, w) - glm::dot(axis, w)) / denominator;
        return true;
    }

    std::shared_ptr<NSIr77PeregrineV::Ir77Server> m_renderer{};

    Ir77GizmoMode m_mode{Ir77GizmoMode::Translate};

    std::uint32_t m_hot{GIZMO_NONE};  // the hovered handle

    Ir77GizmoDrag m_drag{};

    Ir77PVPipelineHandle m_pipeline{IR77PV_NO_PIPELINE};

    bool m_missing{false};  // no edit.gizmo pipeline -- said once
};
}  // namespace NSIr77Scene
