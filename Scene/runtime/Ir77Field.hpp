#pragma once

#include <glm/glm.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77Scene.hpp"
#include "../dictionary/IDIr77Scene.hpp"

#include "../../Ir77RT/types/Ir77Types.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"
#include "../../Ir77RT/runtime/Ir77Return.hpp"

#include "../interface/IIr77Lighting.hpp"
#include "../interface/IIr77Field.hpp"

#include "../types/Ir77Field.hpp"

// the direct link -- Scene -> PeregrineV's server, one way
#include "../../PeregrineV/server/Ir77Server.hpp"

using namespace NSIr77RT;
using namespace NSIr77PeregrineV;

namespace NSIr77Scene {

// =============================================================================================================================================
// Ir77Field -- one compute-driven visual: a surface or particle cloud whose program (GLSL, include/field.glsl is the contract) is compiled from
// source at runtime. Owned by Ir77Animate; the UI reaches it through the scene server's field.* ops.
//
//   setup:     SetRenderer / SetLighting / SetDefaultMaterial
//   program:   CompileField(settings) -> [SetFieldParams / ClearFieldCanvas / ResetField / ShowField]
//   per frame: AppendDispatches(dispatches, dt) once, then AppendDraws(window, pass, items) into each pass list
//   rays:      on a ray-query device a surface is in the TLAS too -- one surface BLAS over the point buffer and a grid index buffer, rebuilt
//              every frame after the program runs, so the field casts ray-traced shadows (on itself and the scene). Particles stay out.
// =============================================================================================================================================
class Ir77Field : public Ir77Enlisted, public IIr77Field, public std::enable_shared_from_this<Ir77Field> {
   public:
    Ir77Field() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77Field>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77Scene>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77Field)
            obj = std::shared_ptr<IIr77Field>(shared_from_this(), static_cast<IIr77Field*>(this));

        else if (iid == GUIDIr77Field)
            obj = std::shared_ptr<Ir77Field>(shared_from_this(), static_cast<Ir77Field*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

   public:
    // PeregrineV's server -- compiles, pipelines, storage buffers, the canvas, dispatches and draws all go through it
    std::shared_ptr<IIr77Return const> SetRenderer(std::shared_ptr<NSIr77PeregrineV::Ir77Server> const& renderer) {
        m_renderer = renderer;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Where the field reports its box (the sun's shadow covers it) and reads the shadow tile count for its shadow draws
    std::shared_ptr<IIr77Return const> SetLighting(std::shared_ptr<IIr77Lighting> const& lighting) {
        m_lighting = lighting;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The material the field's draw items carry -- the scene's grey default (the field shades itself)
    std::shared_ptr<IIr77Return const> SetDefaultMaterial(std::uint32_t const& material) {
        m_default_material = material;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // field -- one compute-driven visual. The program (GLSL, include/field.glsl is the contract) is compiled twice, into field.comp and
    // field.frag, and swapped in only when both compile; the running field keeps going otherwise. Every frame: Field() per element, then the
    // canvas decay and the pens, then the draws (surface: opaque + shadow casters, particles: additive).
    // -------------------------------------------------------------------------------------------------------------------------------------

    // Ir77FieldSettings (types/Ir77Field.hpp) -- everything field.compile carries

    // errors = the compiler's messages, lines numbered from the program's first line (empty on success)
    std::shared_ptr<IIr77Return const> CompileField(Ir77FieldSettings const& settings, std::string& errors) {
        errors.clear();

        if (!m_renderer) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Field: SetRenderer not called.");
        if (settings.source.empty()) return Ir77RETURN<Ir77InvalidArgument>(this, "Ir77Scene: field program is empty.");

        glm::uvec3 const dims{std::max(settings.dims.x, 1u), std::max(settings.dims.y, 1u), std::max(settings.dims.z, 1u)};
        std::uint64_t const count = static_cast<std::uint64_t>(dims.x) * dims.y * dims.z;

        if (count > FIELD_MAX_POINTS) return Ir77RETURN<Ir77OutOfRange>(this, "Ir77Scene: field has more than 16M elements.");
        if (settings.topology == 0 && (dims.x < 2 || dims.y < 2)) return Ir77RETURN<Ir77InvalidArgument>(this, "Ir77Scene: a surface needs at least 2 x 2.");
        if (settings.pens == 0 || settings.pens > 8) return Ir77RETURN<Ir77InvalidArgument>(this, "Ir77Scene: pens must be 1..8.");
        if (settings.pen_target == 1 && settings.topology != 0)
            return Ir77RETURN<Ir77InvalidArgument>(this, "Ir77Scene: point ink needs a surface -- particles have no grid to ink.");

        // 1. both stages compile before anything running is touched
        bool const has_paint = std::regex_search(settings.source, std::regex{R"(\bvec4\s+Paint\s*\()"});
        bool const has_pen = std::regex_search(settings.source, std::regex{R"(\bvec2\s+Pen\s*\()"});

        std::string const defines = std::string{has_paint ? "#define IR77_HAS_PAINT\n" : ""} + (has_pen ? "#define IR77_HAS_PEN\n" : "");

        std::vector<std::uint32_t> compute_spirv{};
        std::vector<std::uint32_t> fragment_spirv{};
        std::string compute_errors{};
        std::string fragment_errors{};

        bool const compute_ok = m_renderer->CompileShader("field.comp", FieldShaderText("IR77_STAGE_COMPUTE", defines, settings.source, "include/field_compute.glsl"),
                                                          compute_spirv, compute_errors)
                                    ->ID() == GUIDIr77OperationSucceeded;

        // ray-traced shadows on the surface when the device has ray queries (lighting.glsl) -- fragment stage only
        bool ray_query{false};
        m_renderer->GetRayQuery(ray_query);

        std::string const fragment_defines = defines + (ray_query ? "#define IR77_RAY_SHADOWS\n" : "");

        bool const fragment_ok = m_renderer->CompileShader("field.frag", FieldShaderText("IR77_STAGE_FRAGMENT", fragment_defines, settings.source, "include/field_fragment.glsl"),
                                                           fragment_spirv, fragment_errors)
                                     ->ID() == GUIDIr77OperationSucceeded;

        if (!compute_ok || !fragment_ok) {
            errors = compute_errors + (compute_errors.empty() || fragment_errors.empty() ? "" : "\n") + fragment_errors;
            return Ir77RETURN<Ir77OperationFailed>(this, "Ir77Scene: field program failed to compile:\n" + errors);
        }

        // 2. swap them in (waits for the device -- the old pipelines may still be recording)
        std::shared_ptr<IIr77Return const> result{};

        if (Failed(m_renderer->ReloadShader("field.comp", compute_spirv), result)) return result;
        if (Failed(m_renderer->ReloadShader("field.frag", fragment_spirv), result)) return result;

        if (Failed(m_renderer->FindPipeline(std::string{"field.surface"}, m_field_surface), result)) return result;
        if (Failed(m_renderer->FindPipeline(std::string{"field.particles"}, m_field_particles), result)) return result;
        if (Failed(m_renderer->FindPipeline(std::string{"field.shadow"}, m_field_shadow), result)) return result;
        if (Failed(m_renderer->FindPipeline(std::string{"field.compute"}, m_field_compute), result)) return result;

        // 3. GPU state -- the canvas once (first program that pens into it), the point buffer and point ink whenever the element count changes
        bool const to_canvas = has_pen && settings.pen_target == 0;
        bool const to_points = has_pen && settings.pen_target == 1;

        if (to_canvas && !m_field_has_canvas) {
            if (Failed(m_renderer->CreateCanvas(std::max(settings.canvas_size, 16u), m_field_canvas_texture, m_field_canvas_image), result)) return result;

            m_field_canvas_size = std::max(settings.canvas_size, 16u);
            m_field_has_canvas = true;
        }

        // the surface BLAS reads the points and the grid -- out of the rays before either changes
        glm::uvec2 const grid{settings.topology == 0 ? glm::uvec2(dims.x, dims.y) : glm::uvec2(0u)};
        if (count != m_field_count || grid != m_field_ray_grid) DefineRayRelease();

        if (count != m_field_count) {
            if (m_field_has_points) m_renderer->DestroyStorageBuffer(m_field_points_buffer);
            if (m_field_has_ink) m_renderer->DestroyStorageBuffer(m_field_ink_buffer);

            m_field_has_ink = false;

            m_field_has_points = false;
            if (Failed(m_renderer->CreateStorageBuffer(static_cast<VkDeviceSize>(count * sizeof(Ir77FieldPointGPU)), nullptr, m_field_points_buffer,
                                                       m_field_points_slot),
                       result))
                return result;

            m_field_has_points = true;
            m_field_count = count;
        }

        // ray queries -- a surface joins the rays (once per grid size)
        if (settings.topology == 0 && !m_field_has_ray) {
            if (Failed(DefineRaySurface(dims), result)) return result;
        }

        // point ink -- one vec4 per element, zeroed; kept across recompiles of the same size like the points
        if (to_points && !m_field_has_ink) {
            if (Failed(m_renderer->CreateStorageBuffer(static_cast<VkDeviceSize>(count * sizeof(glm::vec4)), nullptr, m_field_ink_buffer, m_field_ink_slot),
                       result))
                return result;

            m_field_has_ink = true;
        }

        // 4. settings -> the config row
        Ir77FieldConfig& cfg = m_field_config;

        cfg.domain_x = glm::vec4(settings.x.x, settings.x.y, settings.y.x, settings.y.y);
        cfg.domain_z = glm::vec4(settings.z.x, settings.z.y, settings.value.x, settings.value.y);
        cfg.dims = glm::uvec4(dims, settings.topology);
        cfg.shading = glm::uvec4(settings.shading, settings.colormap, has_paint ? 1u : 0u, to_canvas ? 1u : 0u);
        cfg.contour = glm::vec4(settings.contour_spacing, settings.contour_width, 0.0f, 0.0f);
        cfg.canvas = glm::uvec4(m_field_canvas_image, m_field_canvas_texture, to_canvas ? m_field_canvas_size : 0u, settings.pens);
        cfg.trace = glm::vec4(settings.pen_speed, settings.decay, settings.sweep, 0.0f);
        cfg.steps = glm::uvec4(std::max(settings.steps, 1u), 0u, 0u, 0u);
        cfg.params = {glm::vec4(settings.params[0], settings.params[1], settings.params[2], settings.params[3]),
                      glm::vec4(settings.params[4], settings.params[5], settings.params[6], settings.params[7])};
        cfg.pen_color = settings.pen_color;
        cfg.pen_width = {glm::vec4(settings.pen_width[0], settings.pen_width[1], settings.pen_width[2], settings.pen_width[3]),
                         glm::vec4(settings.pen_width[4], settings.pen_width[5], settings.pen_width[6], settings.pen_width[7])};

        // no point ink -> the config's own slot stands in, so the shaders never index an empty slot
        cfg.ink = glm::uvec4(m_field_has_ink ? m_field_ink_slot : m_field_config_slot, to_points ? 1u : 0u, 0u, 0u);

        if (Failed(DefineFieldConfig(), result)) return result;

        m_field_settings = settings;
        m_field_has_pen = has_pen;
        m_field_ready = true;
        m_field_first = true;
        m_field_clear = true;
        m_field_pen_time = 0.0f;

        // 5. the shadow box now has to cover the field too
        DefineBounds();

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Ir77Scene: field compiled.");
    }

    // The 8 live values every program reads as params[] -- sliders. Applied from the next frame, no recompile.
    std::shared_ptr<IIr77Return const> SetFieldParams(std::array<float, 8> const& params) {
        if (!m_field_ready) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: no field compiled.");

        m_field_settings.params = params;
        m_field_config.params = {glm::vec4(params[0], params[1], params[2], params[3]), glm::vec4(params[4], params[5], params[6], params[7])};

        return DefineFieldConfig();
    }

    // Blanks the canvas on the next frame
    std::shared_ptr<IIr77Return const> ClearFieldCanvas() {
        if (!m_field_ready) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: no field compiled.");

        m_field_clear = true;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Runs the program from its first frame again (f.first), with a blank canvas and the pens back at t = 0
    std::shared_ptr<IIr77Return const> ResetField() {
        if (!m_field_ready) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: no field compiled.");

        m_field_first = true;
        m_field_clear = true;
        m_field_pen_time = 0.0f;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Hidden: no compute, no draws -- the state stays where it was
    std::shared_ptr<IIr77Return const> ShowField(bool const& visible) {
        m_field_visible = visible;

        DefineBounds();

        if (m_field_has_ray) m_renderer->SetSurfaceAccelerationVisible(m_field_ray_surface, visible);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The field's compute for this frame, appended to the frame's dispatch list (recorded once a frame, on the first window Paint draws, so
    // the simulation steps once however many windows draw it): Field() over every element, then (with a Pen) the canvas decay / clear and the pens
    // between last frame's t and this one's. dt = seconds since the last frame (clamped to 0..0.1 here). Hidden or not compiled: nothing.
    std::shared_ptr<IIr77Return const> AppendDispatches(std::vector<Ir77PVDispatch>& dispatches, float const& dt) {
        if (!m_renderer) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Field: SetRenderer not called.");

        if (m_field_ready && m_field_visible) {
            float const step = std::clamp(dt, 0.0f, 0.1f);

            Ir77PVDispatch points{};
            points.pipeline = m_field_compute;
            points.push.user = {m_field_config_slot, m_field_points_slot, FIELD_JOB_POINTS, m_field_first ? 1u : 0u, 0u, 0u, 0u, 0u};
            points.groups = FieldGroups(m_field_count);
            dispatches.push_back(points);

            if (m_field_has_pen) {
                float const t_from = m_field_pen_time;
                float const t_to = t_from + step * m_field_settings.pen_speed;
                m_field_pen_time = t_to;

                // oscilloscope wrap -- the beam passed the right edge this frame
                bool const wrapped = m_field_settings.sweep > 0.0f && std::floor(t_to * m_field_settings.sweep) > std::floor(t_from * m_field_settings.sweep);
                bool const clear = m_field_clear || (wrapped && m_field_settings.wrap_clear);

                if (clear || m_field_settings.decay > 0.0f) {
                    Ir77PVDispatch canvas{};
                    canvas.pipeline = m_field_compute;
                    canvas.push.user = {m_field_config_slot, m_field_points_slot, FIELD_JOB_CANVAS, clear ? 2u : 0u, 0u, 0u, 0u, 0u};
                    canvas.groups = FieldGroups(m_field_settings.pen_target == 1 ? m_field_count
                                                                                 : static_cast<std::uint64_t>(m_field_canvas_size) * m_field_canvas_size);
                    dispatches.push_back(canvas);
                }

                Ir77PVDispatch pen{};
                pen.pipeline = m_field_compute;
                pen.push.user = {m_field_config_slot,      m_field_points_slot,     FIELD_JOB_PEN, 0u, std::bit_cast<std::uint32_t>(t_from),
                                 std::bit_cast<std::uint32_t>(t_to), 0u,          0u};
                pen.groups = FieldGroups(static_cast<std::uint64_t>(std::max(m_field_settings.steps, 1u)) * m_field_settings.pens);
                dispatches.push_back(pen);
            }

            m_field_first = false;
            m_field_clear = false;
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The field's draws for one pass list: surface -> Opaque + one Shadow draw per casting light; particles -> Transparent (additive). A
    // pipeline that can't be built yet just leaves the field out of this frame.
    std::shared_ptr<IIr77Return const> AppendDraws(std::size_t const& window, Ir77PVPass const& pass, std::vector<Ir77PVDrawItem>& items) {
        if (!m_field_ready || !m_field_visible || !m_renderer) return Ir77RETURN<Ir77OperationSucceeded>();

        std::size_t const w = window;

        bool const surface = m_field_settings.topology == 0;
        glm::uvec3 const dims = glm::uvec3(m_field_config.dims);

        std::uint32_t const vertices = surface ? (dims.x - 1) * (dims.y - 1) * 6 : static_cast<std::uint32_t>(m_field_count * 6);

        auto const make = [&](Ir77PVPipelineHandle const pipeline, std::uint32_t const light) {
            Ir77PVDrawItem item{};
            if (m_renderer->MakeProceduralDrawItem(w, pipeline, m_default_material, vertices, 1, item)->ID() != GUIDIr77OperationSucceeded) return;

            item.mode = surface ? 0u : 1u;
            item.user[0] = m_field_config_slot;  // -> pc.user[4]
            item.user[1] = m_field_points_slot;  // -> pc.user[5]
            item.light_index = light;

            items.push_back(item);
        };

        if (pass == Ir77PVPass::Opaque && surface) make(m_field_surface, 0);
        if (pass == Ir77PVPass::Transparent && !surface) make(m_field_particles, 0);

        std::uint32_t shadows{0};
        if (m_lighting) m_lighting->GetShadowCount(shadows);

        if (pass == Ir77PVPass::Shadow && surface)
            for (std::uint32_t tile = 0; tile < shadows; tile++) make(m_field_shadow, tile);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    // One generated field shader: the stage, the program's optional functions, the contract, the program (its line 1 is the error's line 1),
    // then the stage's main()
    std::string FieldShaderText(char const* stage, std::string const& defines, std::string const& source, char const* main) const {
        return std::string{"#version 460\n#extension GL_GOOGLE_include_directive : require\n#define "} + stage + "\n" + defines +
               "#include \"include/bindless.glsl\"\n#include \"include/field.glsl\"\n#line 1\n" + source + "\n#include \"" + main + "\"\n";
    }

    // The config row -- created on the first compile, rewritten afterwards (ordered ahead of the next frame, no wait)
    std::shared_ptr<IIr77Return const> DefineFieldConfig() {
        if (m_field_has_config)
            return m_renderer->UploadStorageBuffer(m_field_config_buffer, &m_field_config, sizeof(Ir77FieldConfig), 0, false);

        std::shared_ptr<IIr77Return const> result{};
        if (Failed(m_renderer->CreateStorageBuffer(sizeof(Ir77FieldConfig), &m_field_config, m_field_config_buffer, m_field_config_slot), result))
            return result;

        m_field_has_config = true;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The surface in the rays: a grid index buffer over the z = 0 slice of the points -- two triangles a cell, the same corners and winding
    // field_vertex.glsl draws -- and a surface BLAS over it and the point buffer (position = Ir77FieldPointGPU::position_size.xyz), rebuilt
    // every frame. No ray queries, or a grid too large to keep a second index copy of -> nothing (the shadow map still covers it).
    std::shared_ptr<IIr77Return const> DefineRaySurface(glm::uvec3 const& dims) {
        bool ray_query{false};
        m_renderer->GetRayQuery(ray_query);
        if (!ray_query) return Ir77RETURN<Ir77OperationSucceeded>();

        std::uint64_t const cells = static_cast<std::uint64_t>(dims.x - 1) * (dims.y - 1);
        if (cells > FIELD_MAX_RAY_CELLS)
            return Ir77RETURN<Ir77OperationSucceeded>(this, "[WARN] Ir77Field: surface over " + std::to_string(FIELD_MAX_RAY_CELLS) +
                                                                " cells -- left out of the rays (shadow-map shadows).");

        std::vector<std::uint32_t> indices{};
        indices.reserve(static_cast<std::size_t>(cells * 6));

        for (std::uint32_t y = 0; y + 1 < dims.y; y++)
            for (std::uint32_t x = 0; x + 1 < dims.x; x++) {
                std::uint32_t const i = x + y * dims.x;

                // (0,0) (1,0) (1,1) / (0,0) (1,1) (0,1)
                indices.insert(indices.end(), {i, i + 1, i + 1 + dims.x, i, i + 1 + dims.x, i + dims.x});
            }

        std::shared_ptr<IIr77Return const> result{};

        if (Failed(m_renderer->CreateStorageBuffer(static_cast<VkDeviceSize>(indices.size() * sizeof(std::uint32_t)), indices.data(), m_field_ray_grid_buffer,
                                                   m_field_ray_grid_slot),
                   result))
            return result;

        if (Failed(m_renderer->CreateSurfaceAcceleration(m_field_points_buffer, sizeof(Ir77FieldPointGPU), dims.x * dims.y, m_field_ray_grid_buffer,
                                                         static_cast<std::uint32_t>(cells * 2), m_field_ray_surface),
                   result)) {
            m_renderer->DestroyStorageBuffer(m_field_ray_grid_buffer);
            return result;
        }

        if (!m_field_visible) m_renderer->SetSurfaceAccelerationVisible(m_field_ray_surface, false);

        m_field_ray_grid = glm::uvec2(dims.x, dims.y);
        m_field_has_ray = true;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The surface out of the rays and its grid freed -- before the point buffer is replaced or the grid changes shape
    void DefineRayRelease() {
        if (!m_field_has_ray) return;

        m_renderer->ReleaseSurfaceAcceleration(m_field_ray_surface);
        m_renderer->DestroyStorageBuffer(m_field_ray_grid_buffer);

        m_field_has_ray = false;
        m_field_ray_grid = glm::uvec2(0u);
    }

    // 64 threads a group, spread over x and y -- field_compute.glsl rebuilds the flat index
    std::array<std::uint32_t, 3> FieldGroups(std::uint64_t const threads) const {
        std::uint64_t const groups = std::max<std::uint64_t>((threads + 63) / 64, 1);
        std::uint64_t const x = std::min<std::uint64_t>(groups, 65535);

        return {static_cast<std::uint32_t>(x), static_cast<std::uint32_t>((groups + x - 1) / x), 1u};
    }

    // The field's box -- grid x / z on the ground plane, the value range up -- to Lighting while the field is up, so the sun's shadow covers it
    void DefineBounds() {
        if (!m_lighting) return;

        m_lighting->SetFieldBounds(m_field_ready && m_field_visible, glm::vec3(m_field_settings.x.x, m_field_settings.z.x, m_field_settings.y.x),
                                   glm::vec3(m_field_settings.x.y, m_field_settings.z.y, m_field_settings.y.y));
    }

   private:
    std::shared_ptr<NSIr77PeregrineV::Ir77Server> m_renderer{};

    std::shared_ptr<IIr77Lighting> m_lighting{};

    std::uint32_t m_default_material{0};

    // field -- GPU layouts, must match include/field.glsl
    struct Ir77FieldPointGPU {
        glm::vec4 position_size{0.0f};
        glm::vec4 color{0.0f};
        glm::vec4 data{0.0f};
    };

    static_assert(sizeof(Ir77FieldPointGPU) == 48, "must match Ir77FieldPoint in field.glsl");

    struct Ir77FieldConfig {
        glm::vec4 domain_x{-1.0f, 1.0f, -1.0f, 1.0f};
        glm::vec4 domain_z{-1.0f, 1.0f, -1.0f, 1.0f};
        glm::uvec4 dims{2u, 2u, 1u, 0u};
        glm::uvec4 shading{0u};
        glm::vec4 contour{0.1f, 1.0f, 0.0f, 0.0f};
        glm::uvec4 canvas{0u};
        glm::vec4 trace{1.0f, 0.0f, 0.0f, 0.0f};
        glm::uvec4 steps{256u, 0u, 0u, 0u};
        std::array<glm::vec4, 2> params{};
        std::array<glm::vec4, 8> pen_color{};
        std::array<glm::vec4, 2> pen_width{};
        glm::uvec4 ink{0u};  // x point-ink slot, y pen target (0 canvas, 1 points)
    };

    static_assert(sizeof(Ir77FieldConfig) == 336, "must match Ir77FieldConfig in field.glsl");

    static constexpr std::uint64_t FIELD_MAX_POINTS{16u * 1024u * 1024u};

    static constexpr std::uint64_t FIELD_MAX_RAY_CELLS{4u * 1024u * 1024u};  // a surface past this stays out of the rays (grid copy: 96 MB)

    static constexpr std::uint32_t FIELD_JOB_POINTS{0};  // field_compute.glsl jobs
    static constexpr std::uint32_t FIELD_JOB_CANVAS{1};
    static constexpr std::uint32_t FIELD_JOB_PEN{2};

    Ir77FieldSettings m_field_settings{};

    Ir77FieldConfig m_field_config{};

    bool m_field_ready{false};  // a program compiled and its buffers exist

    bool m_field_visible{true};

    bool m_field_first{false};  // next frame is the program's first (f.first)

    bool m_field_clear{false};  // next frame blanks the canvas

    bool m_field_has_pen{false};

    std::uint64_t m_field_count{0};  // elements in the point buffer

    float m_field_pen_time{0.0f};  // the pens' t

    bool m_field_has_config{false};

    std::uint32_t m_field_config_buffer{0};

    std::uint32_t m_field_config_slot{BINDLESS_NO_SLOT};

    bool m_field_has_points{false};

    std::uint32_t m_field_points_buffer{0};

    std::uint32_t m_field_points_slot{BINDLESS_NO_SLOT};

    bool m_field_has_ink{false};  // point ink -- one vec4 per element, the pens' target "points"

    std::uint32_t m_field_ink_buffer{0};

    std::uint32_t m_field_ink_slot{BINDLESS_NO_SLOT};

    bool m_field_has_canvas{false};

    std::uint32_t m_field_canvas_size{0};

    std::uint32_t m_field_canvas_texture{BINDLESS_NO_SLOT};  // sampled slot (field.frag)

    std::uint32_t m_field_canvas_image{BINDLESS_NO_SLOT};  // storage image slot (field.comp)

    Ir77PVPipelineHandle m_field_surface{IR77PV_NO_PIPELINE};

    Ir77PVPipelineHandle m_field_particles{IR77PV_NO_PIPELINE};

    Ir77PVPipelineHandle m_field_shadow{IR77PV_NO_PIPELINE};

    Ir77PVPipelineHandle m_field_compute{IR77PV_NO_PIPELINE};

    // ray queries -- the surface BLAS and its grid index buffer
    bool m_field_has_ray{false};

    std::uint32_t m_field_ray_surface{0};  // Acceleration surface handle

    std::uint32_t m_field_ray_grid_buffer{0};

    std::uint32_t m_field_ray_grid_slot{BINDLESS_NO_SLOT};

    glm::uvec2 m_field_ray_grid{0u};  // the grid it was built for -- (0, 0) when none
};
}  // namespace NSIr77Scene
