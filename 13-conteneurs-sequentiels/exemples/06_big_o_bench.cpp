/* ============================================================================
   Section 13.6 : Benchmark insertion au milieu
   Description : Micro-benchmark comparant l'insertion au milieu sur vector,
                 list et deque — démonstration que le Big O seul ne suffit pas
   Fichier source : 06-complexite-big-o.md
   ============================================================================ */
#include <vector>
#include <list>
#include <deque>
#include <chrono>
#include <print>

template <typename Container>
auto bench_insertion_milieu(int n) {
    Container c;

    auto debut = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < n; ++i) {
        auto pos = c.begin();
        std::advance(pos, static_cast<long>(c.size()) / 2);
        c.insert(pos, i);
    }

    auto fin = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(fin - debut);
}

int main() {
    constexpr int N = 50'000;

    auto t_vec  = bench_insertion_milieu<std::vector<int>>(N);
    auto t_list = bench_insertion_milieu<std::list<int>>(N);
    auto t_deq  = bench_insertion_milieu<std::deque<int>>(N);

    std::println("Insertion au milieu ({} éléments) :", N);
    std::println("  vector : {} µs", t_vec.count());
    std::println("  list   : {} µs", t_list.count());
    std::println("  deque  : {} µs", t_deq.count());
}
