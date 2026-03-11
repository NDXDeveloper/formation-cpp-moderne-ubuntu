/* ============================================================================
   Section 13.1 : std::vector<bool> - spécialisation problématique
   Description : Démonstration du comportement particulier de vector<bool>
                 qui retourne un proxy au lieu d'une vraie référence
   Fichier source : 01-vector.md
   ============================================================================ */
#include <vector>
#include <print>

int main() {
    std::vector<bool> flags{true, false, true};

    // Ceci ne compile PAS ou produit un comportement surprenant :
    // bool& ref = flags[0];  // ERREUR : operator[] ne retourne pas bool&

    // Il retourne un proxy object (std::vector<bool>::reference)
    auto ref = flags[0];      // ref est un proxy, pas un bool&

    std::println("flags[0] = {}", static_cast<bool>(ref));
    std::println("size = {}", flags.size());
}
