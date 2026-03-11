/* ============================================================================
   Section 14.4 : std::flat_set et std::flat_multiset (C++23)
   Description : Ensemble ordonné en vecteur trié, opérations ensemblistes,
                 flat_multiset avec doublons
   Fichier source : 04-flat-map-flat-set.md
   ============================================================================ */
#include <flat_set>
#include <algorithm>
#include <vector>
#include <print>

int main() {
    // === flat_set ===
    std::flat_set<int> primes {7, 2, 11, 3, 5, 2, 3};

    for (int p : primes) {
        std::print("{} ", p);
    }
    // 2 3 5 7 11 — trié, sans doublons
    std::println("");

    // Même interface que std::set
    primes.insert(13);
    bool has_five = primes.contains(5);
    std::println("contains 5 = {}", has_five);

    // Requêtes par plage
    auto from = primes.lower_bound(4);
    auto to   = primes.upper_bound(10);
    std::print("Entre 4 et 10 : ");
    for (auto it = from; it != to; ++it) {
        std::print("{} ", *it); // 5 7
    }
    std::println("");

    std::println("---");

    // === Opérations ensemblistes ===
    std::flat_set<int> A {1, 2, 3, 4, 5};
    std::flat_set<int> B {3, 4, 5, 6, 7};

    std::vector<int> result;
    std::set_intersection(
        A.begin(), A.end(),
        B.begin(), B.end(),
        std::back_inserter(result)
    );
    std::print("Intersection : ");
    for (int x : result) std::print("{} ", x);
    // result = {3, 4, 5}
    std::println("");

    std::println("---");

    // === flat_multiset ===
    std::flat_multiset<int> ms {5, 3, 5, 1, 3, 5};
    // Stocké comme un vecteur trié : {1, 3, 3, 5, 5, 5}
    std::print("Nombre de 5 : {}\n", ms.count(5)); // 3

    for (int x : ms) std::print("{} ", x);
    std::println("");
}
