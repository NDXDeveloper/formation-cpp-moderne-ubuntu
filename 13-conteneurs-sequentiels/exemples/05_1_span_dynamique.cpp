/* ============================================================================
   Section 13.5.1 : Span dynamique et statique
   Description : Différence entre span dynamique (sizeof=16) et statique
                 (sizeof=8), déduction automatique selon la source
   Fichier source : 05.1-span-statique-dynamique.md
   ============================================================================ */
#include <span>
#include <vector>
#include <array>
#include <print>

int main() {
    // Span dynamique
    {
        std::vector<int> v{10, 20, 30, 40, 50};
        std::span<int> s(v);
        std::println("size = {}", s.size());         // 5
        std::println("sizeof(s) = {}", sizeof(s));   // 16
    }

    // Span statique
    {
        std::array<int, 5> arr{10, 20, 30, 40, 50};
        std::span<int, 5> s(arr);
        std::println("size = {}", s.size());         // 5
        std::println("sizeof(s) = {}", sizeof(s));   // 8
    }

    // Déduction automatique
    {
        int tab[3] = {1, 2, 3};
        std::array<int, 4> arr{1, 2, 3, 4};
        std::vector<int> vec{1, 2, 3, 4, 5};

        std::span s1(tab);
        std::println("s1 : sizeof={}, extent={}", sizeof(s1), s1.extent);
        std::span s2(arr);
        std::println("s2 : sizeof={}, extent={}", sizeof(s2), s2.extent);
        std::span s3(vec);
        std::println("s3 : sizeof={}", sizeof(s3));
    }
}
