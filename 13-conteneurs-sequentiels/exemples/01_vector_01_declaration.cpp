/* ============================================================================
   Section 13.1 : Déclaration et inclusion
   Description : Déclaration de base de std::vector avec différentes
                 initialisations
   Fichier source : 01-vector.md
   ============================================================================ */
#include <vector>
#include <string>
#include <print>    // C++23

int main() {
    std::vector<int> nombres;                    // vector vide d'entiers
    std::vector<std::string> mots{"hello", "world"};  // initialisé avec 2 éléments
    std::vector<double> valeurs(100, 0.0);       // 100 éléments initialisés à 0.0

    std::println("mots contient {} éléments", mots.size());
    // Sortie : mots contient 2 éléments
}
