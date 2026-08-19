#pragma once

#include <cstdint>

namespace NSIr77PeregrineV {

// Foundation layer — created once, never per-frame.
// -----------------------------------------------------------------------------------------------------------------------------------------
static const std::uint64_t ID_INSTANCE = 0x3847291056173849;

static const std::uint64_t ID_DEVICE = 0x7192836475920384;

static const std::uint64_t ID_QUEUES = 0xdb18f07cf9c54504;

static const std::uint64_t ID_QUEUE_GRAPHICS = 0x1053847291736482;

static const std::uint64_t ID_QUEUE_COMPUTE = 0x5628374019283746;

static const std::uint64_t ID_QUEUE_TRANSFER = 0xb15cdfd845264664;

static const std::uint64_t ID_QUEUE_SPARSE = 0xc9cb2de5ccd64dba;

static const std::uint64_t ID_QUEUE_PROTECTED = 0x3c865b36576b46da;

static const std::uint64_t ID_QUEUE_ENCODE = 0xceb630adbe7f4d74;

static const std::uint64_t ID_QUEUE_DECODE = 0xaec751afba8f4d74;

static const std::uint64_t ID_SWAPCHAIN = 0x2947183056471829;

// Memory layer — peers, created once.
// -----------------------------------------------------------------------------------------------------------------------------------------
static const std::uint64_t ID_ALLOCATION = 0x4729183056471829;

// Resource layer — peers, created once.
// -----------------------------------------------------------------------------------------------------------------------------------------
static const std::uint64_t ID_BUFFER = 0x9183746291047582;

static const std::uint64_t ID_BUFFER_STAGING = 0x1374629104758291;

static const std::uint64_t ID_BUFFER_VERTEX = 0x7482910365847291;

static const std::uint64_t ID_BUFFER_INDEX = 0x3056471829304729;

static const std::uint64_t ID_BUFFER_UNIFORM = 0x6291047582916374;

static const std::uint64_t ID_IMAGE = 0x2910473849165728;

static const std::uint64_t ID_IMAGE_CEF = 0x8473920156384729;

static const std::uint64_t ID_IMAGE_RENDER = 0x4920183756294810;

static const std::uint64_t ID_IMAGE_DEPTH = 0x1638472910583746;

static const std::uint64_t ID_IMAGEVIEW = 0x5291048374619283;

static const std::uint64_t ID_IMAGEVIEW_CEF = 0x7384920165473829;

static const std::uint64_t ID_IMAGEVIEW_RENDER = 0x3749201856382910;

static const std::uint64_t ID_IMAGEVIEW_DEPTH = 0x9201837465291038;

static const std::uint64_t ID_SAMPLER = 0x4638291057483920;

// Descriptor layer — peers, created once.
// -----------------------------------------------------------------------------------------------------------------------------------------
static const std::uint64_t ID_DESCRIPTOR_LAYOUT = 0x2839104756382910;

static const std::uint64_t ID_DESCRIPTOR_SET = 0x6473920158374629;

// Shader and pipeline layer — peers, created once.
// -----------------------------------------------------------------------------------------------------------------------------------------
static const std::uint64_t ID_SHADER_VERT = 0x1829304756182930;

static const std::uint64_t ID_SHADER_FRAG = 0x5374629108473920;

static const std::uint64_t ID_SHADER_COMPUTE = 0x9047382916583742;

static const std::uint64_t ID_PIPELINE_GFX = 0x3820164758392016;

static const std::uint64_t ID_PIPELINE_COMPUTE = 0x7164839205748392;

static const std::uint64_t ID_MATERIAL = 0x2058374916283047;

// Render graph layer — peers, created once.
// -----------------------------------------------------------------------------------------------------------------------------------------
static const std::uint64_t ID_RENDER_GRAPH = 0x8392016475839201;

static const std::uint64_t ID_RENDER_STAGE_3D = 0x4758392016475839;

static const std::uint64_t ID_RENDER_STAGE_CEF = 0x6047583920164758;

static const std::uint64_t ID_RENDER_STAGE_COMPOSITE = 0x1839204756839201;

static const std::uint64_t ID_RENDER_PASS_3D = 0x5920167483920164;

static const std::uint64_t ID_RENDER_PASS_CEF = 0x3164758392016475;

static const std::uint64_t ID_RENDER_PASS_COMPOSITE = 0x7839201647583920;

static const std::uint64_t ID_RENDER_TARGET_COLOR = 0x2647583920164758;

static const std::uint64_t ID_RENDER_TARGET_CEF = 0x9016475839201647;

static const std::uint64_t ID_DEPTH_TARGET = 0x4758302916475839;

// Geometry layer — peers, created once per asset.
// -----------------------------------------------------------------------------------------------------------------------------------------
static const std::uint64_t ID_MESH = 0x1647583920164758;

// Per-frame layer — one instance per back buffer, indexed by swapchain image.
// -----------------------------------------------------------------------------------------------------------------------------------------
static const std::uint64_t ID_FRAME_0 = 0x8302916475839201;

static const std::uint64_t ID_FRAME_1 = 0x5839201647583920;

static const std::uint64_t ID_FRAMEBUFFER_0 = 0x3920164758392016;

static const std::uint64_t ID_FRAMEBUFFER_1 = 0x7583920164758392;

// Inline / structural.
// -----------------------------------------------------------------------------------------------------------------------------------------
static const std::uint64_t ID_BARRIER = 0x2164758392016475;

static const std::uint64_t ID_COMMAND_BUFFER_0 = 0x6475839201647583;

static const std::uint64_t ID_COMMAND_BUFFER_1 = 0x4839201647583920;

static const std::uint64_t ID_SEMAPHORE_IMAGE_AVAIL_0 = 0x9201647583920164;

static const std::uint64_t ID_SEMAPHORE_IMAGE_AVAIL_1 = 0x1647583920164758;

static const std::uint64_t ID_SEMAPHORE_RENDER_DONE_0 = 0x5839201674583920;

static const std::uint64_t ID_SEMAPHORE_RENDER_DONE_1 = 0x3920164758302916;

static const std::uint64_t ID_FENCE_0 = 0x7583920146758392;

static const std::uint64_t ID_FENCE_1 = 0x2016475839201647;

}  // namespace NSIr77PeregrineV