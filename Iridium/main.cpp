#include <stdexcept>
#include <string>
#include <iostream>
#include <limits.h>
#include <stdlib.h>
#include <chrono>
#include <thread>

#include "../Ir77RT/dictionary/IDIr77RET.hpp"
#include "include/internal/cef_linux.h"
#include "include/cef_app.h"

#include "./iridium.hpp"

#include "../Ir77RT/runtime/Ir77Return.hpp"

struct IridiumArgs {
    std::string url{};
    std::string config{};
    std::string log{};
    bool debug{false};
    uint16_t debug_port{};
};

static IridiumArgs ParseArgs(int argc, char* argv[]) {
    IridiumArgs args{};
    for (int i = 1; i < argc; ++i) {
        std::string arg{argv[i]};
        if (arg.starts_with("--url="))
            args.url = arg.substr(6);
        else if (arg.starts_with("--config="))
            args.config = arg.substr(9);
        else if (arg.starts_with("--log="))
            args.log = arg.substr(6);
        else if (arg == "--debug")
            args.debug = true;
        else if (arg.starts_with("--debug-port="))
            args.debug_port = static_cast<uint16_t>(std::stoul(arg.substr(13)));
    }
    return args;
}

int main(int argc, char* argv[]) {
    Ir77::iridium iridium{};

    Ir77RetLog::Clear();
    Ir77RetLog::Enable();
    {
        int fd = ::open("/tmp/Ir77RetLog.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
        const char* msg = "[raw] main started\n";
        ::write(fd, msg, strlen(msg));
        ::close(fd);
    }

    try {
        iridium.InitCEF(argc, argv);
    } catch (std::runtime_error& error) {
        std::string what{error.what()};
        if (what.starts_with("__cef_subprocess__:")) {
            std::string code_str = what.substr(19);
            if (!code_str.empty()) {
                try {
                    return std::stoi(code_str);
                } catch (...) {
                    return 0;
                }
            }
            return 0;
        }
        // Ir77RETURN<Ir77OperationFailed>(nullptr, "Failed: InitCEF");
        return 1;
    }

    IridiumArgs args = ParseArgs(argc, argv);

    try {
        iridium.InitWindow();
    } catch (std::runtime_error error) {
        // Ir77RETURN<Ir77OperationFailed>(nullptr, "Failed: InitWindow");
        return 1;
    }

    try {
        iridium.InitVulkan();
    } catch (std::runtime_error error) {
        // Ir77RETURN<Ir77OperationFailed>(nullptr, "Failed: InitVulkan");
        return 1;
    }

    while (iridium.IsRunning()) {
        auto frame_start = std::chrono::steady_clock::now();

        iridium.WindowStep();
        iridium.ChromeStep();
        iridium.IridiumStep();
        iridium.VulkanStep();

        iridium.VulkanFrameStart();
        iridium.HUD();
        iridium.Composition();
        iridium.VulkanFrameEnd();

        auto frame_end = std::chrono::steady_clock::now();

        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(frame_end - frame_start).count();

        if (elapsed < 5000) std::this_thread::sleep_for(std::chrono::microseconds(5000 - elapsed));
    }

    iridium.Shutdown();

    return 0;
}