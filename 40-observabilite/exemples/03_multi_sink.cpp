/* ============================================================================
   Section 40.1.2 : Niveaux de log et sinks
   Description : Multi-sink — console (info+) et fichier rotatif (debug+)
   Fichier source : 01.2-niveaux-sinks.md
   ============================================================================ */

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <vector>

int main() {
    std::vector<spdlog::sink_ptr> sinks;

    // Console : info et au-dessus, format lisible humain
    auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console->set_level(spdlog::level::info);
    console->set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
    sinks.push_back(console);

    // Fichier : debug et au-dessus, format complet avec thread ID
    auto file = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        "syswatch.log", 50 * 1024 * 1024, 10
    );
    file->set_level(spdlog::level::debug);
    file->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [tid:%t] %v");
    sinks.push_back(file);

    auto logger = std::make_shared<spdlog::logger>("syswatch", sinks.begin(), sinks.end());
    logger->set_level(spdlog::level::debug);  // Laisser passer tout vers les sinks
    spdlog::set_default_logger(logger);

    // Test : debug ne va que dans le fichier, info va partout
    spdlog::debug("This debug goes to file only");
    spdlog::info("Server started on port 8080");
    spdlog::warn("Config file missing, using defaults");
    spdlog::error("Connection to database failed: timeout after 5000ms");

    spdlog::default_logger()->flush();
    return 0;
}
