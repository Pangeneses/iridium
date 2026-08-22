#include "Ir77PeregrineV.hpp"
#include <memory>

#include "../dictionary/IDIr77PVContext.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"

#include "../runtime/GPU/Ir77PVInstance.hpp"
#include "../runtime/GPU/Ir77PVDevice.hpp"
#include "../runtime/GPU/Ir77PVQueue.hpp"
#include "../runtime/GPU/Ir77PVSwapchain.hpp"
#include "../runtime/GPU/Ir77PVCmdBuffer.hpp"

#include "../runtime/Pipeline/Ir77PVPipelineGFX.hpp"

#include "../runtime/Pipeline Layout//Ir77PVLayout001.hpp"

#include "../runtime/Render Pass/Ir77PVRPColor.hpp"

#include "../runtime/Assets/Ir77PVShader.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

std::shared_ptr<IIr77Return const> Ir77PeregrineV::CreateDeviceInterface(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
    auto lhs_mutable = std::const_pointer_cast<IIr77Operand>(lhs);

    auto context = QueryAs<IIr77PeregrineV>(&GUIDIIr77PeregrineV, lhs_mutable.get());

    if (!context.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid PeregrineV Operand.");

    std::uint32_t device_count;
    context->CreateInstance(device_count);

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::DestroyDeviceInterface(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::AddWindow(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::DestroyWindow(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::CreatePipeline(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::DestroyInstance(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::UpdateShaders(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::ClearShaders(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::UploadVertexToBuffer(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::ClearVertexBuffer(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::UploadMaterialToBuffer(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::ClearMaterialBuffer(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::UploadComputeToBuffer(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::ClearComputeBuffer(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::CreateInstance(std::uint32_t& count) {
    m_instance = std::make_shared<Ir77PVInstance>();

    std::shared_ptr<IIr77PVInstance> instance = std::reinterpret_pointer_cast<IIr77PVInstance>(m_instance);

    instance->InitAppInfo();

    instance->InitExtensions();

    instance->InitCreateInfo();

    instance->InitCreateInstance();

    instance->QueryDeviceCount(count);

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::EnumeratePhysicalDevices() {



    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PeregrineV::GetMemberByID(std::uint64_t const& id, std::uint32_t const& device_index,
                                                                 std::shared_ptr<IIr77Enlisted>& obj) {
    obj = m_context.at(device_index).at(id);

    return Ir77RETURN<Ir77OperationSucceeded>();
}

}  // namespace NSIr77PeregrineV