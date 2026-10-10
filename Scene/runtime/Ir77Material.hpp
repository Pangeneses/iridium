#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fastgltf/core.hpp>

#include <array>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77Scene.hpp"
#include "../dictionary/IDIr77Scene.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77Material.hpp"

#include "../PeregrineV/types//Ir77PVTypes.hpp"

using namespace NSIr77RT;
using namespace NSIr77PeregrineV;

namespace NSIr77Scene {

// =============================================================================================================================================
// Ir77Material -- one entry per glTF material, same index as asset.materials (and as the material index on each mesh range).
// Constants map onto Ir77PVMaterialConstants (set 2 binding 0); texture slots are glTF texture indices in set 2 binding order
// (albedo, normal, ORM, emissive, AO), NO_TEXTURE where the material has none. Texture index == Ir77Texture index.
// =============================================================================================================================================
class Ir77Material : public Ir77Enlisted, public IIr77Material, public std::enable_shared_from_this<Ir77Material> {
   public:
    static constexpr std::uint32_t NO_TEXTURE = UINT32_MAX;

   private:
    struct Ir77MaterialEntry {
        Ir77PVMaterialConstants constants{};
        std::array<std::uint32_t, 5> textures{NO_TEXTURE, NO_TEXTURE, NO_TEXTURE, NO_TEXTURE, NO_TEXTURE};
        bool double_sided{false};
        bool blend{false};  // glTF alphaMode BLEND -- drawn in the Transparent pass, sorted back to front
    };

   public:
    Ir77Material() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    ~Ir77Material() {}

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77Material>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77Material>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77Material)
            obj = std::shared_ptr<IIr77Material>(shared_from_this(), static_cast<IIr77Material*>(this));

        else if (iid == GUIDIr77Material)
            obj = std::shared_ptr<Ir77Material>(shared_from_this(), static_cast<Ir77Material*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

   public:
    // Reads every glTF material's PBR factors, texture slots and double-sided flag.
    std::shared_ptr<IIr77Return const> LoadMaterialsFromAsset(fastgltf::Asset const& asset) {
        m_materials.assign(asset.materials.size(), Ir77MaterialEntry{});

        for (std::size_t m = 0; m < asset.materials.size(); m++) {
            fastgltf::Material const& material = asset.materials[m];
            Ir77MaterialEntry& out = m_materials[m];

            // factors
            auto const& base = material.pbrData.baseColorFactor;
            out.constants.base_color = {static_cast<float>(base[0]), static_cast<float>(base[1]), static_cast<float>(base[2]), static_cast<float>(base[3])};

            float const strength = static_cast<float>(material.emissiveStrength);  // KHR_materials_emissive_strength, 1 when absent
            auto const& emissive = material.emissiveFactor;
            out.constants.emissive = {static_cast<float>(emissive[0]) * strength, static_cast<float>(emissive[1]) * strength,
                                      static_cast<float>(emissive[2]) * strength, 1.0f};

            out.constants.roughness = static_cast<float>(material.pbrData.roughnessFactor);
            out.constants.metallic = static_cast<float>(material.pbrData.metallicFactor);
            out.constants.alpha_cutoff = material.alphaMode == fastgltf::AlphaMode::Mask ? static_cast<float>(material.alphaCutoff) : 0.0f;
            out.constants.normal_scale = material.normalTexture.has_value() ? static_cast<float>(material.normalTexture->scale) : 1.0f;

            // texture slots -- set 2 binding order
            if (material.pbrData.baseColorTexture.has_value()) out.textures[0] = static_cast<std::uint32_t>(material.pbrData.baseColorTexture->textureIndex);
            if (material.normalTexture.has_value()) out.textures[1] = static_cast<std::uint32_t>(material.normalTexture->textureIndex);
            if (material.pbrData.metallicRoughnessTexture.has_value())
                out.textures[2] = static_cast<std::uint32_t>(material.pbrData.metallicRoughnessTexture->textureIndex);
            if (material.emissiveTexture.has_value()) out.textures[3] = static_cast<std::uint32_t>(material.emissiveTexture->textureIndex);
            if (material.occlusionTexture.has_value()) out.textures[4] = static_cast<std::uint32_t>(material.occlusionTexture->textureIndex);

            out.double_sided = material.doubleSided;
            out.blend = material.alphaMode == fastgltf::AlphaMode::Blend;
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetMaterialCount(std::uint32_t& count) {
        count = static_cast<std::uint32_t>(m_materials.size());

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // textures: glTF texture index per slot (albedo, normal, ORM, emissive, AO), NO_TEXTURE when unused
    std::shared_ptr<IIr77Return const> GetMaterial(std::uint32_t const& index, Ir77PVMaterialConstants& constants, std::array<std::uint32_t, 5>& textures,
                                                   bool& double_sided) {
        if (index >= m_materials.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Material: material index out of range.");

        Ir77MaterialEntry const& material = m_materials[index];
        constants = material.constants;
        textures = material.textures;
        double_sided = material.double_sided;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // true when the glTF material is alphaMode BLEND
    std::shared_ptr<IIr77Return const> GetMaterialBlend(std::uint32_t const& index, bool& blend) {
        if (index >= m_materials.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Material: material index out of range.");

        blend = m_materials[index].blend;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    std::vector<Ir77MaterialEntry> m_materials{};
};
}  // namespace NSIr77Scene