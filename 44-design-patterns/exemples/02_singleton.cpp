/* ============================================================================
   Section 44.1.1 : Singleton Thread-Safe
   Description : Meyers' Singleton — static local, thread-safe depuis C++11
   Fichier source : 01.1-singleton.md
   ============================================================================ */

#include <print>
#include <string_view>
#include <string>
#include <vector>
#include <mutex>

class Logger {
public:
    static Logger& instance() {
        static Logger inst;
        return inst;
    }

    Logger(const Logger&)            = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&)                 = delete;
    Logger& operator=(Logger&&)      = delete;

    void log(std::string_view message) {
        std::lock_guard lock(mtx_);
        buffer_.emplace_back(message);
        std::print("[LOG] {}\n", message);
    }

private:
    Logger() { std::print("Logger created\n"); }
    ~Logger() { std::print("Logger destroyed\n"); }

    std::mutex mtx_;
    std::vector<std::string> buffer_;
};

int main() {
    Logger::instance().log("Application started");
    Logger::instance().log("Processing request");
    Logger::instance().log("Shutting down");

    auto& l1 = Logger::instance();
    auto& l2 = Logger::instance();
    std::print("Same instance: {}\n", (&l1 == &l2));
}
