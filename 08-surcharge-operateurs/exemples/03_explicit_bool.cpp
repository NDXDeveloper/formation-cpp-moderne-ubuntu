/* ============================================================================
   Section 8.3 : Opérateurs de conversion
   Description : explicit operator bool() — contextes booléens autorisés
                 (if, while, !, &&, ||, ?:) vs conversions absurdes bloquées
   Fichier source : 03-operateurs-conversion.md
   ============================================================================ */
#include <iostream>

class Fichier {
    bool est_ouvert_;

public:
    Fichier(bool ouvert = false) : est_ouvert_{ouvert} {}

    explicit operator bool() const {
        return est_ouvert_;
    }
};

int main() {
    Fichier f{true};

    // Conversions absurdes bloquées par explicit :
    // int x = f;          // ❌ Erreur
    // f + f;              // ❌ Erreur
    // f < 42;             // ❌ Erreur

    std::cout << std::boolalpha;

    // Contextes booléens fonctionnels :
    if (f) { std::cout << "if (f): true\n"; }                     // ✅
    bool ok = f ? true : false;                                    // ✅
    bool b = f && true;                                            // ✅
    if (!Fichier{false}) { std::cout << "!Fichier{false}: true\n"; }  // ✅

    std::cout << "ok = " << ok << "\n";    // true
    std::cout << "b = " << b << "\n";      // true

    // Test avec un fichier fermé
    Fichier ferme{false};
    if (!ferme) { std::cout << "Fichier fermé détecté\n"; }
}
