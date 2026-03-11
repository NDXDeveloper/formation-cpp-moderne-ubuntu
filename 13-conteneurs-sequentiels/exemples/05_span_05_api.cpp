/* ============================================================================
   Section 13.5 : API de std::span
   Description : Accès ([], front, back, data), taille (size, size_bytes,
                 empty), sous-vues (first, last, subspan), itérateurs et tri
   Fichier source : 05-span.md
   ============================================================================ */
#include <span>
#include <vector>
#include <algorithm>
#include <print>
#include <cstdint>

void afficher(std::span<const int> s) {
    for (auto val : s) std::print("{} ", val);
    std::println("");
}

int main() {
    // Accès aux éléments
    {
        std::vector<int> v{10, 20, 30, 40, 50};
        std::span<const int> s(v);

        std::println("s[2] = {}", s[2]);
        std::println("front={}, back={}", s.front(), s.back());
        const int* p = s.data();
        std::println("data()[3] = {}", p[3]);
    }

    // Taille et état
    {
        std::vector<int> v{10, 20, 30};
        std::span<const int> s(v);
        std::span<const int> vide;

        std::println("size = {}", s.size());
        std::println("size_bytes = {}", s.size_bytes());
        std::println("empty = {}", s.empty());
        std::println("vide.empty = {}", vide.empty());
    }

    // Sous-vues
    {
        std::vector<int> v{10, 20, 30, 40, 50, 60, 70};
        std::span<const int> s(v);
        afficher(s.first(3));       // 10 20 30
        afficher(s.last(2));        // 60 70
        afficher(s.subspan(2, 3));  // 30 40 50
        afficher(s.subspan(4));     // 50 60 70
    }

    // Itérateurs + tri
    {
        std::vector<int> v{50, 30, 10, 40, 20};
        std::span<int> s(v);
        std::sort(s.begin(), s.end());
        for (auto val : v) std::print("{} ", val);
        std::println("");
        // 10 20 30 40 50

        for (auto it = s.rbegin(); it != s.rend(); ++it) {
            std::print("{} ", *it);
        }
        std::println("");
        // 50 40 30 20 10
    }
}
