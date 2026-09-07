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

typedef struct IIr77Mesh : virtual public IIr77Enlisted {
    IIr77Mesh() = default;

    virtual std::shared_ptr<IIr77Return const> AddMeshInstance(std::uint32_t const& mesh_instance, glm::mat4 const& world_transform) = 0;

    virtual std::shared_ptr<IIr77Return const> AddMesh(fastgltf::Asset const& mesh, std::uint32_t const& index) = 0;

    virtual ~IIr77Mesh() = default;
}* pIIr77Mesh;
}  // namespace NSIr77Scene
