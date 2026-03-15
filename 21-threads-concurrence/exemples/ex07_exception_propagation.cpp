/* ============================================================================
   Section 21.1 : Gestion des exceptions dans les threads
   Description : Capturer et propager une exception via std::exception_ptr
   Fichier source : 01-std-thread.md
   ============================================================================ */

#include <thread>
#include <stdexcept>
#include <exception>
#include <print>

int main() {
    std::exception_ptr eptr = nullptr;

    std::thread t([&eptr] {
        try {
            // Travail qui peut échouer
            throw std::runtime_error("Échec du traitement");
        } catch (...) {
            eptr = std::current_exception();  // Capture l'exception
        }
    });

    t.join();

    // Relancer l'exception dans le thread principal
    if (eptr) {
        try {
            std::rethrow_exception(eptr);
        } catch (const std::exception& e) {
            std::println("Exception du thread : {}", e.what());
        }
    }
}
