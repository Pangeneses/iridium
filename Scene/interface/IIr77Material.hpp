#pragma once

#include <fastgltf/core.hpp>

#include <array>
#include <cstdint>
#include <memory>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../PeregrineV/types//Ir77PVTypes.hpp"

using namespace NSIr77RT;
using namespace NSIr77PeregrineV;

namespace NSIr77Scene {

// Scene-side materials: glTF PBR factors and texture slots, one entry per glTF material (same index as asset.materials).
// The GPU copy is made by Ir77PVAsset::UploadMaterials from GetMaterial's output.
typedef struct IIr77Material : virtual public IIr77Enlisted {
    IIr77Material() = default;

    virtual std::shared_ptr<IIr77Return const> LoadMaterialsFromAsset(fastgltf::Asset const& asset) = 0;

    virtual std::shared_ptr<IIr77Return const> GetMaterialCount(std::uint32_t& count) = 0;

    // textures: glTF texture index per slot (albedo, normal, ORM, emissive, AO), UINT32_MAX when unused
    virtual std::shared_ptr<IIr77Return const> GetMaterial(std::uint32_t const& index, Ir77PVMaterialConstants& constants,
                                                           std::array<std::uint32_t, 5>& textures, bool& double_sided) = 0;

    // true when the glTF material is alphaMode BLEND
    virtual std::shared_ptr<IIr77Return const> GetMaterialBlend(std::uint32_t const& index, bool& blend) = 0;

    virtual ~IIr77Material() = default;
}* pIIr77Material;
}  // namespace NSIr77Scene