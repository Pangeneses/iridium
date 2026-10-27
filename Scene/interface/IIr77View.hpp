#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77Camera.hpp"

#include "../../PeregrineV/types/Ir77PVTypes.hpp"

using namespace NSIr77RT;
using namespace NSIr77PeregrineV;

namespace NSIr77Scene {

// The cameras, one per view -- each the file's cameras or its own controlled orbit / fly / ortho camera (index == camera count). The camera
// controls drive the focused view.
//   SetCameras -> SetBounds -> per frame: Update(dt) -> WriteCameras(aspect)
typedef struct IIr77View : virtual public IIr77Enlisted {
    IIr77View() = default;

    // The file's cameras -- index count is the controlled camera. View 0 starts on the file's first camera when it has one.
    virtual std::shared_ptr<IIr77Return const> SetCameras(std::shared_ptr<IIr77Camera> const& cameras) = 0;

    // The scene's bounds -- what the controlled cameras frame and their planes hug. Puts every view back where it starts.
    virtual std::shared_ptr<IIr77Return const> SetBounds(glm::vec3 const& low, glm::vec3 const& high) = 0;

    // Held keys from the UI, once per frame -- every view
    virtual std::shared_ptr<IIr77Return const> Update(float const& dt) = 0;

    // One camera per view in the layout, and where each draws. aspect is the window's; each camera gets its area's.
    virtual std::shared_ptr<IIr77Return const> WriteCameras(float const& aspect, std::vector<Ir77PVCamera>& cameras, std::vector<glm::vec4>& areas) = 0;

    // Where each view draws, 1 .. MAX_VIEWS of them -- x, y, width, height as fractions of the window. View 0 is the main camera; views 1 / 2 / 3
    // start orthographic down top / front / side. Until it's called, view 0 fills the window.
    virtual std::shared_ptr<IIr77Return const> SetViewLayout(std::vector<glm::vec4> const& areas) = 0;

    // The view the camera controls drive -- the one under the mouse
    virtual std::shared_ptr<IIr77Return const> FocusView(std::uint32_t const& view) = 0;

    // For the frame stats -- the focused view's camera, and whether it is the controlled one
    virtual std::shared_ptr<IIr77Return const> GetCameraState(std::uint32_t& index, bool& controlled) = 0;

    // ---- the focused view -------------------------------------------------------------------------------------------------------------------

    // Held keys: right / up / forward in -1..1, applied every Update until set back to 0
    virtual std::shared_ptr<IIr77Return const> SetCameraMove(glm::vec3 const& move) = 0;

    // Mouse drag in pixels -- turns the fly camera / swings the orbit camera
    virtual std::shared_ptr<IIr77Return const> SetCameraLook(float const& dx, float const& dy) = 0;

    // Wheel steps -- orbit distance (the ortho box with it), 10% per step
    virtual std::shared_ptr<IIr77Return const> SetCameraZoom(float const& delta) = 0;

    // Multiplies the move speed
    virtual std::shared_ptr<IIr77Return const> SetCameraSpeed(float const& factor) = 0;

    // "orbit" | "fly" -- start from the current view, so switching never jumps; "ortho" -- orbit with an orthographic lens (no perspective,
    // zoom scales the view); "file" -- the file's first camera
    virtual std::shared_ptr<IIr77Return const> SetCameraMode(std::string_view const& mode) = 0;

    // "top" (down -Y) | "front" (down -Z) | "side" (down -X) -- turns the orbit camera around its target; ortho looks straight down for top
    virtual std::shared_ptr<IIr77Return const> SetCameraView(std::string_view const& view) = 0;

    // index == camera count -> the controlled camera
    virtual std::shared_ptr<IIr77Return const> SelectCamera(std::uint32_t const& index) = 0;

    // The next file camera, then the controlled camera, then around again (F2)
    virtual std::shared_ptr<IIr77Return const> NextCamera() = 0;

    // The controlled camera, back where this view starts
    virtual std::shared_ptr<IIr77Return const> ResetCamera() = 0;

    virtual ~IIr77View() = default;
}* pIIr77View;
}  // namespace NSIr77Scene
