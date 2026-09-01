#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <map>
#include <memory>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

struct Ir77PeregrineV;
struct IIr77PVDevice;

typedef struct IIr77PVAsset : virtual public IIr77Enlisted {
    IIr77PVAsset() = default;

    virtual std::shared_ptr<IIr77Return const> SetPeregrineV(std::shared_ptr<Ir77PeregrineV>& context) = 0;

    virtual std::shared_ptr<IIr77Return const> CreateBuffersVertex(const void* vertex_data, VkDeviceSize const& vertex_size_bytes, const void* index_data,
                                                           VkDeviceSize indices_size_bytes, std::uint32_t index_count, VkIndexType index_type) = 0;

    virtual std::shared_ptr<IIr77Return const> CreateBuffersUBO() = 0;

    virtual std::shared_ptr<IIr77Return const> CreateShaders() = 0;

    virtual ~IIr77PVAsset() = default;
}* pIIr77PVAsset;

}  // namespace NSIr77PeregrineV
