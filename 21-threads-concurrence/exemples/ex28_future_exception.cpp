/* ============================================================================
   Section 21.5 : std::future - Propagation automatique des exceptions
   Description : Exception relancée au get() dans le thread appelant
   Fichier source : 05-async-future.md
   ============================================================================ */

#include <future>
#include <stdexcept>
#include <print>

int risky_computation() {
    throw std::runtime_error("Erreur dans le calcul");
    return 42;  // Jamais atteint
}

int main() {
    auto fut = std::async(std::launch::async, risky_computation);

    try {
        int value = fut.get();  // Relance l'exception ici
        std::println("Résultat : {}", value);
    } catch (const std::runtime_error& e) {
        std::println("Exception capturée : {}", e.what());
    }
}
