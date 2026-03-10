/* ============================================================================
   Section 8.5 : Opérateur spaceship <=> (C++20)
   Description : Exemple complet — type Date avec = default, operator<<,
                 tri avec std::sort, tous les opérateurs de comparaison
   Fichier source : 05-operateur-spaceship.md
   ============================================================================ */
#include <compare>
#include <format>
#include <iostream>
#include <algorithm>
#include <vector>

class Date {
    int annee_;
    int mois_;    // 1-12
    int jour_;    // 1-31

public:
    Date(int annee, int mois, int jour)
        : annee_{annee}, mois_{mois}, jour_{jour} {}

    // L'ordre de déclaration (annee, mois, jour) correspond
    // exactement à l'ordre lexicographique souhaité → = default suffit
    std::strong_ordering operator<=>(Date const&) const = default;
    bool operator==(Date const&) const = default;

    friend std::ostream& operator<<(std::ostream& os, Date const& d) {
        os << std::format("{:04d}-{:02d}-{:02d}", d.annee_, d.mois_, d.jour_);
        return os;
    }
};

int main() {
    Date noel{2026, 12, 25};
    Date nouvel_an{2027, 1, 1};
    Date autre_noel{2026, 12, 25};

    std::cout << std::boolalpha;
    std::cout << noel << " < "  << nouvel_an  << "  → " << (noel < nouvel_an)  << "\n";  // true
    std::cout << noel << " == " << autre_noel  << " → " << (noel == autre_noel) << "\n";  // true
    std::cout << nouvel_an << " >= " << noel   << " → " << (nouvel_an >= noel)  << "\n";  // true
    std::cout << noel << " != " << nouvel_an   << " → " << (noel != nouvel_an)  << "\n";  // true

    // Fonctionne directement avec std::sort
    std::vector<Date> dates = {{2026, 3, 15}, {2025, 12, 1}, {2026, 3, 10}};
    std::sort(dates.begin(), dates.end());

    std::cout << "Dates triées : ";
    for (auto const& d : dates) std::cout << d << " ";
    std::cout << "\n";
    // 2025-12-01 2026-03-10 2026-03-15
}
