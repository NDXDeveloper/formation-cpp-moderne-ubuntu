/* ============================================================================
   Section 18.1 : assert et static_assert
   Description : Démonstration complète des assertions runtime (assert) et
                 compile-time (static_assert) — préconditions, invariants,
                 message personnalisé, désactivation NDEBUG, type traits,
                 contraintes de plateforme et de structures
   Fichier source : 01-assert-static-assert.md
   ============================================================================ */

#include <cassert>
#include <print>
#include <type_traits>
#include <vector>
#include <bit>
#include <cstdint>

// === assert : vérification à l'exécution ===

double divide(double numerator, double denominator) {
    assert(denominator != 0.0 && "Le dénominateur ne doit jamais être zéro");
    return numerator / denominator;
}

// === Bons usages d'assert ===

struct Color { int r, g, b; };

class ImageBuffer {
    int width_, height_;
    std::vector<Color> buffer_;
public:
    ImageBuffer(int w, int h) : width_(w), height_(h), buffer_(w * h) {}

    void set_pixel(int x, int y, Color c) {
        assert(x >= 0 && x < width_);
        assert(y >= 0 && y < height_);
        buffer_[y * width_ + x] = c;
    }

    int width() const { return width_; }
    int height() const { return height_; }
};

class CircularBuffer {
    std::vector<int> data_;
    std::size_t capacity_;
    std::size_t count_ = 0;
    std::size_t head_ = 0;
public:
    explicit CircularBuffer(std::size_t cap) : data_(cap), capacity_(cap) {}

    void push(int value) {
        assert(count_ <= capacity_ && "Invariant violé : count dépasse capacity");
        if (count_ < capacity_) {
            data_[(head_ + count_) % capacity_] = value;
            ++count_;
        }
        assert(count_ <= capacity_ && "Invariant violé après push");
    }

    std::size_t size() const { return count_; }
};

enum class State { Running, Stopped, Paused };

void process_state(State state) {
    switch (state) {
        case State::Running:  std::print("  État: Running\n"); break;
        case State::Stopped:  std::print("  État: Stopped\n"); break;
        case State::Paused:   std::print("  État: Paused\n"); break;
        default:
            assert(false && "État inconnu — bug dans la machine à états");
    }
}

// === static_assert : vérification à la compilation ===

// Tailles et alignement
static_assert(sizeof(int) == 4, "Ce code requiert des entiers 32 bits");
static_assert(sizeof(void*) == 8, "Architecture 64 bits requise");
static_assert(alignof(double) == 8);

// constexpr
constexpr int VERSION_MAJOR = 3;
static_assert(VERSION_MAJOR >= 2, "Version minimale requise : 2.x");

// Sérialisation
static_assert(sizeof(float) == 4,  "IEEE 754 single precision attendu");
static_assert(sizeof(double) == 8, "IEEE 754 double precision attendu");

// Endianness (C++20)
static_assert(std::endian::native == std::endian::little,
              "Ce code suppose une architecture little-endian");

// Template contraint
template <typename T>
T add(T a, T b) {
    static_assert(std::is_arithmetic_v<T>,
                  "add() requiert un type arithmétique (entier ou flottant)");
    return a + b;
}

// Structure réseau
struct [[gnu::packed]] NetworkPacketHeader {
    uint8_t  version;
    uint8_t  type;
    uint16_t length;
    uint32_t sequence;
};

static_assert(sizeof(NetworkPacketHeader) == 8,
              "Le header réseau doit faire exactement 8 octets");
static_assert(std::is_trivially_copyable_v<NetworkPacketHeader>,
              "Le header doit être trivialement copiable pour memcpy/send");

// Type traits
class MyResource {
    int* data_;
public:
    MyResource() : data_(nullptr) {}
    MyResource(const MyResource&) = delete;
    MyResource(MyResource&& other) noexcept : data_(other.data_) {
        other.data_ = nullptr;
    }
    ~MyResource() { delete data_; }
};

static_assert(!std::is_copy_constructible_v<MyResource>,
              "MyResource ne doit pas être copiable");
static_assert(std::is_nothrow_move_constructible_v<MyResource>,
              "MyResource doit être déplaçable sans exception");

int main() {
    std::print("=== 1. assert basique (divide) ===\n");
    std::print("  divide(10, 3) = {}\n", divide(10.0, 3.0));
    std::print("  divide(100, 7) = {:.4f}\n", divide(100.0, 7.0));

    std::print("\n=== 2. Préconditions (set_pixel) ===\n");
    ImageBuffer img(100, 100);
    img.set_pixel(5, 3, {255, 0, 0});
    img.set_pixel(0, 0, {0, 255, 0});
    img.set_pixel(99, 99, {0, 0, 255});
    std::print("  3 pixels placés dans image {}x{}\n", img.width(), img.height());

    std::print("\n=== 3. Invariant de classe (CircularBuffer) ===\n");
    CircularBuffer cb(4);
    cb.push(10);
    cb.push(20);
    cb.push(30);
    std::print("  CircularBuffer : {} éléments (capacité 4)\n", cb.size());

    std::print("\n=== 4. Machine à états ===\n");
    process_state(State::Running);
    process_state(State::Paused);
    process_state(State::Stopped);

    std::print("\n=== 5. static_assert — templates ===\n");
    std::print("  add(3, 4) = {}\n", add(3, 4));
    std::print("  add(1.5, 2.3) = {}\n", add(1.5, 2.3));
    // add("a", "b");  // → erreur de compilation

    std::print("\n=== 6. static_assert — vérifications plateforme ===\n");
    std::print("  sizeof(int) = {}\n", sizeof(int));
    std::print("  sizeof(void*) = {}\n", sizeof(void*));
    std::print("  sizeof(NetworkPacketHeader) = {}\n", sizeof(NetworkPacketHeader));
    std::print("  endian = little : {}\n",
               std::endian::native == std::endian::little);

    std::print("\nTous les assert et static_assert passés !\n");
    return 0;
}
