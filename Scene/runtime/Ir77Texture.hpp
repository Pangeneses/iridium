#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>

#include "../build/_deps/stb-src/stb_image.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <thread>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77Scene.hpp"
#include "../dictionary/IDIr77Scene.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77Texture.hpp"

#include "../PeregrineV/types//Ir77PVTypes.hpp"

using namespace NSIr77RT;
using namespace NSIr77PeregrineV;

namespace NSIr77Scene {

// =============================================================================================================================================
// Ir77Texture -- one entry per glTF texture, same index as asset.textures, so material texture indices map straight through.
// Keeps the decoded RGBA8 (for upload) and the original encoded bytes (for saving back into .ir77 without re-encoding).
// srgb is decided by use: textures referenced as base color or emissive are color data; everything else is linear data.
// =============================================================================================================================================
class Ir77Texture : public Ir77Enlisted, public IIr77Texture, public std::enable_shared_from_this<Ir77Texture> {
   private:
    struct Ir77TextureImage {
        std::uint32_t width{0};
        std::uint32_t height{0};
        bool srgb{false};
        bool valid{false};
        std::vector<std::uint8_t> rgba{};     // decoded, width * height * 4
        std::vector<std::uint8_t> encoded{};  // original PNG / JPEG bytes
    };

   public:
    Ir77Texture() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    ~Ir77Texture() {}

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77Texture>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77Texture>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77Texture)
            obj = std::shared_ptr<IIr77Texture>(shared_from_this(), static_cast<IIr77Texture*>(this));

        else if (iid == GUIDIr77Texture)
            obj = std::shared_ptr<Ir77Texture>(shared_from_this(), static_cast<Ir77Texture*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

   public:
    // Decodes every glTF texture, in parallel -- one worker per core, each taking the next texture until none are left. Every texture is
    // written only by the worker that took it, so nothing is shared; all are done before this returns. A texture that can't be read stays
    // invalid (its material slot falls back to the placeholder).
    std::shared_ptr<IIr77Return const> LoadTexturesFromAsset(fastgltf::Asset const& asset) {
        m_textures.assign(asset.textures.size(), Ir77TextureImage{});

        // color or data -- by how materials use each texture
        std::vector<bool> color(asset.textures.size(), false);
        for (fastgltf::Material const& material : asset.materials) {
            if (material.pbrData.baseColorTexture.has_value()) MarkColor(color, material.pbrData.baseColorTexture->textureIndex);
            if (material.emissiveTexture.has_value()) MarkColor(color, material.emissiveTexture->textureIndex);
        }

        for (std::size_t t = 0; t < m_textures.size(); t++) m_textures[t].srgb = color[t];

        std::atomic<std::size_t> next{0};

        std::size_t const workers = std::min<std::size_t>(std::max(std::thread::hardware_concurrency(), 1u), m_textures.size());

        {
            std::vector<std::jthread> pool{};
            pool.reserve(workers);

            for (std::size_t w = 0; w < workers; w++)
                pool.emplace_back([this, &asset, &next]() {
                    for (std::size_t t = next++; t < m_textures.size(); t = next++) DefineTexture(asset, t);
                });
        }  // jthreads join here

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetTextureCount(std::uint32_t& count) {
        count = static_cast<std::uint32_t>(m_textures.size());

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Decoded RGBA8 for upload. Fails for textures that didn't decode -- the caller leaves that slot as a placeholder.
    std::shared_ptr<IIr77Return const> GetTexture(std::uint32_t const& index, std::uint32_t& width, std::uint32_t& height, bool& srgb,
                                                  std::vector<std::uint8_t>& rgba) {
        if (index >= m_textures.size() || !m_textures[index].valid) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Texture: texture not available.");

        Ir77TextureImage const& texture = m_textures[index];
        width = texture.width;
        height = texture.height;
        srgb = texture.srgb;
        rgba = texture.rgba;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    // One texture: its encoded bytes out of the asset, then stb_image to RGBA8. Runs on a decode worker -- touches only m_textures[t].
    void DefineTexture(fastgltf::Asset const& asset, std::size_t const t) {
        fastgltf::Texture const& texture = asset.textures[t];
        if (!texture.imageIndex.has_value()) return;  // only a WebP / KTX2 extension source -- not supported yet

        Ir77TextureImage& out = m_textures[t];

        if (!ImageBytes(asset, asset.images.at(texture.imageIndex.value()), out.encoded)) {
            std::printf("[Ir77Texture] texture %zu: image bytes not available\n", t);
            return;
        }

        int w{0}, h{0}, channels{0};
        stbi_uc* pixels = stbi_load_from_memory(out.encoded.data(), static_cast<int>(out.encoded.size()), &w, &h, &channels, 4);
        if (!pixels) {
            std::printf("[Ir77Texture] texture %zu: decode failed (%s)\n", t, stbi_failure_reason());
            return;
        }

        out.width = static_cast<std::uint32_t>(w);
        out.height = static_cast<std::uint32_t>(h);
        out.rgba.assign(pixels, pixels + static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4);
        out.valid = true;

        stbi_image_free(pixels);
    }

    static void MarkColor(std::vector<bool>& color, std::size_t const& index) {
        if (index < color.size()) color[index] = true;
    }

    // Encoded image bytes, wherever fastgltf left them: inside the GLB (buffer view), or loaded into memory (LoadExternalImages)
    static bool ImageBytes(fastgltf::Asset const& asset, fastgltf::Image const& image, std::vector<std::uint8_t>& bytes) {
        bytes.clear();

        auto const from_buffer = [&](fastgltf::DataSource const& source, std::size_t offset, std::size_t length) {
            std::visit(fastgltf::visitor{
                           [&](fastgltf::sources::Array const& array) {
                               auto const* data = reinterpret_cast<std::uint8_t const*>(array.bytes.data());
                               bytes.assign(data + offset, data + offset + length);
                           },
                           [&](fastgltf::sources::Vector const& vector) {
                               auto const* data = reinterpret_cast<std::uint8_t const*>(vector.bytes.data());
                               bytes.assign(data + offset, data + offset + length);
                           },
                           [](auto const&) {},
                       },
                       source);
        };

        std::visit(fastgltf::visitor{
                       [&](fastgltf::sources::BufferView const& view) {
                           fastgltf::BufferView const& buffer_view = asset.bufferViews.at(view.bufferViewIndex);
                           fastgltf::Buffer const& buffer = asset.buffers.at(buffer_view.bufferIndex);
                           from_buffer(buffer.data, buffer_view.byteOffset, buffer_view.byteLength);
                       },
                       [&](fastgltf::sources::Array const& array) {
                           auto const* data = reinterpret_cast<std::uint8_t const*>(array.bytes.data());
                           bytes.assign(data, data + array.bytes.size());
                       },
                       [&](fastgltf::sources::Vector const& vector) {
                           auto const* data = reinterpret_cast<std::uint8_t const*>(vector.bytes.data());
                           bytes.assign(data, data + vector.bytes.size());
                       },
                       [](auto const&) {},  // URI not loaded -- LoadExternalImages should have handled it
                   },
                   image.data);

        return !bytes.empty();
    }

   private:
    std::vector<Ir77TextureImage> m_textures{};
};
}  // namespace NSIr77Scene