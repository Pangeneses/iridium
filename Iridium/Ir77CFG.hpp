#pragma once
#include <fstream>
#include <string>

namespace NSIr77RT {
/*
#include "nlohmann/json.hpp"

enum class Ir77ServerMode { Client, Server, SelfServe };

inline Ir77ServerMode g_server_mode{Ir77ServerMode::SelfServe};
inline std::string g_server_host{"127.0.0.1"};
inline uint16_t g_server_port{50001};

inline void LoadConfig(std::string const& path) {
    std::ifstream f{path};
    if (!f) return;

    nlohmann::json cfg = nlohmann::json::parse(f, nullptr, false);
    if (cfg.is_discarded()) return;

    auto& ir = cfg["iridium"];

    std::string mode = ir.value("mode", "selfserve");
    if (mode == "server")      g_server_mode = Ir77ServerMode::Server;
    else if (mode == "client") g_server_mode = Ir77ServerMode::Client;
    else                       g_server_mode = Ir77ServerMode::SelfServe;

    if (ir.contains("server")) {
        g_server_host = ir["server"].value("host", "127.0.0.1");
        g_server_port = ir["server"].value("port", 50001);
    }
}

inline bool IsServer()    { return g_server_mode == Ir77ServerMode::Server; }
inline bool IsClient()    { return g_server_mode == Ir77ServerMode::Client; }
inline bool IsSelfServe() { return g_server_mode == Ir77ServerMode::SelfServe; }
*/
}  // namespace NSIr77RT