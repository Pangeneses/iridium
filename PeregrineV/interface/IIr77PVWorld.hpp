#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVPipelineCache.hpp"
#include "IIr77PVTransfer.hpp"
#include "IIr77PVMaterial.hpp"
#include "IIr77PVGeometry.hpp"
#include "IIr77PVAcceleration.hpp"
#include "IIr77PVPaint.hpp"

#include "../types/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// What gets drawn: assets (LoadAsset -- textures, materials, geometry, BLASes, each in its own range of models[] and joint matrices) and,
// every frame, the frame (SubmitFrame -- cameras, lights, transforms, bones -> frame data, skinning, draw lists, TLAS transforms).
//   Ir77PVInit wires it: SetTransfer / SetMaterials / SetGeometry / SetPipelineCache / SetPaint / [SetAcceleration] -> CreateWorld
struct IIr77PVWorld : virtual public IIr77Enlisted {
    IIr77PVWorld() = default;

    // ---- wiring -- Ir77PVInit ------------------------------------------------------------------------------------------------------------------

    virtual std::shared_ptr<IIr77Return const> SetTransfer(std::shared_ptr<IIr77PVTransfer> transfer) = 0;

    virtual std::shared_ptr<IIr77Return const> SetMaterials(std::shared_ptr<IIr77PVMaterial> materials) = 0;

    virtual std::shared_ptr<IIr77Return const> SetGeometry(std::shared_ptr<IIr77PVGeometry> geometry) = 0;

    virtual std::shared_ptr<IIr77Return const> SetPipelineCache(std::shared_ptr<IIr77PVPipelineCache> pipeline_cache) = 0;

    // Where each frame goes -- used from the first SubmitFrame, after CreateCommandBuffers
    virtual std::shared_ptr<IIr77Return const> SetPaint(std::shared_ptr<IIr77PVPaint> paint) = 0;

    // Ray-query devices only -- assets get BLASes, the TLAS follows models[]
    virtual std::shared_ptr<IIr77Return const> SetAcceleration(std::shared_ptr<IIr77PVAcceleration> acceleration) = 0;

    // The grey default material -- primitives without one, and every procedural draw. device = the GPU's name, for the asset reports.
    virtual std::shared_ptr<IIr77Return const> CreateWorld(std::string const& device) = 0;

    // ---- assets ------------------------------------------------------------------------------------------------------------------------------

    // Everything the bundle holds onto the device, blocking. asset_id for SubmitFrame / UnloadAsset.
    virtual std::shared_ptr<IIr77Return const> LoadAsset(Ir77PVAssetBundle const& bundle, std::uint32_t& asset_id, Ir77PVAssetReport& report) = 0;

    // Its geometry, tables and BLASes freed, its models[] and joint ranges back for the next asset. Its draws go with the next SubmitFrame.
    virtual std::shared_ptr<IIr77Return const> UnloadAsset(std::uint32_t const& asset_id) = 0;

    // ---- per frame ---------------------------------------------------------------------------------------------------------------------------

    // The frame -> Paint (frame data, dispatches, draw lists for every window) and Acceleration (models[], which assets are in the rays)
    virtual std::shared_ptr<IIr77Return const> SubmitFrame(Ir77PVFrame const& frame, Ir77PVFrameReport& report) = 0;

    // The grey default's material id -- what procedural draw items carry
    virtual std::shared_ptr<IIr77Return const> GetDefaultMaterial(std::uint32_t& material_id) = 0;

    virtual ~IIr77PVWorld() = default;
};
}  // namespace NSIr77PeregrineV
