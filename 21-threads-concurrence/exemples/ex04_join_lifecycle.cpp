/* ============================================================================
   Section 21.1 : Cycle de vie d'un thread - join()
   Description : Démonstration du blocage de join() et de l'ordre d'exécution
   Fichier source : 01-std-thread.md
   ============================================================================ */

#include <thread>
#include <print>
#include <chrono>

int main() {
    std::thread t([] {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::println("Thread terminé");
    });

    std::println("Avant join — le thread tourne en parallèle");
    t.join();  // Bloque jusqu'à la fin du thread
    std::println("Après join — le thread est terminé");
}
