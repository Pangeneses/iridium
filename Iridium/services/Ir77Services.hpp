#pragma once

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <simdjson.h>

#include "../types/Ir77Config.hpp"

namespace NSIr77 {

// =============================================================================================================================================
// Ir77Service -- a program Iridium runs for as long as it runs itself: MongoDB for the projects, Sapphire (the store Aurum talks to). Nothing
// is installed or left running -- started after iridium.json is read, stopped on exit, and killed by the kernel if Iridium dies first.
//
//   Start()   -- port already answering -> someone else's (a system mongod while developing): used, never stopped. Otherwise spawned:
//                its own process group, stdout / stderr to its log, SIGTERM to it if Iridium exits without stopping it.
//   Check()   -- once a frame, never blocks: reports ready the first time the port answers, and an exit that wasn't asked for.
//   Stop()    -- SIGTERM (a clean shutdown for mongod and node), then SIGKILL after stop_ms.
//
// Linux only (fork / prctl). Not copyable -- it owns a process.
// =============================================================================================================================================

class Ir77Service {
   public:
    Ir77Service() = default;
    Ir77Service(Ir77Service const&) = delete;
    Ir77Service& operator=(Ir77Service const&) = delete;

    ~Ir77Service() { Stop(); }

    bool Start(Ir77ServiceConfig const& config, std::string& error) {
        m_config = config;
        m_ready = false;
        m_reported_exit = false;

        if (!m_config.enabled) return true;

        if (Answers(m_config.port)) {
            m_ready = true;
            std::printf("[SERVICES] %s -- port %u already answers, using that one (not started, not stopped by Iridium)\n", m_config.name.c_str(),
                        static_cast<unsigned>(m_config.port));
            return true;
        }

        std::string const program = Resolve(m_config.binary);
        if (program.empty()) {
            error = m_config.name + ": no program '" + m_config.binary + "' (not a file, not on PATH)";
            return false;
        }

        std::error_code ec{};
        for (std::string const& folder : m_config.folders) std::filesystem::create_directories(folder, ec);
        if (!m_config.log.empty()) std::filesystem::create_directories(std::filesystem::path{m_config.log}.parent_path(), ec);

        // everything the child needs, built before fork -- between fork and exec only async-signal-safe calls (CEF has threads running)
        std::vector<std::string> argv_text{program};
        argv_text.insert(argv_text.end(), m_config.args.begin(), m_config.args.end());

        std::vector<std::string> env_text{};
        for (char** e = environ; e && *e; e++) {
            std::string_view const entry{*e};
            std::string_view const key = entry.substr(0, entry.find('='));
            bool const replaced =
                std::any_of(m_config.env.begin(), m_config.env.end(), [&](std::string const& mine) { return std::string_view{mine}.substr(0, mine.find('=')) == key; });
            if (!replaced) env_text.emplace_back(entry);
        }
        env_text.insert(env_text.end(), m_config.env.begin(), m_config.env.end());

        std::vector<char*> argv{};
        for (std::string& a : argv_text) argv.push_back(a.data());
        argv.push_back(nullptr);

        std::vector<char*> envp{};
        for (std::string& e : env_text) envp.push_back(e.data());
        envp.push_back(nullptr);

        char const* const cwd = m_config.cwd.empty() ? nullptr : m_config.cwd.c_str();
        char const* const log = m_config.log.empty() ? nullptr : m_config.log.c_str();
        pid_t const parent = getpid();

        pid_t const pid = fork();
        if (pid < 0) {
            error = m_config.name + ": fork failed (" + std::strerror(errno) + ")";
            return false;
        }

        if (pid == 0) {
            prctl(PR_SET_PDEATHSIG, SIGTERM);  // Iridium gone (crash included) -> this goes too
            if (getppid() != parent) _exit(127);  // it went before the line above took effect
            setpgid(0, 0);                        // a Ctrl-C at Iridium's terminal is Iridium's to handle, not this one's

            if (cwd && chdir(cwd) != 0) _exit(126);

            if (log) {
                int const fd = open(log, O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0644);
                if (fd >= 0) {
                    dup2(fd, STDOUT_FILENO);
                    dup2(fd, STDERR_FILENO);
                }
            }

            int const null_fd = open("/dev/null", O_RDONLY | O_CLOEXEC);
            if (null_fd >= 0) dup2(null_fd, STDIN_FILENO);

            execve(argv[0], argv.data(), envp.data());
            _exit(127);
        }

        m_pid = pid;
        m_started = std::chrono::steady_clock::now();

        std::printf("[SERVICES] %s -- started, pid %d, waiting on port %u%s%s\n", m_config.name.c_str(), static_cast<int>(pid),
                    static_cast<unsigned>(m_config.port), log ? ", log " : "", log ? log : "");
        return true;
    }

    // Once a frame -- cheap, never blocks
    void Check() {
        if (m_pid <= 0) return;

        int status{0};
        if (waitpid(m_pid, &status, WNOHANG) == m_pid) {
            if (!m_reported_exit)
                std::fprintf(stderr, "[SERVICES] %s -- exited (%s %d) -- see %s\n", m_config.name.c_str(), WIFEXITED(status) ? "code" : "signal",
                             WIFEXITED(status) ? WEXITSTATUS(status) : WTERMSIG(status), m_config.log.empty() ? "the terminal" : m_config.log.c_str());
            m_reported_exit = true;
            m_pid = -1;
            m_ready = false;
            return;
        }

        if (!m_ready && Answers(m_config.port)) {
            m_ready = true;
            std::printf("[SERVICES] %s -- ready on port %u after %lld ms\n", m_config.name.c_str(), static_cast<unsigned>(m_config.port),
                        static_cast<long long>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_started).count()));
        }
    }

    // Blocks until the port answers, the process exits, or timeout_ms passes -- for whoever needs it up before going on
    bool WaitReady(int const timeout_ms) {
        auto const end = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);

        while (std::chrono::steady_clock::now() < end) {
            if (m_ready) return true;
            if (m_pid <= 0) return false;
            Check();
            if (!m_ready) usleep(25'000);
        }

        return m_ready;
    }

    // A clean shutdown, then a hard one -- only for a process this started
    void Stop() {
        if (m_pid <= 0) return;

        kill(m_pid, SIGTERM);

        auto const end = std::chrono::steady_clock::now() + std::chrono::milliseconds(m_config.stop_ms);
        int status{0};

        while (std::chrono::steady_clock::now() < end) {
            if (waitpid(m_pid, &status, WNOHANG) == m_pid) {
                std::printf("[SERVICES] %s -- stopped\n", m_config.name.c_str());
                m_pid = -1;
                m_ready = false;
                return;
            }
            usleep(25'000);
        }

        std::fprintf(stderr, "[SERVICES] %s -- no clean shutdown in %d ms, killed\n", m_config.name.c_str(), m_config.stop_ms);
        kill(m_pid, SIGKILL);
        waitpid(m_pid, &status, 0);
        m_pid = -1;
        m_ready = false;
    }

    bool IsReady() const { return m_ready; }

    bool IsOwned() const { return m_pid > 0; }

   private:
    // Something accepts on 127.0.0.1:port
    static bool Answers(std::uint16_t const port) {
        if (port == 0) return false;

        int const fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
        if (fd < 0) return false;

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        bool const answered = connect(fd, reinterpret_cast<sockaddr const*>(&address), sizeof(address)) == 0;
        close(fd);

        return answered;
    }

    // a bare name -> the first executable of that name on PATH; a path -> itself if executable; '' = not found
    static std::string Resolve(std::string const& binary) {
        if (binary.empty()) return {};

        if (binary.find('/') != std::string::npos) return access(binary.c_str(), X_OK) == 0 ? binary : std::string{};

        char const* const path = std::getenv("PATH");
        std::string_view folders{path ? path : "/usr/local/bin:/usr/bin:/bin"};

        while (!folders.empty()) {
            std::size_t const colon = folders.find(':');
            std::string_view const folder = folders.substr(0, colon);
            folders = colon == std::string_view::npos ? std::string_view{} : folders.substr(colon + 1);

            if (folder.empty()) continue;

            std::string const candidate = std::string{folder} + "/" + binary;
            if (access(candidate.c_str(), X_OK) == 0) return candidate;
        }

        return {};
    }

    Ir77ServiceConfig m_config{};

    pid_t m_pid{-1};

    bool m_ready{false};

    bool m_reported_exit{false};

    std::chrono::steady_clock::time_point m_started{};
};

// =============================================================================================================================================
// Ir77Services -- the two Iridium runs, from iridium.json's "services". Nothing runs until a project is open: MongoDB's data is the project's
// own (<project>/mongodb), so it starts with the project and moves with it. Sapphire starts with the first project and stays -- mongoose
// reconnects by itself when mongod comes back on another project's data.
//
//   "services": {
//     "mongod":   { "enabled": true, "binary": "mongod", "data": "mongodb/data", "log": "mongodb/log/mongod.log", "port": 27018 },
//     "sapphire": { "enabled": true, "binary": "node", "script": "/home/alpha/workspace/sapphire/app.js",
//                   "log": "/home/alpha/workspace/iridium/Projects/sapphire.log", "port": 50002, "database": "aurum" }
//   }
//
// mongod's data and log are relative to the open project's folder. Every other relative path resolves against iridium.json's folder.
// Sapphire gets PORT and MONGO_URI (mongodb://127.0.0.1:<mongod port>/<database>) in its environment -- dotenv never overrides what's already
// set, so these win over its .env -- and runs in its script's folder, so its .env and data folder are where they always are.
// =============================================================================================================================================
struct Ir77ServicesConfig {
    Ir77ServiceConfig mongod{};
    Ir77ServiceConfig sapphire{};

    std::string mongod_data{"mongodb/data"};  // relative to the project folder
    std::string mongod_log{"mongodb/log/mongod.log"};
};

// iridium.json's root -> the two configs. A missing "services" (or a service missing from it) leaves that one disabled.
inline void Ir77ReadServices(simdjson::dom::element const& root, std::filesystem::path const& base, Ir77ServicesConfig& services) {
    auto const text = [&root](std::string const& pointer, std::string const& fallback) {
        std::string_view found{};
        return root.at_pointer(pointer).get(found) == simdjson::SUCCESS ? std::string{found} : fallback;
    };
    auto const number = [&root](std::string const& pointer, std::int64_t const fallback) {
        std::int64_t found{0};
        return root.at_pointer(pointer).get(found) == simdjson::SUCCESS ? found : fallback;
    };
    auto const flag = [&root](std::string const& pointer, bool const fallback) {
        bool found{false};
        return root.at_pointer(pointer).get(found) == simdjson::SUCCESS ? found : fallback;
    };
    auto const present = [&root](std::string const& pointer) { return root.at_pointer(pointer).error() == simdjson::SUCCESS; };

    // a path from the file -> absolute against its folder; a bare program name stays bare (PATH)
    auto const place = [&base](std::string const& path) {
        if (path.empty()) return path;
        std::filesystem::path const p{path};
        return (p.is_absolute() ? p : (base / p)).lexically_normal().string();
    };
    auto const program = [&place](std::string const& binary) { return binary.find('/') == std::string::npos ? binary : place(binary); };

    // mongod -- data and log placed when a project opens (OpenProject); local connections only
    {
        Ir77ServiceConfig& s = services.mongod;
        s.name = "mongod";
        s.enabled = present("/services/mongod") && flag("/services/mongod/enabled", true);
        s.binary = program(text("/services/mongod/binary", "mongod"));
        s.port = static_cast<std::uint16_t>(number("/services/mongod/port", 27018));
        s.stop_ms = static_cast<int>(number("/services/mongod/stop_ms", 15000));  // mongod flushes on the way out

        services.mongod_data = text("/services/mongod/data", services.mongod_data);
        services.mongod_log = text("/services/mongod/log", services.mongod_log);
    }

    // Sapphire -- node on its script, in its script's folder, told where mongod is
    {
        Ir77ServiceConfig& s = services.sapphire;
        s.name = "sapphire";
        s.enabled = present("/services/sapphire") && flag("/services/sapphire/enabled", true);
        s.binary = program(text("/services/sapphire/binary", "node"));
        s.port = static_cast<std::uint16_t>(number("/services/sapphire/port", 50002));
        s.log = place(text("/services/sapphire/log", ""));

        std::string const script = place(text("/services/sapphire/script", "../sapphire/app.js"));
        s.cwd = std::filesystem::path{script}.parent_path().string();
        s.args = {script};

        std::string const database = text("/services/sapphire/database", "aurum");
        s.env = {"PORT=" + std::to_string(s.port), "MONGO_URI=mongodb://127.0.0.1:" + std::to_string(services.mongod.port) + "/" + database};
        s.stop_ms = static_cast<int>(number("/services/sapphire/stop_ms", 5000));
    }
}

class Ir77Services {
   public:
    // the config -- nothing starts until OpenProject
    void SetConfig(Ir77ServicesConfig const& config) { m_config = config; }

    // A project opens: mongod on its data (the last project's stopped first), then Sapphire if it isn't running yet. Neither waited for --
    // Check reports each one ready; the UI retries the store until it answers.
    bool OpenProject(std::filesystem::path const& project, std::string& error) {
        m_mongod.Stop();

        if (m_config.mongod.enabled) {
            auto const place = [&project](std::string const& path) {
                std::filesystem::path const p{path};
                return (p.is_absolute() ? p : (project / p)).lexically_normal().string();
            };

            Ir77ServiceConfig mongod = m_config.mongod;
            std::string const data = place(m_config.mongod_data);
            mongod.log = place(m_config.mongod_log);
            mongod.folders = {data};
            mongod.args = {"--dbpath", data, "--port", std::to_string(mongod.port), "--bind_ip", "127.0.0.1"};

            if (!m_mongod.Start(mongod, error)) return false;
        }

        if (!m_sapphire.IsOwned() && !m_sapphire.IsReady())
            if (!m_sapphire.Start(m_config.sapphire, error)) return false;

        return true;
    }

    // The project closes -- its database stops; Sapphire waits for the next one
    void CloseProject() { m_mongod.Stop(); }

    void Check() {
        m_mongod.Check();
        m_sapphire.Check();
    }

    // Sapphire first -- it may still be writing to mongod
    void Stop() {
        m_sapphire.Stop();
        m_mongod.Stop();
    }

    Ir77Service& Mongod() { return m_mongod; }

    Ir77Service& Sapphire() { return m_sapphire; }

   private:
    Ir77ServicesConfig m_config{};

    Ir77Service m_mongod{};
    Ir77Service m_sapphire{};
};

}  // namespace NSIr77
