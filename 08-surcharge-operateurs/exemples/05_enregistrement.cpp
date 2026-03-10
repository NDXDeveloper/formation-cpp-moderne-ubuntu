/* ============================================================================
   Section 8.5 : Opérateur spaceship <=> (C++20)
   Description : Implémentation personnalisée — ignorer certains membres
                 (timestamp_ non inclus dans la comparaison)
   Fichier source : 05-operateur-spaceship.md
   ============================================================================ */
#include <compare>
#include <string>
#include <print>
#include <cstdint>

class Enregistrement {
    int id_;                 // clé de comparaison
    std::string contenu_;    // clé de comparaison
    uint64_t timestamp_;     // PAS une clé de comparaison (métadonnée)

public:
    Enregistrement(int id, std::string contenu, uint64_t ts)
        : id_{id}, contenu_{std::move(contenu)}, timestamp_{ts} {}

    std::strong_ordering operator<=>(Enregistrement const& rhs) const {
        if (auto cmp = id_ <=> rhs.id_; cmp != 0) return cmp;
        return contenu_ <=> rhs.contenu_;
        // timestamp_ est délibérément ignoré
    }

    bool operator==(Enregistrement const& rhs) const {
        return id_ == rhs.id_ && contenu_ == rhs.contenu_;
        // cohérent avec <=> : timestamp_ ignoré
    }
};

int main() {
    Enregistrement a{1, "hello", 1000};
    Enregistrement b{1, "hello", 2000};   // même id/contenu, timestamp différent
    Enregistrement c{2, "world", 1500};

    std::println("a == b (timestamps différents) → {}", a == b);   // true
    std::println("a <  c → {}", a < c);    // true  (id 1 < id 2)
    std::println("a != c → {}", a != c);   // true
}
