#pragma once

#include <cstdint>

namespace NSIr77PeregrineV {
// GPU INTERFACE
// -----------------------------------------------------------------------------------------------------------------------------------------
static const std::uint64_t ID_INSTANCE = 0x3847291056173849;

static const std::uint64_t ID_DEVICE = 0x7192836475920384;

static const std::uint64_t ID_QUEUES = 0xdb18f07cf9c54504;

static const std::uint64_t ID_SWAPCHAIN_001 = 0x2947183056471829;

static const std::uint64_t ID_SWAPCHAIN_002 = 0x7394422840574737;

static const std::uint64_t ID_SWAPCHAIN_003 = 0x2947183056471829;

static const std::uint64_t ID_SWAPCHAIN_004 = 0x2947183056471829;

static const std::uint64_t ID_COMMAND_BUFFER = 0x6475839201647583;

static const std::uint64_t ID_BARRIER = 0x2164758392016475;

static const std::uint64_t ID_SEMAPHORE = 0x9201647583920164;

// PIPELINES
// -----------------------------------------------------------------------------------------------------------------------------------------
static const std::uint64_t ID_PIPELINE_GFX = 0x4729183056471829;

static const std::uint64_t ID_PIPELINE_COMPUTE = 0x7164839205748392;

// PIPELINE LAYOUTS
// -----------------------------------------------------------------------------------------------------------------------------------------
static const std::uint64_t ID_LAYOUT_001 = 0x2839104756382910; // standard graphics pass

// RENDER PASS
// -----------------------------------------------------------------------------------------------------------------------------------------
static const std::uint64_t ID_RENDER_PASS_COLOR = 0x5920167483920164;

static const std::uint64_t ID_RENDER_PASS_CEF = 0x3164758392016475;

// BUFFER
// -----------------------------------------------------------------------------------------------------------------------------------------
static const std::uint64_t ID_BUFFER = 0x9183746291047582;

static const std::uint64_t ID_BUFFER_STAGING = 0x1374629104758291;

static const std::uint64_t ID_BUFFER_VERTEX = 0x7482910365847291;

static const std::uint64_t ID_BUFFER_INDEX = 0x3056471829304729;

static const std::uint64_t ID_BUFFER_UNIFORM = 0x6291047582916374;

// COMPUTE
// -----------------------------------------------------------------------------------------------------------------------------------------

// ASSETS
// -----------------------------------------------------------------------------------------------------------------------------------------
static const std::uint64_t ID_SHADER = 0x3001943705423802;

static const std::uint64_t ID_SHADER_VERT = 0x1829304756182930;

static const std::uint64_t ID_SHADER_FRAG = 0x5374629108473920;

static const std::uint64_t ID_SHADER_COMPUTE = 0x9047382916583742;

static const std::uint64_t ID_MATERIAL = 0x2058374916283047;

static const std::uint64_t ID_MESH = 0x1647583920164758;
}  // namespace NSIr77PeregrineV