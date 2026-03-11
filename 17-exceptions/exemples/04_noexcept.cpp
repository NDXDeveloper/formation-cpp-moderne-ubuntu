/* ============================================================================
   Section 17.4 : noexcept et garanties d'exception
   Description : Les trois niveaux de garantie d'exception (basique, forte,
                 no-throw), idiome copy-and-swap pour la garantie forte,
                 impact de noexcept sur std::vector (move vs copy)
   Fichier source : 04-noexcept.md
   ============================================================================ */

#include <stdexcept>
#include <print>
#include <string>
#include <vector>
#include <utility>
#include <type_traits>
#include <cstring>

// === Garantie forte : idiome copy-modify-swap ===
struct Compte {
    std::string nom;
    double solde;

    void debiter(double montant) {
        if (montant > solde)
            throw std::runtime_error("Solde insuffisant");
        solde -= montant;
    }
    void crediter(double montant) {
        solde += montant;
    }
    friend void swap(Compte& a, Compte& b) noexcept {
        using std::swap;
        swap(a.nom, b.nom);
        swap(a.solde, b.solde);
    }
};

void transferer_fort(Compte& source, Compte& destination, double montant) {
    // Travailler sur des copies
    Compte source_copie = source;
    Compte destination_copie = destination;

    source_copie.debiter(montant);
    destination_copie.crediter(montant);

    // Commit : les swap sont noexcept
    using std::swap;
    swap(source, source_copie);
    swap(destination, destination_copie);
}

// === Impact de noexcept sur vector : move vs copy ===
struct Rapide {
    std::string data;
    Rapide() : data("hello") {}
    Rapide(const Rapide& o) : data(o.data) {
        std::print("    copie Rapide\n");
    }
    Rapide(Rapide&& o) noexcept : data(std::move(o.data)) {
        std::print("    move Rapide\n");
    }
    Rapide& operator=(Rapide&&) noexcept = default;
};

struct Lent {
    std::string data;
    Lent() : data("hello") {}
    Lent(const Lent& o) : data(o.data) {
        std::print("    copie Lent\n");
    }
    Lent(Lent&& o) : data(std::move(o.data)) {  // PAS noexcept !
        std::print("    move Lent\n");
    }
    Lent& operator=(Lent&&) = default;
};

// === Buffer avec move noexcept ===
class Buffer {
public:
    Buffer() = default;
    explicit Buffer(std::size_t taille)
        : taille_(taille), data_(new int[taille]{})
    {}

    ~Buffer() { delete[] data_; }

    Buffer(const Buffer& other)
        : taille_(other.taille_), data_(new int[other.taille_])
    {
        std::memcpy(data_, other.data_, taille_ * sizeof(int));
    }

    Buffer(Buffer&& other) noexcept
        : taille_(other.taille_), data_(other.data_)
    {
        other.taille_ = 0;
        other.data_ = nullptr;
    }

    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            delete[] data_;
            data_ = other.data_;
            taille_ = other.taille_;
            other.data_ = nullptr;
            other.taille_ = 0;
        }
        return *this;
    }

    Buffer& operator=(const Buffer& other) {
        if (this != &other) {
            auto copie = other;
            std::swap(taille_, copie.taille_);
            std::swap(data_, copie.data_);
        }
        return *this;
    }

    std::size_t taille() const noexcept { return taille_; }

private:
    std::size_t taille_ = 0;
    int*        data_ = nullptr;
};

int main() {
    std::print("=== 1. Garantie forte (copy-modify-swap) ===\n");
    Compte alice{"Alice", 1000.0};
    Compte bob{"Bob", 500.0};

    std::print("Avant : Alice={}, Bob={}\n", alice.solde, bob.solde);
    transferer_fort(alice, bob, 200.0);
    std::print("Après : Alice={}, Bob={}\n", alice.solde, bob.solde);

    // Tentative avec solde insuffisant
    try {
        transferer_fort(alice, bob, 5000.0);
    } catch (const std::runtime_error& e) {
        std::print("Erreur : {} → état inchangé : Alice={}, Bob={}\n",
                   e.what(), alice.solde, bob.solde);
    }

    std::print("\n=== 2. noexcept move → vector déplace ===\n");
    {
        std::vector<Rapide> v;
        v.reserve(1);
        v.emplace_back();
        std::print("  push_back (réallocation) :\n");
        v.push_back(Rapide{});  // réallocation → MOVE (noexcept)
    }

    std::print("\n=== 3. move sans noexcept → vector copie ===\n");
    {
        std::vector<Lent> v;
        v.reserve(1);
        v.emplace_back();
        std::print("  push_back (réallocation) :\n");
        v.push_back(Lent{});  // réallocation → COPIE (pas noexcept)
    }

    std::print("\n=== 4. static_assert sur les traits noexcept ===\n");
    static_assert(std::is_nothrow_move_constructible_v<Rapide>,
                  "Rapide doit être nothrow-move-constructible");
    static_assert(!std::is_nothrow_move_constructible_v<Lent>,
                  "Lent n'est PAS nothrow-move-constructible");
    static_assert(std::is_nothrow_move_constructible_v<Buffer>,
                  "Buffer doit être nothrow-move-constructible");
    static_assert(std::is_nothrow_move_assignable_v<Buffer>,
                  "Buffer doit être nothrow-move-assignable");
    std::print("Tous les static_assert passés !\n");

    std::print("\n=== 5. Buffer dans un vector ===\n");
    {
        std::vector<Buffer> v;
        v.push_back(Buffer(100));
        v.push_back(Buffer(200));
        std::print("vector<Buffer> : {} éléments, tailles = {} et {}\n",
                   v.size(), v[0].taille(), v[1].taille());
    }

    std::print("\nProgramme terminé.\n");
    return 0;
}
