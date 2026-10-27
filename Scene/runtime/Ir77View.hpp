#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

#include <algorithm>
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
// Ir77View -- the cameras: the file's, then the controlled orbit / fly / ortho camera the UI drives (index == camera count). Owned by
// Ir77Animate; the UI reaches it through the scene server's camera.* ops.
//
//   scene:     SetCameras (the file's) -> SetBounds (frames the controlled camera)
//   UI:        SetCameraMove / Look / Zoom / Speed / Mode / View, SelectCamera / NextCamera / ResetCamera, SetViewLayout -- next frame
//   per frame: Update(dt) -> WriteCameras(aspect) per window
//
// Layout: "single" -- one view, the live camera. "quad" -- four, in PeregrineV's 2 x 2 order (left to right, then top to bottom): top,
// front and side, orthographic and framed on the scene's bounds, then the live camera.
// =============================================================================================================================================
class Ir77View : public Ir77Enlisted, public IIr77View, public std::enable_shared_from_this<Ir77View> {
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
    // The file's cameras -- index count is the controlled camera. Starts on the file's first camera when it has one.
    std::shared_ptr<IIr77Return const> SetCameras(std::shared_ptr<IIr77Camera> const& cameras) {
        m_cameras = cameras;

        m_camera_count = 0;
        if (m_cameras) m_cameras->GetCameraCount(m_camera_count);

        m_camera_index = 0;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The scene's bounds -- what the controlled camera frames and its planes hug. Re-frames the controlled camera on them.
    std::shared_ptr<IIr77Return const> SetBounds(glm::vec3 const& low, glm::vec3 const& high) {
        m_bounds_min = low;
        m_bounds_max = high;

        ResetCameraControl();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Held keys from the UI, once per frame
    std::shared_ptr<IIr77Return const> Update(float const& dt) {
        UpdateCameraControl(dt);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The camera a window draws with: the file camera (rebuilt with this aspect) or the controlled camera (orbit / fly). Remembers its view
    // and lens, so the next switch to orbit / fly starts from it. camera.time is the caller's.
    std::shared_ptr<IIr77Return const> WriteCamera(float const& aspect, Ir77PVCamera& camera) {
        float const radius = std::max(glm::length(m_bounds_max - m_bounds_min) * 0.5f, 0.01f);

        camera = Ir77PVCamera{};
        bool built = false;

        if (m_camera_index < m_camera_count && m_cameras) {
            float const fallback_far = std::max(100.0f, radius * 20.0f);  // file camera without zfar
            built = m_cameras->BuildCamera(m_camera_index, aspect, fallback_far, camera)->ID() == GUIDIr77OperationSucceeded;
        }

        if (!built) camera = MakeControlledCamera(aspect);

        // reversed-Z -- PeregrineV's main pass clears depth to 0 and keeps the greater (Ir77PVCmdBuffer)
        DefineReversedDepth(camera);

        m_camera_view = camera.view;  // where the next switch to orbit / fly starts from

        // the lens too -- an orthographic projection has none, it keeps the last one
        if (built || !m_camera_ortho) m_camera_fov = glm::degrees(2.0f * std::atan(1.0f / std::abs(camera.proj[1][1])));

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // One camera per view, in the layout's order -- the live one is WriteCamera's. Each cell of the quad keeps the window's aspect.
    std::shared_ptr<IIr77Return const> WriteCameras(float const& aspect, std::vector<Ir77PVCamera>& cameras) {
        cameras.clear();

        Ir77PVCamera live{};
        WriteCamera(aspect, live);

        if (m_quad) {
            cameras.push_back(MakeFixedCamera(glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), aspect));  // top: down -Y, -Z up the screen
            cameras.push_back(MakeFixedCamera(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), aspect));   // front: down -Z
            cameras.push_back(MakeFixedCamera(glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), aspect));   // side: down -X
        }

        cameras.push_back(live);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // "single" | "quad" -- see the layout note at the top of the class
    std::shared_ptr<IIr77Return const> SetViewLayout(std::string_view const& layout) {
        if (layout != "single" && layout != "quad") return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: view layout must be single or quad.");

        m_quad = layout == "quad";

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // For the frame stats -- the active camera, and whether it is the controlled one
    std::shared_ptr<IIr77Return const> GetCameraState(std::uint32_t& index, bool& controlled) {
        index = m_camera_index;
        controlled = m_camera_index >= m_camera_count;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // cameras -- the file's, then the controlled one (index == camera count). The UI drives these; they take effect on the next frame.
    // -------------------------------------------------------------------------------------------------------------------------------------

    // Held keys: right / up / forward in -1..1, applied every Update until set back to 0
    std::shared_ptr<IIr77Return const> SetCameraMove(glm::vec3 const& move) {
        DefineCameraTakeover();
        m_camera_move = glm::clamp(move, glm::vec3(-1.0f), glm::vec3(1.0f));

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Mouse drag in pixels -- turns the fly camera / swings the orbit camera
    std::shared_ptr<IIr77Return const> SetCameraLook(float const& dx, float const& dy) {
        DefineCameraTakeover();
        m_camera_yaw -= dx * CAMERA_LOOK_RADIANS_PER_PIXEL;
        m_camera_pitch = glm::clamp(m_camera_pitch - dy * CAMERA_LOOK_RADIANS_PER_PIXEL, -CAMERA_PITCH_LIMIT, CAMERA_PITCH_LIMIT);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Wheel steps -- orbit distance, 10% per step
    std::shared_ptr<IIr77Return const> SetCameraZoom(float const& delta) {
        DefineCameraTakeover();
        m_camera_distance = std::max(m_camera_distance * std::pow(1.1f, -delta), 0.01f);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Multiplies the move speed
    std::shared_ptr<IIr77Return const> SetCameraSpeed(float const& factor) {
        if (factor <= 0.0f) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: camera speed factor must be above 0.");

        m_camera_speed *= factor;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // "orbit" | "fly" -- start from the current view, so switching never jumps; "ortho" -- orbit with an orthographic lens (no perspective,
    // zoom scales the view); "file" -- the file's first camera
    std::shared_ptr<IIr77Return const> SetCameraMode(std::string_view const& mode) {
        if (mode == "orbit" || mode == "fly" || mode == "ortho") {
            DefineCameraFromView(mode == "fly");
            m_camera_ortho = mode == "ortho";
            m_camera_index = m_camera_count;
        } else if (mode == "file") {
            if (m_camera_count == 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: the file has no cameras.");
            m_camera_index = 0;
        } else {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: camera mode must be orbit, fly, ortho or file.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // "top" (down -Y) | "front" (down -Z) | "side" (down -X) -- turns the orbit camera around its target; ortho looks straight down for top
    std::shared_ptr<IIr77Return const> SetCameraView(std::string_view const& view) {
        DefineCameraTakeover();
        m_camera_fly = false;

        if (view == "top") {
            m_camera_yaw = glm::pi<float>();
            m_camera_pitch = m_camera_ortho ? -glm::half_pi<float>() : -CAMERA_PITCH_LIMIT;
        } else if (view == "front") {
            m_camera_yaw = glm::pi<float>();
            m_camera_pitch = 0.0f;
        } else if (view == "side") {
            m_camera_yaw = -glm::half_pi<float>();
            m_camera_pitch = 0.0f;
        } else {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: camera view must be top, front or side.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // index == camera count -> the controlled camera
    std::shared_ptr<IIr77Return const> SelectCamera(std::uint32_t const& index) {
        if (index > m_camera_count) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: no such camera.");

        m_camera_index = index;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The next file camera, then the controlled camera, then around again (F2)
    std::shared_ptr<IIr77Return const> NextCamera() {
        m_camera_index = (m_camera_index + 1) % (m_camera_count + 1);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Orbit, framed on the scene
    std::shared_ptr<IIr77Return const> ResetCamera() {
        ResetCameraControl();
        m_camera_index = m_camera_count;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    // Driving the camera while a file camera is up takes it over -- the controlled camera starts from the current view
    void DefineCameraTakeover() {
        if (m_camera_index >= m_camera_count) return;

        DefineCameraFromView(m_camera_fly);
        m_camera_index = m_camera_count;
    }

    // Orbit on the scene: target its centre, back off to frame its bounds, a little above
    void ResetCameraControl() {
        float const radius = std::max(glm::length(m_bounds_max - m_bounds_min) * 0.5f, 0.01f);

        m_camera_fly = false;
        m_camera_target = (m_bounds_min + m_bounds_max) * 0.5f;
        m_camera_distance = radius * 2.5f;
        m_camera_yaw = glm::pi<float>();  // looking down -Z, from the +Z side
        m_camera_pitch = -0.35f;
        m_camera_speed = radius * 0.5f;  // units per second at full key
        m_camera_move = glm::vec3(0.0f);
    }

    // Takes over from whatever was on screen last frame -- file camera, orbit or fly
    void DefineCameraFromView(bool const fly) {
        glm::mat4 const world = glm::inverse(m_camera_view);
        glm::vec3 const eye = glm::vec3(world[3]);
        glm::vec3 const forward = glm::normalize(-glm::vec3(world[2]));  // the view looks down its -Z

        m_camera_fly = fly;
        m_camera_yaw = std::atan2(forward.x, forward.z);
        m_camera_pitch = glm::clamp(std::asin(glm::clamp(forward.y, -1.0f, 1.0f)), -CAMERA_PITCH_LIMIT, CAMERA_PITCH_LIMIT);

        if (fly) {
            m_camera_position = eye;
        } else {
            m_camera_distance = std::max(glm::length(m_camera_target - eye), 0.01f);
            m_camera_target = eye + forward * m_camera_distance;  // keep the view; orbit around the point straight ahead
        }
    }

    // Held keys, every Update. Fly: walk along the view. Orbit: forward dollies in / out, right / up pan the target.
    void UpdateCameraControl(float const dt) {
        if (m_camera_move == glm::vec3(0.0f) || dt <= 0.0f) return;

        glm::vec3 const forward = CameraForward();
        glm::vec3 const right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 const up = glm::cross(right, forward);

        float const step = m_camera_speed * dt;

        if (m_camera_fly) {
            m_camera_position += (right * m_camera_move.x + up * m_camera_move.y + forward * m_camera_move.z) * step;
        } else {
            m_camera_target += (right * m_camera_move.x + up * m_camera_move.y) * step;
            m_camera_distance = std::max(m_camera_distance * std::exp(-m_camera_move.z * dt), 0.01f);
        }
    }

    glm::vec3 CameraForward() const {
        return glm::vec3(std::cos(m_camera_pitch) * std::sin(m_camera_yaw), std::sin(m_camera_pitch), std::cos(m_camera_pitch) * std::cos(m_camera_yaw));
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

    // The controlled camera's matrices. Planes hug the scene: whole bounds in front of far, near as large as depth precision allows.
    Ir77PVCamera MakeControlledCamera(float const aspect) const {
        glm::vec3 const center = (m_bounds_min + m_bounds_max) * 0.5f;
        float const radius = std::max(glm::length(m_bounds_max - m_bounds_min) * 0.5f, 0.01f);

        glm::vec3 const forward = CameraForward();
        glm::vec3 const eye = m_camera_fly ? m_camera_position : m_camera_target - forward * m_camera_distance;

        float const zfar = glm::length(eye - center) + radius * 2.0f;
        float const znear = std::max(zfar * 0.0001f, 0.001f);

        // perspective -- Vulkan clip space is Y-down, so the projection is flipped
        if (!m_camera_ortho || m_camera_fly) {
            Ir77PVCamera camera{};
            camera.eye = glm::vec4(eye, 1.0f);
            camera.view = glm::lookAtRH(eye, eye + forward, glm::vec3(0.0f, 1.0f, 0.0f));
            camera.proj = glm::perspectiveRH_ZO(glm::radians(m_camera_fov), aspect, znear, zfar);
            camera.proj[1][1] *= -1.0f;
            camera.view_proj = camera.proj * camera.view;

            return camera;
        }

        // orthographic -- the box the perspective lens would see at the target's distance, so switching keeps the framing. Straight down,
        // screen up is the horizontal heading (yaw).
        float const half_height = m_camera_distance * std::tan(glm::radians(m_camera_fov) * 0.5f);
        glm::vec3 const up = std::abs(forward.y) > 0.99f ? glm::vec3(std::sin(m_camera_yaw), 0.0f, std::cos(m_camera_yaw)) : glm::vec3(0.0f, 1.0f, 0.0f);

        Ir77PVCamera camera{};
        camera.eye = glm::vec4(eye, 1.0f);
        camera.view = glm::lookAtRH(eye, eye + forward, up);
        camera.proj = glm::orthoRH_ZO(-half_height * aspect, half_height * aspect, -half_height, half_height, znear, zfar);
        camera.proj[1][1] *= -1.0f;
        camera.view_proj = camera.proj * camera.view;

        return camera;
    }

    // A quad view's fixed camera: orthographic down forward, the whole scene in the box, screen up along up -- the controls don't move it
    Ir77PVCamera MakeFixedCamera(glm::vec3 const& forward, glm::vec3 const& up, float const aspect) const {
        glm::vec3 const center = (m_bounds_min + m_bounds_max) * 0.5f;
        float const radius = std::max(glm::length(m_bounds_max - m_bounds_min) * 0.5f, 0.01f);

        glm::vec3 const eye = center - forward * radius * 2.0f;

        // the bounds' sphere fits the shorter side
        float const half_height = aspect >= 1.0f ? radius : radius / aspect;

        Ir77PVCamera camera{};
        camera.eye = glm::vec4(eye, 1.0f);
        camera.view = glm::lookAtRH(eye, center, up);
        camera.proj = glm::orthoRH_ZO(-half_height * aspect, half_height * aspect, -half_height, half_height, radius * 0.5f, radius * 3.5f);
        camera.proj[1][1] *= -1.0f;

        DefineReversedDepth(camera);

        return camera;
    }

   private:
    std::shared_ptr<IIr77Camera> m_cameras{};  // the file's -- Ir77Pose's component

    glm::vec3 m_bounds_min{-1.0f};

    glm::vec3 m_bounds_max{1.0f};

    // cameras -- index m_camera_count is the controlled camera; "next" / F2 cycles
    std::uint32_t m_camera_count{0};

    std::uint32_t m_camera_index{0};

    // controlled camera -- orbit (target, distance) or fly (position); yaw / pitch aim both
    bool m_camera_fly{false};

    glm::vec3 m_camera_target{0.0f};

    glm::vec3 m_camera_position{0.0f};

    float m_camera_distance{1.0f};

    float m_camera_yaw{0.0f};

    float m_camera_pitch{0.0f};

    float m_camera_speed{1.0f};  // units per second at full key

    glm::vec3 m_camera_move{0.0f};  // held keys: right, up, forward in -1..1

    glm::mat4 m_camera_view{1.0f};  // last frame's view, whichever camera drew it

    float m_camera_fov{60.0f};  // vertical, degrees -- taken from whichever camera drew last, so a takeover keeps the lens

    static constexpr float CAMERA_LOOK_RADIANS_PER_PIXEL{0.004f};

    static constexpr float CAMERA_PITCH_LIMIT{1.5f};  // just short of straight up / down

    bool m_camera_ortho{false};  // the controlled orbit camera draws orthographic

    bool m_quad{false};  // view layout -- four views instead of one
};
}  // namespace NSIr77Scene
