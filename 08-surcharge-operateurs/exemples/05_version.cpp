/* ============================================================================
   Section 8.5 : Opérateur spaceship <=> (C++20)
   Description : Comparaison hétérogène — Version vs Version et Version vs int,
                 avec réécriture automatique (1 < v → v <=> 1 > 0)
   Fichier source : 05-operateur-spaceship.md
   ============================================================================ */
#include <compare>
#include <print>

class Version {
    int majeure_, mineure_, patch_;

public:
    Version(int maj, int min, int pat)
        : majeure_{maj}, mineure_{min}, patch_{pat} {}

    // Comparaison Version <=> Version
    std::strong_ordering operator<=>(Version const&) const = default;
    bool operator==(Version const&) const = default;

    // Comparaison Version <=> int (compare uniquement la version majeure)
    std::strong_ordering operator<=>(int majeure) const {
        return majeure_ <=> majeure;
    }

    bool operator==(int majeure) const {
        return majeure_ == majeure;
    }
};

int main() {
    Version v{2, 1, 3};

    std::println("{}", v > Version{1, 9, 9});   // true  (2.1.3 > 1.9.9)
    std::println("{}", v < 3);                   // true  (majeure 2 < 3)
    std::println("{}", 1 < v);                   // true  — réécriture automatique !
}
