#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <fastgltf/core.hpp>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77Scene {

typedef struct IIr77Texture : virtual public IIr77Enlisted {
    IIr77Texture() = default;

    virtual std::shared_ptr<IIr77Return const> LoadTexturesFromAsset(fastgltf::Asset const& textures) = 0;

    virtual std::shared_ptr<IIr77Return const> GetTextureCount(std::uint32_t& count) = 0;
    
    virtual std::shared_ptr<IIr77Return const> GetTexture(std::uint32_t const& index, std::uint32_t& width, std::uint32_t& height, bool& srgb,
                                                          std::vector<std::uint8_t>& rgba) = 0;

    virtual ~IIr77Texture() = default;
}* pIIr77Texture;
}  // namespace NSIr77Scene
