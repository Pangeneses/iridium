#pragma once

#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../../Ir77RT/runtime/Ir77Return.hpp"

#include "../Windows/runtime/CEFMessageLoop.hpp"
#include "../Windows/runtime/CEFApp.hpp"
#include "../Windows/runtime/CEFRTState.hpp"
#include "../Windows/runtime/CEFClient.hpp"
#include "../Windows/runtime/CEFMouseEvent.hpp"

#include "../PeregrineV/dictionary/IDIr77PVContext.hpp"

#include "../PeregrineV/server/Ir77PeregrineV.hpp"
#include "../PeregrineV/server/Ir77PVCEF.hpp"
#include "../PeregrineV/server/Ir77PVPaint.hpp"
#include "../PeregrineV/server/Ir77PVAsset.hpp"

#include "../PeregrineV/interface/IIr77PVDevice.hpp"
#include "Ir77PVTypes.hpp"

using namespace CEF;
using namespace NSIr77PeregrineV;

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

        std::vector<SDL_Window*> windows{m_window};

        m_windows.emplace(ID_DEVICE_002, windows);

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
        m_vulkan->SetCurrentDevice(ID_DEVICE_002);
        m_vulkan->CreateSurfaces(m_windows);
        m_vulkan->EnumerateDeviceQueues();
        m_vulkan->CreateLogicalDevices();
        m_vulkan->CreateAllocator();

        // layouts + passes (Main only; Shadow pipelines are skipped until CreateRenderPass(Shadow) is called)
        Require(m_vulkan->CreateLayouts());
        Require(m_vulkan->CreateRenderPasses());
        Require(m_vulkan->CreateSwapchains());

        // frame resources -- the placeholder texture must exist before the descriptor sets and materials bind it
        Require(m_vulkan->CreatePlaceholderTexture());
        Require(m_vulkan->CreateFrameBuffers());
        Require(m_vulkan->CreateDescriptorSets());

        Require(m_asset->CreateTestbed());

        // CEF overlay -- one per window, uses the CEF layout from CreateLayouts and each window's swapchain
        Require(m_cef->CreateOverlays());

        std::shared_ptr<IIr77PVOverlay> overlay{};
        Require(m_cef->GetOverlay(0, overlay), "Get overlay.");

        std::weak_ptr<IIr77PVOverlay> weak_overlay = overlay;

        m_rt_state->GetRenderHandler()->SetPaintCallback([weak_overlay](const void* buffer, int w, int h) {
            if (auto const locked = weak_overlay.lock()) locked->UploadFrame(buffer, w, h);
        });

        // pipelines need shaders, layouts and the Main pass
        Require(m_asset->CreateShaders(), "Create shaders.");

        Require(m_vulkan->CreatePipelines());

        // command buffers wire the overlay, so they come after CreateBufferCEF and CreatePipelines
        Require(m_paint->CreateCommandBuffers());

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
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_resize_event).count();

            if (elapsed > 10) {
                m_resize_pending = false;

                m_cef_width = m_pending_width;
                m_cef_height = m_pending_height;

                m_rt_state->GetRenderHandler()->SetViewSize(m_cef_width, m_cef_height);
                m_rt_state->GetBrowser()->GetHost()->WasResized();
                m_rt_state->GetBrowser()->GetHost()->Invalidate(PET_VIEW);
            }
        }
    }

    void ChromeStep() { m_message_loop->CEFDoMessageLoop(m_rt_state->GetBrowser()); }

    void IridiumStep() {}

    void VulkanStep() {}

    void VulkanFrameStart() {
        float const seconds = std::chrono::duration<float>(std::chrono::steady_clock::now() - m_start_time).count();

        m_asset->UpdateTestbed(m_paint, seconds);

        m_paint->Draw();
    }

    void HUD() {}

    void Composition() {}

    void VulkanFrameEnd() {}

    void Shutdown() {
        m_rt_state->GetRenderHandler()->SetPaintCallback(nullptr);
        m_rt_state->DestroyRTState();

        if (m_window) {
            SDL_DestroyWindow(m_window);
            m_window = nullptr;
        }

        SDL_Quit();
    }

   private:
    void Require(std::shared_ptr<IIr77Return const> const& result, char const* step = "") {
        if (result->ID() != GUIDIr77OperationSucceeded) {
            std::cerr << "Iridium: init step failed: " << step << "\n";
            throw std::runtime_error{step};
        }
    }

   private:
    std::shared_ptr<CEFMessageLoop> m_message_loop;

    std::shared_ptr<CEFRTState> m_rt_state;

    std::shared_ptr<Ir77PeregrineV> m_vulkan;

    std::shared_ptr<Ir77PVCEF> m_cef;

    std::shared_ptr<Ir77PVPaint> m_paint;

    std::shared_ptr<Ir77PVAsset> m_asset;

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

    std::chrono::steady_clock::time_point m_last_cef_pump{};

    std::chrono::steady_clock::time_point m_last_cef_resize{};

    std::chrono::steady_clock::time_point m_last_resize_event{};

    std::chrono::steady_clock::time_point m_start_time{std::chrono::steady_clock::now()};
};

}  // namespace Ir77