#include "Ir77PVContext.hpp"

#include "../dictionary/IDIr77PVContext.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"


#include "../runtime/Ir77PVInstance.hpp"
#include "../runtime/Ir77PVDevice.hpp"
#include "../runtime/Ir77PVQueue.hpp"
#include "../runtime/Ir77PVSurface.hpp"
#include "../runtime/Ir77PVAllocation.hpp"

/*
#include "../runtime/Ir77PVBarrier.hpp"
#include "../runtime/Ir77PVBuffer.hpp"
#include "../runtime/Ir77PVCommandBuffer.hpp"
#include "../runtime/Ir77PVDepthTarget.hpp"
#include "../runtime/Ir77PVDescriptorLayout.hpp"
#include "../runtime/Ir77PVDescriptorSet.hpp"
#include "../runtime/Ir77PVFrame.hpp"
#include "../runtime/Ir77PVFramebuffer.hpp"
#include "../runtime/Ir77PVImage.hpp"
#include "../runtime/Ir77PVImageView.hpp"
#include "../runtime/Ir77PVMaterial.hpp"
#include "../runtime/Ir77PVMesh.hpp"
#include "../runtime/Ir77PVPipeline.hpp"
#include "../runtime/Ir77PVRenderGraph.hpp"
#include "../runtime/Ir77PVRenderPass.hpp"
#include "../runtime/Ir77PVRenderStage.hpp"
#include "../runtime/Ir77PVRenderTarget.hpp"
#include "../runtime/Ir77PVSampler.hpp"
#include "../runtime/Ir77PVSemaphore.hpp"
#include "../runtime/Ir77PVShader.hpp"
#include "../runtime/Ir77PVShutdown.hpp"
*/

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

std::shared_ptr<IIr77Return const> Ir77PVContext::BuildContext() {
    m_context.clear();
    m_context.emplace(ID_INSTANCE, std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVInstance>()));
    m_context.emplace(ID_DEVICE, std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVDevice>()));
    m_context.emplace(ID_QUEUES, std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVQueue>()));
    m_context.emplace(ID_SURFACE, std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVSurface>()));
    m_context.emplace(ID_ALLOCATION, std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVAllocation>()));

    // Uncomment as implementations are ready:
    // m_context.emplace(ID_BUFFER,                    std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVBuffer>()));
    // m_context.emplace(ID_BUFFER_STAGING,            std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVBuffer>()));
    // m_context.emplace(ID_BUFFER_VERTEX,             std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVBuffer>()));
    // m_context.emplace(ID_BUFFER_INDEX,              std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVBuffer>()));
    // m_context.emplace(ID_BUFFER_UNIFORM,            std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVBuffer>()));
    // m_context.emplace(ID_IMAGE,                     std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVImage>()));
    // m_context.emplace(ID_IMAGE_CEF,                 std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVImage>()));
    // m_context.emplace(ID_IMAGE_RENDER,              std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVImage>()));
    // m_context.emplace(ID_IMAGE_DEPTH,               std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVImage>()));
    // m_context.emplace(ID_IMAGEVIEW,                 std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVImageView>()));
    // m_context.emplace(ID_IMAGEVIEW_CEF,             std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVImageView>()));
    // m_context.emplace(ID_IMAGEVIEW_RENDER,          std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVImageView>()));
    // m_context.emplace(ID_IMAGEVIEW_DEPTH,           std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVImageView>()));
    // m_context.emplace(ID_SAMPLER,                   std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVSampler>()));
    // m_context.emplace(ID_DESCRIPTOR_LAYOUT,         std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVDescriptorLayout>()));
    // m_context.emplace(ID_DESCRIPTOR_SET,            std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVDescriptorSet>()));
    // m_context.emplace(ID_SHADER_VERT,               std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVShader>()));
    // m_context.emplace(ID_SHADER_FRAG,               std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVShader>()));
    // m_context.emplace(ID_SHADER_COMPUTE,            std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVShader>()));
    // m_context.emplace(ID_PIPELINE_GFX,              std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVPipeline>()));
    // m_context.emplace(ID_PIPELINE_COMPUTE,          std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVPipeline>()));
    // m_context.emplace(ID_MATERIAL,                  std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVMaterial>()));
    // m_context.emplace(ID_RENDER_GRAPH,              std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVRenderGraph>()));
    // m_context.emplace(ID_RENDER_STAGE_3D,           std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVRenderStage>()));
    // m_context.emplace(ID_RENDER_STAGE_CEF,          std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVRenderStage>()));
    // m_context.emplace(ID_RENDER_STAGE_COMPOSITE,    std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVRenderStage>()));
    // m_context.emplace(ID_RENDER_PASS_3D,            std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVRenderPass>()));
    // m_context.emplace(ID_RENDER_PASS_CEF,           std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVRenderPass>()));
    // m_context.emplace(ID_RENDER_PASS_COMPOSITE,     std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVRenderPass>()));
    // m_context.emplace(ID_RENDER_TARGET_COLOR,       std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVRenderTarget>()));
    // m_context.emplace(ID_RENDER_TARGET_CEF,         std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVRenderTarget>()));
    // m_context.emplace(ID_DEPTH_TARGET,              std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVDepthTarget>()));
    // m_context.emplace(ID_MESH,                      std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVMesh>()));
    // m_context.emplace(ID_FRAME_0,                   std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVFrame>()));
    // m_context.emplace(ID_FRAME_1,                   std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVFrame>()));
    // m_context.emplace(ID_FRAMEBUFFER_0,             std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVFrameBuffer>()));
    // m_context.emplace(ID_FRAMEBUFFER_1,             std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVFrameBuffer>()));
    // m_context.emplace(ID_BARRIER,                   std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVCommandBuffer>()));
    // m_context.emplace(ID_COMMAND_BUFFER_0,          std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVCommandBuffer>()));
    // m_context.emplace(ID_COMMAND_BUFFER_1,          std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVSemaphore>()));
    // m_context.emplace(ID_SEMAPHORE_IMAGE_AVAIL_0,   std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVSemaphore>()));
    // m_context.emplace(ID_SEMAPHORE_IMAGE_AVAIL_1,   std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVSemaphore>()));
    // m_context.emplace(ID_SEMAPHORE_RENDER_DONE_0,   std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVSemaphore>()));
    // m_context.emplace(ID_SEMAPHORE_RENDER_DONE_1,   std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVSemaphore>()));
    // m_context.emplace(ID_FENCE_0,                   std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVFence>()));
    // m_context.emplace(ID_FENCE_1,                   std::static_pointer_cast<IIr77Enlisted>(std::make_shared<Ir77PVFence>()));

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PVContext::GetContext(std::map<std::uint64_t const, std::shared_ptr<IIr77Enlisted>>& context) {
    context = m_context;

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PVContext::GetMemberByID(std::uint64_t const& id, std::shared_ptr<IIr77Enlisted> member) {
    member = m_context.at(id);

    return Ir77RETURN<Ir77OperationSucceeded>();
}

}  // namespace NSIr77PeregrineV