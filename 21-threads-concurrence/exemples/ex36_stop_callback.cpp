/* ============================================================================
   Section 21.7 : std::stop_callback - Réagir à l'arrêt
   Description : Callback exécuté automatiquement à la demande d'arrêt
   Fichier source : 07-jthread.md
   ============================================================================ */

#include <thread>
#include <stop_token>
#include <chrono>
#include <print>

void interruptible_worker(std::stop_token stoken) {
    // Enregistrer une action à exécuter à l'arrêt
    std::stop_callback on_stop(stoken, [] {
        std::println("  → Callback d'arrêt exécuté !");
    });

    using namespace std::chrono_literals;
    int iteration = 0;
    while (!stoken.stop_requested()) {
        std::println("  Travail en cours (itération {})", iteration++);
        std::this_thread::sleep_for(100ms);
    }
    std::println("  Worker terminé après {} itérations", iteration);
}

int main() {
    std::jthread t(interruptible_worker);

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(350ms);
    std::println("main: request_stop()");
    t.request_stop();
    t.join();
    std::println("main: terminé");
}
