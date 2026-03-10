/* ============================================================================
   Section 8.4 : Opérateur d'appel de fonction (operator())
   Description : Foncteur avec état mutable — Compteur non-const,
                 utilisation de std::ref avec les algorithmes STL
   Fichier source : 04-operateur-appel.md
   ============================================================================ */
#include <print>
#include <vector>
#include <algorithm>
#include <functional>

class Compteur {
    int compte_ = 0;

public:
    int operator()(int x) {   // non-const : modifie l'état
        ++compte_;
        return x * compte_;
    }

    int total() const { return compte_; }
};

int main() {
    Compteur c;
    std::println("{}", c(10));   // 10  (compte_ = 1)
    std::println("{}", c(10));   // 20  (compte_ = 2)
    std::println("{}", c(10));   // 30  (compte_ = 3)
    std::println("Appels : {}", c.total());  // 3

    // Avec std::ref pour préserver l'état dans les algorithmes
    std::vector<int> v = {1, 2, 3, 4, 5};
    Compteur compteur;
    std::for_each(v.begin(), v.end(), std::ref(compteur));
    std::println("Total après for_each : {}", compteur.total());  // 5
}
