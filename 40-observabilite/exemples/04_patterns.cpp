/* ============================================================================
   Section 40.1.3 : Pattern de formatage
   Description : Quatre patterns recommandes — dev, production, Docker, JSON
   Fichier source : 01.3-pattern-formatage.md
   ============================================================================ */

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

int main() {
    spdlog::set_level(spdlog::level::trace);

    // --- Pattern développement local ---
    spdlog::set_pattern("[%T.%e] [%^%-8l%$] [%s:%#] %v");
    SPDLOG_INFO("Server started on port {}", 8080);
    SPDLOG_DEBUG("Loaded {} rules from config.yaml", 12);
    SPDLOG_ERROR("Connection refused: timeout");

    spdlog::info(""); // séparateur

    // --- Pattern production (fichier) ---
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [pid:%P tid:%t] %v");
    spdlog::info("Server started on port 8080");
    spdlog::error("Query timeout after 5032ms");

    spdlog::set_pattern("%v"); // reset
    spdlog::info(""); // séparateur

    // --- Pattern Docker/Kubernetes (console) ---
    spdlog::set_pattern("[%Y-%m-%dT%H:%M:%S.%e%z] [%l] [%n] %v");
    spdlog::info("Server started on port 8080");
    spdlog::debug("Connection accepted from 10.0.1.42");

    spdlog::set_pattern("%v");
    spdlog::info("");

    // --- Pattern JSON structuré ---
    spdlog::set_pattern(
        R"({"time":"%Y-%m-%dT%H:%M:%S.%e%z","level":"%l","logger":"%n","pid":%P,"tid":%t,"msg":"%v"})");
    spdlog::info("Server started on port 8080");
    spdlog::error("Query timeout after 5032ms");

    return 0;
}
