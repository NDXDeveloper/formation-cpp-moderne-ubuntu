/* ============================================================================
   Section 13.1.1 : Benchmark reserve()
   Description : Mesure comparative des performances avec et sans reserve()
                 sur 10 millions d'insertions
   Fichier source : 01.1-fonctionnement-interne.md
   ============================================================================ */
#include <vector>
#include <chrono>
#include <print>

int main() {
    constexpr int N = 10'000'000;

    // Sans reserve
    auto t1 = std::chrono::high_resolution_clock::now();
    {
        std::vector<int> v;
        for (int i = 0; i < N; ++i) v.push_back(i);
    }
    auto t2 = std::chrono::high_resolution_clock::now();

    // Avec reserve
    auto t3 = std::chrono::high_resolution_clock::now();
    {
        std::vector<int> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) v.push_back(i);
    }
    auto t4 = std::chrono::high_resolution_clock::now();

    auto sans = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
    auto avec = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3);

    std::println("Sans reserve : {} ms", sans.count());
    std::println("Avec reserve : {} ms", avec.count());
}
