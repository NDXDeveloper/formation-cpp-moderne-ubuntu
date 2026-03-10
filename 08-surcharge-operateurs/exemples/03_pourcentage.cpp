/* ============================================================================
   Section 8.3 : Opérateurs de conversion
   Description : Conversion implicite vs explicit — Pourcentage vers double.
                 Démontre les pièges de la conversion implicite et la
                 solution avec explicit
   Fichier source : 03-operateurs-conversion.md
   ============================================================================ */
#include <iostream>

// Version avec conversion IMPLICITE
class PourcentageImplicite {
    double valeur_;
public:
    explicit PourcentageImplicite(double val) : valeur_{val} {}
    operator double() const { return valeur_ / 100.0; }
};

// Version avec conversion EXPLICITE
class PourcentageExplicite {
    double valeur_;
public:
    explicit PourcentageExplicite(double val) : valeur_{val} {}
    explicit operator double() const { return valeur_ / 100.0; }
};

int main() {
    std::cout << "=== Conversion implicite ===\n";
    PourcentageImplicite tva{20.0};
    double facteur = tva;            // conversion implicite → 0.2
    double prix = 100.0 * tva;       // 100.0 * 0.2 = 20.0
    int i = tva;                     // double → int (troncature silencieuse)
    auto r = tva + tva;              // résultat est un double, pas un Pourcentage

    std::cout << "facteur = " << facteur << "\n";   // 0.2
    std::cout << "prix = " << prix << "\n";         // 20
    std::cout << "i = " << i << "\n";               // 0
    std::cout << "tva + tva = " << r << "\n";       // 0.4

    std::cout << "\n=== Conversion explicite ===\n";
    PourcentageExplicite tva2{20.0};
    // double d1 = tva2;                 // ❌ Erreur : conversion implicite interdite
    double d2 = static_cast<double>(tva2);  // ✅ Cast explicite
    double d3 = double(tva2);               // ✅ Cast fonctionnel

    std::cout << "static_cast<double>(tva2) = " << d2 << "\n";  // 0.2
    std::cout << "double(tva2) = " << d3 << "\n";               // 0.2
}
