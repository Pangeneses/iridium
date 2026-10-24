#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
#include <string_view>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77Camera.hpp"

#include "../../PeregrineV/types/Ir77PVTypes.hpp"

using namespace NSIr77RT;
using namespace NSIr77PeregrineV;

namespace NSIr77Scene {

// The cameras -- the file's, then the controlled orbit / fly / ortho camera (index == camera count) the UI drives.
//   SetCameras -> SetBounds -> per frame: Update(dt) -> WriteCamera(aspect)
typedef struct IIr77View : virtual public IIr77Enlisted {
    IIr77View() = default;

    // The file's cameras -- index count is the controlled camera. Starts on the file's first camera when it has one.
    virtual std::shared_ptr<IIr77Return const> SetCameras(std::shared_ptr<IIr77Camera> const& cameras) = 0;

    // The scene's bounds -- what the controlled camera frames and its planes hug. Re-frames the controlled camera on them.
    virtual std::shared_ptr<IIr77Return const> SetBounds(glm::vec3 const& low, glm::vec3 const& high) = 0;

    // Held keys from the UI, once per frame
    virtual std::shared_ptr<IIr77Return const> Update(float const& dt) = 0;

    // The camera a window draws with: the file camera (rebuilt with this aspect) or the controlled camera (orbit / fly). Remembers its view
    // and lens, so the next switch to orbit / fly starts from it. camera.time is the caller's.
    virtual std::shared_ptr<IIr77Return const> WriteCamera(float const& aspect, Ir77PVCamera& camera) = 0;

    // For the frame stats -- the active camera, and whether it is the controlled one
    virtual std::shared_ptr<IIr77Return const> GetCameraState(std::uint32_t& index, bool& controlled) = 0;

    // Held keys: right / up / forward in -1..1, applied every Update until set back to 0
    virtual std::shared_ptr<IIr77Return const> SetCameraMove(glm::vec3 const& move) = 0;

    // Mouse drag in pixels -- turns the fly camera / swings the orbit camera
    virtual std::shared_ptr<IIr77Return const> SetCameraLook(float const& dx, float const& dy) = 0;

    // Wheel steps -- orbit distance, 10% per step
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

    // Orbit, framed on the scene
    virtual std::shared_ptr<IIr77Return const> ResetCamera() = 0;

    virtual ~IIr77View() = default;
}* pIIr77View;
}  // namespace NSIr77Scene
