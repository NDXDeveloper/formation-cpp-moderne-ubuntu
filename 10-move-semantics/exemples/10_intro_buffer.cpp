/* ============================================================================
   Section 10.0 : Introduction — Sémantique de Mouvement
   Description : Buffer avec copie profonde vs déplacement, catégories de
                 valeurs (lvalue/rvalue), vector move, impact de noexcept
   Fichier source : README.md
   ============================================================================ */
#include <memory>
#include <print>
#include <cstring>
#include <vector>

// === Buffer avec copie et move (lignes 39-107) ===
class Buffer {
    size_t taille_;
    char* data_;

public:
    Buffer(size_t taille) : taille_(taille), data_(new char[taille]) {
        std::print("[Buffer] Allocation de {} octets\n", taille_);
    }

    Buffer(const Buffer& other) : taille_(other.taille_), data_(new char[other.taille_]) {
        std::memcpy(data_, other.data_, taille_);
        std::print("[Buffer] Copie de {} octets\n", taille_);
    }

    Buffer(Buffer&& other) noexcept
        : taille_(other.taille_), data_(other.data_)
    {
        other.taille_ = 0;
        other.data_ = nullptr;
        std::print("[Buffer] Déplacement de {} octets (coût : ~0)\n", taille_);
    }

    ~Buffer() {
        delete[] data_;
        if (taille_ > 0)
            std::print("[Buffer] Libération de {} octets\n", taille_);
    }
};

// === Catégories de valeurs (lignes 144-152) ===
void test_categories() {
    std::print("\n=== Catégories de valeurs ===\n");
    Buffer a(1024);

    std::print("--- Copie (lvalue) ---\n");
    Buffer b = a;                // copie

    std::print("--- Move (rvalue temporaire) ---\n");
    Buffer c = Buffer(1024);     // RVO ou move

    std::print("--- Move (std::move) ---\n");
    Buffer d = std::move(a);     // move
}

// === Vector move (lignes 164-172) ===
void test_vector_move() {
    std::print("\n=== Vector move ===\n");
    std::vector<int> v(10'000'000, 42);
    std::print("v.size() = {}\n", v.size());

    std::vector<int> copie = v;
    std::print("copie.size() = {}\n", copie.size());

    std::vector<int> deplace = std::move(v);
    std::print("deplace.size() = {}\n", deplace.size());
    std::print("v.size() après move = {}\n", v.size());
}

int main() {
    std::print("=== Buffer ===\n");
    {
        Buffer buf(1024);
    }

    test_categories();
    test_vector_move();
    std::print("\n✅ Tous les exemples passés\n");
}
