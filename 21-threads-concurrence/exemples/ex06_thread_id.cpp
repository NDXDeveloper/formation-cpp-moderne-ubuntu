/* ============================================================================
   Section 21.1 : Identification des threads
   Description : Obtenir et afficher les IDs de threads, hardware_concurrency
   Fichier source : 01-std-thread.md
   ============================================================================ */

#include <thread>
#include <print>

int main() {
    std::thread t([] {
        auto id = std::this_thread::get_id();
        std::println("Thread enfant, ID : {}", id);
    });

    std::println("Thread principal, ID : {}", std::this_thread::get_id());
    std::println("ID du thread enfant (vu du parent) : {}", t.get_id());

    t.join();

    // Nombre de threads matériels
    unsigned int n = std::thread::hardware_concurrency();
    std::println("Threads matériels disponibles : {}", n);
}
