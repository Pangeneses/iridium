#pragma once

#include <vk_mem_alloc.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "../types/Ir77PVTypes.hpp"

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/types/Ir77Types.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"
#include "../../Ir77RT/runtime/Ir77Return.hpp"

#include "../interface/IIr77PVDevice.hpp"
#include "../interface/IIr77PVDescriptorSet.hpp"
#include "../interface/IIr77PVBuffer.hpp"
#include "../interface/IIr77PVTexture.hpp"
#include "../interface/IIr77PVTransfer.hpp"
#include "../interface/IIr77PVMaterial.hpp"

#include "./Ir77PVTexture.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// =============================================================================================================================================
// Ir77PVMaterial -- textures, canvases and material rows.
//
//   startup:   Ir77PVInit wires it -- SetDevice / SetAllocator / SetDescriptorSet / SetTransfer / SetMaterialTable / SetPlaceholderSlots
//   load:      CreateTexture / CreateCanvas -> CreateMaterial / UploadMaterials -> material ids for Geometry's draw items (GetMaterial)
//   unload:    DestroyMaterials / DestroyTexture -- through Transfer's graveyard
// One device; it never sees Ir77PVInit.
//
// Bindless: textures get texture slots, materials are rows in the device's material table (holding texture slots). A material id is its row.
// A destroyed row goes back on the free list only when no frame in flight can read it, so a row is never rewritten under a frame.
// =============================================================================================================================================
class Ir77PVMaterial : public Ir77Enlisted, public IIr77PVMaterial, public std::enable_shared_from_this<Ir77PVMaterial> {
   public:
    Ir77PVMaterial() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    // Ir77PVInit releases this after Geometry, before Transfer, its allocator and device -- all still alive here
    ~Ir77PVMaterial() {
        if (!m_device) return;

        VkDevice vk_device{VK_NULL_HANDLE};
        m_device->GetDevice(&vk_device);
        if (vk_device != VK_NULL_HANDLE) vkDeviceWaitIdle(vk_device);

        m_textures.clear();  // frees its own memory -- the device is idle and the allocator still alive here
        m_canvases.clear();
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVMaterial>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PeregrineV>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77PVMaterial)
            obj = std::shared_ptr<IIr77PVMaterial>(shared_from_this(), static_cast<IIr77PVMaterial*>(this));

        else if (iid == GUIDIr77PVMaterial)
            obj = std::shared_ptr<Ir77PVMaterial>(shared_from_this(), static_cast<Ir77PVMaterial*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

    // =========================================================================================================================================
    // wiring -- Ir77PVInit sets these as each piece exists
    // =========================================================================================================================================
   public:
    std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) {
        m_device = device;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetAllocator(VmaAllocator allocator) {
        m_allocator = allocator;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The device's bindless table -- textures and canvases get their slots in it
    std::shared_ptr<IIr77Return const> SetDescriptorSet(std::shared_ptr<IIr77PVDescriptorSet> descriptor_set) {
        m_descriptor_set = descriptor_set;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Uploads -- the material rows go up through it
    std::shared_ptr<IIr77Return const> SetTransfer(std::shared_ptr<IIr77PVTransfer> transfer) {
        m_transfer = transfer;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The device's material table (MATERIAL_CAPACITY rows) -- UploadMaterials writes rows into it
    std::shared_ptr<IIr77Return const> SetMaterialTable(std::shared_ptr<IIr77PVBuffer> material_table) {
        m_material_table = material_table;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Texture slots of the placeholders -- what a material row gets where no texture is given
    std::shared_ptr<IIr77Return const> SetPlaceholderSlots(std::map<Ir77PVPlaceholderKind, std::uint32_t> const& slots) {
        m_placeholder_slots = slots;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // textures -- built on Ir77PVTexture (the same class as the placeholders), so they get its kinds, mip chain and sampler handling.
    // srgb = true for color data (albedo, emissive) -> Color kind (sRGB); false for data maps (normal, ORM, AO) -> Data kind (UNORM).
    // Blocking: Ir77PVTexture uploads and waits on its own. Registered in the bindless table -- GetTextureSlot gives its slot.
    // =========================================================================================================================================
    std::shared_ptr<IIr77Return const> CreateTexture(std::uint8_t const* rgba, std::uint32_t const& width, std::uint32_t const& height, bool const& srgb,
                                                     std::uint32_t& texture_id) {
        if (rgba == nullptr || width == 0 || height == 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVMaterial: empty texture.");

        if (!m_device || !m_descriptor_set) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVMaterial: device or bindless table not set.");

        VkCommandPool pool{VK_NULL_HANDLE};
        VkQueue queue{VK_NULL_HANDLE};

        auto const pool_result = m_device->GetUploadPool(pool, queue);
        if (pool_result->ID() != GUIDIr77OperationSucceeded) return pool_result;

        auto texture = std::static_pointer_cast<IIr77PVTexture>(std::make_shared<Ir77PVTexture>());

        texture->SetDevice(m_device);
        texture->SetAllocator(m_allocator);
        texture->SetKind(srgb ? Ir77PVTextureKind::Color : Ir77PVTextureKind::Data);  // first -- resets format and sampler to the kind's defaults
        texture->SetSampler(Ir77PVSamplerDesc{});                                     // linear / repeat; anisotropy needs samplerAnisotropy on the device

        if (texture->CreateFromPixels(pool, queue, rgba, width, height, true)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVMaterial: texture upload failed.");

        VkDescriptorImageInfo info{};
        texture->GetImageInfo(&info);

        std::uint32_t slot{BINDLESS_NO_SLOT};

        auto const result = m_descriptor_set->RegisterTexture(info.imageView, info.sampler, Ir77PVSlotKind::Texture, slot);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        // a destroyed texture's id first -- ids name nothing on the GPU, so they are reusable at once
        if (!m_free_textures.empty()) {
            texture_id = m_free_textures.back();
            m_free_textures.pop_back();

            m_textures[texture_id] = texture;
            m_texture_slots[texture_id] = slot;
        } else {
            texture_id = static_cast<std::uint32_t>(m_textures.size());
            m_textures.push_back(texture);
            m_texture_slots.push_back(slot);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // A canvas -- size x size RGBA8, cleared to transparent, that compute writes (storage image slot) and fragment shaders sample (texture
    // slot). Stays in VK_IMAGE_LAYOUT_GENERAL for both; the Compute pass's barriers order the writes against the reads.
    std::shared_ptr<IIr77Return const> CreateCanvas(std::uint32_t const& size, std::uint32_t& texture_slot, std::uint32_t& image_slot) {
        if (!m_device || !m_descriptor_set) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVMaterial: device or bindless table not set.");

        VkCommandPool pool{VK_NULL_HANDLE};
        VkQueue queue{VK_NULL_HANDLE};

        std::shared_ptr<IIr77Return const> result{};
        if (Failed(m_device->GetUploadPool(pool, queue), result)) return result;

        auto texture = std::static_pointer_cast<IIr77PVTexture>(std::make_shared<Ir77PVTexture>());

        texture->SetDevice(m_device);
        texture->SetAllocator(m_allocator);
        texture->SetKind(Ir77PVTextureKind::Storage);  // first -- RGBA8 UNORM, clamp to edge

        if (Failed(texture->CreateStorage(pool, queue, size, size), result)) return result;

        VkDescriptorImageInfo info{};
        texture->GetImageInfo(&info);  // imageLayout = GENERAL

        if (Failed(m_descriptor_set->RegisterTexture(info.imageView, info.sampler, Ir77PVSlotKind::Canvas, texture_slot, info.imageLayout), result))
            return result;
        if (Failed(m_descriptor_set->RegisterStorageImage(info.imageView, Ir77PVSlotKind::CanvasImage, image_slot), result)) return result;

        m_canvases.push_back(texture);  // kept for the device's lifetime

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Bindless texture slot of a texture -- what material rows and shaders use
    std::shared_ptr<IIr77Return const> GetTextureSlot(std::uint32_t const& texture_id, std::uint32_t& slot) {
        if (texture_id >= m_texture_slots.size() || !m_textures[texture_id]) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVMaterial: unknown texture.");

        slot = m_texture_slots[texture_id];

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The texture and its slot into Transfer's graveyard -- freed once no frame in flight can sample it. The id is free for the next texture
    // now. Destroy the materials that use it first (or with it).
    std::shared_ptr<IIr77Return const> DestroyTexture(std::uint32_t const& texture_id) {
        if (texture_id >= m_textures.size() || !m_textures[texture_id]) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVMaterial: unknown texture.");
        if (!m_transfer) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVMaterial: transfer not set.");

        m_transfer->Bury({.textures = {m_textures[texture_id]}, .slots = {{BINDLESS_BINDING_TEXTURES, m_texture_slots[texture_id]}}});

        m_textures[texture_id].reset();
        m_texture_slots[texture_id] = BINDLESS_NO_SLOT;
        m_free_textures.push_back(texture_id);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // materials -- rows in the device's material table: constants + 5 texture slots (placeholders where no texture is given)
    // =========================================================================================================================================
    std::shared_ptr<IIr77Return const> CreateMaterial(Ir77PVMaterialConstants const& constants, std::uint32_t& material_id) {
        return CreateMaterial(constants, Ir77PVMaterialTextures{}, material_id);
    }

    std::shared_ptr<IIr77Return const> CreateMaterial(Ir77PVMaterialConstants const& constants, Ir77PVMaterialTextures const& textures,
                                                      std::uint32_t& material_id) {
        std::vector<std::uint32_t> ids{};
        auto const result = UploadMaterials({constants}, {textures}, ids);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        material_id = ids.front();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> UploadMaterials(std::vector<Ir77PVMaterialConstants> const& constants, std::vector<std::uint32_t>& material_ids) {
        return UploadMaterials(constants, std::vector<Ir77PVMaterialTextures>(constants.size()), material_ids);
    }

    // One staging copy for every new row. Rows come from the free list first (DestroyMaterials puts them there once no frame in flight can
    // read them), then the unused end of the table. A material id is its row.
    std::shared_ptr<IIr77Return const> UploadMaterials(std::vector<Ir77PVMaterialConstants> const& constants,
                                                       std::vector<Ir77PVMaterialTextures> const& textures, std::vector<std::uint32_t>& material_ids) {
        if (constants.size() != textures.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVMaterial: constants / textures count mismatch.");

        if (!m_material_table) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVMaterial: material table not set.");
        if (!m_transfer) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVMaterial: transfer not set.");

        std::size_t const available = m_free_rows.size() + (MATERIAL_CAPACITY - m_row_used.size());
        if (constants.size() > available)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVMaterial: material table full (" + std::to_string(MATERIAL_CAPACITY) +
                                                           " rows) -- unload something or raise MATERIAL_CAPACITY.");

        std::vector<Ir77PVMaterialRow> rows(constants.size());

        for (std::size_t i = 0; i < constants.size(); i++) {
            auto const result = DefineMaterialRow(constants[i], textures[i], rows[i]);
            if (result->ID() != GUIDIr77OperationSucceeded) return result;
        }

        // where each goes -- free rows first, then the end of the table
        std::vector<std::uint32_t> places(rows.size(), 0);

        for (std::size_t i = 0; i < rows.size(); i++) {
            if (!m_free_rows.empty()) {
                places[i] = m_free_rows.back();
                m_free_rows.pop_back();
            } else {
                places[i] = static_cast<std::uint32_t>(m_row_used.size());
                m_row_used.push_back(0);
            }
        }

        VkBuffer table{VK_NULL_HANDLE};
        m_material_table->GetBuffer(0, &table);

        std::vector<Ir77PVUploadEntry> entries(rows.size());

        for (std::size_t i = 0; i < rows.size(); i++) {
            entries[i].dst = table;
            entries[i].data = &rows[i];
            entries[i].size = sizeof(Ir77PVMaterialRow);
            entries[i].dst_offset = static_cast<VkDeviceSize>(places[i]) * sizeof(Ir77PVMaterialRow);
        }

        if (m_transfer->Upload(entries)->ID() != GUIDIr77OperationSucceeded) {
            for (std::uint32_t const row : places) m_free_rows.push_back(row);  // nothing reads them -- free again at once
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVMaterial: material upload failed.");
        }

        for (std::uint32_t const row : places) m_row_used[row] = 1;

        material_ids = places;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Upload Materials.");
    }

    std::shared_ptr<IIr77Return const> GetMaterial(std::uint32_t const& material_id, Ir77PVMaterialData& material) {
        if (material_id >= m_row_used.size() || !m_row_used[material_id]) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVMaterial: unknown material.");

        material = Ir77PVMaterialData{};
        material.row = material_id;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The rows are gone now (GetMaterial fails) and back on the free list once no frame in flight can read them -- through Transfer's
    // graveyard. Unknown or already destroyed ids are skipped. Remove the draws using them first.
    std::shared_ptr<IIr77Return const> DestroyMaterials(std::vector<std::uint32_t> const& material_ids) {
        if (!m_transfer) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVMaterial: transfer not set.");

        std::vector<std::uint32_t> rows{};

        for (std::uint32_t const id : material_ids) {
            if (id >= m_row_used.size() || !m_row_used[id]) continue;

            m_row_used[id] = 0;
            rows.push_back(id);
        }

        if (rows.empty()) return Ir77RETURN<Ir77OperationSucceeded>();

        // Materials may be gone by the time the grave expires (teardown drops graves without running them, but stay safe)
        std::weak_ptr<Ir77PVMaterial> const self = weak_from_this();

        return m_transfer->Bury({.released = [self, rows]() {
            if (auto const materials = self.lock()) materials->m_free_rows.insert(materials->m_free_rows.end(), rows.begin(), rows.end());
        }});
    }

   private:
    // One material-table row: the constants plus a texture slot per map -- the texture's when given, a placeholder's otherwise
    std::shared_ptr<IIr77Return const> DefineMaterialRow(Ir77PVMaterialConstants const& constants, Ir77PVMaterialTextures const& textures,
                                                         Ir77PVMaterialRow& row) {
        auto const white_found = m_placeholder_slots.find(Ir77PVPlaceholderKind::White);
        auto const normal_found = m_placeholder_slots.find(Ir77PVPlaceholderKind::Normal);

        if (white_found == m_placeholder_slots.end() || normal_found == m_placeholder_slots.end())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVMaterial: placeholder slots not set.");

        std::uint32_t const white{white_found->second};
        std::uint32_t const flat_normal{normal_found->second};

        // albedo, normal, ORM, emissive, AO
        std::array<std::uint32_t, 5> const ids{textures.albedo, textures.normal, textures.orm, textures.emissive, textures.ao};
        std::array<std::uint32_t, 5> slots{white, flat_normal, white, white, white};

        for (std::size_t i = 0; i < ids.size(); i++) {
            if (ids[i] == IR77PV_NO_TEXTURE) continue;
            if (GetTextureSlot(ids[i], slots[i])->ID() != GUIDIr77OperationSucceeded)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVMaterial: material references an unknown texture.");
        }

        row = Ir77PVMaterialRow{};
        row.constants = constants;
        row.textures0 = {slots[0], slots[1], slots[2], slots[3]};
        row.textures1 = {slots[4], 0, 0, 0};

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    // wiring -- Ir77PVInit
    std::shared_ptr<IIr77PVDevice> m_device{};

    VmaAllocator m_allocator{VK_NULL_HANDLE};

    std::shared_ptr<IIr77PVDescriptorSet> m_descriptor_set{};

    std::shared_ptr<IIr77PVTransfer> m_transfer{};

    std::shared_ptr<IIr77PVBuffer> m_material_table{};  // MATERIAL_CAPACITY rows -- UploadMaterials appends

    std::map<Ir77PVPlaceholderKind, std::uint32_t> m_placeholder_slots{};

    // owned
    std::vector<std::uint8_t> m_row_used{};  // [row] -- 1 while a material holds it; the table's used end is its size

    std::vector<std::uint32_t> m_free_rows{};  // destroyed rows no frame in flight can read any more

    std::vector<std::shared_ptr<IIr77PVTexture>> m_textures{};  // [texture_id] -- empty once destroyed

    std::vector<std::uint32_t> m_texture_slots{};  // [texture_id] -> bindless texture slot

    std::vector<std::uint32_t> m_free_textures{};  // destroyed texture ids

    std::vector<std::shared_ptr<IIr77PVTexture>> m_canvases{};  // CreateCanvas -- for the device's lifetime
};
}  // namespace NSIr77PeregrineV
