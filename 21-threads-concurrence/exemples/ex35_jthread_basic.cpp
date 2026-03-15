/* ============================================================================
   Section 21.7 : std::jthread - Thread avec join automatique et stop_token
   Description : Worker stoppable via stop_token, join automatique au destructeur
   Fichier source : 07-jthread.md
   ============================================================================ */

#include <thread>
#include <chrono>
#include <print>

void worker(std::stop_token stoken, int id, const std::string& name) {
    using namespace std::chrono_literals;
    while (!stoken.stop_requested()) {
        std::println("[{}] {} travaille...", id, name);
        std::this_thread::sleep_for(100ms);
    }
    std::println("[{}] {} arrêté proprement", id, name);
}

int main() {
    {
        std::jthread t(worker, 1, "Alice");

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(350ms);
        std::println("Demande d'arrêt...");
        // Le destructeur de jthread appelle request_stop() puis join()
    }
    std::println("Thread terminé et joint automatiquement");
}
