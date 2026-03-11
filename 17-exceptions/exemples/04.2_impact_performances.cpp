/* ============================================================================
   Section 17.4.2 : Impact sur les performances
   Description : std::move_if_noexcept en action (Deplacable vs NonSur),
                 impact sur std::variant (valueless_by_exception),
                 vérification des traits noexcept avec static_assert
   Fichier source : 04.2-impact-performances.md
   ============================================================================ */

#include <print>
#include <utility>
#include <type_traits>
#include <vector>
#include <variant>
#include <string>
#include <stdexcept>
#include <cstring>

// === Observer std::move_if_noexcept en action ===
struct Deplacable {
    Deplacable() = default;
    Deplacable(const Deplacable&) { std::print("  → copie\n"); }
    Deplacable(Deplacable&&) noexcept { std::print("  → déplacement\n"); }
    Deplacable& operator=(Deplacable&&) noexcept = default;
};

struct NonSur {
    NonSur() = default;
    NonSur(const NonSur&) { std::print("  → copie\n"); }
    NonSur(NonSur&&) { std::print("  → déplacement\n"); } // pas noexcept !
    NonSur& operator=(NonSur&&) = default;
};

// === MoveOnly sans noexcept ===
struct MoveOnly {
    MoveOnly() = default;
    MoveOnly(const MoveOnly&) = delete;
    MoveOnly(MoveOnly&&) { std::print("  → déplacement (move-only)\n"); }
    MoveOnly& operator=(MoveOnly&&) = default;
};

// === Ressource pour vérification des traits ===
class Ressource {
public:
    explicit Ressource(std::size_t taille = 4096)
        : taille_(taille)
        , donnees_(new char[taille]{})
    {}

    ~Ressource() { delete[] donnees_; }

    Ressource(const Ressource& other)
        : taille_(other.taille_)
        , donnees_(new char[other.taille_])
    {
        std::memcpy(donnees_, other.donnees_, taille_);
    }

    Ressource& operator=(const Ressource& other) {
        if (this != &other) {
            auto copie = other;
            swap(*this, copie);
        }
        return *this;
    }

    Ressource(Ressource&& other) noexcept
        : taille_(other.taille_)
        , donnees_(other.donnees_)
    {
        other.taille_ = 0;
        other.donnees_ = nullptr;
    }

    Ressource& operator=(Ressource&& other) noexcept {
        if (this != &other) {
            delete[] donnees_;
            taille_ = other.taille_;
            donnees_ = other.donnees_;
            other.taille_ = 0;
            other.donnees_ = nullptr;
        }
        return *this;
    }

    friend void swap(Ressource& a, Ressource& b) noexcept {
        using std::swap;
        swap(a.taille_, b.taille_);
        swap(a.donnees_, b.donnees_);
    }

    std::size_t taille() const noexcept { return taille_; }

private:
    std::size_t taille_;
    char*       donnees_;
};

static_assert(std::is_nothrow_move_constructible_v<Ressource>,
              "Le vector copiera au lieu de déplacer !");
static_assert(std::is_nothrow_move_assignable_v<Ressource>,
              "L'affectation par déplacement n'est pas noexcept !");

// === Fragile pour démontrer valueless_by_exception ===
struct Fragile {
    Fragile() = default;
    Fragile(int) { throw std::runtime_error("construction échouée"); }
    Fragile(const Fragile&) = default;
    Fragile(Fragile&&) = default;
};

int main() {
    std::print("=== 1. std::move_if_noexcept — Deplacable (noexcept move) ===\n");
    {
        Deplacable d;
        std::print("move_if_noexcept :\n");
        [[maybe_unused]] auto d2 = std::move_if_noexcept(d);  // → déplacement
    }

    std::print("\n=== 2. std::move_if_noexcept — NonSur (move sans noexcept) ===\n");
    {
        NonSur n;
        std::print("move_if_noexcept :\n");
        [[maybe_unused]] auto n2 = std::move_if_noexcept(n);  // → copie (fallback sûr)
    }

    std::print("\n=== 3. MoveOnly sans noexcept — move forcé ===\n");
    {
        std::vector<MoveOnly> v;
        v.push_back(MoveOnly{});  // utilise le move malgré l'absence de noexcept
        std::print("  vector<MoveOnly> accepte le move-only\n");
    }

    std::print("\n=== 4. Ressource dans un vector ===\n");
    {
        std::vector<Ressource> v;
        v.push_back(Ressource(1024));
        v.push_back(Ressource(2048));
        v.push_back(Ressource(4096));
        std::print("  vector<Ressource> : {} éléments (tailles : {}, {}, {})\n",
                   v.size(), v[0].taille(), v[1].taille(), v[2].taille());
    }

    std::print("\n=== 5. Impact sur std::variant (valueless_by_exception) ===\n");
    {
        std::variant<int, Fragile> v = 42;
        std::print("  Avant : variant contient int = {}\n", std::get<int>(v));
        std::print("  valueless_by_exception = {}\n", v.valueless_by_exception());

        try {
            // emplace<Fragile>(42) détruit l'int, puis tente de construire Fragile(42)
            // qui lève → le variant se retrouve sans valeur
            v.emplace<Fragile>(42);
        } catch (const std::exception& e) {
            std::print("  Exception : {}\n", e.what());
            std::print("  valueless_by_exception = {}\n", v.valueless_by_exception());
        }
    }

    std::print("\n=== 6. Vérification des traits noexcept ===\n");
    std::print("  Deplacable nothrow-move : {}\n",
               std::is_nothrow_move_constructible_v<Deplacable>);
    std::print("  NonSur nothrow-move : {}\n",
               std::is_nothrow_move_constructible_v<NonSur>);
    std::print("  MoveOnly nothrow-move : {}\n",
               std::is_nothrow_move_constructible_v<MoveOnly>);
    std::print("  Ressource nothrow-move : {}\n",
               std::is_nothrow_move_constructible_v<Ressource>);

    std::print("\nTous les static_assert passés !\n");
    return 0;
}
