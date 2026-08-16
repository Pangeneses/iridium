#pragma once

#include <string>
#include <iostream>
#include <memory>
#include <stdexcept>

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>

#include "../Windows/runtime/CEFMessageLoop.hpp"
#include "../Windows/runtime/CEFApp.hpp"
#include "../Windows/runtime/CEFRTState.hpp"
#include "../Windows/runtime/CEFClient.hpp"
#include "../Windows/runtime/CEFMouseEvent.hpp"

// #include "../REDOS/service/Ir77REDOS.hpp"
// #include "../PeregrineV/service/Ir77PeregrineV.hpp"

#include "../../Ir77RT/runtime/Ir77Return.hpp"

using namespace CEF;
// using namespace NSIr77REDOS;

namespace Ir77 {

class iridium {
   public:
    iridium() {
        m_message_loop = std::make_shared<CEFMessageLoop>();
        m_rt_state = std::make_shared<CEFRTState>();
    }

    // iridium.hpp — InitCEF
    void InitCEF(int argc, char* argv[]) {
        m_rt_state->Initialize(argc, argv);
        Ir77RETURN<Ir77OperationSucceeded>(nullptr, "Succeeded: InitCEF");
    }

    void InitWindow() {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            Ir77RETURN<Ir77OperationFailed>(nullptr, "SDL_Init failed: " + std::string{SDL_GetError()});
            throw std::runtime_error{"SDL_Init failed: " + std::string{SDL_GetError()}};
        }

        m_window = SDL_CreateWindow("Iridium", 1280, 720, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

        if (!m_window) {
            Ir77RETURN<Ir77OperationFailed>(nullptr, "SDL_CreateWindow failed: " + std::string{SDL_GetError()});
            throw std::runtime_error{"SDL_CreateWindow failed: " + std::string{SDL_GetError()}};
        }

        Ir77RETURN<Ir77OperationSucceeded>(nullptr, "Succeeded: SDL");

        running = true;
    }

    void InitVulkan() {
        m_adapter = std::make_shared<IIr77PVRenderer>();

        m_adapter->SetWindow(m_window);

        m_rt_state->GetRenderHandler()->SetPaintCallback([this](const void* buffer, int w, int h) { m_adapter->Ir77VulkanUploadCEF(buffer, w, h); });

        Ir77RETURN<Ir77OperationSucceeded>(nullptr, "Initialize adapter.");

        m_adapter->Initialize();
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

    void VulkanFrameStart() {}

    void HUD() {}

    void Composition() {}

    void VulkanFrameEnd() { m_adapter->Ir77VulkanFrame(); }

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

    SDL_Window* m_window = nullptr;

    std::shared_ptr<IIr77PVRenderer> m_adapter;

    bool running = false;
};

}  // namespace Ir77