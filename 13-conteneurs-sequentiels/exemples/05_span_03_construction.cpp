/* ============================================================================
   Section 13.5 : Construction et conversions implicites
   Description : Construction de std::span depuis vector, array, tableau C,
                 pointeur+taille, et span vide
   Fichier source : 05-span.md
   ============================================================================ */
#include <span>
#include <vector>
#include <array>
#include <print>

void afficher(std::span<const int> s) {
    std::print("[");
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (i > 0) std::print(", ");
        std::print("{}", s[i]);
    }
    std::println("]");
}

int main() {
    std::vector<int> vec{10, 20, 30};
    afficher(vec);             // [10, 20, 30]

    std::array<int, 4> arr{1, 2, 3, 4};
    afficher(arr);             // [1, 2, 3, 4]

    int tab[] = {100, 200};
    afficher(tab);             // [100, 200]

    afficher({vec.data(), 2}); // [10, 20]

    std::span<const int> s1(vec);
    afficher(s1);              // [10, 20, 30]

    afficher({});              // []
}
