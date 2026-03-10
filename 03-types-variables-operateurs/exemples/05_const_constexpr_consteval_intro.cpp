/* ============================================================================
   Section 3.5 : const, constexpr et consteval - Introduction
   Description : Spectre d'evaluation, constexpr square, consteval cube,
                 consteval compile_time_hash
   Fichier source : 05-const-constexpr-consteval.md
   ============================================================================ */
#include <print>
#include <cstdint>

// --- constexpr (lignes 81-86) ---
constexpr int square(int n) { return n * n; }

// --- consteval (lignes 91-92) ---
consteval int cube(int n) { return n * n * n; }

// --- consteval hash (lignes 169-178) ---
consteval uint32_t compile_time_hash(const char* str) {
    uint32_t hash = 0;
    while (*str) {
        hash = hash * 31 + static_cast<uint32_t>(*str++);
    }
    return hash;
}

int main() {
    // constexpr square
    constexpr int a = square(5);
    std::print("square(5) = {}\n", a);
    static_assert(a == 25);

    // consteval cube
    constexpr int b = cube(3);
    std::print("cube(3) = {}\n", b);
    static_assert(b == 27);

    // consteval hash
    constexpr auto id = compile_time_hash("user_created");
    std::print("hash('user_created') = {}\n", id);

    return 0;
}
