#pragma once

#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#include <mutex>
#include <atomic>
#include <string>

namespace NSIr77RT {

class Ir77RetLog {
   public:
    static void Enable() { enabled_ = true; }

    static void Write(std::string const& msg) {
        if (!enabled_ || msg.empty()) return;

        int fd = ::open("/tmp/Ir77RetLog.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd < 0) return;

        std::string line = "[pid:" + std::to_string(::getpid()) + "] " + msg + '\n';
        ::write(fd, line.data(), line.size());
        ::close(fd);
    }

    static void Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        ::unlink("/tmp/Ir77RetLog.txt");
        ::unlink("/tmp/Ir77RetLog.lock");
    }

   private:
    static inline std::mutex mutex_;
    static inline std::atomic<bool> enabled_{false};
};


}  // namespace Ir77RT