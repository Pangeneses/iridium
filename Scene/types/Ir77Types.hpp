#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "../../PeregrineV/types/Ir77Types.hpp"

using namespace NSIr77PeregrineV;

namespace NSIr77Scene {
// =============================================================================================================================================
// The scene graph -- everything in a scene is an entity. The scene itself is the root: it isn't an entity, it holds the project's metadata,
// and its children are the entities whose parent is ''. An entity is the same core for every kind, plus that kind's payload:
//   collection  a container -- its children sit relative to it (it starts on the origin)
//   model       one .glb of the project's Models/ -- geometry, materials, skin, clips. Several entities may share one file (instances).
//   light       directional / point / spot -- it shines down its local -Z
//   camera      a lens -- stored with the scene; looking through one comes with world cameras
// Any entity may parent any other (a wheel under its car); transforms compose down the tree, visibility too (a hidden collection hides
// everything under it). Removing an entity removes everything under it.
// =============================================================================================================================================
enum class Ir77GraphKind : std::uint8_t { Collection, Model, Light, Camera };

inline constexpr std::uint32_t GRAPH_NO_RUNTIME{UINT32_MAX};  // nothing behind it in World (a collection, a camera)

inline constexpr std::size_t GRAPH_ID_LENGTH{16};  // 64-bit, lower-case hex

struct Ir77GraphCamera {
    float fov{60.0f};  // vertical, degrees
    bool ortho{false};
    float near{0.1f};
    float far{1000.0f};
};

struct Ir77GraphEntity {
    // core -- every kind
    std::string id{};  // 16 hex digits, made once (Ir77Graph::NewId) and kept through save and load
    std::string name{};
    Ir77GraphKind kind{Ir77GraphKind::Collection};
    std::string parent{};    // '' = the scene root
    std::uint32_t order{0};  // among its siblings -- the outliner's order
    bool visible{true};
    glm::vec3 t{0.0f};  // relative to the parent
    glm::quat r{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 s{1.0f};

    // payloads
    std::string model{};       // model -- <project>/Models/<model>.glb
    Ir77PVLight light{};       // light -- type, color, intensity (glTF units), range, cones
    Ir77GraphCamera camera{};  // camera

    // what World has behind it -- a model's entity, a light's world light; handed out again on every load, never stored
    std::uint32_t runtime{GRAPH_NO_RUNTIME};
};

inline char const* GraphKindName(Ir77GraphKind const kind) {
    switch (kind) {
        case Ir77GraphKind::Model:
            return "model";
        case Ir77GraphKind::Light:
            return "light";
        case Ir77GraphKind::Camera:
            return "camera";
        default:
            return "collection";
    }
}

// "collection" | "model" | "light" | "camera" -> kind; false for anything else
inline bool GraphKindFromName(std::string_view const name, Ir77GraphKind& kind) {
    if (name == "collection")
        kind = Ir77GraphKind::Collection;
    else if (name == "model")
        kind = Ir77GraphKind::Model;
    else if (name == "light")
        kind = Ir77GraphKind::Light;
    else if (name == "camera")
        kind = Ir77GraphKind::Camera;
    else
        return false;

    return true;
}

// What an import brings besides geometry, textures and animations when the op doesn't say -- a file's lights and cameras. Stripped: the scene
// graph places lights and cameras itself (Aurum, stored in the project's MongoDB).
inline constexpr bool SCENE_IMPORT_LIGHTS{false};

inline constexpr bool SCENE_IMPORT_CAMERAS{false};

// ---- edit -- the model in edit mode (Ir77Edit; Shaders/include/edit.glsl) -----------------------------------------------------------------------

// One cage point on the GPU -- std430, 32 bytes. Must match Ir77EditPoint in edit.glsl. One per edit vertex per instance it draws through.
struct Ir77EditPoint {
    glm::vec3 position{0.0f};  // its mesh's instance space
    std::uint32_t slot{0};     // models[] slot -- the asset's instance base + the instance
    std::uint32_t vertex{0};   // the edit vertex it shows -- its selection flag (Ir77EditMesh::vertices index)
    std::uint32_t pad0{0};
    std::uint32_t pad1{0};
    std::uint32_t pad2{0};
};

static_assert(sizeof(Ir77EditPoint) == 32, "must match Ir77EditPoint in edit.glsl");

// What the active entity is being worked in -- Blender's interaction modes. Object: entities are picked and moved whole. Edit: the active
// model's vertices / edges / faces (the edit cage). Sculpt and the paints are named now so the UI and the dump carry them; the engine refuses
// them until they're built (InteractionModeBuilt).
enum class Ir77InteractionMode : std::uint8_t { Object, Edit, Sculpt, VertexPaint, WeightPaint, TexturePaint };

inline char const* InteractionModeName(Ir77InteractionMode const mode) {
    switch (mode) {
        case Ir77InteractionMode::Edit:
            return "edit";
        case Ir77InteractionMode::Sculpt:
            return "sculpt";
        case Ir77InteractionMode::VertexPaint:
            return "vertex_paint";
        case Ir77InteractionMode::WeightPaint:
            return "weight_paint";
        case Ir77InteractionMode::TexturePaint:
            return "texture_paint";
        default:
            return "object";
    }
}

// "object" | "edit" | "sculpt" | "vertex_paint" | "weight_paint" | "texture_paint" -> mode; false for anything else
inline bool InteractionModeFromName(std::string_view const name, Ir77InteractionMode& mode) {
    if (name == "object")
        mode = Ir77InteractionMode::Object;
    else if (name == "edit")
        mode = Ir77InteractionMode::Edit;
    else if (name == "sculpt")
        mode = Ir77InteractionMode::Sculpt;
    else if (name == "vertex_paint")
        mode = Ir77InteractionMode::VertexPaint;
    else if (name == "weight_paint")
        mode = Ir77InteractionMode::WeightPaint;
    else if (name == "texture_paint")
        mode = Ir77InteractionMode::TexturePaint;
    else
        return false;

    return true;
}

// the active model's outline in object mode -- rgb + a, the edit cage's selected orange (edit.glsl's EDIT_SELECTED_COLOR)
inline constexpr glm::vec4 SELECT_OUTLINE_COLOR{1.0f, 0.55f, 0.1f, 1.0f};

// the modes the engine can enter now -- the rest are refused with "not built yet"
inline bool InteractionModeBuilt(Ir77InteractionMode const mode) { return mode == Ir77InteractionMode::Object || mode == Ir77InteractionMode::Edit; }

// What a click selects -- vertices, edges (both ends) or faces (all three corners). The selection is always a set of vertices; an edge or a
// face shows selected when all of its vertices are. Shaders: pc.user[7] (EDIT_MODE_* in edit.glsl).
enum class Ir77EditMode : std::uint8_t { Vertex, Edge, Face };

inline char const* EditModeName(Ir77EditMode const mode) {
    switch (mode) {
        case Ir77EditMode::Edge:
            return "edge";
        case Ir77EditMode::Face:
            return "face";
        default:
            return "vertex";
    }
}

// "vertex" | "edge" | "face" -> mode; false for anything else
inline bool EditModeFromName(std::string_view const name, Ir77EditMode& mode) {
    if (name == "vertex")
        mode = Ir77EditMode::Vertex;
    else if (name == "edge")
        mode = Ir77EditMode::Edge;
    else if (name == "face")
        mode = Ir77EditMode::Face;
    else
        return false;

    return true;
}

// One glTF vertex an edit vertex stands for -- moving the edit vertex moves every one of these
struct Ir77EditSource {
    bool skinned{false};      // the skinned stream (else the static one)
    std::uint32_t vertex{0};  // in that stream (Ir77Mesh's vertex buffer)
};

// One vertex of the edit mesh -- a position once, however many glTF vertices the file split it into (UV / normal seams)
struct Ir77EditVertex {
    glm::vec3 position{0.0f};  // its mesh's instance space (skinned: posed by the rest palette)
    std::uint32_t group{0};    // Ir77EditMesh::groups -- the mesh it belongs to
    std::vector<Ir77EditSource> sources{};
};

// One glTF mesh's instances -- every vertex of the mesh draws once per instance
struct Ir77EditGroup {
    std::uint32_t first_instance{0};  // the asset's own slot
    std::uint32_t instance_count{0};
};

// The edit mesh -- what selection works on, in the model's own space. Edges and faces are vertex indices.
struct Ir77EditMesh {
    std::vector<Ir77EditVertex> vertices{};
    std::vector<glm::uvec2> edges{};
    std::vector<glm::uvec3> faces{};
    std::vector<Ir77EditGroup> groups{};
};

// The view a click in edit mode came from -- its camera, the cursor and the view's size in pixels, the ray under the cursor (world space)
struct Ir77EditView {
    glm::mat4 view{1.0f};
    glm::mat4 proj{1.0f};
    glm::mat4 view_proj{1.0f};
    glm::vec2 cursor{0.0f};  // pixels from the view's top left
    glm::vec2 size{1.0f};    // the view in pixels
    glm::vec3 origin{0.0f};
    glm::vec3 ray{0.0f, 0.0f, -1.0f};
};

// What a grab moves along -- the plane facing the camera through the selection's centre, or one world axis through it
enum class Ir77EditAxis : std::uint8_t { Free, X, Y, Z };

// "" | "free" | "x" | "y" | "z" -> axis; false for anything else
inline bool EditAxisFromName(std::string_view const name, Ir77EditAxis& axis) {
    if (name.empty() || name == "free")
        axis = Ir77EditAxis::Free;
    else if (name == "x")
        axis = Ir77EditAxis::X;
    else if (name == "y")
        axis = Ir77EditAxis::Y;
    else if (name == "z")
        axis = Ir77EditAxis::Z;
    else
        return false;

    return true;
}

// ---- gizmo -- the transform handles at the selection's median (Ir77Edit; Shaders/gizmo.vert) --------------------------------------------------

// What the gizmo does -- move, turn or size the selection. Shaders: pc.user[4] (GIZMO_MODE_* in gizmo.vert).
enum class Ir77GizmoMode : std::uint8_t { Translate, Rotate, Scale };

inline char const* GizmoModeName(Ir77GizmoMode const mode) {
    switch (mode) {
        case Ir77GizmoMode::Rotate:
            return "rotate";
        case Ir77GizmoMode::Scale:
            return "scale";
        default:
            return "translate";
    }
}

// "translate" | "rotate" | "scale" -> mode; false for anything else
inline bool GizmoModeFromName(std::string_view const name, Ir77GizmoMode& mode) {
    if (name == "translate")
        mode = Ir77GizmoMode::Translate;
    else if (name == "rotate")
        mode = Ir77GizmoMode::Rotate;
    else if (name == "scale")
        mode = Ir77GizmoMode::Scale;
    else
        return false;

    return true;
}

// a gizmo handle -- the X / Y / Z axis (arrow, ring or box), the centre (free move / uniform size); GIZMO_NONE = none. Shaders: pc.user[5].
inline constexpr std::uint32_t GIZMO_X{0};

inline constexpr std::uint32_t GIZMO_Y{1};

inline constexpr std::uint32_t GIZMO_Z{2};

inline constexpr std::uint32_t GIZMO_CENTRE{3};

inline constexpr std::uint32_t GIZMO_NONE{UINT32_MAX};

// One gizmo segment in its own units -- the centre at 0, an axis 1 long; handle = what grabbing it does. Must match Segment() in gizmo.vert.
struct Ir77GizmoSegment {
    glm::vec3 a{0.0f};
    glm::vec3 b{0.0f};
    std::uint32_t handle{GIZMO_NONE};
};

// object mode's gizmo drag on the active entity -- the server's: where it started (what a cancel puts back), and its parent's world (what a
// new world matrix is taken back into, as the entity's own t / r / s)
struct Ir77ObjectDrag {
    bool active{false};
    std::string id{};
    glm::mat4 start_world{1.0f};
    glm::mat4 parent_world{1.0f};
    glm::vec3 t{0.0f};
    glm::quat r{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 s{1.0f};
    bool changed{false};
};

// a gizmo drag under way -- the handle pressed, where from, on what (Ir77Gizmo). The placement is the gizmo's at the press: its pivot and its
// axes (unit columns -- world, or an object's own for a scale).
struct Ir77GizmoDrag {
    bool active{false};
    std::uint32_t handle{GIZMO_NONE};
    Ir77GizmoMode mode{Ir77GizmoMode::Translate};
    glm::vec3 pivot{0.0f};
    glm::mat3 basis{1.0f};
    glm::vec3 origin{0.0f};                // the press's ray
    glm::vec3 ray{0.0f, 0.0f, -1.0f};
    glm::vec2 centre_pixel{0.0f};          // the pivot on screen at the press
    glm::vec2 cursor_pixel{0.0f};          // the cursor at the press
    glm::vec2 axis_pixel{1.0f, 0.0f};      // the handle's axis on screen (unit) -- sizing along it
    glm::vec3 delta{0.0f};                 // a translate's last move -- held while the ray grazes its line / plane
};

// its size on screen -- this fraction of half the view's height, at any distance (gizmo.vert GIZMO_FRACTION)
inline constexpr float GIZMO_FRACTION{0.22f};

// how close a press must land on a handle, in pixels
inline constexpr float GIZMO_PICK_PIXELS{10.0f};

// a ring's segments (gizmo.vert GIZMO_RING_SEGMENTS); an arrowhead's length and half width, a box's and the centre square's half size
inline constexpr std::uint32_t GIZMO_RING_SEGMENTS{48};

inline constexpr float GIZMO_HEAD_LENGTH{0.2f};

inline constexpr float GIZMO_HEAD_WIDTH{0.07f};

inline constexpr float GIZMO_BOX{0.06f};

inline constexpr float GIZMO_SQUARE{0.08f};

// a vertex's selection flag (edit.glsl EDIT_SELECTED)
inline constexpr std::uint32_t EDIT_SELECTED{1u};

// split vertices whose normals agree this closely were one smooth vertex -- an edit keeps them smooth (cos ~2.5 degrees)
inline constexpr float EDIT_SMOOTH_DOT{0.999f};

// confirmed edits the undo history keeps -- the oldest goes past this
inline constexpr std::size_t EDIT_UNDO_STEPS{256};

// a box select's depth buffer is at most this many pixels a side -- a bigger box is sampled coarser
inline constexpr std::uint32_t EDIT_BOX_DEPTH_SIDE{2048};

// how far in front of a hiding surface a point may sit and still pass, relative (reversed-Z) -- the cage's own bias, so on-surface counts
inline constexpr float EDIT_BOX_DEPTH_BIAS{0.0015f};

// how close a click must land, in pixels -- to a vertex's point, to an edge's line
inline constexpr float EDIT_PICK_VERTEX_PIXELS{10.0f};

inline constexpr float EDIT_PICK_EDGE_PIXELS{8.0f};

// World's "no entity" -- nothing in edit mode, a pick that hit nothing
inline constexpr std::uint32_t EDIT_NO_ENTITY{UINT32_MAX};

// vertices per element -- a point's square and an edge's strip (two triangles), a face's triangle (edit_*.vert)
inline constexpr std::uint32_t EDIT_VERTICES_PER_ELEMENT{6};

inline constexpr std::uint32_t EDIT_VERTICES_PER_FACE{3};

// ---- commit -- an edited model back into its .glb (Ir77World::CommitEdits, services/Ir77Flatten.hpp Rewrite) ----------------------------------

// Where one Ir77Mesh range came from in the file -- its glTF mesh and primitive, and how many vertices the file gives that primitive
struct Ir77MeshOrigin {
    std::uint32_t mesh{0};
    std::uint32_t primitive{0};
    std::uint32_t vertex_count{0};
};

// One file primitive's edited vertices, in the file's own vertex order -- what Rewrite writes over POSITION and NORMAL (TANGENT is dropped,
// so the load after rebuilds it from the new shape)
struct Ir77FlattenOverride {
    std::vector<glm::vec3> positions{};
    std::vector<glm::vec3> normals{};
    std::vector<std::uint8_t> set{};  // per file vertex -- 0: no stream vertex came from it (unreferenced), the file's own value stays
};

// A clip the model was playing when it went into edit mode -- held while it's in edit mode (it rests in its rest pose), played again from the
// start when it leaves. The dump reports these as its playing clips, so a save in edit mode stores what the model plays.
struct Ir77EditClip {
    std::uint32_t clip{0};
    float speed{1.0f};
    bool loop{true};
};
}
