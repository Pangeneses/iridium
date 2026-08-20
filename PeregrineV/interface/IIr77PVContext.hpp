#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <map>
#include <memory>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

struct Ir77Window {
    SDL_Window* window{nullptr};
};

struct IIr77PVPregrineV;
struct IIr77PVQueue;

// Device-level (one per physical GPU):
typedef struct IIr77PVContext : virtual public IIr77Enlisted {
    IIr77PVContext() = default;

    virtual std::shared_ptr<IIr77Return const> BuildContext() = 0;

    virtual std::shared_ptr<IIr77Return const> InitializePipeline() = 0;

    virtual std::shared_ptr<IIr77Return const> CurrentDevice(std::uint32_t& index) = 0;

    virtual std::shared_ptr<IIr77Return const> GetWindow(Ir77Window& window) = 0;

    virtual std::shared_ptr<IIr77Return const> GetContext(std::map<std::uint64_t const, std::shared_ptr<IIr77Enlisted>>& context) = 0;

    virtual std::shared_ptr<IIr77Return const> GetMemberByID(std::uint64_t const& id, std::shared_ptr<IIr77Enlisted>& context) = 0;

    virtual ~IIr77PVContext() = default;
}* PIr77PVContext;

}  // namespace NSIr77PeregrineV