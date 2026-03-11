/* ============================================================================
   Section 13.1.3 : Stockage d'index vs itérateurs
   Description : Démonstration de la robustesse des index face aux
                 réallocations, contrairement aux pointeurs/itérateurs
   Fichier source : 01.3-invalidation-iterateurs.md
   ============================================================================ */
#include <vector>
#include <print>

int main() {
    std::vector<std::string> noms{"Alice", "Bob", "Charlie"};

    // Solution 1 : stocker l'INDEX plutôt que le pointeur
    std::size_t idx_bob = 1;

    for (int i = 0; i < 100; ++i) {
        noms.emplace_back("Utilisateur_" + std::to_string(i));
    }

    std::println("Bob via index : {}", noms[idx_bob]);  // OK

    // Solution 2 : utiliser reserve() pour garantir l'absence de réallocation
    std::vector<std::string> prenoms;
    prenoms.reserve(200);
    prenoms.push_back("Alice");
    prenoms.push_back("Bob");

    std::string* ptr = &prenoms[1];
    for (int i = 0; i < 100; ++i) {
        prenoms.emplace_back("Prenom_" + std::to_string(i));
    }
    std::println("Bob via pointeur (reserve garanti) : {}", *ptr);  // OK
}
