#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVDevice.hpp"
#include "IIr77PVDescriptorSet.hpp"
#include "IIr77PVBuffer.hpp"
#include "IIr77PVTransfer.hpp"

#include "../types/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// Textures, canvases and material rows. Bindless: textures get texture slots, materials are rows in the device's material table (holding
// texture slots, placeholders where none is given). Geometry reads materials back by id for its draw items.
//   Ir77PVInit wires it: SetDevice / SetAllocator / SetDescriptorSet / SetTransfer / SetMaterialTable / SetPlaceholderSlots
typedef struct IIr77PVMaterial : virtual public IIr77Enlisted {
    IIr77PVMaterial() = default;

    // ---- wiring -- Ir77PVInit sets these as each piece exists ---------------------------------------------------------------------------

    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    virtual std::shared_ptr<IIr77Return const> SetAllocator(VmaAllocator allocator) = 0;

    // The device's bindless table -- textures and canvases get their slots in it
    virtual std::shared_ptr<IIr77Return const> SetDescriptorSet(std::shared_ptr<IIr77PVDescriptorSet> descriptor_set) = 0;

    // Uploads -- the material rows go up through it
    virtual std::shared_ptr<IIr77Return const> SetTransfer(std::shared_ptr<IIr77PVTransfer> transfer) = 0;

    // The device's material table (MATERIAL_CAPACITY rows) -- UploadMaterials writes rows into it
    virtual std::shared_ptr<IIr77Return const> SetMaterialTable(std::shared_ptr<IIr77PVBuffer> material_table) = 0;

    // Texture slots of the placeholders -- what a material row gets where no texture is given
    virtual std::shared_ptr<IIr77Return const> SetPlaceholderSlots(std::map<Ir77PVPlaceholderKind, std::uint32_t> const& slots) = 0;

    // ---- textures ----------------------------------------------------------------------------------------------------------------------------

    // textures -- built on Ir77PVTexture (the same class as the placeholders), so they get its kinds, mip chain and sampler handling.
    // srgb = true for color data (albedo, emissive) -> Color kind (sRGB); false for data maps (normal, ORM, AO) -> Data kind (UNORM).
    // Blocking: Ir77PVTexture uploads and waits on its own. Registered in the bindless table -- GetTextureSlot gives its slot.
    virtual std::shared_ptr<IIr77Return const> CreateTexture(std::uint8_t const* rgba, std::uint32_t const& width, std::uint32_t const& height,
                                                             bool const& srgb, std::uint32_t& texture_id) = 0;

    // A canvas -- size x size RGBA8, cleared to transparent, that compute writes (storage image slot) and fragment shaders sample (texture
    // slot). Stays in VK_IMAGE_LAYOUT_GENERAL for both; the Compute pass's barriers order the writes against the reads.
    virtual std::shared_ptr<IIr77Return const> CreateCanvas(std::uint32_t const& size, std::uint32_t& texture_slot, std::uint32_t& image_slot) = 0;

    // Bindless texture slot of a texture -- what material rows and shaders use
    virtual std::shared_ptr<IIr77Return const> GetTextureSlot(std::uint32_t const& texture_id, std::uint32_t& slot) = 0;

    // The texture and its slot into Transfer's graveyard -- freed once no frame in flight can sample it. The id is free for the next texture
    // now. Destroy the materials that use it first (or with it).
    virtual std::shared_ptr<IIr77Return const> DestroyTexture(std::uint32_t const& texture_id) = 0;

    // ---- materials -- rows in the device's material table: constants + 5 texture slots ---------------------------------------------------

    virtual std::shared_ptr<IIr77Return const> CreateMaterial(Ir77PVMaterialConstants const& constants, std::uint32_t& material_id) = 0;

    virtual std::shared_ptr<IIr77Return const> CreateMaterial(Ir77PVMaterialConstants const& constants, Ir77PVMaterialTextures const& textures,
                                                              std::uint32_t& material_id) = 0;

    virtual std::shared_ptr<IIr77Return const> UploadMaterials(std::vector<Ir77PVMaterialConstants> const& constants,
                                                               std::vector<std::uint32_t>& material_ids) = 0;

    // One staging copy for every new row. Rows come from the free list first (DestroyMaterials), then the unused end of the table -- a row
    // is only ever rewritten once no frame in flight can read it.
    virtual std::shared_ptr<IIr77Return const> UploadMaterials(std::vector<Ir77PVMaterialConstants> const& constants,
                                                               std::vector<Ir77PVMaterialTextures> const& textures,
                                                               std::vector<std::uint32_t>& material_ids) = 0;

    virtual std::shared_ptr<IIr77Return const> GetMaterial(std::uint32_t const& material_id, Ir77PVMaterialData& material) = 0;

    // The rows are gone now (GetMaterial fails) and back on the free list once no frame in flight can read them -- through Transfer's
    // graveyard. Remove the draws using them first.
    virtual std::shared_ptr<IIr77Return const> DestroyMaterials(std::vector<std::uint32_t> const& material_ids) = 0;

    virtual ~IIr77PVMaterial() = default;
}* pIIr77PVMaterial;
}  // namespace NSIr77PeregrineV
