/* ============================================================================
   Section 40.5 : Structured logging — JSON logs pour agregation
   Description : Custom sink spdlog produisant du JSON valide via nlohmann/json
   Fichier source : 05-json-logs.md
   ============================================================================ */

#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/log_msg.h>
#include <nlohmann/json.hpp>
#include <mutex>
#include <iostream>
#include <unistd.h>

// --- Custom JSON sink ---------------------------------------------------
template<typename Mutex>
class json_sink : public spdlog::sinks::base_sink<Mutex> {
public:
    void add_static_field(const std::string& key, const std::string& value) {
        static_fields_[key] = value;
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        nlohmann::json j;

        // Horodatage ISO 8601 UTC
        auto epoch = msg.time.time_since_epoch();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(epoch);
        auto millis  = std::chrono::duration_cast<std::chrono::milliseconds>(epoch)
                       - std::chrono::duration_cast<std::chrono::milliseconds>(seconds);
        std::time_t t = std::chrono::system_clock::to_time_t(msg.time);
        std::tm tm{};
        gmtime_r(&t, &tm);
        char time_buf[64];
        std::snprintf(time_buf, sizeof(time_buf),
                      "%04d-%02d-%02dT%02d:%02d:%02d.%03ldZ",
                      tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                      tm.tm_hour, tm.tm_min, tm.tm_sec,
                      static_cast<long>(millis.count()));
        j["time"] = time_buf;

        j["level"]  = spdlog::level::to_string_view(msg.level).data();
        j["logger"] = std::string(msg.logger_name.data(), msg.logger_name.size());
        j["msg"]    = std::string(msg.payload.data(), msg.payload.size());
        j["tid"]    = msg.thread_id;
        j["pid"]    = static_cast<int>(getpid());

        if (!msg.source.empty()) {
            j["source"]["file"] = msg.source.filename;
            j["source"]["line"] = msg.source.line;
            j["source"]["func"] = msg.source.funcname;
        }

        for (const auto& [key, value] : static_fields_) {
            j[key] = value;
        }

        std::string line = j.dump(-1, ' ', false,
                                  nlohmann::json::error_handler_t::replace);
        line.push_back('\n');
        std::cout << line;
    }

    void flush_() override { std::cout.flush(); }

private:
    std::map<std::string, std::string> static_fields_;
};

using json_sink_mt = json_sink<std::mutex>;

// --- Programme principal ------------------------------------------------
int main() {
    auto sink = std::make_shared<json_sink_mt>();
    sink->add_static_field("service", "syswatch");
    sink->add_static_field("version", "1.2.0");
    sink->add_static_field("environment", "production");

    auto logger = std::make_shared<spdlog::logger>("syswatch", sink);
    spdlog::set_default_logger(logger);

    // Message normal
    spdlog::info("Server started on port 8080");

    // Message avec guillemets (échappement JSON vérifié)
    SPDLOG_ERROR("Failed to parse file \"config.yaml\": unexpected token at line {}", 42);

    // Message avec retour à la ligne
    spdlog::warn("Multi-line\nerror message");

    // Message avec backslash
    spdlog::info("Path: C:\\Users\\test\\file.txt");

    return 0;
}
