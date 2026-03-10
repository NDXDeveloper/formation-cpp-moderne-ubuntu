/* ============================================================================
   Section 8.4 : Opérateur d'appel de fonction (operator())
   Description : std::function — effacement de type pour stocker foncteurs,
                 lambdas et fonctions libres dans une même variable
   Fichier source : 04-operateur-appel.md
   ============================================================================ */
#include <functional>
#include <print>

class SuperieurA {
    int seuil_;
public:
    explicit SuperieurA(int seuil) : seuil_{seuil} {}

    bool operator()(int x) const {
        return x > seuil_;
    }
};

bool est_positif(int x) { return x > 0; }

int main() {
    std::function<bool(int)> predicat;

    // Peut contenir un foncteur :
    predicat = SuperieurA{10};
    std::println("{}", predicat(15));   // true

    // Ou une lambda :
    predicat = [](int x) { return x % 2 == 0; };
    std::println("{}", predicat(15));   // false

    // Ou une fonction libre :
    predicat = est_positif;
    std::println("{}", predicat(15));   // true
}
