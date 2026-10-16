#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <simdjson.h>

#include "../../Ir77RT/runtime/Ir77Return.hpp"

#include "../Windows/runtime/CEFMessageLoop.hpp"
#include "../Windows/runtime/CEFApp.hpp"
#include "../Windows/runtime/CEFRTState.hpp"
#include "../Windows/runtime/CEFClient.hpp"
#include "../Windows/runtime/CEFMouseEvent.hpp"

#include "../PeregrineV/server/Ir77PeregrineV.hpp"
#include "../PeregrineV/server/Ir77PVCEF.hpp"
#include "../PeregrineV/server/Ir77PVPaint.hpp"
#include "../PeregrineV/server/Ir77PVAsset.hpp"

#include "../PeregrineV/interface/IIr77PVDevice.hpp"

#include "../PeregrineV/types/Ir77PVTypes.hpp"

#include "../Scene/server/Ir77Scene.hpp"

#include "./types/Ir77Stats.hpp"

using namespace CEF;
using namespace NSIr77PeregrineV;
using namespace NSIr77Scene;

namespace Ir77 {

class iridium {
   public:
    iridium() {
        m_message_loop = std::make_shared<CEFMessageLoop>();
        m_rt_state = std::make_shared<CEFRTState>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // init
    // -------------------------------------------------------------------------------------------------------------------------------------
    void InitializePipeline(int argc, char* argv[]) {
        m_rt_state->Initialize(argc, argv);

        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "x11");

        if (!SDL_Init(SDL_INIT_VIDEO)) {
            Ir77RETURN<Ir77OperationFailed>(nullptr, "SDL_Init failed: " + std::string{SDL_GetError()});
            throw std::runtime_error{"SDL_Init failed: " + std::string{SDL_GetError()}};
        }

        m_window = SDL_CreateWindow("Iridium", 1280, 720, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

        if (!m_window) {
            Ir77RETURN<Ir77OperationFailed>(nullptr, "SDL_CreateWindow failed: " + std::string{SDL_GetError()});
            throw std::runtime_error{"SDL_CreateWindow failed: " + std::string{SDL_GetError()}};
        }

        SDL_ShowWindow(m_window);

        SDL_StartTextInput(m_window);  // SDL_EVENT_TEXT_INPUT -> typed characters for the UI's inputs

        std::vector<SDL_Window*> windows{m_window};

        m_vulkan = std::make_shared<Ir77PeregrineV>();
        m_cef = std::make_shared<Ir77PVCEF>();
        m_paint = std::make_shared<Ir77PVPaint>();
        m_asset = std::make_shared<Ir77PVAsset>();

        m_cef->SetPeregrineV(m_vulkan);
        m_paint->SetPeregrineV(m_vulkan);
        m_asset->SetPeregrineV(m_vulkan);

        // instance / device
        m_vulkan->CreateInstance();

        m_vulkan->EnumeratePhysicalDevices();
        Require(m_vulkan->CreateDisplayDevice(windows, m_device_id), "Create display device.");
        m_windows.emplace(m_device_id, windows);
        Require(m_vulkan->CreateSurfaces(m_windows), "Create surfaces.");
        m_vulkan->CreateAllocator();

        // layouts + passes (Main only; Shadow pipelines are skipped until CreateRenderPass(Shadow) is called)
        Require(m_vulkan->CreateLayouts());
        Require(m_vulkan->CreateRenderPasses());
        Require(m_vulkan->CreateSwapchains());

        // frame resources -- the placeholder texture must exist before the descriptor sets and materials bind it
        Require(m_vulkan->CreatePlaceholderTexture());
        Require(m_vulkan->CreateFrameBuffers());
        Require(m_vulkan->CreateDescriptorSets());

        // CEF overlay -- one per window, uses the CEF layout from CreateLayouts and each window's swapchain
        Require(m_cef->CreateOverlays());

        std::shared_ptr<IIr77PVOverlay> overlay{};
        Require(m_cef->GetOverlay(0, overlay), "Get overlay.");

        std::weak_ptr<IIr77PVOverlay> weak_overlay = overlay;

        // CEF paint -> overlay. During a resize only the paint at the requested size is shown, and it is presented right here,
        // inside the paint, rather than on the next loop iteration.
        m_rt_state->GetRenderHandler()->SetPaintCallback([this, weak_overlay](const void* buffer, int w, int h) {
            // while a resize is in flight, a paint at any other size is stale -- drop it and keep showing the last good frame
            // std::cerr << "in flight: " << m_cef_in_flight << " cef w x h: " << m_cef_width << " x " << m_cef_height << " vs w x h: " << w << "x" << h <<
            // "\n";

            m_ui_paints++;

            auto const locked = weak_overlay.lock();
            if (!locked) return;

            // overlay mid-rebuild -- this paint is lost; WindowStep asks CEF for another
            bool resizing{false};
            locked->IsResizing(resizing);
            if (resizing) {
                m_cef_repaint = true;
                m_ui_dropped_resizing++;
                return;
            }

            if (m_cef_in_flight) {
                auto const waited = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_last_cef_resize).count();

                // stale size -- drop it, unless CEF has had long enough and still hasn't painted the requested size
                if ((w != m_cef_width || h != m_cef_height) && waited < CEF_RESIZE_TIMEOUT_MS) {
                    m_cef_repaint = true;
                    m_ui_dropped_stale++;
                    return;
                }

                m_cef_in_flight = false;  // CEF has caught up -- WindowStep may send the next size
            }

            if (locked->UploadFrame(buffer, w, h)->ID() == GUIDIr77OperationSucceeded) {
                if (m_ui_uploads == 0)
                    std::printf(
                        "[UI] first frame %dx%d after %lld ms\n", w, h,
                        static_cast<long long>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_start_time).count()));
                m_ui_uploads++;
            } else
                m_ui_upload_failed++;

            m_ui_last_paint = std::to_string(w) + "x" + std::to_string(h);
            m_cef_repaint = false;

            if (!m_render_ready) return;

            auto const since_resize = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_last_resize_event).count();
        });

        // pipelines need shaders, layouts and the Main pass
        Require(m_asset->CreateShaders(), "Create shaders.");

        std::vector<Ir77PVPipelineKind> kinds;
        Require(m_asset->GetPipelineKinds(kinds));

        Require(m_vulkan->CreatePipelines(kinds));

        // content -- the glTF scene
        m_scene_ready = LoadScene(m_scene_path, false)->ID() == GUIDIr77OperationSucceeded;

        Require(m_asset->FindPipeline(std::string{"ripple"}, m_scene_pipeline));

        if (!m_scene_ready) {
            std::cerr << "Iridium: scene '" << m_scene_path << "' failed to load \n";
            throw std::runtime_error{"LoadScene failed"};
        }

        // command buffers wire the overlay, so they come after CreateBufferCEF and CreatePipelines
        Require(m_paint->CreateCommandBuffers());

        m_render_ready = true;  // from here on a CEF paint may present a frame directly

        Ir77RETURN<Ir77OperationSucceeded>(nullptr, "Initialize adapter.");
    }

    bool IsRunning() { return running; }
    void WindowStep() {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;

                case SDL_EVENT_WINDOW_RESIZED:
                case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
                    SDL_GetWindowSizeInPixels(m_window, &m_window_width, &m_window_height);

                    m_pending_width = m_window_width;
                    m_pending_height = m_window_height;

                    m_last_resize_event = std::chrono::steady_clock::now();
                    m_resize_pending = true;

                    break;
                }

                // F2 cycles file cameras, then the controlled camera -- kept from CEF
                case SDL_EVENT_KEY_DOWN: {
                    if (event.key.key == SDLK_F2 && !event.key.repeat) {
                        m_scene_camera_index = (m_scene_camera_index + 1) % (m_scene_camera_count + 1);
                        break;
                    }

                    CEFInputEvent(&event, m_rt_state->GetBrowser());
                    break;
                }

                case SDL_EVENT_MOUSE_WHEEL: {
                    float mouseX, mouseY;
                    SDL_GetMouseState(&mouseX, &mouseY);

                    CefMouseEvent cef_event;
                    cef_event.x = mouseX;
                    cef_event.y = mouseY;

                    const int scroll_scale = 120;
                    int deltaX = event.wheel.x * scroll_scale;
                    int deltaY = event.wheel.y * scroll_scale;

                    m_rt_state->GetBrowser()->GetHost()->SendMouseWheelEvent(cef_event, deltaX, deltaY);
                    break;
                }

                default:
                    CEFInputEvent(&event, m_rt_state->GetBrowser());
                    break;
            }
        }

        if (m_resize_pending) {
            auto const now = std::chrono::steady_clock::now();
            auto const since_applied = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_cef_resize).count();

            // handshake -- the next size goes to CEF only after CEF has painted the last one (or it timed out)
            if (!m_cef_in_flight || since_applied >= CEF_RESIZE_TIMEOUT_MS) {
                m_resize_pending = false;

                if (m_pending_width != m_cef_width || m_pending_height != m_cef_height) {
                    m_last_cef_resize = now;

                    m_cef_width = m_pending_width;
                    m_cef_height = m_pending_height;
                    m_cef_in_flight = true;

                    m_rt_state->GetRenderHandler()->SetViewSize(m_cef_width, m_cef_height);
                    m_rt_state->GetBrowser()->GetHost()->WasResized();
                    m_rt_state->GetBrowser()->GetHost()->Invalidate(PET_VIEW);
                }
            }
        }

        // a dropped paint on a static page is never repeated -- once nothing is in flight, ask CEF to paint again
        if (m_cef_in_flight) {
            auto const waited = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_last_cef_resize).count();

            if (waited >= CEF_RESIZE_TIMEOUT_MS) {
                m_cef_in_flight = false;
                m_cef_repaint = true;
            }
        }

        if (m_cef_repaint && !m_cef_in_flight) {
            m_cef_repaint = false;
            m_rt_state->GetBrowser()->GetHost()->Invalidate(PET_VIEW);
        }

        // [UI] status until the first upload -- tells which stage a blank UI is stuck at
        if (m_ui_uploads == 0) {
            auto const now = std::chrono::steady_clock::now();
            if (now - m_ui_last_status >= std::chrono::seconds(1)) {
                auto const elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_start_time).count();
                // nothing painted yet -- poke CEF once; if the UI appears after this line, it was a missed first paint, not a load failure
                // stage 1 (2 s): resize + invalidate. stage 2 (5 s): reload -- a stalled compositor never recovers on its own
                if (m_ui_paints == 0 && elapsed_ms >= 2000 && m_ui_kicked == 0) {
                    std::cerr << "[UI] no paint after 2 s -- WasResized + Invalidate" << std::endl;
                    m_rt_state->GetBrowser()->GetHost()->WasResized();
                    m_rt_state->GetBrowser()->GetHost()->Invalidate(PET_VIEW);
                    m_ui_kicked = 1;
                } else if (m_ui_paints == 0 && elapsed_ms >= 5000 && m_ui_kicked == 1) {
                    // keep this run's CEF log before anything else can overwrite it
                    std::string const snapshot{"/tmp/ir77_blank_" + std::to_string(std::time(nullptr)) + ".log"};
                    std::error_code ec{};
                    std::filesystem::copy_file("/tmp/cef_debug.log", snapshot, std::filesystem::copy_options::overwrite_existing, ec);

                    std::cerr << "[UI] no paint after 5 s -- Reload (CEF log saved to " << snapshot << ")" << std::endl;
                    m_rt_state->GetBrowser()->ReloadIgnoreCache();
                    m_ui_kicked = 2;
                }

                m_ui_last_status = now;

                std::printf(
                    "[UI] paints %u, uploads %u, upload failed %u, dropped stale %u, dropped resizing %u, in flight %d, repaint %d, "
                    "cef %dx%d, window %dx%d, last paint %s\n",
                    m_ui_paints, m_ui_uploads, m_ui_upload_failed, m_ui_dropped_stale, m_ui_dropped_resizing, m_cef_in_flight, m_cef_repaint, m_cef_width,
                    m_cef_height, m_window_width, m_window_height, m_ui_last_paint.c_str());
            }
        }
    }

    // TEST: pump CEF for ~2 ms per frame instead of one call -- if blanks stop, the cause is message-loop starvation
    void ChromeStep() {
        auto const until = std::chrono::steady_clock::now() + std::chrono::milliseconds(2);

        do m_message_loop->CEFDoMessageLoop(m_rt_state->GetBrowser());
        while (std::chrono::steady_clock::now() < until);
    }

    // Angular -> Iridium: every window.cefQuery request since the last frame, applied in arrival order before the frame is built
    void IridiumStep() {
        m_rt_state->GetClient()->TakeQueries(m_queries);

        for (std::string const& query : m_queries) ApplyQuery(query);
    }

    void VulkanStep() {
        auto const frame_start = std::chrono::steady_clock::now();
        float const seconds = std::chrono::duration<float>(frame_start - m_start_time).count();

        // uploads, indirect show/hide writes, graveyard -- before anything records
        m_asset->Tick(m_frame);

        float dt{0.0f};

        if (m_scene_ready) {
            dt = m_vstep_seconds > 0.0f ? seconds - m_vstep_seconds : 0.0f;
            m_vstep_seconds = seconds;

            m_scene->Update(dt);                         // everything scene-side: animation, world transforms
            UpdateCameraControl(dt);                     // held keys from the UI move the controlled camera
            auto const uploaded = UploadScene(seconds);  // the frame's results -> PeregrineV
            if (uploaded->ID() != GUIDIr77OperationSucceeded && !m_upload_error_logged) {
                std::printf("[ERROR] UploadScene failed -- no draws this frame\n");
                m_upload_error_logged = true;
            }
        } else {
            throw std::runtime_error{"LoadScene failed"};
        }

        auto const draw_start = std::chrono::steady_clock::now();
        m_paint->Draw();
        auto const draw_end = std::chrono::steady_clock::now();

        UpdateFrameStats(dt, frame_start, draw_start, draw_end);

        // assumes Paint advances one frame-in-flight per Draw -- swap for Paint's own index if it exposes one
        m_frame = (m_frame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void Shutdown() {
        m_scene.reset();
        m_paint.reset();
        m_asset.reset();
        m_cef.reset();
        m_vulkan.reset();

        if (m_window) {
            SDL_DestroyWindow(m_window);
            m_window = nullptr;
        }

        SDL_Quit();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // actions -- the app interface. App logic decides; these only forward to the scene. They take effect on the next frame's Update.
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> PlayAnimation(std::string const& name, bool const& loop, float const& speed = 1.0f) {
        if (!m_scene_ready) return Ir77RETURN<Ir77NotConfigured>(nullptr, "Iridium: no scene loaded.");

        return m_scene->PlayAnimation(name, loop, speed);
    }

    std::shared_ptr<IIr77Return const> StopAnimation(std::string const& name) {
        if (!m_scene_ready) return Ir77RETURN<Ir77NotConfigured>(nullptr, "Iridium: no scene loaded.");

        return m_scene->StopAnimation(name);
    }

    std::shared_ptr<IIr77Return const> StopAllAnimations() {
        if (!m_scene_ready) return Ir77RETURN<Ir77NotConfigured>(nullptr, "Iridium: no scene loaded.");

        return m_scene->StopAllAnimations();
    }

    std::shared_ptr<IIr77Return const> SelectCamera(std::uint32_t const& index) {
        if (index > m_scene_camera_count) return Ir77RETURN<Ir77NotConfigured>(nullptr, "Iridium: no such camera.");

        m_scene_camera_index = index;  // == camera count -> the controlled camera

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // stats for the app display -- copies, safe to keep
    std::shared_ptr<IIr77Return const> GetSceneStats(Ir77SceneStats& stats) const {
        stats = m_scene_stats;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetFrameStats(Ir77FrameStats& stats) const {
        stats = m_frame_stats;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    // -------------------------------------------------------------------------------------------------------------------------------------
    // scene glue -- Ir77Scene (glTF) -> Ir77PVAsset / Paint
    // -------------------------------------------------------------------------------------------------------------------------------------

    // Once, after CreatePipelines. Parses the glTF, stacks every primitive into one upload, sorts the commands into material runs.
    std::shared_ptr<IIr77Return const> LoadScene(std::string const& path, bool const& use_mesh) {
        m_scene = std::make_shared<Ir77Scene>();

        auto result = m_scene->ParseAsset(path);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        std::shared_ptr<IIr77Mesh> meshes{};
        m_scene->GetMeshes(meshes);

        // commands ordered by material -> each material is one contiguous run, one draw item
        auto const build_runs = [](std::vector<Ir77MeshRange> const& ranges, std::vector<std::uint32_t> const& materials,
                                   std::vector<VkDrawIndexedIndirectCommand>& commands, std::vector<Ir77SceneRun>& runs) {
            std::vector<std::size_t> order(ranges.size());
            std::iota(order.begin(), order.end(), 0);
            std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) { return materials[a] < materials[b]; });

            runs.clear();
            for (std::size_t k = 0; k < order.size(); k++) {
                Ir77MeshRange const& r = ranges[order[k]];
                commands.push_back(VkDrawIndexedIndirectCommand{r.index_count, r.instance_count, r.index_offset, r.vertex_offset, r.first_instance});

                std::uint32_t const material = materials[order[k]];
                if (runs.empty() || runs.back().gltf_material != material) runs.push_back({material, static_cast<std::uint32_t>(k), 0});
                runs.back().command_count++;
            }
        };

        // static stream -- vertex path or mesh shading
        Ir77PVSceneUpload upload{};
        std::vector<Ir77MeshRange> ranges{};
        std::vector<std::uint32_t> materials{};

        meshes->GetVertexBuffer(upload.vertices);
        meshes->GetIndexBuffer(upload.indices);
        meshes->GetMeshRanges(ranges);
        meshes->GetRangeMaterials(materials);
        meshes->GetInstanceBuffer(m_scene_transforms);
        meshes->GetBounds(m_scene_min, m_scene_max);

        // BLEND materials -> Transparent pass (sorted per object each frame); everything else stays in the batched opaque runs
        std::shared_ptr<IIr77Material> blend_source{};
        m_scene->GetMaterials(blend_source);

        auto const split_blend = [&](std::vector<Ir77MeshRange>& in_ranges, std::vector<std::uint32_t>& in_materials, std::vector<Ir77MeshRange>& blend_ranges,
                                     std::vector<std::uint32_t>& blend_materials) {
            std::vector<Ir77MeshRange> keep_ranges{};
            std::vector<std::uint32_t> keep_materials{};

            for (std::size_t r = 0; r < in_ranges.size(); r++) {
                bool blend{false};
                blend_source->GetMaterialBlend(in_materials[r], blend);  // no material / out of range -> stays opaque

                if (blend) {
                    blend_ranges.push_back(in_ranges[r]);
                    blend_materials.push_back(in_materials[r]);
                } else {
                    keep_ranges.push_back(in_ranges[r]);
                    keep_materials.push_back(in_materials[r]);
                }
            }

            in_ranges = std::move(keep_ranges);
            in_materials = std::move(keep_materials);
        };

        m_scene_blend_draws.clear();

        std::vector<Ir77MeshRange> blend_ranges{};
        std::vector<std::uint32_t> blend_materials{};
        split_blend(ranges, materials, blend_ranges, blend_materials);

        build_runs(ranges, materials, upload.indirect_commands, m_scene_runs);

        // static blend ranges -- compacted into their own small vertex-path scene (the mesh path has no Transparent pipeline)
        Ir77PVSceneUpload blend_upload{};

        for (std::size_t b = 0; b < blend_ranges.size(); b++) {
            Ir77MeshRange const& r = blend_ranges[b];
            if (r.index_count == 0) continue;

            std::uint32_t highest{0};  // indices are relative to vertex_offset -- the highest one bounds the vertex span
            for (std::uint32_t i = 0; i < r.index_count; i++) highest = std::max(highest, upload.indices[r.index_offset + i]);

            m_scene_blend_draws.push_back({blend_materials[b], static_cast<std::uint32_t>(blend_upload.indirect_commands.size()), r.first_instance, false});

            blend_upload.indirect_commands.push_back(VkDrawIndexedIndirectCommand{r.index_count, r.instance_count,
                                                                                  static_cast<std::uint32_t>(blend_upload.indices.size()),
                                                                                  static_cast<std::int32_t>(blend_upload.vertices.size()), r.first_instance});

            blend_upload.indices.insert(blend_upload.indices.end(), upload.indices.begin() + r.index_offset,
                                        upload.indices.begin() + r.index_offset + r.index_count);
            blend_upload.vertices.insert(blend_upload.vertices.end(), upload.vertices.begin() + r.vertex_offset,
                                         upload.vertices.begin() + r.vertex_offset + highest + 1);
        }

        // skinned stream -- vertex path only, Skinned pipeline
        Ir77PVSceneUpload skinned_upload{};
        std::vector<Ir77MeshRange> skinned_ranges{};
        std::vector<std::uint32_t> skinned_materials{};

        meshes->GetSkinnedVertexBuffer(skinned_upload.vertices_skinned);
        meshes->GetSkinnedIndexBuffer(skinned_upload.indices_skinned);
        meshes->GetSkinnedMeshRanges(skinned_ranges);
        meshes->GetSkinnedRangeMaterials(skinned_materials);

        std::vector<Ir77MeshRange> skinned_blend_ranges{};
        std::vector<std::uint32_t> skinned_blend_materials{};
        split_blend(skinned_ranges, skinned_materials, skinned_blend_ranges, skinned_blend_materials);

        build_runs(skinned_ranges, skinned_materials, skinned_upload.indirect_commands, m_scene_skinned_runs);

        // skinned blend ranges -- appended after the opaque runs in the skinned scene (same buffers, SkinnedTransparent pipeline)
        for (std::size_t b = 0; b < skinned_blend_ranges.size(); b++) {
            Ir77MeshRange const& r = skinned_blend_ranges[b];

            m_scene_blend_draws.push_back(
                {skinned_blend_materials[b], static_cast<std::uint32_t>(skinned_upload.indirect_commands.size()), r.first_instance, true});

            skinned_upload.indirect_commands.push_back(
                VkDrawIndexedIndirectCommand{r.index_count, r.instance_count, r.index_offset, r.vertex_offset, r.first_instance});
        }

        m_scene_has_skinned = !skinned_upload.indirect_commands.empty();

        if (upload.indirect_commands.empty() && skinned_upload.indirect_commands.empty() && blend_upload.indirect_commands.empty())
            return Ir77RETURN<Ir77NotConfigured>(nullptr, "LoadScene: no drawable primitives.");

        // grey fallback -- primitives with no glTF material
        Ir77PVMaterialConstants constants{};
        constants.base_color = {0.8f, 0.8f, 0.8f, 1.0f};

        result = m_asset->CreateMaterial(constants, m_scene_default_material);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        // textures -- glTF texture index -> Asset texture id; IR77PV_NO_TEXTURE where decode / upload failed
        std::shared_ptr<IIr77Texture> textures{};
        m_scene->GetTextures(textures);

        std::uint32_t texture_count{0};
        textures->GetTextureCount(texture_count);

        std::vector<std::uint32_t> texture_ids(texture_count, IR77PV_NO_TEXTURE);

        for (std::uint32_t t = 0; t < texture_count; t++) {
            std::uint32_t width{0}, height{0};
            bool srgb{false};
            std::vector<std::uint8_t> rgba{};

            if (textures->GetTexture(t, width, height, srgb, rgba)->ID() != GUIDIr77OperationSucceeded) continue;
            if (m_asset->CreateTexture(rgba.data(), width, height, srgb, texture_ids[t])->ID() != GUIDIr77OperationSucceeded)
                texture_ids[t] = IR77PV_NO_TEXTURE;
        }

        // materials -- glTF material index -> Asset material id, uploaded in one batch
        std::shared_ptr<IIr77Material> const& materials_in = blend_source;

        std::uint32_t material_count{0};
        materials_in->GetMaterialCount(material_count);

        std::vector<Ir77PVMaterialConstants> material_constants(material_count);
        std::vector<Ir77PVMaterialTextures> material_textures(material_count);

        auto const texture_id = [&](std::uint32_t const& gltf_texture) {
            return gltf_texture < texture_ids.size() ? texture_ids[gltf_texture] : IR77PV_NO_TEXTURE;
        };

        for (std::uint32_t m = 0; m < material_count; m++) {
            std::array<std::uint32_t, 5> slots{};
            bool double_sided{false};
            materials_in->GetMaterial(m, material_constants[m], slots, double_sided);

            material_textures[m].albedo = texture_id(slots[0]);
            material_textures[m].normal = texture_id(slots[1]);
            material_textures[m].orm = texture_id(slots[2]);
            material_textures[m].emissive = texture_id(slots[3]);
            material_textures[m].ao = texture_id(slots[4]);
        }

        m_scene_material_ids.clear();
        if (material_count > 0) {
            result = m_asset->UploadMaterials(material_constants, material_textures, m_scene_material_ids);
            if (result->ID() != GUIDIr77OperationSucceeded) return result;
        }

        // static: mesh path when the device has it, vertex path otherwise -- same upload either way
        m_scene_mesh = false;
        if (!ranges.empty()) {
            std::uint32_t records{0};
            m_scene_mesh = use_mesh && m_asset->UploadMeshlets(upload, m_scene_id, records)->ID() == GUIDIr77OperationSucceeded;

            if (!m_scene_mesh) {
                result = m_asset->UploadScene(upload, m_scene_id);
                if (result->ID() != GUIDIr77OperationSucceeded) return result;
            }
        }

        // skinned: its own scene upload (skinned vertex / index channels + its own indirect commands)
        if (m_scene_has_skinned) {
            result = m_asset->UploadScene(skinned_upload, m_scene_skinned_id);
            if (result->ID() != GUIDIr77OperationSucceeded) return result;
        }

        // static transparent: its own compacted scene
        if (!blend_upload.indirect_commands.empty()) {
            result = m_asset->UploadScene(blend_upload, m_scene_blend_id);
            if (result->ID() != GUIDIr77OperationSucceeded) return result;
        }

        // lights -- header + array, packed once; a default sun when the file has none so the scene isn't black
        std::shared_ptr<IIr77Light> lights{};
        m_scene->GetLights(lights);

        std::vector<Ir77PVLight> light_list{};
        lights->GetLightsBuffer(light_list);

        if (light_list.empty()) {
            Ir77PVLight sun{};
            sun.direction = glm::normalize(glm::vec3(-0.4f, -1.0f, -0.3f));
            sun.color = glm::vec3(1.0f);
            sun.intensity = 1.0f;
            sun.type = 0;  // directional
            light_list.push_back(sun);
        }

        if (light_list.size() > 64) light_list.resize(64);  // Ir77PVFrameSizes::lights capacity

        Ir77PVLightsHeader header{};
        header.count = static_cast<std::uint32_t>(light_list.size());

        m_scene_lights.resize(sizeof(header) + light_list.size() * sizeof(Ir77PVLight));
        std::memcpy(m_scene_lights.data(), &header, sizeof(header));
        std::memcpy(m_scene_lights.data() + sizeof(header), light_list.data(), light_list.size() * sizeof(Ir77PVLight));

        // file cameras -- index m_scene_camera_count is the controlled camera; start on the file's first camera if it has one
        m_scene->GetCameras(m_scene_cameras);
        m_scene_cameras->GetCameraCount(m_scene_camera_count);
        m_scene_camera_index = 0;

        ResetCameraControl();  // the controlled camera starts framed on the scene

        // every clip in the file plays by default, looping -- the app takes over with PlayAnimation / StopAnimation
        m_scene->PlayAllAnimations(true);

        // stats -- filled once here, read by the app through GetSceneStats
        m_scene_stats = Ir77SceneStats{};
        m_scene_stats.path = path;
        m_scene_stats.render_path = m_scene_mesh ? "mesh shading indirect" : "vertex indirect";

        m_asset->GetDeviceName(m_scene_stats.gpu);

        m_scene->GetAssetCounts(m_scene_stats.nodes, m_scene_stats.meshes, m_scene_stats.materials, m_scene_stats.textures, m_scene_stats.cameras,
                                m_scene_stats.skins, m_scene_stats.bones, m_scene_stats.clips);

        std::shared_ptr<IIr77Animation> animations{};
        m_scene->GetAnimations(animations);

        m_scene_stats.clip_names.assign(m_scene_stats.clips, std::string{});
        for (std::uint32_t c = 0; c < m_scene_stats.clips; c++) animations->GetClipName(c, m_scene_stats.clip_names[c]);

        m_scene_stats.static_vertices = upload.vertices.size();
        m_scene_stats.static_indices = upload.indices.size();
        m_scene_stats.skinned_vertices = skinned_upload.vertices_skinned.size();
        m_scene_stats.skinned_indices = skinned_upload.indices_skinned.size();
        m_scene_stats.static_primitives = static_cast<std::uint32_t>(ranges.size() + blend_ranges.size());
        m_scene_stats.skinned_primitives = static_cast<std::uint32_t>(skinned_ranges.size() + skinned_blend_ranges.size());
        m_scene_stats.instances = static_cast<std::uint32_t>(m_scene_transforms.size());
        m_scene_stats.opaque_runs = static_cast<std::uint32_t>(m_scene_runs.size());
        m_scene_stats.skinned_runs = static_cast<std::uint32_t>(m_scene_skinned_runs.size());
        m_scene_stats.transparent_objects = static_cast<std::uint32_t>(m_scene_blend_draws.size());
        m_scene_stats.lights = light_list;

        if (m_scene_transforms.size() > 8192) m_scene_stats.warnings.push_back("more than 8192 instances -- raise the Instances capacity");

        PrintSceneStats();  // console copy, for when the app UI isn't up

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Every frame, after m_scene->Update: copies the frame's results into PeregrineV -- camera, instance transforms, lights,
    // one draw item per material run, for every window. No scene logic here.
    std::shared_ptr<IIr77Return const> UploadScene(float const& seconds) {
        std::size_t const windows = m_windows.at(m_device_id).size();

        m_scene->WriteInstanceTransforms(m_scene_transforms);

        // bones -- uint bone_base[8192] then every joint palette, the layout skinned.vert reads at set 0 binding 4
        bool const skinned = m_scene_has_skinned;
        if (skinned) {
            constexpr std::size_t base_slots = 8192;      // must match bone_base[] in skinned.vert
            constexpr std::size_t max_bones = 256 * 128;  // Ir77PVFrameSizes::bones capacity after the bases

            m_scene->WriteBoneMatrices(m_scene_bones);
            m_scene->WriteInstanceBoneBases(m_scene_bone_bases, std::min(m_scene_transforms.size(), base_slots));

            std::size_t const bone_count = std::min(m_scene_bones.size(), max_bones);

            m_scene_bones_packed.assign(base_slots * sizeof(std::uint32_t) + bone_count * sizeof(glm::mat4), 0);
            std::memcpy(m_scene_bones_packed.data(), m_scene_bone_bases.data(), m_scene_bone_bases.size() * sizeof(std::uint32_t));
            std::memcpy(m_scene_bones_packed.data() + base_slots * sizeof(std::uint32_t), m_scene_bones.data(), bone_count * sizeof(glm::mat4));
        }

        float const radius = std::max(glm::length(m_scene_max - m_scene_min) * 0.5f, 0.01f);

        for (std::size_t w = 0; w < windows; w++) {
            float const aspect = m_window_height > 0 ? static_cast<float>(m_window_width) / static_cast<float>(m_window_height) : 1.78f;

            // file camera (rebuilt with this window's aspect), or the controlled camera (orbit / fly, driven from the UI)
            Ir77PVCamera camera{};
            bool built = false;

            if (m_scene_camera_index < m_scene_camera_count) {
                float const fallback_far = std::max(100.0f, radius * 20.0f);  // file camera without zfar
                built = m_scene_cameras->BuildCamera(m_scene_camera_index, aspect, fallback_far, camera)->ID() == GUIDIr77OperationSucceeded;
            }

            if (!built) camera = MakeControlledCamera(aspect);

            m_camera_view = camera.view;  // where the next switch to orbit / fly starts from
            m_camera_fov = glm::degrees(2.0f * std::atan(1.0f / std::abs(camera.proj[1][1])));  // the lens too

            camera.time = glm::vec4(seconds, seconds - m_last_seconds, static_cast<float>(m_frame_number), 0.0f);

            m_paint->SetFrameData(w, Ir77PVBufferSlot::Camera, &camera, sizeof(camera));
            m_paint->SetFrameData(w, Ir77PVBufferSlot::Instances, m_scene_transforms.data(), m_scene_transforms.size() * sizeof(glm::mat4));
            m_paint->SetFrameData(w, Ir77PVBufferSlot::Lights, m_scene_lights.data(), m_scene_lights.size());
            if (skinned) m_paint->SetFrameData(w, Ir77PVBufferSlot::Bones, m_scene_bones_packed.data(), m_scene_bones_packed.size());

            // one draw item per material run
            std::vector<Ir77PVDrawItem> items{};
            items.reserve(m_scene_runs.size());

            for (Ir77SceneRun const& run : m_scene_runs) {
                // glTF material -> Asset material id; primitives without one use the grey default
                std::uint32_t const material =
                    run.gltf_material < m_scene_material_ids.size() ? m_scene_material_ids[run.gltf_material] : m_scene_default_material;

                Ir77PVDrawItem item{};
                auto const made =
                    m_scene_mesh ? m_asset->MakeMeshletSceneDrawItem(w, m_scene_id, material, run.first_command, run.command_count, item)
                                 : m_asset->MakeIndirectDrawItem(w, m_scene_pipeline, m_scene_id, material, run.first_command, run.command_count, false, item);
                if (made->ID() != GUIDIr77OperationSucceeded) return made;

                items.push_back(item);
            }

            // skinned runs -- Skinned pipeline over the skinned scene's buffers
            for (Ir77SceneRun const& run : m_scene_skinned_runs) {
                std::uint32_t const material =
                    run.gltf_material < m_scene_material_ids.size() ? m_scene_material_ids[run.gltf_material] : m_scene_default_material;

                Ir77PVDrawItem item{};
                auto const made = m_asset->MakeIndirectDrawItem(w, Ir77PVPipelineKind::Skinned, m_scene_skinned_id, material, run.first_command,
                                                                run.command_count, true, item);
                if (made->ID() != GUIDIr77OperationSucceeded) return made;

                items.push_back(item);
            }

            m_paint->SetDraws(w, Ir77PVPass::Opaque, items);
            m_frame_stats.opaque_items = static_cast<std::uint32_t>(items.size());

            // transparent -- one draw per object, back to front from this window's eye (instances within one object keep their order)
            glm::vec3 const eye = glm::vec3(glm::inverse(camera.view)[3]);

            std::vector<float> distance(m_scene_blend_draws.size());
            for (std::size_t d = 0; d < m_scene_blend_draws.size(); d++) {
                glm::vec3 const position = glm::vec3(m_scene_transforms[m_scene_blend_draws[d].first_instance][3]);
                distance[d] = glm::dot(position - eye, position - eye);
            }

            std::vector<std::size_t> order(m_scene_blend_draws.size());
            std::iota(order.begin(), order.end(), 0);
            std::sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) { return distance[a] > distance[b]; });

            std::vector<Ir77PVDrawItem> blend_items{};
            blend_items.reserve(order.size() * 2);

            for (std::size_t const d : order) {
                Ir77SceneBlendDraw const& draw = m_scene_blend_draws[d];
                std::uint32_t const material =
                    draw.gltf_material < m_scene_material_ids.size() ? m_scene_material_ids[draw.gltf_material] : m_scene_default_material;

                std::uint32_t const scene = draw.skinned ? m_scene_skinned_id : m_scene_blend_id;

                // far side first (front faces culled), then near side -- one pipeline, the cull flips per draw
                Ir77PVPipelineKind const kind{draw.skinned ? Ir77PVPipelineKind::SkinnedTransparent : Ir77PVPipelineKind::Transparent};

                for (bool const back_faces : {true, false}) {
                    Ir77PVDrawItem item{};
                    auto const made = m_asset->MakeIndirectDrawItem(w, kind, scene, material, draw.command, 1, draw.skinned, item);
                    if (made->ID() != GUIDIr77OperationSucceeded) return made;

                    item.back_faces = back_faces;
                    blend_items.push_back(item);
                }
            }

            m_paint->SetDraws(w, Ir77PVPass::Transparent, blend_items);
            m_frame_stats.transparent_items = static_cast<std::uint32_t>(blend_items.size());
        }

        m_last_seconds = seconds;
        m_frame_number++;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Once per frame, after Draw -- timings, smoothed fps, active camera, playing clips
    void UpdateFrameStats(float const& dt, std::chrono::steady_clock::time_point const& frame_start, std::chrono::steady_clock::time_point const& draw_start,
                          std::chrono::steady_clock::time_point const& draw_end) {
        auto const ms = [](auto const& from, auto const& to) { return std::chrono::duration<float, std::milli>(to - from).count(); };

        Ir77FrameStats& f = m_frame_stats;
        f.frame++;
        f.dt_ms = dt * 1000.0f;
        if (dt > 0.0f) f.fps = f.fps > 0.0f ? f.fps * 0.9f + (1.0f / dt) * 0.1f : 1.0f / dt;
        f.scene_ms = ms(frame_start, draw_start);
        f.draw_ms = ms(draw_start, draw_end);
        f.camera_index = m_scene_camera_index;
        f.orbit_camera = m_scene_camera_index >= m_scene_camera_count;

        f.playing.clear();
        if (!m_scene_ready) return;

        std::uint32_t count{0};
        m_scene->GetPlayerCount(count);

        for (std::uint32_t p = 0; p < count; p++) {
            Ir77ClipStat clip_stat{};
            std::uint32_t clip{UINT32_MAX};

            if (m_scene->GetPlayer(p, clip, clip_stat.time, clip_stat.duration, clip_stat.speed, clip_stat.loop)->ID() != GUIDIr77OperationSucceeded) continue;
            if (clip < m_scene_stats.clip_names.size()) clip_stat.name = m_scene_stats.clip_names[clip];

            f.playing.push_back(clip_stat);
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // UI commands -- window.cefQuery requests from Angular, JSON: {"camera": "<command>", ...}
    //   move  forward / right / up in -1..1 -- held-key state, applied every frame (0 on key up)
    //   look  dx / dy in pixels             -- mouse drag; turns the fly camera / swings the orbit camera
    //   zoom  delta in wheel steps          -- orbit distance, 10% per step
    //   speed value                         -- multiplies move speed
    //   mode  value "orbit" | "fly" | "file" -- orbit / fly start from the current view, so switching never jumps
    //   next                                -- next file camera, then the controlled camera (F2)
    //   reset                               -- orbit framed on the scene
    // -------------------------------------------------------------------------------------------------------------------------------------
    void ApplyQuery(std::string const& query) {
        simdjson::dom::element root{};
        std::string_view command{};

        if (m_query_parser.parse(query).get(root) != simdjson::SUCCESS || root["camera"].get(command) != simdjson::SUCCESS) {
            std::printf("[WARN] UI query ignored: %s\n", query.c_str());
            return;
        }

        if (!m_query_seen) {
            std::printf("[UI] first query: %s\n", query.c_str());
            m_query_seen = true;
        }

        // driving the camera from a file camera takes it over -- the controlled camera starts from the current view
        if ((command == "move" || command == "look" || command == "zoom") && m_scene_camera_index < m_scene_camera_count) {
            DefineCameraFromView(m_camera_fly);
            m_scene_camera_index = m_scene_camera_count;
        }

        auto const number = [&root](char const* key) {
            double value{0.0};
            if (root[key].get(value) != simdjson::SUCCESS) value = 0.0;  // missing / not a number -> 0
            return static_cast<float>(value);
        };

        if (command == "move") {
            m_camera_move = glm::clamp(glm::vec3(number("right"), number("up"), number("forward")), glm::vec3(-1.0f), glm::vec3(1.0f));
        } else if (command == "look") {
            m_camera_yaw -= number("dx") * CAMERA_LOOK_RADIANS_PER_PIXEL;
            m_camera_pitch = glm::clamp(m_camera_pitch - number("dy") * CAMERA_LOOK_RADIANS_PER_PIXEL, -CAMERA_PITCH_LIMIT, CAMERA_PITCH_LIMIT);
        } else if (command == "zoom") {
            m_camera_distance = std::max(m_camera_distance * std::pow(1.1f, -number("delta")), 0.01f);
        } else if (command == "speed") {
            float const factor = number("value");
            if (factor > 0.0f) m_camera_speed *= factor;
        } else if (command == "mode") {
            std::string_view mode{};
            if (root["value"].get(mode) != simdjson::SUCCESS) mode = {};

            if (mode == "orbit" || mode == "fly") {
                DefineCameraFromView(mode == "fly");
                m_scene_camera_index = m_scene_camera_count;
            } else if (mode == "file" && m_scene_camera_count > 0) {
                m_scene_camera_index = 0;
            }
        } else if (command == "next") {
            m_scene_camera_index = (m_scene_camera_index + 1) % (m_scene_camera_count + 1);
        } else if (command == "reset") {
            ResetCameraControl();
            m_scene_camera_index = m_scene_camera_count;
        } else {
            std::printf("[WARN] UI camera command unknown: %s\n", query.c_str());
        }
    }

    // Orbit on the scene: target its centre, back off to frame its bounds, a little above
    void ResetCameraControl() {
        float const radius = std::max(glm::length(m_scene_max - m_scene_min) * 0.5f, 0.01f);

        m_camera_fly = false;
        m_camera_target = (m_scene_min + m_scene_max) * 0.5f;
        m_camera_distance = radius * 2.5f;
        m_camera_yaw = glm::pi<float>();  // looking down -Z, from the +Z side
        m_camera_pitch = -0.35f;
        m_camera_speed = radius * 0.5f;  // units per second at full key
        m_camera_move = glm::vec3(0.0f);
    }

    // Takes over from whatever was on screen last frame -- file camera, orbit or fly
    void DefineCameraFromView(bool const fly) {
        glm::mat4 const world = glm::inverse(m_camera_view);
        glm::vec3 const eye = glm::vec3(world[3]);
        glm::vec3 const forward = glm::normalize(-glm::vec3(world[2]));  // the view looks down its -Z

        m_camera_fly = fly;
        m_camera_yaw = std::atan2(forward.x, forward.z);
        m_camera_pitch = glm::clamp(std::asin(glm::clamp(forward.y, -1.0f, 1.0f)), -CAMERA_PITCH_LIMIT, CAMERA_PITCH_LIMIT);

        if (fly) {
            m_camera_position = eye;
        } else {
            m_camera_distance = std::max(glm::length(m_camera_target - eye), 0.01f);
            m_camera_target = eye + forward * m_camera_distance;  // keep the view; orbit around the point straight ahead
        }
    }

    // Held keys, every frame. Fly: walk along the view. Orbit: forward dollies in / out, right / up pan the target.
    void UpdateCameraControl(float const dt) {
        if (m_camera_move == glm::vec3(0.0f) || dt <= 0.0f) return;

        glm::vec3 const forward = CameraForward();
        glm::vec3 const right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 const up = glm::cross(right, forward);

        float const step = m_camera_speed * dt;

        if (m_camera_fly) {
            m_camera_position += (right * m_camera_move.x + up * m_camera_move.y + forward * m_camera_move.z) * step;
        } else {
            m_camera_target += (right * m_camera_move.x + up * m_camera_move.y) * step;
            m_camera_distance = std::max(m_camera_distance * std::exp(-m_camera_move.z * dt), 0.01f);
        }
    }

    glm::vec3 CameraForward() const {
        return glm::vec3(std::cos(m_camera_pitch) * std::sin(m_camera_yaw), std::sin(m_camera_pitch), std::cos(m_camera_pitch) * std::cos(m_camera_yaw));
    }

    // The controlled camera's matrices. Planes hug the scene: whole bounds in front of far, near as large as depth precision allows.
    Ir77PVCamera MakeControlledCamera(float const aspect) const {
        glm::vec3 const center = (m_scene_min + m_scene_max) * 0.5f;
        float const radius = std::max(glm::length(m_scene_max - m_scene_min) * 0.5f, 0.01f);

        glm::vec3 const forward = CameraForward();
        glm::vec3 const eye = m_camera_fly ? m_camera_position : m_camera_target - forward * m_camera_distance;

        float const zfar = glm::length(eye - center) + radius * 2.0f;
        float const znear = std::max(zfar * 0.0001f, 0.001f);

        return Ir77PVAsset::MakeCamera(eye, eye + forward, m_camera_fov, aspect, znear, zfar);
    }

    // The scene stats as console lines -- the printf replacement until the app displays them
    void PrintSceneStats() const {
        Ir77SceneStats const& s = m_scene_stats;

        std::printf("[STATS] %s\n", s.path.c_str());
        std::printf("[STATS] gpu %s, %s\n", s.gpu.c_str(), s.render_path.c_str());
        std::printf("[STATS] nodes %u, meshes %u, materials %u, textures %u, cameras %u, skins %u, bones %u, clips %u\n", s.nodes, s.meshes, s.materials,
                    s.textures, s.cameras, s.skins, s.bones, s.clips);
        std::printf("[STATS] static %llu verts / %u prims, skinned %llu verts / %u prims, %u instances\n", static_cast<unsigned long long>(s.static_vertices),
                    s.static_primitives, static_cast<unsigned long long>(s.skinned_vertices), s.skinned_primitives, s.instances);
        std::printf("[STATS] runs: %u opaque, %u skinned; %u transparent objects\n", s.opaque_runs, s.skinned_runs, s.transparent_objects);

        for (std::size_t l = 0; l < s.lights.size(); l++)
            std::printf("[STATS] light %zu: type %d, intensity %.2f, pos (%.2f, %.2f, %.2f)\n", l, s.lights[l].type, s.lights[l].intensity,
                        s.lights[l].position.x, s.lights[l].position.y, s.lights[l].position.z);

        for (std::size_t c = 0; c < s.clip_names.size(); c++) std::printf("[STATS] clip %zu: '%s'\n", c, s.clip_names[c].c_str());

        for (std::string const& warning : s.warnings) std::printf("[WARN] %s\n", warning.c_str());
    }

    void Require(std::shared_ptr<IIr77Return const> const& result, char const* step = "") {
        if (result->ID() != GUIDIr77OperationSucceeded) {
            std::cerr << "Iridium: init step failed: " << step << "\n";
            throw std::runtime_error{step};
        }
    }

   private:
    // [UI] diagnostics -- remove once the blank-UI case is found
    std::uint32_t m_ui_paints{0};
    std::uint32_t m_ui_uploads{0};
    std::uint32_t m_ui_upload_failed{0};
    std::uint32_t m_ui_dropped_stale{0};
    std::uint32_t m_ui_dropped_resizing{0};
    std::string m_ui_last_paint{"none"};
    std::chrono::steady_clock::time_point m_ui_last_status{};
    std::uint32_t m_ui_kicked{0};

    // one material run -- contiguous indirect commands sharing a glTF material
    struct Ir77SceneRun {
        std::uint32_t gltf_material{UINT32_MAX};
        std::uint32_t first_command{0};
        std::uint32_t command_count{0};
    };

    // one transparent object -- a single indirect command, sorted by camera distance every frame
    struct Ir77SceneBlendDraw {
        std::uint32_t gltf_material{UINT32_MAX};
        std::uint32_t command{0};         // in the static blend scene, or in the skinned scene when skinned
        std::uint32_t first_instance{0};  // models[] slot -- its translation is the sort position
        bool skinned{false};
    };

    std::shared_ptr<CEFMessageLoop> m_message_loop;

    std::shared_ptr<CEFRTState> m_rt_state;

    std::shared_ptr<Ir77PeregrineV> m_vulkan;

    std::shared_ptr<Ir77PVCEF> m_cef;

    std::shared_ptr<Ir77PVPaint> m_paint;

    std::shared_ptr<Ir77PVAsset> m_asset;

    // one id for both the window map and SetCurrentDevice -- they must match
    std::uint64_t m_device_id{0};

    std::uint32_t m_frame{0};  // frame in flight, for Asset::Tick

    float m_last_seconds{0.0f};  // camera.time.y

    std::uint32_t m_frame_number{0};  // camera.time.z

    float m_vstep_seconds{0.0f};  // previous frame's clock, for the scene's dt

    // scene
    std::string m_scene_path{"/home/alpha/workspace/iridium/Public/sheet.glb"};  // set to your model

    std::shared_ptr<Ir77Scene> m_scene{};

    std::vector<Ir77SceneRun> m_scene_runs{};

    std::vector<Ir77SceneRun> m_scene_skinned_runs{};  // commands index the skinned scene's buffers

    bool m_upload_error_logged{false};

    Ir77SceneStats m_scene_stats{};  // filled by LoadScene

    Ir77FrameStats m_frame_stats{};  // refreshed every frame

    std::uint32_t m_scene_skinned_id{0};  // UploadScene id of the skinned stream

    bool m_scene_has_skinned{false};  // skinned scene uploaded (opaque or transparent)

    std::vector<Ir77SceneBlendDraw> m_scene_blend_draws{};

    std::uint32_t m_scene_blend_id{0};  // UploadScene id of the compacted static transparent scene

    std::vector<glm::mat4> m_scene_bones{};

    std::vector<std::uint32_t> m_scene_bone_bases{};

    std::vector<std::uint8_t> m_scene_bones_packed{};  // bone_base[8192] + palettes, as set 0 binding 4 expects

    std::vector<glm::mat4> m_scene_transforms{};

    std::vector<std::uint8_t> m_scene_lights{};  // Ir77PVLightsHeader + Ir77PVLight[], as set 0 binding 1 expects

    std::shared_ptr<IIr77Camera> m_scene_cameras{};

    std::uint32_t m_scene_camera_count{0};

    std::uint32_t m_scene_camera_index{0};  // == m_scene_camera_count -> the controlled camera; F2 / "next" cycles

    // controlled camera -- orbit (target, distance) or fly (position); yaw / pitch aim both
    bool m_camera_fly{false};

    glm::vec3 m_camera_target{0.0f};

    glm::vec3 m_camera_position{0.0f};

    float m_camera_distance{1.0f};

    float m_camera_yaw{0.0f};

    float m_camera_pitch{0.0f};

    float m_camera_speed{1.0f};  // units per second at full key

    glm::vec3 m_camera_move{0.0f};  // held keys: right, up, forward in -1..1

    glm::mat4 m_camera_view{1.0f};  // last frame's view, whichever camera drew it

    float m_camera_fov{60.0f};  // vertical, degrees -- taken from whichever camera drew last, so a takeover keeps the lens

    static constexpr float CAMERA_LOOK_RADIANS_PER_PIXEL{0.004f};

    static constexpr float CAMERA_PITCH_LIMIT{1.5f};  // just short of straight up / down

    // UI queries -- drained each frame by IridiumStep
    std::vector<std::string> m_queries{};

    simdjson::dom::parser m_query_parser{};

    bool m_query_seen{false};  // [UI] first query logged

    glm::vec3 m_scene_min{-1.0f};

    glm::vec3 m_scene_max{1.0f};

    std::uint32_t m_scene_id{0};  // UploadScene id, or UploadMeshlets id when m_scene_mesh

    std::uint32_t m_scene_default_material{0};

    std::vector<std::uint32_t> m_scene_material_ids{};  // [glTF material] -> Asset material id

    bool m_scene_mesh{false};

    Ir77PVPipelineHandle m_scene_pipeline{IR77PV_NO_PIPELINE};  // what the static scene draws with

    bool m_scene_ready{false};

    SDL_Window* m_window{nullptr};

    std::map<std::uint64_t, std::vector<SDL_Window*>> m_windows;

    bool running = true;

    int m_window_width{1280};

    int m_window_height{720};

    int m_cef_width{1280};

    int m_cef_height{720};

    int m_pending_width{1280};

    int m_pending_height{720};

    bool m_resize_pending = false;

    bool m_cef_in_flight{false};  // a size was sent to CEF and its paint at that size hasn't arrived yet

    bool m_cef_repaint{false};

    static constexpr long long CEF_RESIZE_TIMEOUT_MS{100};  // give up waiting for CEF's paint at the requested size after this long

    bool m_render_ready{false};  // Vulkan + Paint fully set up

    std::chrono::steady_clock::time_point m_last_cef_resize{};

    std::chrono::steady_clock::time_point m_last_resize_event{};

    std::chrono::steady_clock::time_point m_start_time{std::chrono::steady_clock::now()};
};

}  // namespace Ir77