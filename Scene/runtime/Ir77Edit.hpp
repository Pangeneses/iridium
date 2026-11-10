#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77Scene.hpp"
#include "../dictionary/IDIr77Scene.hpp"

#include "../../Ir77RT/types/Ir77Types.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"
#include "../../Ir77RT/runtime/Ir77Return.hpp"

#include "../interface/IIr77Mesh.hpp"
#include "../interface/IIr77Material.hpp"
#include "../interface/IIr77Edit.hpp"
#include "../interface/IIr77Gizmo.hpp"

#include "../types/Ir77Types.hpp"

#include "./Ir77Skinning.hpp"

// the direct link -- Scene -> PeregrineV's server, one way
#include "../../PeregrineV/server/Ir77Server.hpp"

using namespace NSIr77RT;
using namespace NSIr77PeregrineV;

namespace NSIr77Scene {

// =============================================================================================================================================
// Ir77Edit -- edit mode: the model's edit mesh, its selection, and the cage drawn over it (edit_faces.vert / edit_edges.vert /
// edit_points.vert / edit.frag). Owned by Ir77World, which builds it when a model goes into edit mode and adds its draws to every window's
// opaque list after the grid.
//
//   setup:     SetRenderer
//   edit mode: CreateCage(meshes, instance_base, bones, bone_bases) -- the edit mesh from the model's triangles: a vertex once per mesh
//              (a glTF vertex is split at every UV / normal seam -- each edit vertex keeps the glTF vertices it stands for), every triangle
//              side once as an edge, every triangle as a face. Skinned geometry in its rest pose (the same sum skinned.vert makes). Then the
//              cage: points (Ir77EditPoint, one per vertex per instance), edges, faces, and a flag per vertex. ReleaseCage when it leaves.
//   selecting: SetMode (vertex / edge / face), Pick (a click -- the nearest visible element), SelectAll. The selection is vertices; an edge or
//              face shows selected when all of its vertices are. Only the flags go up again.
//   grab:      BeginGrab (the selection, from the cursor's ray) -> MoveGrab (on the plane facing the camera through the selection's centre,
//              or along one world axis) -> EndGrab (confirm, or everything back). Each move rewrites the edit vertices, every glTF vertex
//              behind them (skinned ones through their rest skin matrix into the bind pose), the normals around the move -- split vertices
//              that shared a normal stay smooth, hard edges stay hard -- the cage's points, and the renderer's copy of just the stretch of
//              each stream that changed. A confirmed grab goes into the model's mesh (Ir77Mesh) -- what a save writes out -- and onto the undo
//              history (every stream vertex it touched, before and after); a cancelled one puts back exactly what it found.
//   history:   Undo / Redo -- a confirmed edit back or again, positions and normals as they were. Kept while the model is in edit mode.
//   per frame: AppendDraws(window, items) -- selected faces tinted, edges, then points (vertex mode). World puts them in the transparent list,
//              after the model's own transparent draws, so an alpha (BLEND) model never washes its cage out.
//
// The cage is placed on the GPU through the frame's instance table, so it follows the model's transform with no rebuild. A model in edit
// mode plays nothing (World holds its clips), so the rest pose the cage was built in is the pose it draws in.
// =============================================================================================================================================
class Ir77Edit : public Ir77Enlisted, public IIr77Edit, public std::enable_shared_from_this<Ir77Edit> {
   public:
    Ir77Edit() {
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
        seat_shared_uuid<&GUIDIr77Edit>(uid);

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

        else if (iid == GUIDIIr77Edit)
            obj = std::shared_ptr<IIr77Edit>(shared_from_this(), static_cast<IIr77Edit*>(this));

        else if (iid == GUIDIr77Edit)
            obj = std::shared_ptr<Ir77Edit>(shared_from_this(), static_cast<Ir77Edit*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }
   public:
    // PeregrineV's server -- the cage's storage buffers and draws go through it
    std::shared_ptr<IIr77Return const> SetRenderer(std::shared_ptr<NSIr77PeregrineV::Ir77Server> const& renderer) {
        m_renderer = renderer;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The transform gizmo -- World's, shared with object mode
    std::shared_ptr<IIr77Return const> SetGizmo(std::shared_ptr<IIr77Gizmo> const& gizmo) {
        m_gizmo = gizmo;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The model's triangles -> the edit mesh, uploaded as the cage; nothing selected, vertex mode. Replaces the cage before.
    std::shared_ptr<IIr77Return const> CreateCage(std::shared_ptr<IIr77Mesh> const& meshes, std::shared_ptr<IIr77Material> const& materials,
                                                  std::uint32_t const& asset, std::uint32_t const& instance_base, std::vector<glm::mat4> const& bones,
                                                  std::vector<std::uint32_t> const& bone_bases) {
        if (!m_renderer) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Edit: SetRenderer not called.");
        if (!meshes) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Edit: no meshes.");

        ReleaseCage();

        meshes->GetVertexBuffer(m_vertices);
        meshes->GetIndexBuffer(m_indices);
        meshes->GetMeshRanges(m_ranges);
        meshes->GetSkinnedVertexBuffer(m_vertices_skinned);
        meshes->GetSkinnedIndexBuffer(m_indices_skinned);
        meshes->GetSkinnedMeshRanges(m_ranges_skinned);

        // which ranges hide what's behind them -- every material but BLEND (MASK writes depth where it's solid; counted solid)
        auto const occluding = [&](std::vector<std::uint32_t> const& range_materials, std::size_t const count, std::vector<std::uint8_t>& out) {
            out.assign(count, 1);
            for (std::size_t r = 0; r < count && r < range_materials.size() && materials; r++) {
                bool blend{false};
                if (materials->GetMaterialBlend(range_materials[r], blend)->ID() == GUIDIr77OperationSucceeded && blend) out[r] = 0;
            }
        };

        std::vector<std::uint32_t> range_materials{};
        meshes->GetRangeMaterials(range_materials);
        occluding(range_materials, m_ranges.size(), m_range_occludes);

        range_materials.clear();
        meshes->GetSkinnedRangeMaterials(range_materials);
        occluding(range_materials, m_ranges_skinned.size(), m_range_occludes_skinned);

        m_meshes = meshes;
        m_asset = asset;
        m_bones = bones;
        m_bone_bases = bone_bases;
        m_modified = false;

        BuildMesh(m_mesh);
        m_instance_base = instance_base;
        m_mode = Ir77EditMode::Vertex;
        m_flags.assign(m_mesh.vertices.size(), 0u);

        if (m_mesh.vertices.empty() || m_mesh.faces.empty()) return Ir77RETURN<Ir77OperationSucceeded>(this, "[WARN] Ir77Edit: the model has no triangles -- no cage.");

        // the GPU side -- every vertex, edge and face once per instance of its mesh
        std::vector<Ir77EditPoint> points{};
        std::vector<glm::uvec2> edges{};
        std::vector<glm::uvec4> faces{};
        BuildCage(points, edges, faces);
        m_points_cpu = points;

        std::shared_ptr<IIr77Return const> result{};

        if (Failed(CreateBuffer(points.data(), points.size() * sizeof(Ir77EditPoint), m_points), result) ||
            Failed(CreateBuffer(edges.data(), edges.size() * sizeof(glm::uvec2), m_edges), result) ||
            Failed(CreateBuffer(faces.data(), faces.size() * sizeof(glm::uvec4), m_faces), result) ||
            Failed(CreateBuffer(m_flags.data(), m_flags.size() * sizeof(std::uint32_t), m_flags_gpu), result)) {
            ReleaseCage();
            return result;
        }

        m_point_count = static_cast<std::uint32_t>(points.size());
        m_edge_count = static_cast<std::uint32_t>(edges.size());
        m_face_count = static_cast<std::uint32_t>(faces.size());

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Ir77Edit: " + std::to_string(m_mesh.vertices.size()) + " vertices, " +
                                                            std::to_string(m_mesh.edges.size()) + " edges, " + std::to_string(m_mesh.faces.size()) + " faces.");
    }

    // No cage, no edit mesh -- DestroyStorageBuffer waits out the frames in flight
    std::shared_ptr<IIr77Return const> ReleaseCage() {
        if (m_grab.active) EndGrab(false);

        for (Ir77EditBuffer* buffer : {&m_points, &m_edges, &m_faces, &m_flags_gpu}) {
            if (buffer->alive) m_renderer->DestroyStorageBuffer(buffer->id);
            *buffer = Ir77EditBuffer{};
        }

        m_mesh = Ir77EditMesh{};
        m_flags.clear();
        m_points_cpu.clear();
        m_undo.clear();
        m_redo.clear();
        m_meshes.reset();
        m_point_count = 0;
        m_edge_count = 0;
        m_face_count = 0;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetMode(Ir77EditMode const& mode) {
        m_mode = mode;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // A click -- the nearest visible element under the cursor, by the mode
    std::shared_ptr<IIr77Return const> Pick(Ir77EditView const& view, std::vector<glm::mat4> const& transforms, bool const& extend, bool& hit) {
        hit = false;
        if (m_mesh.vertices.empty()) return Ir77RETURN<Ir77OperationSucceeded>();

        // every vertex of every instance where it sits this frame -- [vertex][instance]
        std::vector<std::vector<glm::vec3>> world{};
        WorldPositions(transforms, world);

        std::vector<std::uint32_t> element{};  // the vertices the hit stands for

        if (m_mode == Ir77EditMode::Vertex) hit = PickVertex(view, world, element);
        if (m_mode == Ir77EditMode::Edge) hit = PickEdge(view, world, element);
        if (m_mode == Ir77EditMode::Face) hit = PickFace(view, world, element);

        if (!extend) std::fill(m_flags.begin(), m_flags.end(), 0u);

        if (hit) {
            // extend: an element already wholly selected comes out, anything else goes in
            bool const all = std::all_of(element.begin(), element.end(), [&](std::uint32_t const v) { return (m_flags[v] & EDIT_SELECTED) != 0; });
            std::uint32_t const value = extend && all ? 0u : EDIT_SELECTED;

            for (std::uint32_t const v : element) m_flags[v] = value;
        }

        if (hit || !extend) return UploadFlags();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // A box drag: every visible element inside the box (pixels in the view, two opposite corners) -- vertices inside it; edges with both ends
    // inside; faces whose centre is inside. Visible = not behind a face that hides what's behind it, tested against a depth buffer of those
    // faces drawn over the box. extend: added to the selection; otherwise it replaces it. selected = the elements it took.
    std::shared_ptr<IIr77Return const> BoxSelect(Ir77EditView const& view, glm::vec2 const& a, glm::vec2 const& b, std::vector<glm::mat4> const& transforms,
                                                 bool const& extend, std::uint32_t& selected) {
        selected = 0;
        if (m_mesh.vertices.empty()) return Ir77RETURN<Ir77OperationSucceeded>();

        glm::vec2 const low = glm::min(a, b);
        glm::vec2 const high = glm::max(a, b);

        std::vector<std::vector<glm::vec3>> world{};
        WorldPositions(transforms, world);

        Ir77EditDepth depth{};
        BuildDepth(view, world, low, high, depth);

        // a world point inside the box and in front of (or on) the nearest hiding surface there
        auto const visible = [&](glm::vec3 const& point) {
            glm::vec4 const clip = view.view_proj * glm::vec4(point, 1.0f);
            if (clip.w <= 1e-6f) return false;

            glm::vec2 const pixel = (glm::vec2(clip) / clip.w * 0.5f + 0.5f) * view.size;
            if (pixel.x < low.x || pixel.y < low.y || pixel.x > high.x || pixel.y > high.y) return false;

            return depth.Visible(pixel, clip.z / clip.w);
        };

        std::vector<std::uint8_t> take(m_mesh.vertices.size(), 0);

        if (m_mode == Ir77EditMode::Vertex) {
            for (std::uint32_t v = 0; v < world.size(); v++)
                for (glm::vec3 const& point : world[v])
                    if (visible(point)) {
                        take[v] = 1;
                        selected++;
                        break;
                    }
        } else if (m_mode == Ir77EditMode::Edge) {
            for (glm::uvec2 const& e : m_mesh.edges)
                for (std::size_t i = 0; i < world[e.x].size(); i++)
                    if (visible(world[e.x][i]) && visible(world[e.y][i])) {
                        take[e.x] = take[e.y] = 1;
                        selected++;
                        break;
                    }
        } else {
            for (glm::uvec3 const& f : m_mesh.faces)
                for (std::size_t i = 0; i < world[f.x].size(); i++)
                    if (visible((world[f.x][i] + world[f.y][i] + world[f.z][i]) / 3.0f)) {
                        take[f.x] = take[f.y] = take[f.z] = 1;
                        selected++;
                        break;
                    }
        }

        for (std::size_t v = 0; v < m_flags.size(); v++)
            if (take[v]) m_flags[v] |= EDIT_SELECTED;
            else if (!extend) m_flags[v] &= ~EDIT_SELECTED;

        return UploadFlags();
    }

    std::shared_ptr<IIr77Return const> SelectAll(bool const& select) {
        std::fill(m_flags.begin(), m_flags.end(), select ? EDIT_SELECTED : 0u);

        return UploadFlags();
    }

    std::shared_ptr<IIr77Return const> GetSelection(Ir77EditMode& mode, std::uint32_t& selected) const {
        mode = m_mode;
        selected = static_cast<std::uint32_t>(std::count_if(m_flags.begin(), m_flags.end(), [](std::uint32_t const f) { return (f & EDIT_SELECTED) != 0; }));

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The cage's draws for one window -- selected faces, edges, then (vertex mode) points. The pipelines are looked up once; a manifest without
    // them leaves the cage out and says so once.
    std::shared_ptr<IIr77Return const> AppendDraws(std::size_t const& window, std::vector<Ir77PVDrawItem>& items) {
        if (!m_points.alive || m_missing || !m_renderer) return Ir77RETURN<Ir77OperationSucceeded>();

        if (m_faces_pipeline == IR77PV_NO_PIPELINE) {
            bool const found = m_renderer->FindPipeline(std::string{"edit.faces"}, m_faces_pipeline)->ID() == GUIDIr77OperationSucceeded &&
                               m_renderer->FindPipeline(std::string{"edit.edges"}, m_edges_pipeline)->ID() == GUIDIr77OperationSucceeded &&
                               m_renderer->FindPipeline(std::string{"edit.points"}, m_points_pipeline)->ID() == GUIDIr77OperationSucceeded;

            if (!found) {
                m_faces_pipeline = IR77PV_NO_PIPELINE;
                m_edges_pipeline = IR77PV_NO_PIPELINE;
                m_points_pipeline = IR77PV_NO_PIPELINE;
                m_missing = true;

                return Ir77RETURN<Ir77NotConfigured>(this, "[WARN] Ir77Edit: no \"edit.faces\" / \"edit.edges\" / \"edit.points\" pipelines in the manifest -- no cage.");
            }
        }

        // user -> pc.user[4..7]: points, the element buffer (faces / edges), the flags, the mode
        auto const draw = [&](Ir77PVPipelineHandle const pipeline, std::uint32_t const vertices, std::uint32_t const elements_slot) {
            Ir77PVDrawItem item{};
            if (vertices == 0 || m_renderer->MakeProceduralDrawItem(window, pipeline, vertices, 1, item)->ID() != GUIDIr77OperationSucceeded) return;

            item.user[0] = m_points.slot;
            item.user[1] = elements_slot;
            item.user[2] = m_flags_gpu.slot;
            item.user[3] = static_cast<std::uint32_t>(m_mode);
            items.push_back(item);
        };

        draw(m_faces_pipeline, m_face_count * EDIT_VERTICES_PER_FACE, m_faces.slot);
        draw(m_edges_pipeline, m_edge_count * EDIT_VERTICES_PER_ELEMENT, m_edges.slot);
        if (m_mode == Ir77EditMode::Vertex) draw(m_points_pipeline, m_point_count * EDIT_VERTICES_PER_ELEMENT, m_edges.slot);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // ---- grab -----------------------------------------------------------------------------------------------------------------------------

    // A grab of the selection from the cursor's ray -- the plane facing the camera (the start ray's direction) through the selection's centre
    // is what it slides on. Nothing selected: grabbing = false. The selection's positions now are what EndGrab(false) puts back.
    std::shared_ptr<IIr77Return const> BeginGrab(Ir77EditView const& view, std::vector<glm::mat4> const& transforms, bool& grabbing) {
        grabbing = false;
        if (m_grab.active) EndGrab(false);

        Ir77EditGrab grab{};

        glm::vec3 centre{0.0f};
        std::uint32_t count{0};

        for (std::uint32_t v = 0; v < m_mesh.vertices.size(); v++) {
            if ((m_flags[v] & EDIT_SELECTED) == 0) continue;

            Ir77EditVertex const& vertex = m_mesh.vertices[v];
            grab.vertices.push_back({v, vertex.position});

            for (Ir77EditSource const& source : vertex.sources)
                grab.sources.push_back({source, source.skinned ? m_vertices_skinned[source.vertex].position : m_vertices[source.vertex].position});

            glm::vec3 const world = glm::vec3(GroupModel(transforms, vertex.group) * glm::vec4(vertex.position, 1.0f));
            grab.start_world.push_back(world);

            centre += world;
            count++;
        }

        if (count == 0) return Ir77RETURN<Ir77OperationSucceeded>();

        // each mesh's world -> instance space (its first instance -- the one the cage is moved through)
        grab.to_instance.resize(m_mesh.groups.size());
        for (std::uint32_t g = 0; g < m_mesh.groups.size(); g++) grab.to_instance[g] = glm::inverse(GroupModel(transforms, g));

        grab.pivot = centre / static_cast<float>(count);
        grab.normal = view.ray;
        grab.origin = view.origin;
        grab.ray = view.ray;
        grab.active = true;

        m_grab = std::move(grab);
        grabbing = true;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The cursor's ray now -> the selection moved by where it meets the plane (or the axis line) less where the start ray did. delta = the
    // move, world space. The model, the cage and the normals around the move follow at once.
    std::shared_ptr<IIr77Return const> MoveGrab(Ir77EditView const& view, Ir77EditAxis const& axis, glm::vec3& delta) {
        delta = glm::vec3(0.0f);
        if (!m_grab.active) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Edit: no grab.");

        // a gizmo drag -- the gizmo's matrix about the pivot it was pressed at
        if (m_grab.handle != GIZMO_NONE) {
            if (!m_gizmo) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Edit: no gizmo.");

            glm::mat4 world{1.0f};
            bool changed{false};

            std::shared_ptr<IIr77Return const> result{};
            if (Failed(m_gizmo->Drag(view, world, changed), result)) return result;

            delta = glm::vec3(world[3]);
            m_grab.changed = changed;

            return ApplyGrab(world);
        }

        if (axis == Ir77EditAxis::Free) {
            glm::vec3 start{0.0f};
            glm::vec3 now{0.0f};
            if (!OnPlane(m_grab.origin, m_grab.ray, start) || !OnPlane(view.origin, view.ray, now)) delta = m_grab.delta;  // grazing -- hold
            else delta = now - start;
        } else {
            glm::vec3 const line = axis == Ir77EditAxis::X ? glm::vec3(1, 0, 0) : axis == Ir77EditAxis::Y ? glm::vec3(0, 1, 0) : glm::vec3(0, 0, 1);

            float start{0.0f};
            float now{0.0f};
            if (!OnLine(m_grab.origin, m_grab.ray, line, start) || !OnLine(view.origin, view.ray, line, now)) delta = m_grab.delta;
            else delta = line * (now - start);
        }

        m_grab.delta = delta;
        m_grab.changed = glm::dot(delta, delta) > 0.0f;

        return ApplyGrab(glm::translate(glm::mat4(1.0f), delta));
    }

    // confirm: the move stays, written into the model's mesh (what a save writes out); otherwise everything goes back to where it started
    std::shared_ptr<IIr77Return const> EndGrab(bool const& confirm) {
        if (!m_grab.active) return Ir77RETURN<Ir77OperationSucceeded>();

        // the step -- before: where the grab found everything it touched; after: where it is now
        Ir77EditStep step{};
        for (Ir77EditGrabVertex const& grabbed : m_grab.vertices) {
            step.before.push_back({grabbed.vertex, grabbed.start});
            step.after.push_back({grabbed.vertex, m_mesh.vertices[grabbed.vertex].position});
        }
        for (auto const& [key, state] : m_grab.captured) {
            step.stream_before.push_back(state);
            step.stream_after.push_back(StreamState(state.source));
        }

        bool const moved = confirm && m_grab.changed;
        if (m_grab.handle != GIZMO_NONE && m_gizmo) m_gizmo->End();
        m_grab = Ir77EditGrab{};

        // cancelled, or confirmed where it started -- exactly as it was (the file's normals too, not rebuilt ones)
        if (!moved) {
            if (step.stream_before.empty()) return Ir77RETURN<Ir77OperationSucceeded>();

            std::shared_ptr<IIr77Return const> result{};
            if (Failed(ApplyState(step.before, step.stream_before), result)) return result;

            return Ir77RETURN<Ir77OperationSucceeded>();
        }

        m_undo.push_back(std::move(step));
        if (m_undo.size() > EDIT_UNDO_STEPS) m_undo.erase(m_undo.begin());
        m_redo.clear();

        m_modified = true;

        return WriteMesh();
    }

    // The last confirmed edit back (undo) or again (redo). Nothing to do: done = false. A grab under way is cancelled first.
    std::shared_ptr<IIr77Return const> Undo(bool& done) {
        done = false;
        if (m_grab.active) EndGrab(false);
        if (m_undo.empty()) return Ir77RETURN<Ir77OperationSucceeded>();

        Ir77EditStep step = std::move(m_undo.back());
        m_undo.pop_back();

        std::shared_ptr<IIr77Return const> result{};
        if (Failed(ApplyState(step.before, step.stream_before), result)) return result;

        m_redo.push_back(std::move(step));
        done = true;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> Redo(bool& done) {
        done = false;
        if (m_grab.active) EndGrab(false);
        if (m_redo.empty()) return Ir77RETURN<Ir77OperationSucceeded>();

        Ir77EditStep step = std::move(m_redo.back());
        m_redo.pop_back();

        std::shared_ptr<IIr77Return const> result{};
        if (Failed(ApplyState(step.after, step.stream_after), result)) return result;

        m_undo.push_back(std::move(step));
        m_modified = true;
        done = true;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // the model's file now holds every confirmed edit -- modified until the next one
    std::shared_ptr<IIr77Return const> MarkSaved() {
        m_modified = false;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // how many steps undo and redo can go
    std::shared_ptr<IIr77Return const> GetHistory(std::uint32_t& undo, std::uint32_t& redo) const {
        undo = static_cast<std::uint32_t>(m_undo.size());
        redo = static_cast<std::uint32_t>(m_redo.size());

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // grabbing = a grab is under way; modified = a confirmed edit since the cage was built (what a save has to write)
    std::shared_ptr<IIr77Return const> GetGrab(bool& grabbing, bool& modified) const {
        grabbing = m_grab.active;
        modified = m_modified;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // ---- gizmo ----------------------------------------------------------------------------------------------------------------------------

    std::shared_ptr<IIr77Return const> SetGizmoMode(Ir77GizmoMode const& mode) {
        if (m_grab.active && m_grab.handle != GIZMO_NONE) EndGrab(false);  // a transform under way was the old mode's
        if (!m_gizmo) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Edit: no gizmo.");

        return m_gizmo->SetMode(mode);
    }

    std::shared_ptr<IIr77Return const> GetGizmo(Ir77GizmoMode& mode, std::uint32_t& hot) const {
        mode = Ir77GizmoMode::Translate;
        hot = GIZMO_NONE;
        if (!m_gizmo) return Ir77RETURN<Ir77OperationSucceeded>();

        return m_gizmo->GetMode(mode, hot);
    }

    std::shared_ptr<IIr77Return const> HoverGizmo(Ir77EditView const& view, std::vector<glm::mat4> const& transforms, std::uint32_t& handle) {
        handle = GIZMO_NONE;
        if (!m_gizmo) return Ir77RETURN<Ir77OperationSucceeded>();

        glm::mat4 placement{1.0f};
        if (!Placement(transforms, placement)) return m_gizmo->ClearHot();

        return m_gizmo->Hover(view, placement, handle);
    }

    std::shared_ptr<IIr77Return const> ClearGizmoHot() {
        if (!m_gizmo) return Ir77RETURN<Ir77OperationSucceeded>();

        return m_gizmo->ClearHot();
    }

    // a press on a handle -> the selection's grab, then the gizmo's drag on it; MoveGrab / EndGrab drive both
    std::shared_ptr<IIr77Return const> PressGizmo(Ir77EditView const& view, std::vector<glm::mat4> const& transforms, std::uint32_t& handle) {
        handle = GIZMO_NONE;
        if (!m_gizmo) return Ir77RETURN<Ir77OperationSucceeded>();

        glm::mat4 placement{1.0f};
        if (!Placement(transforms, placement)) return Ir77RETURN<Ir77OperationSucceeded>();

        std::shared_ptr<IIr77Return const> result{};

        std::uint32_t at{GIZMO_NONE};
        if (Failed(m_gizmo->HandleAt(view, placement, at), result)) return result;
        if (at == GIZMO_NONE) return Ir77RETURN<Ir77OperationSucceeded>();

        bool grabbing{false};
        if (Failed(BeginGrab(view, transforms, grabbing), result)) return result;
        if (!grabbing) return Ir77RETURN<Ir77OperationSucceeded>();

        if (Failed(m_gizmo->Press(view, placement, handle), result)) return result;

        m_grab.handle = handle;
        if (handle == GIZMO_NONE) EndGrab(false);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The gizmo at the selection's median, world axes
    std::shared_ptr<IIr77Return const> AppendGizmo(std::size_t const& window, std::vector<glm::mat4> const& transforms, std::vector<Ir77PVDrawItem>& items) {
        if (!m_gizmo || m_mesh.vertices.empty()) return Ir77RETURN<Ir77OperationSucceeded>();

        glm::mat4 placement{1.0f};
        if (!Placement(transforms, placement)) return Ir77RETURN<Ir77OperationSucceeded>();

        return m_gizmo->Append(window, placement, items);
    }

    std::shared_ptr<IIr77Return const> GetCageCounts(std::uint32_t& points, std::uint32_t& edges) const {
        points = static_cast<std::uint32_t>(m_mesh.vertices.size());
        edges = static_cast<std::uint32_t>(m_mesh.edges.size());

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    // one edit vertex / one stream vertex at one moment -- what a step puts back
    struct Ir77EditVertexState {
        std::uint32_t vertex{0};
        glm::vec3 position{0.0f};
    };

    struct Ir77EditStreamState {
        Ir77EditSource source{};
        glm::vec3 position{0.0f};
        glm::vec3 normal{0.0f};
    };

    // one confirmed edit -- everything it changed, before and after
    struct Ir77EditStep {
        std::vector<Ir77EditVertexState> before{};
        std::vector<Ir77EditVertexState> after{};
        std::vector<Ir77EditStreamState> stream_before{};
        std::vector<Ir77EditStreamState> stream_after{};
    };

    // a grab under way -- what it moves, from where, on what
    struct Ir77EditGrabVertex {
        std::uint32_t vertex{0};
        glm::vec3 start{0.0f};  // instance space
    };

    struct Ir77EditGrabSource {
        Ir77EditSource source{};
        glm::vec3 start{0.0f};  // its stream's space -- skinned: the bind pose
    };

    struct Ir77EditGrab {
        bool active{false};
        std::uint32_t handle{GIZMO_NONE};          // the gizmo handle driving it (Ir77Gizmo has the drag); GIZMO_NONE: a free grab
        std::vector<glm::vec3> start_world{};      // per grabbed vertex -- world space, through its mesh's first instance
        std::vector<Ir77EditGrabVertex> vertices{};
        std::vector<Ir77EditGrabSource> sources{};
        std::vector<glm::mat4> to_instance{};  // per group -- world -> its first instance's space
        glm::vec3 pivot{0.0f};                 // the selection's centre, world space
        glm::vec3 normal{0.0f, 0.0f, -1.0f};   // the plane's -- the start ray
        glm::vec3 origin{0.0f};                // the start ray
        glm::vec3 ray{0.0f, 0.0f, -1.0f};
        glm::vec3 delta{0.0f};                 // the last move (a translate)
        bool changed{false};                   // the selection is somewhere other than where it started
        std::unordered_map<std::uint64_t, Ir77EditStreamState> captured{};  // SourceKey -> as it was before the grab first touched it
    };

    // ---- box select, inside ------------------------------------------------------------------------------------------------------------

    // A small depth buffer over a box of the view -- the nearest hiding face's reversed-Z depth (bigger = nearer) per cell. A point passes when
    // it's at least as near as the surface there, less the cage's own bias, so a vertex on the surface counts as visible.
    struct Ir77EditDepth {
        glm::vec2 low{0.0f};
        float scale{1.0f};  // cells per pixel
        std::uint32_t width{0};
        std::uint32_t height{0};
        std::vector<float> nearest{};  // 0 = nothing (the far plane)

        bool Visible(glm::vec2 const& pixel, float const z) const {
            if (width == 0 || height == 0) return true;

            std::uint32_t const x = std::min(static_cast<std::uint32_t>(std::max((pixel.x - low.x) * scale, 0.0f)), width - 1);
            std::uint32_t const y = std::min(static_cast<std::uint32_t>(std::max((pixel.y - low.y) * scale, 0.0f)), height - 1);

            return z * (1.0f + EDIT_BOX_DEPTH_BIAS) >= nearest[y * width + x];
        }
    };

    // Every face that hides what's behind it (not BLEND), every instance, drawn into the box's cells -- z / w interpolated across the triangle
    // on screen (it's linear there). A face with a corner behind the eye is left out.
    void BuildDepth(Ir77EditView const& view, std::vector<std::vector<glm::vec3>> const& world, glm::vec2 const& low, glm::vec2 const& high,
                    Ir77EditDepth& depth) const {
        glm::vec2 const extent = glm::max(high - low, glm::vec2(1.0f));

        depth.low = low;
        depth.scale = std::min(1.0f, static_cast<float>(EDIT_BOX_DEPTH_SIDE) / std::max(extent.x, extent.y));
        depth.width = static_cast<std::uint32_t>(std::ceil(extent.x * depth.scale)) + 1;
        depth.height = static_cast<std::uint32_t>(std::ceil(extent.y * depth.scale)) + 1;
        depth.nearest.assign(static_cast<std::size_t>(depth.width) * depth.height, 0.0f);

        // a world point -> its cell coordinates and z / w
        auto const cell = [&](glm::vec3 const& point, glm::vec3& out) {
            glm::vec4 const clip = view.view_proj * glm::vec4(point, 1.0f);
            if (clip.w <= 1e-6f) return false;

            glm::vec2 const pixel = (glm::vec2(clip) / clip.w * 0.5f + 0.5f) * view.size;
            out = glm::vec3((pixel - low) * depth.scale, clip.z / clip.w);
            return true;
        };

        for (std::size_t face = 0; face < m_mesh.faces.size(); face++) {
            if (face < m_face_occludes.size() && m_face_occludes[face] == 0) continue;

            glm::uvec3 const& f = m_mesh.faces[face];

            for (std::size_t i = 0; i < world[f.x].size(); i++) {
                glm::vec3 p0{0.0f};
                glm::vec3 p1{0.0f};
                glm::vec3 p2{0.0f};
                if (!cell(world[f.x][i], p0) || !cell(world[f.y][i], p1) || !cell(world[f.z][i], p2)) continue;

                float const area = (p1.x - p0.x) * (p2.y - p0.y) - (p1.y - p0.y) * (p2.x - p0.x);
                if (std::abs(area) < 1e-12f) continue;

                int const x0 = std::max(0, static_cast<int>(std::floor(std::min({p0.x, p1.x, p2.x}))));
                int const y0 = std::max(0, static_cast<int>(std::floor(std::min({p0.y, p1.y, p2.y}))));
                int const x1 = std::min(static_cast<int>(depth.width) - 1, static_cast<int>(std::ceil(std::max({p0.x, p1.x, p2.x}))));
                int const y1 = std::min(static_cast<int>(depth.height) - 1, static_cast<int>(std::ceil(std::max({p0.y, p1.y, p2.y}))));

                for (int y = y0; y <= y1; y++)
                    for (int x = x0; x <= x1; x++) {
                        glm::vec2 const q{static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f};

                        float const w0 = ((p1.x - q.x) * (p2.y - q.y) - (p1.y - q.y) * (p2.x - q.x)) / area;
                        float const w1 = ((p2.x - q.x) * (p0.y - q.y) - (p2.y - q.y) * (p0.x - q.x)) / area;
                        float const w2 = 1.0f - w0 - w1;
                        if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f) continue;

                        float const z = w0 * p0.z + w1 * p1.z + w2 * p2.z;
                        float& at = depth.nearest[static_cast<std::size_t>(y) * depth.width + x];
                        at = std::max(at, z);
                    }
            }
        }
    }

    // ---- gizmo, inside --------------------------------------------------------------------------------------------------------------------

    // the selection's median -- the mean of its vertices, world space, through each mesh's first instance. false: nothing selected.
    bool Median(std::vector<glm::mat4> const& transforms, glm::vec3& centre) const {
        if (m_grab.active) {
            // under way -- where the selection is now (the pivot moved with a translate; it stays for a turn or a size)
            glm::vec3 sum{0.0f};
            for (Ir77EditGrabVertex const& grabbed : m_grab.vertices) {
                Ir77EditVertex const& vertex = m_mesh.vertices[grabbed.vertex];
                sum += glm::vec3(GroupModel(transforms, vertex.group) * glm::vec4(vertex.position, 1.0f));
            }

            if (m_grab.vertices.empty()) return false;
            centre = sum / static_cast<float>(m_grab.vertices.size());
            return true;
        }

        glm::vec3 sum{0.0f};
        std::uint32_t count{0};

        for (std::uint32_t v = 0; v < m_mesh.vertices.size() && v < m_flags.size(); v++) {
            if ((m_flags[v] & EDIT_SELECTED) == 0) continue;

            Ir77EditVertex const& vertex = m_mesh.vertices[v];
            sum += glm::vec3(GroupModel(transforms, vertex.group) * glm::vec4(vertex.position, 1.0f));
            count++;
        }

        if (count == 0) return false;

        centre = sum / static_cast<float>(count);
        return true;
    }

    // the gizmo's placement -- at the selection's median, world axes. false: nothing selected.
    bool Placement(std::vector<glm::mat4> const& transforms, glm::mat4& placement) const {
        glm::vec3 centre{0.0f};
        if (!Median(transforms, centre)) return false;

        placement = glm::translate(glm::mat4(1.0f), centre);
        return true;
    }

    // a group's first instance this frame -- the cage moves through it
    glm::mat4 GroupModel(std::vector<glm::mat4> const& transforms, std::uint32_t const group) const {
        std::size_t const local = m_mesh.groups[group].first_instance;
        return local < transforms.size() ? transforms[local] : glm::mat4(1.0f);
    }

    // where a ray meets the grab's plane -- false when it runs along it or away from it
    bool OnPlane(glm::vec3 const& origin, glm::vec3 const& ray, glm::vec3& hit) const {
        float const facing = glm::dot(ray, m_grab.normal);
        if (std::abs(facing) < 1e-6f) return false;

        float const t = glm::dot(m_grab.pivot - origin, m_grab.normal) / facing;
        if (t <= 0.0f) return false;

        hit = origin + ray * t;
        return true;
    }

    // the point of the line through the pivot along axis nearest a ray, as its distance s along the axis -- false when they run parallel
    bool OnLine(glm::vec3 const& origin, glm::vec3 const& ray, glm::vec3 const& axis, float& s) const {
        glm::vec3 const w = m_grab.pivot - origin;
        float const b = glm::dot(axis, ray);
        float const denominator = 1.0f - b * b;  // both unit length
        if (denominator < 1e-6f) return false;

        s = (b * glm::dot(ray, w) - glm::dot(axis, w)) / denominator;
        return true;
    }

    // The selection at its start positions + delta (world): the edit vertices, every glTF vertex behind them (skinned ones moved back through
    // their rest skin matrix into the bind pose), the normals of every triangle around them, the cage points -- then the renderer, only the
    // stretch of each stream that changed.
    std::shared_ptr<IIr77Return const> ApplyGrab(glm::mat4 const& world) {
        // each grabbed vertex's start through the world transform, back into its mesh's instance space -- what it moved there
        m_grab_moves.assign(m_mesh.vertices.size(), glm::vec3(0.0f));

        for (std::size_t k = 0; k < m_grab.vertices.size(); k++) {
            Ir77EditGrabVertex const& grabbed = m_grab.vertices[k];
            Ir77EditVertex& vertex = m_mesh.vertices[grabbed.vertex];

            glm::vec3 const moved_world = glm::vec3(world * glm::vec4(m_grab.start_world[k], 1.0f));
            vertex.position = glm::vec3(m_grab.to_instance[vertex.group] * glm::vec4(moved_world, 1.0f));
            m_grab_moves[grabbed.vertex] = vertex.position - grabbed.start;

            Ir77EditGroup const& group = m_mesh.groups[vertex.group];
            for (std::uint32_t i = 0; i < group.instance_count; i++) m_points_cpu[m_point_start[grabbed.vertex] + i].position = vertex.position;
        }

        // the glTF vertices -- and the stretch of each stream that changed
        glm::uvec2 span[2]{glm::uvec2(UINT32_MAX, 0u), glm::uvec2(UINT32_MAX, 0u)};  // low, high (exclusive) per stream
        auto const touch = [&](int const s, std::uint32_t const v) {
            span[s].x = std::min(span[s].x, v);
            span[s].y = std::max(span[s].y, v + 1);
        };

        for (Ir77EditGrabSource const& grabbed : m_grab.sources) {
            std::uint32_t const v = grabbed.source.vertex;

            glm::vec3 shift{0.0f};

            Capture(grabbed.source);

            if (grabbed.source.skinned) {
                Ir77PVVertexSkinned& vertex = m_vertices_skinned[v];
                glm::mat4 const skin = Ir77SkinMatrix(vertex, m_bones, SkinBase(v));
                shift = glm::vec3(glm::inverse(skin) * glm::vec4(SourceMove(grabbed.source), 0.0f));
                vertex.position = grabbed.start + shift;
                touch(1, v);
            } else {
                m_vertices[v].position = grabbed.start + SourceMove(grabbed.source);
                touch(0, v);
            }
        }

        // the normals around the move -- every triangle with a moved corner, then every corner of those
        for (int s = 0; s < 2; s++) {
            if (span[s].x == UINT32_MAX) continue;

            std::unordered_set<std::uint32_t> triangles{};
            for (Ir77EditGrabSource const& grabbed : m_grab.sources)
                if ((grabbed.source.skinned ? 1 : 0) == s)
                    for (std::uint32_t const t : m_vertex_triangles[s][grabbed.source.vertex]) triangles.insert(t);

            std::unordered_set<std::uint32_t> corners{};
            for (std::uint32_t const t : triangles) {
                glm::uvec3 const& triangle = m_triangles[s][t];
                corners.insert(triangle.x);
                corners.insert(triangle.y);
                corners.insert(triangle.z);
            }

            for (std::uint32_t const v : corners) {
                glm::vec3 sum{0.0f};

                // its own triangles, and those of the glTF vertices split from it that shared its normal (a UV seam, not a hard edge)
                glm::vec3 const was = m_original_normals[s][v];
                auto const vertex = m_source_vertex.find(SourceKey(Ir77EditSource{s == 1, v}));

                if (vertex == m_source_vertex.end()) {
                    for (std::uint32_t const t : m_vertex_triangles[s][v]) sum += FaceNormal(s, m_triangles[s][t]);
                } else {
                    for (Ir77EditSource const& sibling : m_mesh.vertices[vertex->second].sources) {
                        if ((sibling.skinned ? 1 : 0) != s || glm::dot(m_original_normals[s][sibling.vertex], was) < EDIT_SMOOTH_DOT) continue;

                        for (std::uint32_t const t : m_vertex_triangles[s][sibling.vertex]) sum += FaceNormal(s, m_triangles[s][t]);  // area weighted
                    }
                }

                float const length = glm::length(sum);
                if (length < 1e-12f) continue;  // degenerate all round -- keep what it had

                glm::vec3 const normal = sum / length;

                // it and its smooth siblings -- one normal, so the seam stays invisible
                auto const set = [&](std::uint32_t const at) {
                    Capture(Ir77EditSource{s == 1, at});
                    (s == 0 ? m_vertices[at].normal : m_vertices_skinned[at].normal) = normal;
                    touch(s, at);
                };

                set(v);

                if (vertex != m_source_vertex.end())
                    for (Ir77EditSource const& sibling : m_mesh.vertices[vertex->second].sources)
                        if ((sibling.skinned ? 1 : 0) == s && glm::dot(m_original_normals[s][sibling.vertex], was) >= EDIT_SMOOTH_DOT) set(sibling.vertex);
            }
        }

        return Upload(span);
    }

    // The cage's points, then the stretch of each stream in span (low, high exclusive; low UINT32_MAX = nothing) to the renderer
    std::shared_ptr<IIr77Return const> Upload(glm::uvec2 const (&span)[2]) {
        std::shared_ptr<IIr77Return const> result{};

        if (m_points.alive && Failed(m_renderer->UploadStorageBuffer(m_points.id, m_points_cpu.data(),
                                                                     static_cast<VkDeviceSize>(m_points_cpu.size() * sizeof(Ir77EditPoint)), 0, false),
                                     result))
            return result;

        glm::uvec2 const static_range = span[0].x == UINT32_MAX ? glm::uvec2(0u) : glm::uvec2(span[0].x, span[0].y - span[0].x);
        glm::uvec2 const skinned_range = span[1].x == UINT32_MAX ? glm::uvec2(0u) : glm::uvec2(span[1].x, span[1].y - span[1].x);

        if (static_range.y == 0 && skinned_range.y == 0) return Ir77RETURN<Ir77OperationSucceeded>();

        return m_renderer->UpdateAssetVertices(m_asset, m_vertices, m_vertices_skinned, static_range, skinned_range);
    }

    // a stream vertex as it was before this grab first touched it -- what cancel and undo put back
    void Capture(Ir77EditSource const& source) {
        std::uint64_t const key = SourceKey(source);
        if (m_grab.captured.count(key)) return;

        m_grab.captured.emplace(key, StreamState(source));
    }

    Ir77EditStreamState StreamState(Ir77EditSource const& source) const {
        return source.skinned ? Ir77EditStreamState{source, m_vertices_skinned[source.vertex].position, m_vertices_skinned[source.vertex].normal}
                              : Ir77EditStreamState{source, m_vertices[source.vertex].position, m_vertices[source.vertex].normal};
    }

    // One side of a step back in place -- the edit vertices (and their cage points), the stream vertices' positions and normals -- then the
    // renderer and the model's mesh
    std::shared_ptr<IIr77Return const> ApplyState(std::vector<Ir77EditVertexState> const& vertices, std::vector<Ir77EditStreamState> const& stream) {
        for (Ir77EditVertexState const& state : vertices) {
            if (state.vertex >= m_mesh.vertices.size()) continue;

            Ir77EditVertex& vertex = m_mesh.vertices[state.vertex];
            vertex.position = state.position;

            Ir77EditGroup const& group = m_mesh.groups[vertex.group];
            for (std::uint32_t i = 0; i < group.instance_count; i++) m_points_cpu[m_point_start[state.vertex] + i].position = state.position;
        }

        glm::uvec2 span[2]{glm::uvec2(UINT32_MAX, 0u), glm::uvec2(UINT32_MAX, 0u)};

        for (Ir77EditStreamState const& state : stream) {
            int const s = state.source.skinned ? 1 : 0;
            std::uint32_t const v = state.source.vertex;

            if (s == 1 && v < m_vertices_skinned.size()) {
                m_vertices_skinned[v].position = state.position;
                m_vertices_skinned[v].normal = state.normal;
            } else if (s == 0 && v < m_vertices.size()) {
                m_vertices[v].position = state.position;
                m_vertices[v].normal = state.normal;
            } else {
                continue;
            }

            span[s].x = std::min(span[s].x, v);
            span[s].y = std::max(span[s].y, v + 1);
        }

        std::shared_ptr<IIr77Return const> result{};
        if (Failed(Upload(span), result)) return result;

        return WriteMesh();
    }

    // the streams into the model's mesh -- what picking, the next cage and a save read
    std::shared_ptr<IIr77Return const> WriteMesh() {
        std::shared_ptr<IIr77Return const> result{};
        if (!m_meshes) return Ir77RETURN<Ir77OperationSucceeded>();

        if (!m_vertices.empty() && Failed(m_meshes->UpdateVertices(m_vertices), result)) return result;
        if (!m_vertices_skinned.empty() && Failed(m_meshes->UpdateSkinnedVertices(m_vertices_skinned), result)) return result;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // a source's move in its instance's space -- its edit vertex's
    glm::vec3 SourceMove(Ir77EditSource const& source) const {
        auto const found = m_source_vertex.find(SourceKey(source));
        return found != m_source_vertex.end() && found->second < m_grab_moves.size() ? m_grab_moves[found->second] : glm::vec3(0.0f);
    }

    static std::uint64_t SourceKey(Ir77EditSource const& source) { return (static_cast<std::uint64_t>(source.skinned ? 1 : 0) << 32) | source.vertex; }

    // where a skinned vertex's palette starts -- its range's first instance's
    std::uint32_t SkinBase(std::uint32_t const v) const {
        auto const found = m_skin_base.find(v);
        return found != m_skin_base.end() ? found->second : 0u;
    }

    // a stream triangle's normal, area weighted (unnormalized cross)
    glm::vec3 FaceNormal(int const s, glm::uvec3 const& t) const {
        glm::vec3 const a = s == 0 ? m_vertices[t.x].position : m_vertices_skinned[t.x].position;
        glm::vec3 const b = s == 0 ? m_vertices[t.y].position : m_vertices_skinned[t.y].position;
        glm::vec3 const c = s == 0 ? m_vertices[t.z].position : m_vertices_skinned[t.z].position;
        return glm::cross(b - a, c - a);
    }

    // one storage buffer of the cage
    struct Ir77EditBuffer {
        bool alive{false};
        std::uint32_t id{0};
        std::uint32_t slot{BINDLESS_NO_SLOT};
    };

    // a welded vertex -- its mesh and the exact position bits
    struct Ir77EditKey {
        std::uint32_t group{0};
        std::uint32_t x{0};
        std::uint32_t y{0};
        std::uint32_t z{0};

        bool operator==(Ir77EditKey const& other) const = default;
    };

    struct Ir77EditKeyHash {
        std::size_t operator()(Ir77EditKey const& key) const {
            std::uint64_t h = 1469598103934665603ull;  // FNV-1a over the four words
            for (std::uint32_t const word : {key.group, key.x, key.y, key.z}) {
                h ^= word;
                h *= 1099511628211ull;
            }
            return static_cast<std::size_t>(h);
        }
    };

    std::shared_ptr<IIr77Return const> CreateBuffer(void const* data, std::size_t const bytes, Ir77EditBuffer& buffer) {
        std::shared_ptr<IIr77Return const> result{};

        // an empty list still gets a buffer -- the shaders always have a slot to index
        std::uint32_t const zero[4]{};
        void const* const source = bytes > 0 ? data : zero;
        std::size_t const size = bytes > 0 ? bytes : sizeof(zero);

        if (Failed(m_renderer->CreateStorageBuffer(static_cast<VkDeviceSize>(size), source, buffer.id, buffer.slot), result)) return result;

        buffer.alive = true;
        return result;
    }

    // the flags to the GPU -- in submission order, ahead of the next frame
    std::shared_ptr<IIr77Return const> UploadFlags() {
        if (!m_flags_gpu.alive || m_flags.empty()) return Ir77RETURN<Ir77OperationSucceeded>();

        return m_renderer->UploadStorageBuffer(m_flags_gpu.id, m_flags.data(), static_cast<VkDeviceSize>(m_flags.size() * sizeof(std::uint32_t)), 0, false);
    }

    // The model's two streams -> the edit mesh. A mesh's ranges share its instances (the same first_instance), so first_instance names the
    // mesh: vertices weld within a mesh by exact position, edges are every triangle side once, faces every triangle.
    // The streams as they were read (LoadStreams); every triangle of a stream also goes into its adjacency, for the normals an edit rebuilds.
    void BuildMesh(Ir77EditMesh& mesh) {
        mesh = Ir77EditMesh{};

        std::vector<Ir77PVVertex> const& vertices = m_vertices;
        std::vector<std::uint32_t> const& indices = m_indices;
        std::vector<Ir77MeshRange> const& ranges = m_ranges;
        std::vector<Ir77PVVertexSkinned> const& vertices_skinned = m_vertices_skinned;
        std::vector<std::uint32_t> const& indices_skinned = m_indices_skinned;
        std::vector<Ir77MeshRange> const& ranges_skinned = m_ranges_skinned;
        std::vector<glm::mat4> const& bones = m_bones;
        std::vector<std::uint32_t> const& bone_bases = m_bone_bases;

        for (int s = 0; s < 2; s++) {
            m_triangles[s].clear();
            m_vertex_triangles[s].assign(s == 0 ? vertices.size() : vertices_skinned.size(), {});
        }

        m_source_group.clear();
        m_source_vertex.clear();
        m_face_occludes.clear();
        m_skin_base.clear();

        // the normals as the file had them -- which split vertices were one smooth vertex
        m_original_normals[0].clear();
        m_original_normals[1].clear();
        for (Ir77PVVertex const& v : vertices) m_original_normals[0].push_back(v.normal);
        for (Ir77PVVertexSkinned const& v : vertices_skinned) m_original_normals[1].push_back(v.normal);

        std::unordered_map<std::uint32_t, std::uint32_t> group_of{};  // first_instance -> groups index
        std::unordered_map<Ir77EditKey, std::uint32_t, Ir77EditKeyHash> welded{};
        std::unordered_set<std::uint64_t> seen{};

        auto const bits = [](float const f) {
            float const value = f == 0.0f ? 0.0f : f;  // -0 and +0 are one position
            std::uint32_t word{0};
            std::memcpy(&word, &value, sizeof(word));
            return word;
        };

        auto const group = [&](Ir77MeshRange const& range) {
            auto const [found, added] = group_of.try_emplace(range.first_instance, static_cast<std::uint32_t>(mesh.groups.size()));
            if (added) mesh.groups.push_back({range.first_instance, range.instance_count});
            return found->second;
        };

        auto const vertex = [&](glm::vec3 const& position, std::uint32_t const g, Ir77EditSource const& source) {
            Ir77EditKey const key{g, bits(position.x), bits(position.y), bits(position.z)};

            auto const [found, added] = welded.try_emplace(key, static_cast<std::uint32_t>(mesh.vertices.size()));
            if (added) mesh.vertices.push_back({position, g, {}});

            std::vector<Ir77EditSource>& sources = mesh.vertices[found->second].sources;
            if (std::none_of(sources.begin(), sources.end(), [&](auto const& s) { return s.skinned == source.skinned && s.vertex == source.vertex; }))
                sources.push_back(source);

            return found->second;
        };

        auto const edge = [&](std::uint32_t a, std::uint32_t b) {
            if (a == b) return;  // a degenerate triangle's collapsed side
            if (a > b) std::swap(a, b);

            if (seen.insert((static_cast<std::uint64_t>(a) << 32) | b).second) mesh.edges.emplace_back(a, b);
        };

        // one stream -- position(v) is a vertex where it rests in its instance's space
        auto const stream = [&](std::vector<std::uint32_t> const& stream_indices, std::size_t const vertex_count, std::vector<Ir77MeshRange> const& stream_ranges,
                                bool const skinned, auto const& position) {
            std::vector<std::uint8_t> const& occludes = skinned ? m_range_occludes_skinned : m_range_occludes;

            for (std::size_t r = 0; r < stream_ranges.size(); r++) {
                Ir77MeshRange const& range = stream_ranges[r];
                if (range.index_count < 3 || range.vertex_offset < 0 || range.instance_count == 0) continue;

                bool const solid = r >= occludes.size() || occludes[r] != 0;

                std::uint32_t const g = group(range);
                std::size_t const base = static_cast<std::size_t>(range.vertex_offset);

                for (std::uint32_t i = 0; i + 2 < range.index_count; i += 3) {
                    std::uint32_t corners[3]{};
                    bool inside{true};

                    std::uint32_t sources[3]{};

                    for (std::uint32_t c = 0; c < 3; c++) {
                        std::size_t const at = range.index_offset + i + c;
                        if (at >= stream_indices.size() || base + stream_indices[at] >= vertex_count) {
                            inside = false;
                            break;
                        }

                        std::uint32_t const v = static_cast<std::uint32_t>(base + stream_indices[at]);
                        sources[c] = v;
                        corners[c] = vertex(position(v, range.first_instance), g, Ir77EditSource{skinned, v});

                        m_source_group[SourceKey(Ir77EditSource{skinned, v})] = g;
                        m_source_vertex[SourceKey(Ir77EditSource{skinned, v})] = corners[c];
                        if (skinned) m_skin_base[v] = range.first_instance < bone_bases.size() ? bone_bases[range.first_instance] : 0u;
                    }

                    if (!inside) continue;

                    // the stream's own triangle -- its normals are rebuilt from these, seams and hard edges kept
                    int const s = skinned ? 1 : 0;
                    std::uint32_t const triangle = static_cast<std::uint32_t>(m_triangles[s].size());
                    m_triangles[s].emplace_back(sources[0], sources[1], sources[2]);
                    for (std::uint32_t const v : sources) m_vertex_triangles[s][v].push_back(triangle);

                    edge(corners[0], corners[1]);
                    edge(corners[1], corners[2]);
                    edge(corners[2], corners[0]);

                    if (corners[0] != corners[1] && corners[1] != corners[2] && corners[2] != corners[0]) {
                        mesh.faces.emplace_back(corners[0], corners[1], corners[2]);
                        m_face_occludes.push_back(solid ? 1 : 0);
                    }
                }
            }
        };

        stream(indices, vertices.size(), ranges, false, [&](std::uint32_t const v, std::uint32_t) { return vertices[v].position; });

        // the rest pose -- a mesh's instances share one palette layout; its first instance's palette poses the shared vertices
        stream(indices_skinned, vertices_skinned.size(), ranges_skinned, true, [&](std::uint32_t const v, std::uint32_t const first) {
            std::uint32_t const bone_base = first < bone_bases.size() ? bone_bases[first] : 0;
            return Ir77SkinPosition(vertices_skinned[v], bones, bone_base);
        });
    }

    // The edit mesh on the GPU -- vertex v of a mesh with n instances is points [start(v), start(v) + n), one per instance. Edges and faces are
    // repeated per instance over those points.
    void BuildCage(std::vector<Ir77EditPoint>& points, std::vector<glm::uvec2>& edges, std::vector<glm::uvec4>& faces) {
        m_point_start.assign(m_mesh.vertices.size(), 0u);

        for (std::uint32_t v = 0; v < m_mesh.vertices.size(); v++) {
            Ir77EditVertex const& vertex = m_mesh.vertices[v];
            Ir77EditGroup const& group = m_mesh.groups[vertex.group];

            m_point_start[v] = static_cast<std::uint32_t>(points.size());

            for (std::uint32_t i = 0; i < group.instance_count; i++) {
                Ir77EditPoint point{};
                point.position = vertex.position;
                point.slot = m_instance_base + group.first_instance + i;
                point.vertex = v;
                points.push_back(point);
            }
        }

        auto const instances = [&](std::uint32_t const v) { return m_mesh.groups[m_mesh.vertices[v].group].instance_count; };

        for (glm::uvec2 const& e : m_mesh.edges)
            for (std::uint32_t i = 0; i < instances(e.x); i++) edges.emplace_back(m_point_start[e.x] + i, m_point_start[e.y] + i);

        for (glm::uvec3 const& f : m_mesh.faces)
            for (std::uint32_t i = 0; i < instances(f.x); i++) faces.emplace_back(m_point_start[f.x] + i, m_point_start[f.y] + i, m_point_start[f.z] + i, 0u);
    }

    // [vertex][instance] -> world, where the model sits this frame
    void WorldPositions(std::vector<glm::mat4> const& transforms, std::vector<std::vector<glm::vec3>>& world) const {
        world.assign(m_mesh.vertices.size(), {});

        for (std::size_t v = 0; v < m_mesh.vertices.size(); v++) {
            Ir77EditVertex const& vertex = m_mesh.vertices[v];
            Ir77EditGroup const& group = m_mesh.groups[vertex.group];

            for (std::uint32_t i = 0; i < group.instance_count; i++) {
                std::size_t const local = group.first_instance + i;
                glm::mat4 const model = local < transforms.size() ? transforms[local] : glm::mat4(1.0f);
                world[v].push_back(glm::vec3(model * glm::vec4(vertex.position, 1.0f)));
            }
        }
    }

    // a world point -> pixels in the view (false: behind the eye)
    static bool Project(Ir77EditView const& view, glm::vec3 const& point, glm::vec2& pixel) {
        glm::vec4 const clip = view.view_proj * glm::vec4(point, 1.0f);
        if (clip.w <= 1e-6f) return false;

        pixel = (glm::vec2(clip) / clip.w * 0.5f + 0.5f) * view.size;
        return true;
    }

    // Moller-Trumbore, both faces -- t along the ray, false: no hit
    static bool RayTriangle(glm::vec3 const& origin, glm::vec3 const& ray, glm::vec3 const& p0, glm::vec3 const& p1, glm::vec3 const& p2, float& t) {
        glm::vec3 const e1 = p1 - p0;
        glm::vec3 const e2 = p2 - p0;
        glm::vec3 const h = glm::cross(ray, e2);
        float const det = glm::dot(e1, h);
        if (std::abs(det) < 1e-12f) return false;

        float const inv = 1.0f / det;
        glm::vec3 const s = origin - p0;
        float const u = glm::dot(s, h) * inv;
        if (u < 0.0f || u > 1.0f) return false;

        glm::vec3 const q = glm::cross(s, e1);
        float const v = glm::dot(ray, q) * inv;
        if (v < 0.0f || u + v > 1.0f) return false;

        t = glm::dot(e2, q) * inv;
        return t > 0.0f;
    }

    // The nearest face along the ray from origin through point -- false when nothing covers the point (it's visible). The faces the point is a
    // corner of can't cover it; the 0.1% margin keeps the surface it sits on from counting. Only faces that hide what's behind them count --
    // a BLEND (alpha) material's are seen through, so the vertices behind them stay pickable, as they're drawn.
    bool Covered(glm::vec3 const& origin, glm::vec3 const& point, std::vector<std::vector<glm::vec3>> const& world) const {
        glm::vec3 const to = point - origin;
        float const distance = glm::length(to);
        if (distance < 1e-8f) return false;

        glm::vec3 const ray = to / distance;

        for (std::size_t face = 0; face < m_mesh.faces.size(); face++) {
            if (face < m_face_occludes.size() && m_face_occludes[face] == 0) continue;

            glm::uvec3 const& f = m_mesh.faces[face];
            for (std::size_t i = 0; i < world[f.x].size(); i++) {
                float t{0.0f};
                if (RayTriangle(origin, ray, world[f.x][i], world[f.y][i], world[f.z][i], t) && t < distance * 0.999f) return true;
            }
        }

        return false;
    }

    // the nearest visible point within EDIT_PICK_VERTEX_PIXELS of the cursor
    bool PickVertex(Ir77EditView const& view, std::vector<std::vector<glm::vec3>> const& world, std::vector<std::uint32_t>& element) const {
        struct Candidate {
            float pixels;
            std::uint32_t vertex;
            glm::vec3 point;
        };

        std::vector<Candidate> candidates{};

        for (std::uint32_t v = 0; v < world.size(); v++)
            for (glm::vec3 const& point : world[v]) {
                glm::vec2 pixel{0.0f};
                if (!Project(view, point, pixel)) continue;

                float const pixels = glm::length(pixel - view.cursor);
                if (pixels <= EDIT_PICK_VERTEX_PIXELS) candidates.push_back({pixels, v, point});
            }

        std::sort(candidates.begin(), candidates.end(), [](auto const& a, auto const& b) { return a.pixels < b.pixels; });

        for (Candidate const& candidate : candidates)
            if (!Covered(view.origin, candidate.point, world)) {
                element = {candidate.vertex};
                return true;
            }

        return false;
    }

    // the nearest visible edge within EDIT_PICK_EDGE_PIXELS -- tested for cover where it passes closest to the cursor
    bool PickEdge(Ir77EditView const& view, std::vector<std::vector<glm::vec3>> const& world, std::vector<std::uint32_t>& element) const {
        struct Candidate {
            float pixels;
            std::uint32_t edge;
            glm::vec3 point;
        };

        std::vector<Candidate> candidates{};

        for (std::uint32_t e = 0; e < m_mesh.edges.size(); e++) {
            glm::uvec2 const& edge = m_mesh.edges[e];

            for (std::size_t i = 0; i < world[edge.x].size(); i++) {
                glm::vec3 const a = world[edge.x][i];
                glm::vec3 const b = world[edge.y][i];

                glm::vec2 pa{0.0f};
                glm::vec2 pb{0.0f};
                if (!Project(view, a, pa) || !Project(view, b, pb)) continue;

                glm::vec2 const along = pb - pa;
                float const length2 = glm::dot(along, along);
                float const s = length2 > 1e-12f ? std::clamp(glm::dot(view.cursor - pa, along) / length2, 0.0f, 1.0f) : 0.0f;

                float const pixels = glm::length(pa + along * s - view.cursor);
                if (pixels <= EDIT_PICK_EDGE_PIXELS) candidates.push_back({pixels, e, glm::mix(a, b, s)});  // screen s -- close enough for cover
            }
        }

        std::sort(candidates.begin(), candidates.end(), [](auto const& a, auto const& b) { return a.pixels < b.pixels; });

        for (Candidate const& candidate : candidates)
            if (!Covered(view.origin, candidate.point, world)) {
                element = {m_mesh.edges[candidate.edge].x, m_mesh.edges[candidate.edge].y};
                return true;
            }

        return false;
    }

    // the nearest face the ray under the cursor goes through
    bool PickFace(Ir77EditView const& view, std::vector<std::vector<glm::vec3>> const& world, std::vector<std::uint32_t>& element) const {
        float nearest = std::numeric_limits<float>::max();
        std::size_t hit = m_mesh.faces.size();

        for (std::size_t f = 0; f < m_mesh.faces.size(); f++) {
            glm::uvec3 const& face = m_mesh.faces[f];

            for (std::size_t i = 0; i < world[face.x].size(); i++) {
                float t{0.0f};
                if (RayTriangle(view.origin, view.ray, world[face.x][i], world[face.y][i], world[face.z][i], t) && t < nearest) {
                    nearest = t;
                    hit = f;
                }
            }
        }

        if (hit == m_mesh.faces.size()) return false;

        element = {m_mesh.faces[hit].x, m_mesh.faces[hit].y, m_mesh.faces[hit].z};
        return true;
    }

   private:
    std::shared_ptr<NSIr77PeregrineV::Ir77Server> m_renderer{};

    Ir77EditMesh m_mesh{};  // what selection works on -- the model's own space

    std::vector<std::uint32_t> m_flags{};  // per edit vertex -- EDIT_SELECTED

    std::vector<std::uint32_t> m_point_start{};  // per edit vertex -- its first point on the GPU

    Ir77EditMode m_mode{Ir77EditMode::Vertex};

    std::uint32_t m_instance_base{0};

    // the model's streams as the cage was built from them -- an edit changes them here, then the renderer and (on confirm) the mesh
    std::shared_ptr<IIr77Mesh> m_meshes{};

    std::uint32_t m_asset{IR77PV_NO_ASSET};  // its renderer asset

    std::vector<Ir77PVVertex> m_vertices{};

    std::vector<std::uint32_t> m_indices{};

    std::vector<Ir77MeshRange> m_ranges{};

    std::vector<Ir77PVVertexSkinned> m_vertices_skinned{};

    std::vector<std::uint32_t> m_indices_skinned{};

    std::vector<Ir77MeshRange> m_ranges_skinned{};

    std::vector<glm::mat4> m_bones{};  // the rest palette -- skinned edits go back through it

    std::vector<std::uint32_t> m_bone_bases{};

    // per stream (0 static, 1 skinned): its triangles (stream vertex indices) and, per vertex, the triangles it's a corner of
    std::array<std::vector<glm::uvec3>, 2> m_triangles{};

    std::array<std::vector<std::vector<std::uint32_t>>, 2> m_vertex_triangles{};

    std::unordered_map<std::uint64_t, std::uint32_t> m_source_group{};  // SourceKey -> its mesh group

    std::unordered_map<std::uint32_t, std::uint32_t> m_skin_base{};  // skinned vertex -> its palette base

    std::unordered_map<std::uint64_t, std::uint32_t> m_source_vertex{};  // SourceKey -> its edit vertex

    // per range: hides what's behind it (not a BLEND material); per face, the same -- what the pick's cover test counts
    std::vector<std::uint8_t> m_range_occludes{};

    std::vector<std::uint8_t> m_range_occludes_skinned{};

    std::vector<std::uint8_t> m_face_occludes{};

    std::array<std::vector<glm::vec3>, 2> m_original_normals{};  // per stream -- as the cage was built

    std::vector<Ir77EditPoint> m_points_cpu{};  // the points buffer as uploaded -- a grab rewrites it

    Ir77EditGrab m_grab{};

    std::vector<glm::vec3> m_grab_moves{};  // per edit vertex -- the last ApplyGrab's move in its instance space (what its glTF vertices follow)

    std::shared_ptr<IIr77Gizmo> m_gizmo{};  // World's -- shared with object mode

    // confirmed edits while in edit mode -- Undo walks back, Redo forward; a new edit clears what Redo had
    std::vector<Ir77EditStep> m_undo{};

    std::vector<Ir77EditStep> m_redo{};

    bool m_modified{false};  // a confirmed edit since the cage was built

    // the cage on the GPU -- points (Ir77EditPoint), edges (uvec2 of points), faces (uvec4 of points), flags (uint per edit vertex)
    Ir77EditBuffer m_points{};

    Ir77EditBuffer m_edges{};

    Ir77EditBuffer m_faces{};

    Ir77EditBuffer m_flags_gpu{};

    std::uint32_t m_point_count{0};

    std::uint32_t m_edge_count{0};

    std::uint32_t m_face_count{0};

    Ir77PVPipelineHandle m_faces_pipeline{IR77PV_NO_PIPELINE};

    Ir77PVPipelineHandle m_edges_pipeline{IR77PV_NO_PIPELINE};

    Ir77PVPipelineHandle m_points_pipeline{IR77PV_NO_PIPELINE};

    bool m_missing{false};  // the manifest has no edit pipelines -- said once
};
}  // namespace NSIr77Scene
