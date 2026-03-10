/* ============================================================================
   Section 7.1.2 : Heritage multiple — Mix-ins
   Description : Heritage multiple de mix-ins independants (NonCopiable,
                 Loggable) — usage idiomatique sans risque de diamant
   Fichier source : 01.2-heritage-multiple.md
   ============================================================================ */
#include <print>
#include <string>

class NonCopiable {
public:
    NonCopiable() = default;
    NonCopiable(NonCopiable const&) = delete;
    NonCopiable& operator=(NonCopiable const&) = delete;
};

class Loggable {
public:
    void log(std::string const& msg) const {
        std::println("[LOG] {}", msg);
    }
};

class Connexion : public NonCopiable, public Loggable {
    std::string hote_;
public:
    explicit Connexion(std::string hote) : hote_{std::move(hote)} {
        log("Connexion créée vers " + hote_);
    }
};

int main() {
    Connexion c{"example.com"};
    c.log("Envoi de données...");

    // Connexion c2 = c;  // ❌ Ne compile pas : NonCopiable
}
