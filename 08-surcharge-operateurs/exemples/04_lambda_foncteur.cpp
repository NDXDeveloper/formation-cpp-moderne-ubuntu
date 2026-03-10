/* ============================================================================
   Section 8.4 : Opérateur d'appel de fonction (operator())
   Description : Equivalence lambda/foncteur — une lambda est un foncteur
                 anonyme généré par le compilateur. Démo mutable
   Fichier source : 04-operateur-appel.md
   ============================================================================ */
#include <print>

int main() {
    // Lambda avec capture
    int seuil = 10;
    auto filtre = [seuil](int x) { return x > seuil; };

    std::println("filtre(15) = {}", filtre(15));  // true
    std::println("filtre(5) = {}", filtre(5));    // false

    // Lambda mutable — équivalent à operator() non-const
    auto compteur = [n = 0]() mutable { return ++n; };
    std::println("compteur() = {}", compteur());  // 1
    std::println("compteur() = {}", compteur());  // 2
    std::println("compteur() = {}", compteur());  // 3

    // Lambda générique — équivalent à operator() template
    auto afficheur = [](auto const& valeur) {
        std::println("{}", valeur);
    };
    afficheur(42);
    afficheur(3.14);
    afficheur("hello");
}
