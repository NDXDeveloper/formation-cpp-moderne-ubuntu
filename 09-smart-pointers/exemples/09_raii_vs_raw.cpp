/* ============================================================================
   Section 9.0 : Introduction — Smart Pointers
   Description : RAII appliqué aux pointeurs — unique_ptr vs pointeurs bruts
   Fichier source : README.md
   ============================================================================ */
#include <memory>
#include <stdexcept>
#include <print>

// Simule une condition d'erreur
bool condition_erreur() { return true; }

// ❌ Ancien style : fuite mémoire si exception
void traiter_donnees_ancien() {
    int* donnees = new int[1000];
    donnees[0] = 42;
    std::print("Ancien style : donnees[0] = {}\n", donnees[0]);

    try {
        if (condition_erreur()) {
            throw std::runtime_error("Erreur de traitement");
            // ⚠️ delete[] donnees n'est jamais appelé → MEMORY LEAK
        }
        delete[] donnees;
    } catch (const std::exception& e) {
        std::print("Exception attrapée (ancien) : {}\n", e.what());
        delete[] donnees;  // Il faut penser à libérer ici aussi !
    }
}

// ✅ C++ moderne : pas de fuite grâce à unique_ptr
void traiter_donnees_moderne() {
    auto donnees = std::make_unique<int[]>(1000);
    donnees[0] = 42;
    std::print("Moderne : donnees[0] = {}\n", donnees[0]);

    try {
        if (condition_erreur()) {
            throw std::runtime_error("Erreur de traitement");
            // ✅ donnees est automatiquement libéré par le destructeur
        }
    } catch (const std::exception& e) {
        std::print("Exception attrapée (moderne) : {}\n", e.what());
        // Pas besoin de delete — unique_ptr gère tout
    }
}

int main() {
    std::print("=== Pointeurs bruts (ancien style) ===\n");
    traiter_donnees_ancien();

    std::print("\n=== Smart pointers (C++ moderne) ===\n");
    traiter_donnees_moderne();

    std::print("\n✅ Aucune fuite mémoire\n");
}
