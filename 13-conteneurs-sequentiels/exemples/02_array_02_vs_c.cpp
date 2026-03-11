/* ============================================================================
   Section 13.2 : std::array vs tableaux C
   Description : Comparaison entre tableau C (décroissance en pointeur) et
                 std::array (taille préservée, copie, comparaison, STL)
   Fichier source : 02-array.md
   ============================================================================ */
#include <array>
#include <algorithm>
#include <print>

// Le tableau C décroît en pointeur — la taille est perdue
void afficher_c(int arr[], int taille) {
    for (int i = 0; i < taille; ++i) {
        std::print("{} ", arr[i]);
    }
    std::println("");
}

// La taille est encodée dans le type — impossible de se tromper
void afficher(const std::array<int, 3>& arr) {
    for (const auto& val : arr) {
        std::print("{} ", val);
    }
    std::println("");
}

int main() {
    // Tableau C
    int tab[] = {10, 20, 30};
    afficher_c(tab, 3);

    // std::array
    std::array<int, 3> atab{10, 20, 30};
    afficher(atab);

    // Copie et comparaison
    std::array<int, 4> a{1, 2, 3, 4};
    std::array<int, 4> b = a;           // copie
    std::array<int, 4> c{};
    c = a;                              // affectation
    std::println("a == b : {}", a == b);  // true

    // Compatibilité STL
    std::array<int, 6> s{30, 10, 50, 20, 40, 60};
    std::sort(s.begin(), s.end());
    for (const auto& val : s) {
        std::print("{} ", val);
    }
    std::println("");
    // Sortie : 10 20 30 40 50 60
}
