#include "Ir77PVShader.hpp"

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include "../../dictionary/IDIIr77PeregrineV.hpp"
#include "../../dictionary/IDIr77PVContext.hpp"

#include "../../interface/IIr77PeregrineV.hpp"

#include "../../interface/IIr77PVDevice.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {
std::uint32_t Ir77PVShader::CurrentDevice() {
    std::shared_ptr<IIr77PeregrineV> context = QueryAs<IIr77PeregrineV>(&GUIDIIr77PeregrineV, m_context.get());

    std::uint32_t index;
    context->CurrentDevice(index);

    return index;
}

VkDevice Ir77PVShader::GetDevice() {
    std::shared_ptr<IIr77PeregrineV> context = QueryAs<IIr77PeregrineV>(&GUIDIIr77PeregrineV, m_context.get());

    std::shared_ptr<IIr77Enlisted> enlisted;
    context->GetMemberByID(ID_DEVICE, enlisted);

    std::shared_ptr<IIr77PVDevice> device = QueryAs<IIr77PVDevice>(&GUIDIIr77PVDevice, enlisted.get());

    VkDevice vk_device;
    device->GetVkDevice(&vk_device);

    return vk_device;
}
}  // namespace NSIr77PeregrineV