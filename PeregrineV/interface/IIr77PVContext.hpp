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

struct IIr77PVPregrineV;
struct IIr77PVQueue;

// Device-level (one per physical GPU):
typedef struct IIr77PVContext : virtual public IIr77Enlisted {
    IIr77PVContext() = default;

    virtual std::shared_ptr<IIr77Return const> BuildContext() = 0;

    virtual std::shared_ptr<IIr77Return const> GetContext(std::map<std::uint64_t const, std::shared_ptr<IIr77Enlisted>>& context) = 0;

    virtual std::shared_ptr<IIr77Return const> GetMemberByID(std::uint64_t const& id, std::shared_ptr<IIr77Enlisted>& context) = 0;

    virtual ~IIr77PVContext() = default;
}* PIr77PVContext;

static constexpr std::uint64_t ID_INSTANCE = 0x75AA1E2D97FADA35ULL;
static constexpr std::uint64_t ID_DEVICE = 0x6E0765CFE8524B33ULL;
static constexpr std::uint64_t ID_QUEUE_GFX = 0x0CCE3DACD822AF87ULL;
static constexpr std::uint64_t ID_QUEUE_COMPUTE = 0xE48AEE23E468E6E0ULL;
static constexpr std::uint64_t ID_SURFACE = 0x94A4B2E16BCEB6D2ULL;
static constexpr std::uint64_t ID_MEMORY = 0x64BE028864A11FE8ULL;
static constexpr std::uint64_t ID_MEMORY_STRATEGY = 0xB47298371B8FB91EULL;
static constexpr std::uint64_t ID_ALLOCATION = 0x9910B4FE58662B3BULL;
static constexpr std::uint64_t ID_BUFFER = 0x224A8F5BFCD4B8A0ULL;
static constexpr std::uint64_t ID_BUFFER_STAGING = 0x368D42C5F3BC7927ULL;
static constexpr std::uint64_t ID_BUFFER_VERTEX = 0xB6B4906B4C187801ULL;
static constexpr std::uint64_t ID_BUFFER_INDEX = 0xBEF94C5CCB551E3AULL;
static constexpr std::uint64_t ID_BUFFER_UNIFORM = 0x2B8D641ECC5CA5E7ULL;
static constexpr std::uint64_t ID_IMAGE = 0x5F5DF4FA1662F55EULL;
static constexpr std::uint64_t ID_IMAGE_CEF = 0x717AB19682BF0075ULL;
static constexpr std::uint64_t ID_IMAGE_RENDER = 0xBF2B8F6B1CB11B2FULL;
static constexpr std::uint64_t ID_IMAGE_DEPTH = 0xFEB20C04A733AAB0ULL;
static constexpr std::uint64_t ID_IMAGEVIEW = 0x72CA667E0A172483ULL;
static constexpr std::uint64_t ID_IMAGEVIEW_CEF = 0xCBD348B17CC3B999ULL;
static constexpr std::uint64_t ID_IMAGEVIEW_RENDER = 0xDC9ADC4F1A4299B6ULL;
static constexpr std::uint64_t ID_IMAGEVIEW_DEPTH = 0xA3134CA16FFBE7DAULL;
static constexpr std::uint64_t ID_SAMPLER = 0x149BF33DACBC9E76ULL;
static constexpr std::uint64_t ID_DESCRIPTOR_LAYOUT = 0x5B0FC4833A521BB1ULL;
static constexpr std::uint64_t ID_DESCRIPTOR_SET = 0x030D8480127F1154ULL;
static constexpr std::uint64_t ID_SHADER_VERT = 0x6AC6B194F91AB7D2ULL;
static constexpr std::uint64_t ID_SHADER_FRAG = 0x1BB0E0E9D73625C4ULL;
static constexpr std::uint64_t ID_SHADER_COMPUTE = 0x24DA2130EE39E356ULL;
static constexpr std::uint64_t ID_PIPELINE_GFX = 0x39287694D2A68D26ULL;
static constexpr std::uint64_t ID_PIPELINE_COMPUTE = 0x4C8E162FA38F5B12ULL;
static constexpr std::uint64_t ID_MATERIAL = 0x0385C96A2843C0CEULL;
static constexpr std::uint64_t ID_RENDER_GRAPH = 0xDE9ED1C52D55888AULL;
static constexpr std::uint64_t ID_RENDER_STAGE_3D = 0x14E630414EFB64A4ULL;
static constexpr std::uint64_t ID_RENDER_STAGE_2D = 0xA2C014F6D93231E0ULL;
static constexpr std::uint64_t ID_RENDER_PASS = 0x658515676F8CFCDDULL;
static constexpr std::uint64_t ID_FRAMEBUFFER = 0x03C3ADA0E02AEC91ULL;
static constexpr std::uint64_t ID_FRAME = 0x29634218BAB8E521ULL;
static constexpr std::uint64_t ID_SEMAPHORE = 0xA66539F604059046ULL;
static constexpr std::uint64_t ID_BARRIER = 0x706869F2BC1F779FULL;
static constexpr std::uint64_t ID_MESH = 0x9C655631B5AF58BAULL;
static constexpr std::uint64_t ID_SHUTDOWN = 0x32B42D1559E49A23ULL;

}  // namespace NSIr77PeregrineV