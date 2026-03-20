/* ============================================================================
   Section 40.4 : Health checks et readiness probes
   Description : Serveur HTTP minimal avec /healthz et /readyz (cpp-httplib)
   Fichier source : 04-health-checks.md
   ============================================================================ */

#include <httplib.h>
#include <spdlog/spdlog.h>
#include <atomic>
#include <thread>
#include <chrono>

static std::atomic<bool> alive{true};
static std::atomic<bool> ready{false};

int main() {
    spdlog::set_level(spdlog::level::info);

    // Démarrer le serveur de santé dans un thread séparé
    httplib::Server svr;

    svr.Get("/healthz", [](const httplib::Request&, httplib::Response& res) {
        if (alive.load(std::memory_order_relaxed)) {
            res.status = 200;
            res.set_content(R"({"status":"ok"})", "application/json");
        } else {
            res.status = 503;
            res.set_content(R"({"status":"dead"})", "application/json");
        }
    });

    svr.Get("/readyz", [](const httplib::Request&, httplib::Response& res) {
        if (ready.load(std::memory_order_relaxed)) {
            res.status = 200;
            res.set_content(R"({"status":"ready"})", "application/json");
        } else {
            res.status = 503;
            res.set_content(R"({"status":"not_ready"})", "application/json");
        }
    });

    std::thread server_thread([&svr]() {
        spdlog::info("Health server listening on :8081");
        svr.listen("0.0.0.0", 8081);
    });

    // Laisser le serveur démarrer
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Phase de démarrage
    spdlog::info("Starting up (loading config, connecting DB)...");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Signaler que le service est prêt
    ready.store(true, std::memory_order_relaxed);
    spdlog::info("Service ready, /readyz now returns 200");

    // Laisser tourner un moment
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Arrêt propre
    spdlog::info("Shutting down gracefully");
    ready.store(false, std::memory_order_relaxed);
    svr.stop();
    server_thread.join();

    return 0;
}
