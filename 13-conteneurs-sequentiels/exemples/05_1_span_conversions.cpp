/* ============================================================================
   Section 13.5.1 : Conversions entre spans
   Description : Conversion statique→dynamique (implicite), dynamique→statique
                 (via first<N>/last<N>), et versions statiques/dynamiques
                 de first/last/subspan
   Fichier source : 05.1-span-statique-dynamique.md
   ============================================================================ */
#include <span>
#include <vector>
#include <print>

int main() {
    // Dynamique → statique via first<N>/last<N>
    {
        std::vector<int> v{10, 20, 30, 40, 50};
        std::span<int> s_dyn(v);

        std::span<int, 3> debut = s_dyn.first<3>();
        std::span<int, 2> fin   = s_dyn.last<2>();

        std::println("debut : sizeof={}", sizeof(debut));  // 8
        std::println("fin   : sizeof={}", sizeof(fin));    // 8

        for (auto val : debut) std::print("{} ", val);
        std::println("");  // 10 20 30
        for (auto val : fin) std::print("{} ", val);
        std::println("");  // 40 50
    }

    // Versions statiques vs dynamiques de first/last/subspan
    {
        std::vector<int> v{10, 20, 30, 40, 50, 60};
        std::span<int> s(v);

        auto a = s.first<3>();
        auto b = s.last<2>();
        auto c = s.subspan<1, 3>();
        std::println("sizeof(a)={}, sizeof(b)={}, sizeof(c)={}",
                      sizeof(a), sizeof(b), sizeof(c));
        // 8, 8, 8

        std::size_t n = 3;
        auto d = s.first(n);
        auto e = s.last(n);
        auto f = s.subspan(1, n);
        std::println("sizeof(d)={}, sizeof(e)={}, sizeof(f)={}",
                      sizeof(d), sizeof(e), sizeof(f));
        // 16, 16, 16
    }
}
