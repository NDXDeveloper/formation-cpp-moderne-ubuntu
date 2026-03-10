/* ============================================================================
   Section 8.4 : Opérateur d'appel de fonction (operator())
   Description : Policy-based design — Cache paramétré par un foncteur
                 politique (ToutCacher, PetitesValeurs)
   Fichier source : 04-operateur-appel.md
   ============================================================================ */
#include <unordered_map>
#include <string>
#include <print>

template <typename Politique>
class Cache {
    std::unordered_map<std::string, std::string> data_;
    Politique politique_;

public:
    explicit Cache(Politique pol = {}) : politique_{std::move(pol)} {}

    void inserer(std::string const& cle, std::string const& valeur) {
        if (politique_(cle, valeur)) {
            data_[cle] = valeur;
        }
    }

    std::size_t taille() const { return data_.size(); }
};

// Politique : tout mettre en cache
struct ToutCacher {
    bool operator()(std::string const&, std::string const&) const {
        return true;
    }
};

// Politique : ne cacher que les petites valeurs
struct PetitesValeurs {
    std::size_t max_taille_;
    explicit PetitesValeurs(std::size_t max) : max_taille_{max} {}

    bool operator()(std::string const&, std::string const& valeur) const {
        return valeur.size() <= max_taille_;
    }
};

int main() {
    Cache<ToutCacher> cache_complet;
    Cache<PetitesValeurs> cache_leger{PetitesValeurs{5}};

    cache_complet.inserer("a", "hello world");
    cache_complet.inserer("b", "hi");
    std::println("cache_complet : {} éléments", cache_complet.taille());  // 2

    cache_leger.inserer("a", "hello world");  // trop long (11 > 5) → rejeté
    cache_leger.inserer("b", "hi");           // ok (2 <= 5) → accepté
    std::println("cache_leger   : {} éléments", cache_leger.taille());    // 1
}
