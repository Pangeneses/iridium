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
#include "../PeregrineV/server/Ir77PVPaint.hpp"
#include "../PeregrineV/server/Ir77PVAsset.hpp"

#include "../PeregrineV/interface/IIr77PVDevice.hpp"

using namespace CEF;
using namespace NSIr77PeregrineV;

namespace Ir77 {

class iridium {
   public:
    iridium() {
        m_message_loop = std::make_shared<CEFMessageLoop>();
        m_rt_state = std::make_shared<CEFRTState>();
    }

    // iridium.hpp — InitCEF 
    void InitializePipeline(int argc, char* argv[]) {
        //m_rt_state->Initialize(argc, argv);
        
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

        m_vulkan->CreateLayout();

        m_vulkan->CreateRenderPass();

        m_vulkan->CreateSwapchains();

        m_asset->CreateShaders();

        m_vulkan->CreatePipelineGFX();

        m_paint->CreateCommandBuffers();

        // m_rt_state->GetRenderHandler()->SetPaintCallback([this](const void* buffer, int w, int h) { m_adapter->Ir77VulkanUploadCEF(buffer, w, h); });

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
        m_paint->Next();

        m_paint->Draw();
    }

    void HUD() {}

    void Composition() {}

    void VulkanFrameEnd() { //m_adapter->Ir77VulkanFrame(); 
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

    std::shared_ptr<IIr77PVPaint> m_paint;

    std::shared_ptr<IIr77PVAsset> m_asset;

    std::map<std::uint64_t, std::shared_ptr<IIr77PVDevice>> m_devices;

    SDL_Window* m_window;

    std::map<std::uint64_t, std::vector<SDL_Window*>> m_windows;

    bool running = true;
};

}  // namespace Ir77