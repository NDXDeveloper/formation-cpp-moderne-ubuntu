/* ============================================================================
   Section 14.3 : Opérations ensemblistes avec les algorithmes STL
   Description : Union, intersection, différence, différence symétrique,
                 inclusion, intersection manuelle sur unordered_set
   Fichier source : 03-set-unordered-set.md
   ============================================================================ */
#include <algorithm>
#include <set>
#include <unordered_set>
#include <iterator>
#include <print>

int main() {
    std::set<int> A {1, 2, 3, 4, 5};
    std::set<int> B {3, 4, 5, 6, 7};
    std::set<int> result;

    // Union (A ∪ B)
    std::set_union(
        A.begin(), A.end(),
        B.begin(), B.end(),
        std::inserter(result, result.begin())
    );
    std::print("Union : ");
    for (int x : result) std::print("{} ", x);
    // result = {1, 2, 3, 4, 5, 6, 7}
    std::println("");

    // Intersection (A ∩ B)
    result.clear();
    std::set_intersection(
        A.begin(), A.end(),
        B.begin(), B.end(),
        std::inserter(result, result.begin())
    );
    std::print("Intersection : ");
    for (int x : result) std::print("{} ", x);
    // result = {3, 4, 5}
    std::println("");

    // Différence (A \ B)
    result.clear();
    std::set_difference(
        A.begin(), A.end(),
        B.begin(), B.end(),
        std::inserter(result, result.begin())
    );
    std::print("Différence : ");
    for (int x : result) std::print("{} ", x);
    // result = {1, 2}
    std::println("");

    // Différence symétrique (A △ B)
    result.clear();
    std::set_symmetric_difference(
        A.begin(), A.end(),
        B.begin(), B.end(),
        std::inserter(result, result.begin())
    );
    std::print("Symétrique : ");
    for (int x : result) std::print("{} ", x);
    // result = {1, 2, 6, 7}
    std::println("");

    // Inclusion (A ⊆ B)
    std::set<int> C {3, 4};
    bool is_subset = std::includes(A.begin(), A.end(), C.begin(), C.end());
    // is_subset == true : C ⊆ A
    std::println("C ⊆ A : {}", is_subset);

    std::println("---");

    // Intersection manuelle sur unordered_set — O(min(|A|, |B|))
    std::unordered_set<int> UA {1, 2, 3, 4, 5};
    std::unordered_set<int> UB {3, 4, 5, 6, 7};

    // Itérer sur le plus petit, chercher dans le plus grand
    std::unordered_set<int> uresult;
    const auto& [smaller, larger] = (UA.size() <= UB.size())
        ? std::tie(UA, UB) : std::tie(UB, UA);

    for (const auto& elem : smaller) {
        if (larger.contains(elem)) {
            uresult.insert(elem);
        }
    }
    std::print("Unordered intersection : ");
    for (int x : uresult) std::print("{} ", x);
    std::println("");
}
