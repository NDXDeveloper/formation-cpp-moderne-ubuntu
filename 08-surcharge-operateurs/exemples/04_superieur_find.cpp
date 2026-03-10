/* ============================================================================
   Section 8.4 : Opérateur d'appel de fonction (operator())
   Description : Foncteur vs fonction libre — SuperieurA avec seuil
                 configurable, utilisation avec std::find_if
   Fichier source : 04-operateur-appel.md
   ============================================================================ */
#include <algorithm>
#include <vector>
#include <print>

// Fonction libre — pas d'état configurable
bool est_superieur_a_10(int x) {
    return x > 10;   // seuil codé en dur
}

// Foncteur — seuil configurable
class SuperieurA {
    int seuil_;
public:
    explicit SuperieurA(int seuil) : seuil_{seuil} {}

    bool operator()(int x) const {
        return x > seuil_;
    }
};

int main() {
    SuperieurA sup5{5};
    SuperieurA sup100{100};

    std::println("{}", sup5(7));      // true
    std::println("{}", sup5(3));      // false
    std::println("{}", sup100(42));   // false

    std::vector<int> nombres = {3, 7, 12, 5, 18, 1, 9};

    // Avec la fonction libre — seuil figé
    auto it1 = std::find_if(nombres.begin(), nombres.end(), est_superieur_a_10);
    if (it1 != nombres.end())
        std::println("Premier > 10 : {}", *it1);  // 12

    // Avec le foncteur — seuil configurable
    auto it2 = std::find_if(nombres.begin(), nombres.end(), SuperieurA{15});
    if (it2 != nombres.end())
        std::println("Premier > 15 : {}", *it2);  // 18
}
