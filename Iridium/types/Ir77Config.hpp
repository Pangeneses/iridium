#pragma once

#include <cstdint>
#include <string>

namespace Ir77 {

// Command line. Each field that is set overrides the same setting from iridium.json.
//   --config=<path>  --url=<url>  --debug-port=<n>  --log=<path>  --debug
typedef struct Ir77Args {
    std::string config{};
    std::string url{};
    std::string log{};
    bool debug{false};
    std::uint16_t debug_port{0};
}* pIr77Args;

// How this instance takes part in the network -- iridium.mode
enum class Ir77ServerMode : std::uint8_t { SelfServe, Server, Client };

// iridium.json -- this machine's settings, read-only. Everything here has a default, so Iridium runs without the file.
// Paths are resolved against the folder the file is in (the executable's folder by default) and stored absolute.
typedef struct Ir77Config {
    std::string path{};  // the file these came from -- empty when built-in defaults are in use

    // window
    std::string title{"Iridium"};
    int width{1280};
    int height{720};

    // ui -- the Angular app CEF opens, and where DevTools listens
    std::string url{"http://localhost:50003/landing"};
    std::uint16_t debug_port{9222};

    // render
    std::string shader_root{"Shaders"};  // engine shaders + pipelines.json
    bool mesh_path{false};               // static scenes on the task / mesh path when the GPU has it

    // iridium -- network role
    Ir77ServerMode mode{Ir77ServerMode::SelfServe};
    std::string server_host{"127.0.0.1"};
    std::uint16_t server_port{50001};

    // projects -- the folder unpacked .ir77 projects live in (each: Shaders/, Models/, Models/Textures/)
    std::string projects{"Projects"};

    // startup -- what to show until the Angular app opens a project itself
    std::string startup_project{};   // folder name under projects
    std::string startup_scene{};     // file under that project's Models/
    std::string startup_pipeline{};  // manifest pipeline for the scene's static runs -- empty = the "static" role
}* pIr77Config;

}  // namespace Ir77