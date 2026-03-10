/* ============================================================================
   Section 8.3 : Opérateurs de conversion
   Description : Conversion vers types complexes — string_view implicite
                 (léger) vs string explicit (copie). Asymétrie intentionnelle
   Fichier source : 03-operateurs-conversion.md
   ============================================================================ */
#include <string>
#include <string_view>
#include <iostream>

class Identifiant {
    std::string valeur_;

public:
    explicit Identifiant(std::string val) : valeur_{std::move(val)} {}

    // Conversion vers std::string_view (légère, pas de copie)
    operator std::string_view() const noexcept {
        return valeur_;
    }

    // Conversion explicite vers std::string (copie)
    explicit operator std::string() const {
        return valeur_;
    }
};

void afficher(std::string_view s) {
    std::cout << "afficher: " << s << "\n";
}

int main() {
    Identifiant id{"ABC-123"};

    std::string_view sv = id;                       // ✅ Implicite — léger
    afficher(id);                                    // ✅ Implicite

    // std::string s1 = id;                          // ❌ Erreur — explicit
    std::string s2 = static_cast<std::string>(id);  // ✅ Cast explicite

    std::cout << "sv = " << sv << "\n";
    std::cout << "s2 = " << s2 << "\n";
}
