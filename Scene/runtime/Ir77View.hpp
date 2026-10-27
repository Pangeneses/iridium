#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77Scene.hpp"
#include "../dictionary/IDIr77Scene.hpp"

#include "../../Ir77RT/types/Ir77Types.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"
#include "../../Ir77RT/runtime/Ir77Return.hpp"

#include "../interface/IIr77Camera.hpp"
#include "../interface/IIr77View.hpp"

#include "../../PeregrineV/types/Ir77PVTypes.hpp"

using namespace NSIr77RT;
using namespace NSIr77PeregrineV;

namespace NSIr77Scene {

// =============================================================================================================================================
// Ir77View -- the cameras, one per view. Every view is a full camera of its own: the file's cameras, or its own controlled orbit / fly /
// ortho camera (index == camera count). Owned by Ir77Animate; the UI reaches it through the scene server's camera.* ops.
//
//   scene:     SetCameras (the file's) -> SetBounds (frames every view's controlled camera)
//   UI:        SetViewLayout, FocusView -- then SetCameraMove / Look / Zoom / Speed / Mode / View, SelectCamera / NextCamera / ResetCamera
//              drive the focused view only -- next frame
//   per frame: Update(dt) -> WriteCameras(aspect) per window
//
// Layout: the UI says where each view draws -- 1 .. MAX_VIEWS areas, x, y, width, height as fractions of the window, anywhere (the part the
// UI doesn't cover, side by side, picture in picture). Until it does, view 0 fills the window. Each view's camera is built for its area's
// aspect. View 0 is the main camera; views 1 / 2 / 3 start orthographic looking down top / front / side. Reset puts a view back where it
// started. A layout with fewer views than the focused one focuses view 0.
// =============================================================================================================================================
class Ir77View : public Ir77Enlisted, public IIr77View, public std::enable_shared_from_this<Ir77View> {
   private:
    // one view's camera -- which camera it shows, and its controlled camera: orbit (target, distance) or fly (position); yaw / pitch aim both
    struct Ir77ViewCamera {
        std::uint32_t index{0};  // file camera; == camera count -> the controlled camera
        bool fly{false};
        bool ortho{false};  // the controlled orbit camera draws orthographic
        glm::vec3 target{0.0f};
        glm::vec3 position{0.0f};
        float distance{1.0f};
        float yaw{0.0f};
        float pitch{0.0f};
        float speed{1.0f};         // units per second at full key
        glm::vec3 move{0.0f};      // held keys: right, up, forward in -1..1
        glm::mat4 view{1.0f};      // last frame's view, whichever camera drew it -- where a takeover starts from
        float fov{60.0f};          // vertical, degrees -- taken from whichever camera drew last, so a takeover keeps the lens
    };

   public:
    Ir77View() {
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
        seat_shared_uuid<&GUIDIr77View>(uid);

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

        else if (iid == GUIDIIr77View)
            obj = std::shared_ptr<IIr77View>(shared_from_this(), static_cast<IIr77View*>(this));

        else if (iid == GUIDIr77View)
            obj = std::shared_ptr<Ir77View>(shared_from_this(), static_cast<Ir77View*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

   public:
    // The file's cameras -- index count is the controlled camera. View 0 starts on the file's first camera when it has one; the others on
    // their controlled cameras.
    std::shared_ptr<IIr77Return const> SetCameras(std::shared_ptr<IIr77Camera> const& cameras) {
        m_cameras = cameras;

        m_camera_count = 0;
        if (m_cameras) m_cameras->GetCameraCount(m_camera_count);

        for (std::size_t v = 0; v < m_views.size(); v++) m_views[v].index = v == 0 ? 0 : m_camera_count;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The scene's bounds -- what the controlled cameras frame and their planes hug. Puts every view's controlled camera back where it starts.
    std::shared_ptr<IIr77Return const> SetBounds(glm::vec3 const& low, glm::vec3 const& high) {
        m_bounds_min = low;
        m_bounds_max = high;

        for (std::uint32_t v = 0; v < MAX_VIEWS; v++) ResetView(v);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Held keys from the UI, once per frame
    std::shared_ptr<IIr77Return const> Update(float const& dt) {
        for (Ir77ViewCamera& camera : m_views) UpdateCameraControl(camera, dt);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // One camera per view in the layout, and where each draws. aspect is the window's; each camera gets its area's. camera.time is the
    // caller's.
    std::shared_ptr<IIr77Return const> WriteCameras(float const& aspect, std::vector<Ir77PVCamera>& cameras, std::vector<glm::vec4>& areas) {
        areas = m_areas;
        cameras.assign(m_areas.size(), Ir77PVCamera{});

        for (std::size_t v = 0; v < cameras.size(); v++) WriteViewCamera(m_views[v], aspect * m_areas[v].z / m_areas[v].w, cameras[v]);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Where each view draws, 1 .. MAX_VIEWS of them -- x, y, width, height as fractions of the window, each inside it
    std::shared_ptr<IIr77Return const> SetViewLayout(std::vector<glm::vec4> const& areas) {
        if (areas.empty() || areas.size() > MAX_VIEWS) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: a view layout has 1 to 4 views.");

        for (glm::vec4 const& area : areas)
            if (area.x < 0.0f || area.y < 0.0f || area.z <= 0.0f || area.w <= 0.0f || area.x + area.z > AREA_SLACK || area.y + area.w > AREA_SLACK)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: every view area must have a size and lie inside the window (fractions 0 .. 1).");

        m_areas = areas;

        if (m_active >= m_areas.size()) FocusView(0);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The view the camera controls drive -- the one under the mouse. Its old view stops moving.
    std::shared_ptr<IIr77Return const> FocusView(std::uint32_t const& view) {
        if (view >= m_areas.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: no such view in this layout.");

        m_views[m_active].move = glm::vec3(0.0f);
        m_active = view;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // For the frame stats -- the focused view's camera, and whether it is the controlled one
    std::shared_ptr<IIr77Return const> GetCameraState(std::uint32_t& index, bool& controlled) {
        index = m_views[m_active].index;
        controlled = index >= m_camera_count;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // the focused view's camera -- the file's, then the controlled one (index == camera count). The UI drives these; next frame.
    // -------------------------------------------------------------------------------------------------------------------------------------

    // Held keys: right / up / forward in -1..1, applied every Update until set back to 0
    std::shared_ptr<IIr77Return const> SetCameraMove(glm::vec3 const& move) {
        Ir77ViewCamera& camera = m_views[m_active];

        DefineCameraTakeover(camera);
        camera.move = glm::clamp(move, glm::vec3(-1.0f), glm::vec3(1.0f));

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Mouse drag in pixels -- turns the fly camera / swings the orbit camera
    std::shared_ptr<IIr77Return const> SetCameraLook(float const& dx, float const& dy) {
        Ir77ViewCamera& camera = m_views[m_active];

        DefineCameraTakeover(camera);
        camera.yaw -= dx * CAMERA_LOOK_RADIANS_PER_PIXEL;
        camera.pitch = glm::clamp(camera.pitch - dy * CAMERA_LOOK_RADIANS_PER_PIXEL, -CAMERA_PITCH_LIMIT, CAMERA_PITCH_LIMIT);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Wheel steps -- orbit distance (the ortho box with it), 10% per step
    std::shared_ptr<IIr77Return const> SetCameraZoom(float const& delta) {
        Ir77ViewCamera& camera = m_views[m_active];

        DefineCameraTakeover(camera);
        camera.distance = std::max(camera.distance * std::pow(1.1f, -delta), 0.01f);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Multiplies the move speed
    std::shared_ptr<IIr77Return const> SetCameraSpeed(float const& factor) {
        if (factor <= 0.0f) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: camera speed factor must be above 0.");

        m_views[m_active].speed *= factor;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // "orbit" | "fly" -- start from the current view, so switching never jumps; "ortho" -- orbit with an orthographic lens (no perspective,
    // zoom scales the view); "file" -- the file's first camera
    std::shared_ptr<IIr77Return const> SetCameraMode(std::string_view const& mode) {
        Ir77ViewCamera& camera = m_views[m_active];

        if (mode == "orbit" || mode == "fly" || mode == "ortho") {
            DefineCameraFromView(camera, mode == "fly");
            camera.ortho = mode == "ortho";
            camera.index = m_camera_count;
        } else if (mode == "file") {
            if (m_camera_count == 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: the file has no cameras.");
            camera.index = 0;
        } else {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: camera mode must be orbit, fly, ortho or file.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // "top" (down -Y) | "front" (down -Z) | "side" (down -X) -- turns the orbit camera around its target; ortho looks straight down for top
    std::shared_ptr<IIr77Return const> SetCameraView(std::string_view const& view) {
        Ir77ViewCamera& camera = m_views[m_active];

        DefineCameraTakeover(camera);

        if (!AimCamera(camera, view)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: camera view must be top, front or side.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // index == camera count -> the controlled camera
    std::shared_ptr<IIr77Return const> SelectCamera(std::uint32_t const& index) {
        if (index > m_camera_count) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: no such camera.");

        m_views[m_active].index = index;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The next file camera, then the controlled camera, then around again (F2)
    std::shared_ptr<IIr77Return const> NextCamera() {
        Ir77ViewCamera& camera = m_views[m_active];

        camera.index = (camera.index + 1) % (m_camera_count + 1);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The controlled camera, back where this view starts
    std::shared_ptr<IIr77Return const> ResetCamera() {
        ResetView(m_active);
        m_views[m_active].index = m_camera_count;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    // The camera a view draws with: the file camera (rebuilt with this aspect) or its controlled camera. Remembers its view and lens, so the
    // next switch to orbit / fly starts from it.
    void WriteViewCamera(Ir77ViewCamera& view, float const aspect, Ir77PVCamera& camera) const {
        float const radius = std::max(glm::length(m_bounds_max - m_bounds_min) * 0.5f, 0.01f);

        camera = Ir77PVCamera{};
        bool built = false;

        if (view.index < m_camera_count && m_cameras) {
            float const fallback_far = std::max(100.0f, radius * 20.0f);  // file camera without zfar
            built = m_cameras->BuildCamera(view.index, aspect, fallback_far, camera)->ID() == GUIDIr77OperationSucceeded;
        }

        if (!built) camera = MakeControlledCamera(view, aspect);

        // reversed-Z -- PeregrineV's main pass clears depth to 0 and keeps the greater (Ir77PVCmdBuffer)
        DefineReversedDepth(camera);

        view.view = camera.view;

        // the lens too -- an orthographic projection has none, it keeps the last one
        if (built || !view.ortho) view.fov = glm::degrees(2.0f * std::atan(1.0f / std::abs(camera.proj[1][1])));
    }

    // Where a view's controlled camera starts: orbit on the scene; views 1 / 2 / 3 orthographic down top / front / side
    void ResetView(std::uint32_t const view) {
        Ir77ViewCamera& camera = m_views[view];

        ResetCameraControl(camera);

        camera.ortho = view != 0;

        if (view == 1) AimCamera(camera, "top");
        if (view == 2) AimCamera(camera, "front");
        if (view == 3) AimCamera(camera, "side");
    }

    // Turns an orbit camera to look down an axis; false for an unknown name
    bool AimCamera(Ir77ViewCamera& camera, std::string_view const& view) const {
        if (view == "top") {
            camera.yaw = glm::pi<float>();
            camera.pitch = camera.ortho ? -glm::half_pi<float>() : -CAMERA_PITCH_LIMIT;
        } else if (view == "front") {
            camera.yaw = glm::pi<float>();
            camera.pitch = 0.0f;
        } else if (view == "side") {
            camera.yaw = -glm::half_pi<float>();
            camera.pitch = 0.0f;
        } else {
            return false;
        }

        camera.fly = false;

        return true;
    }

    // Driving a view while a file camera is up takes it over -- its controlled camera starts from the current view
    void DefineCameraTakeover(Ir77ViewCamera& camera) const {
        if (camera.index >= m_camera_count) return;

        DefineCameraFromView(camera, camera.fly);
        camera.index = m_camera_count;
    }

    // Orbit on the scene: target its centre, back off to frame its bounds, a little above
    void ResetCameraControl(Ir77ViewCamera& camera) const {
        float const radius = std::max(glm::length(m_bounds_max - m_bounds_min) * 0.5f, 0.01f);

        camera.fly = false;
        camera.target = (m_bounds_min + m_bounds_max) * 0.5f;
        camera.distance = radius * 2.5f;
        camera.yaw = glm::pi<float>();  // looking down -Z, from the +Z side
        camera.pitch = -0.35f;
        camera.speed = radius * 0.5f;  // units per second at full key
        camera.move = glm::vec3(0.0f);
    }

    // Takes over from whatever the view showed last frame -- file camera, orbit or fly
    void DefineCameraFromView(Ir77ViewCamera& camera, bool const fly) const {
        glm::mat4 const world = glm::inverse(camera.view);
        glm::vec3 const eye = glm::vec3(world[3]);
        glm::vec3 const forward = glm::normalize(-glm::vec3(world[2]));  // the view looks down its -Z

        camera.fly = fly;
        camera.yaw = std::atan2(forward.x, forward.z);
        camera.pitch = glm::clamp(std::asin(glm::clamp(forward.y, -1.0f, 1.0f)), -CAMERA_PITCH_LIMIT, CAMERA_PITCH_LIMIT);

        if (fly) {
            camera.position = eye;
        } else {
            camera.distance = std::max(glm::length(camera.target - eye), 0.01f);
            camera.target = eye + forward * camera.distance;  // keep the view; orbit around the point straight ahead
        }
    }

    // Held keys, every Update. Fly: walk along the view. Orbit: forward dollies in / out, right / up pan the target.
    void UpdateCameraControl(Ir77ViewCamera& camera, float const dt) const {
        if (camera.move == glm::vec3(0.0f) || dt <= 0.0f) return;

        glm::vec3 const forward = CameraForward(camera);
        // straight down / up has no horizon -- screen up is the heading (yaw), as MakeControlledCamera draws it
        glm::vec3 const heading = glm::vec3(std::sin(camera.yaw), 0.0f, std::cos(camera.yaw));
        glm::vec3 const right = glm::normalize(glm::cross(forward, std::abs(forward.y) > 0.99f ? heading : glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 const up = glm::cross(right, forward);

        float const step = camera.speed * dt;

        if (camera.fly) {
            camera.position += (right * camera.move.x + up * camera.move.y + forward * camera.move.z) * step;
        } else {
            camera.target += (right * camera.move.x + up * camera.move.y) * step;
            camera.distance = std::max(camera.distance * std::exp(-camera.move.z * dt), 0.01f);
        }
    }

    glm::vec3 CameraForward(Ir77ViewCamera const& camera) const {
        return glm::vec3(std::cos(camera.pitch) * std::sin(camera.yaw), std::sin(camera.pitch), std::cos(camera.pitch) * std::cos(camera.yaw));
    }

    // Standard Vulkan depth (near 0, far 1) -> reversed (near 1, far 0): z' = w - z, for perspective and orthographic alike. Float depth is
    // densest near 0, so reversed puts that precision at the distance, where standard depth runs out -- what stops crossing meshes from
    // fighting. Every camera passes through here; x, y and w are untouched (the lens read back from proj[1][1] is unchanged).
    void DefineReversedDepth(Ir77PVCamera& camera) const {
        glm::mat4 reverse{1.0f};
        reverse[2][2] = -1.0f;  // z' = -z ...
        reverse[3][2] = 1.0f;   //      ... + w

        camera.proj = reverse * camera.proj;
        camera.view_proj = camera.proj * camera.view;
    }

    // A view's controlled camera matrices. Planes hug the scene: whole bounds in front of far, near as large as depth precision allows.
    Ir77PVCamera MakeControlledCamera(Ir77ViewCamera const& view, float const aspect) const {
        glm::vec3 const center = (m_bounds_min + m_bounds_max) * 0.5f;
        float const radius = std::max(glm::length(m_bounds_max - m_bounds_min) * 0.5f, 0.01f);

        glm::vec3 const forward = CameraForward(view);
        glm::vec3 const eye = view.fly ? view.position : view.target - forward * view.distance;

        float const zfar = glm::length(eye - center) + radius * 2.0f;
        float const znear = std::max(zfar * 0.0001f, 0.001f);

        // perspective -- Vulkan clip space is Y-down, so the projection is flipped
        if (!view.ortho || view.fly) {
            Ir77PVCamera camera{};
            camera.eye = glm::vec4(eye, 1.0f);
            camera.view = glm::lookAtRH(eye, eye + forward, glm::vec3(0.0f, 1.0f, 0.0f));
            camera.proj = glm::perspectiveRH_ZO(glm::radians(view.fov), aspect, znear, zfar);
            camera.proj[1][1] *= -1.0f;
            camera.view_proj = camera.proj * camera.view;

            return camera;
        }

        // orthographic -- the box the perspective lens would see at the target's distance, so switching keeps the framing. Straight down,
        // screen up is the horizontal heading (yaw).
        float const half_height = view.distance * std::tan(glm::radians(view.fov) * 0.5f);
        glm::vec3 const up = std::abs(forward.y) > 0.99f ? glm::vec3(std::sin(view.yaw), 0.0f, std::cos(view.yaw)) : glm::vec3(0.0f, 1.0f, 0.0f);

        Ir77PVCamera camera{};
        camera.eye = glm::vec4(eye, 1.0f);
        camera.view = glm::lookAtRH(eye, eye + forward, up);
        camera.proj = glm::orthoRH_ZO(-half_height * aspect, half_height * aspect, -half_height, half_height, znear, zfar);
        camera.proj[1][1] *= -1.0f;
        camera.view_proj = camera.proj * camera.view;

        return camera;
    }

   private:
    std::shared_ptr<IIr77Camera> m_cameras{};  // the file's -- Ir77Pose's component

    glm::vec3 m_bounds_min{-1.0f};

    glm::vec3 m_bounds_max{1.0f};

    std::uint32_t m_camera_count{0};  // the file's cameras -- a view's index == this is its controlled camera

    std::array<Ir77ViewCamera, MAX_VIEWS> m_views{};  // [view] -- view 0 is the main camera

    std::vector<glm::vec4> m_areas{glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)};  // [view] -- the layout: where each draws, fractions of the window

    std::uint32_t m_active{0};  // the view the camera controls drive

    static constexpr float AREA_SLACK{1.0001f};  // an area may end this far past the window's edge (float arithmetic) -- PeregrineV clamps it

    static constexpr float CAMERA_LOOK_RADIANS_PER_PIXEL{0.004f};

    static constexpr float CAMERA_PITCH_LIMIT{1.5f};  // just short of straight up / down
};
}  // namespace NSIr77Scene
