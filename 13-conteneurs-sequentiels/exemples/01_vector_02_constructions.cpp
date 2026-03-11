/* ============================================================================
   Section 13.1 : Constructions courantes
   Description : Différentes façons de construire un std::vector (vide, taille,
                 liste d'initialisation, copie, déplacement, itérateurs, CTAD)
   Fichier source : 01-vector.md
   ============================================================================ */
#include <vector>
#include <print>

int main() {
    // 1. Construction vide
    std::vector<int> v1;                // vide, size=0, capacity=0

    // 2. Construction avec taille et valeur par défaut
    std::vector<int> v2(5);             // {0, 0, 0, 0, 0}
    std::vector<int> v3(5, 42);         // {42, 42, 42, 42, 42}

    // 3. Construction par liste d'initialisation (C++11)
    std::vector<int> v4{1, 2, 3, 4, 5};

    // 4. Construction par copie
    std::vector<int> v5 = v4;           // copie profonde

    // 5. Construction par déplacement (C++11)
    std::vector<int> v6 = std::move(v4);
    // v4 est maintenant dans un état valide mais indéterminé (typiquement vide)

    // 6. Construction à partir d'une paire d'itérateurs
    std::vector<int> v7(v6.begin(), v6.begin() + 3);  // {1, 2, 3}

    // 7. Déduction de type (CTAD, C++17)
    std::vector v8{10, 20, 30};         // déduit std::vector<int>

    std::println("v6 : size={}, v4 : size={}", v6.size(), v4.size());
    // Sortie : v6 : size=5, v4 : size=0
}
