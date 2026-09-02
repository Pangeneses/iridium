#pragma once

#include <string>
#include <iostream>
#include <memory>
#include <stdexcept>

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>

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

    bool IsResizing() {
        bool resizing;
        m_vulkan->IsResizing(resizing);

        return resizing;
    }

    Ir77PVInputBuffer BuildTestTriangle() {
        Ir77PVInputBuffer buffer{};

        buffer.vertex_data = {
            {{0.0f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 0.0f}},
            {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
            {{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        };

        static std::vector<std::uint32_t> indices = {2, 1, 0};

        return buffer;
    }

    Ir77PVCameraUBO BuildTestCamera(std::uint32_t width, std::uint32_t height) {
        Ir77PVCameraUBO camera{};

        float aspect = static_cast<float>(width) / static_cast<float>(height);

        camera.projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        camera.projection[1][1] *= -1.0f; 

        camera.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f),
                                  glm::vec3(0.0f, 0.0f, 0.0f),
                                  glm::vec3(0.0f, 1.0f, 0.0f)
        );

        camera.model = glm::mat4(1.0f);  // identity — no transform for this single test object

        return camera;
    }

    // iridium.hpp — InitCEF
    void InitializePipeline(int argc, char* argv[]) {
        // m_rt_state->Initialize(argc, argv);

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

        m_windows.emplace(ID_DEVICE_001, windows);

        auto peregrinev = std::make_shared<Ir77PeregrineV>();

        m_vulkan = peregrinev;

        m_cef = std::make_shared<Ir77PVCEF>();

        m_cef->SetPeregrineV(peregrinev);

        m_paint = std::make_shared<Ir77PVPaint>();

        m_paint->SetPeregrineV(peregrinev);

        m_asset = std::make_shared<Ir77PVAsset>();

        m_asset->SetPeregrineV(peregrinev);

        m_vulkan->CreateInstance();

        m_vulkan->SetCurrentDevice(ID_DEVICE_001);

        m_vulkan->EnumeratePhysicalDevices(m_devices);

        m_vulkan->CreateSurfaces(m_windows);

        m_vulkan->EnumerateDeviceQueues();

        m_vulkan->CreateLogicalDevices();

        m_vulkan->CreateAllocator();

        std::vector<Ir77PVInputBuffer> buffers{BuildTestTriangle()};

        m_asset->UploadVertexBuffer(buffers);

        m_vulkan->CreateLayoutUBO();

        m_asset->CreateBuffersUBO();

        m_vulkan->CreateRenderPass();

        m_vulkan->CreateSwapchains();

        m_cef->CreateLayoutCEF();

        m_cef->CreateBufferCEF();

        std::shared_ptr<Ir77PVBufferCEF> cef_buffer;
        m_cef->GetBufferCEF(0, cef_buffer);

        // m_rt_state->GetRenderHandler()->SetPaintCallback([cef_buffer](const void* buffer, int w, int h) { cef_buffer->UploadFrame(buffer, w, h); });

        m_asset->CreateShaders();

        m_vulkan->CreatePipelineGFX();

        m_vulkan->CreatePipelineCEF();

        m_paint->CreateCommandBuffers();

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
                    m_pending_width = event.window.data1;
                    m_pending_height = event.window.data2;
                    break;
                default:
                    CEFInputEvent(&event, m_rt_state->GetBrowser());
                    break;
            }
        }
    }

    void ChromeStep() { m_message_loop->CEFDoMessageLoop(m_rt_state->GetBrowser()); }

    void IridiumStep() {}

    void VulkanStep() {}

    void VulkanFrameStart() {
        auto camera_data = BuildTestCamera(m_pending_width, m_pending_height);
        m_asset->UpdateBuffersUBO({&camera_data}, {sizeof(camera_data)});

        m_paint->Draw();
    }

    void HUD() {}

    void Composition() {}

    void VulkanFrameEnd() {  // m_adapter->Ir77VulkanFrame();
    }

    void Shutdown() {
        m_rt_state->DestroyRTState();

        if (m_window) {
            SDL_DestroyWindow(m_window);
            m_window = nullptr;
        }

        SDL_Quit();
    }

   private:
    std::shared_ptr<CEFMessageLoop> m_message_loop;

    std::shared_ptr<CEFRTState> m_rt_state;

    std::shared_ptr<IIr77PeregrineV> m_vulkan;

    std::shared_ptr<IIr77PVCEF> m_cef;

    std::shared_ptr<IIr77PVPaint> m_paint;

    std::shared_ptr<IIr77PVAsset> m_asset;

    std::map<std::uint64_t, std::shared_ptr<IIr77PVDevice>> m_devices;

    SDL_Window* m_window;

    std::map<std::uint64_t, std::vector<SDL_Window*>> m_windows;

    bool running = true;

    int m_pending_width{1280};

    int m_pending_height{720};

    std::chrono::steady_clock::time_point m_last_resize_event;

    static constexpr int RESIZE_SETTLE_MS = 100;
};

}  // namespace Ir77