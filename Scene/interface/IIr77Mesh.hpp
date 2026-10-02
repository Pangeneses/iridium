#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <fastgltf/core.hpp>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../PeregrineV/server/Ir77PVTypes.hpp"

using namespace NSIr77RT;
using namespace NSIr77PeregrineV;

namespace NSIr77Scene {

typedef struct IIr77Mesh : virtual public IIr77Enlisted {
    IIr77Mesh() = default;

    virtual std::shared_ptr<IIr77Return const> AddMeshInstance(std::uint32_t const& mesh_instance, glm::mat4 const& world_transform) = 0;

    virtual std::shared_ptr<IIr77Return const> AddMesh(fastgltf::Asset const& mesh, std::uint32_t const& index) = 0;

    virtual std::shared_ptr<IIr77Return const> PopulateMeshRanges() = 0;

    virtual std::shared_ptr<IIr77Return const> GetVertexBuffer(std::vector<Ir77PVVertex>& vertex_buffer) = 0;

    virtual std::shared_ptr<IIr77Return const> GetIndexBuffer(std::vector<uint32_t>& index_buffer) = 0;

    virtual std::shared_ptr<IIr77Return const> GetInstanceBuffer(std::vector<glm::mat4>& instance_buffer) = 0;

    virtual std::shared_ptr<IIr77Return const> GetMeshRanges(std::vector<Ir77MeshRange>& mesh_ranges) = 0;

    virtual ~IIr77Mesh() = default;
}* pIIr77Mesh;
}  // namespace NSIr77Scene
