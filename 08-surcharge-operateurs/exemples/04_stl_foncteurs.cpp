/* ============================================================================
   Section 8.4 : Opérateur d'appel de fonction (operator())
   Description : Foncteurs prédéfinis de la STL — std::greater, std::plus,
                 versions transparentes (C++14/C++17 CTAD)
   Fichier source : 04-operateur-appel.md
   ============================================================================ */
#include <functional>
#include <vector>
#include <algorithm>
#include <numeric>
#include <print>

int main() {
    std::vector<int> v = {5, 2, 8, 1, 9};

    // std::greater<int> — tri décroissant
    std::sort(v.begin(), v.end(), std::greater<int>{});
    std::print("Tri décroissant : ");
    for (auto x : v) std::print("{} ", x);
    std::println("");
    // 9 8 5 2 1

    // std::plus<int> — accumulation
    int somme = std::accumulate(v.begin(), v.end(), 0, std::plus<int>{});
    std::println("Somme = {}", somme);  // 25

    // Versions transparentes (C++14 / C++17 CTAD)
    std::sort(v.begin(), v.end(), std::greater<>{});   // C++14
    std::sort(v.begin(), v.end(), std::greater{});     // C++17
    std::println("Toujours trié décroissant : {} {} {} {} {}",
                 v[0], v[1], v[2], v[3], v[4]);  // 9 8 5 2 1
}
