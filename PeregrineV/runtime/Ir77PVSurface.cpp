#include "Ir77PVSurface.hpp"

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include "../dictionary/IDIIr77PeregrineV.hpp"

#include "../service/IIr77PeregrineV.hpp"

#include "../interface/IIr77PVInstance.hpp"
#include "../interface/IIr77PVDevice.hpp"
#include "../interface/IIr77PVQueue.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {
SDL_Window* Ir77PVSurface::GetWindow() {
    std::shared_ptr<IIr77PeregrineV> peregrine = QueryAs<IIr77PeregrineV>(&GUIDIIr77PeregrineV, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    peregrine->Context(ID_INSTANCE, enlisted);

    SDL_Window* sdl_window;
    peregrine->GetWindow(&sdl_window);

    return sdl_window;
}

VkInstance Ir77PVSurface::GetInstance() {
    std::shared_ptr<IIr77PeregrineV> peregrine = QueryAs<IIr77PeregrineV>(&GUIDIIr77PeregrineV, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    peregrine->Context(ID_INSTANCE, enlisted);

    std::shared_ptr<IIr77PVInstance> instance = QueryAs<IIr77PVInstance>(&GUIDIIr77PVInstance, m_context.get());

    VkInstance vk_instance;
    instance->GetVkInstance(&vk_instance);

    return vk_instance;
}

VkPhysicalDevice Ir77PVSurface::GetPhysicalDevice() {
    std::shared_ptr<IIr77PeregrineV> peregrine = QueryAs<IIr77PeregrineV>(&GUIDIIr77PeregrineV, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    peregrine->Context(ID_INSTANCE, enlisted);

    std::shared_ptr<IIr77PVDevice> device = QueryAs<IIr77PVDevice>(&GUIDIIr77PVDevice, enlisted.get());

    VkPhysicalDevice vk_phys_device;
    device->GetVkPhysicalDevice(&vk_phys_device);

    return vk_phys_device;
}

VkDevice Ir77PVSurface::GetDevice() {
    std::shared_ptr<IIr77PeregrineV> peregrine = QueryAs<IIr77PeregrineV>(&GUIDIIr77PeregrineV, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    peregrine->Context(ID_INSTANCE, enlisted);

    std::shared_ptr<IIr77PVDevice> device = QueryAs<IIr77PVDevice>(&GUIDIIr77PVDevice, enlisted.get());

    VkDevice vk_device;
    device->GetVkDevice(&vk_device);

    return vk_device;
}

std::uint32_t Ir77PVSurface::GetGraphicsFamily() {
    std::shared_ptr<IIr77PeregrineV> peregrine = QueryAs<IIr77PeregrineV>(&GUIDIIr77PeregrineV, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    peregrine->Context(ID_INSTANCE, enlisted);

    std::shared_ptr<IIr77PVQueue> queue = QueryAs<IIr77PVQueue>(&GUIDIIr77PVQueue, enlisted.get());

    std::uint32_t gfx_family = 0;
    queue->GetGraphicsFamily(gfx_family);

    return gfx_family;
}

std::uint32_t Ir77PVSurface::GetPresentFamily() {
    std::shared_ptr<IIr77PeregrineV> peregrine = QueryAs<IIr77PeregrineV>(&GUIDIIr77PeregrineV, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    peregrine->Context(ID_INSTANCE, enlisted);

    std::shared_ptr<IIr77PVQueue> queue = QueryAs<IIr77PVQueue>(&GUIDIIr77PVQueue, enlisted.get());

    std::uint32_t present_family = 0;
    queue->GetPresentFamily(present_family);

    return present_family;
}

}  // namespace NSIr77PeregrineV