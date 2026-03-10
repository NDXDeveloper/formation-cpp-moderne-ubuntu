/* ============================================================================
   Section 8.5 : Opérateur spaceship <=> (C++20)
   Description : <=> et conteneurs STL — utilisation avec std::set et std::map,
                 operator< généré automatiquement par <=>
   Fichier source : 05-operateur-spaceship.md
   ============================================================================ */
#include <compare>
#include <set>
#include <map>
#include <string>
#include <print>

class Cle {
    int a_, b_;
public:
    Cle(int a, int b) : a_{a}, b_{b} {}

    auto operator<=>(Cle const&) const = default;
    bool operator==(Cle const&) const = default;
};

int main() {
    std::set<Cle> ensemble;
    std::map<Cle, std::string> dictionnaire;

    ensemble.insert(Cle{1, 2});
    ensemble.insert(Cle{1, 3});
    ensemble.insert(Cle{1, 2});   // doublon — pas inséré

    std::println("ensemble.size() = {}", ensemble.size());   // 2

    dictionnaire[Cle{1, 2}] = "hello";
    dictionnaire[Cle{1, 3}] = "world";
    std::println("dictionnaire.size() = {}", dictionnaire.size());   // 2
}
