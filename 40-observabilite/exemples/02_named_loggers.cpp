/* ============================================================================
   Section 40.1.1 : Installation et configuration de spdlog
   Description : Loggers nommes par composant avec niveaux independants
   Fichier source : 01.1-installation-spdlog.md
   ============================================================================ */

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <string>

int main() {
    // Créer des loggers dédiés par composant
    auto net_logger = spdlog::stdout_color_mt("network");
    auto db_logger  = spdlog::stdout_color_mt("database");

    // Chaque logger a son propre niveau
    net_logger->set_level(spdlog::level::debug);
    db_logger->set_level(spdlog::level::warn);

    // Les messages identifient leur source
    std::string client_addr = "10.0.1.42";
    int query_duration = 342;
    net_logger->info("Connection accepted from {}", client_addr);
    db_logger->warn("Query slow: {}ms", query_duration);

    // debug ne s'affiche que pour network (database est en warn)
    net_logger->debug("Packet received: {} bytes", 1024);
    db_logger->debug("This should NOT appear");

    // Récupérer un logger depuis n'importe où via spdlog::get()
    auto logger = spdlog::get("network");
    if (logger) {
        logger->info("Retrieved via spdlog::get()");
    }

    return 0;
}
