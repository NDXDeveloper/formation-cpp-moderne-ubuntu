/* ============================================================================
   Section 3.3.2 : reinterpret_cast - Reinterpretation memoire
   Description : Inspection octets, pointeur/entier, memcpy, bit_cast
   Fichier source : 03.2-reinterpret-cast.md
   ============================================================================ */
#include <print>
#include <cstdint>
#include <cstring>
#include <bit>

int main() {
    // --- Inspection des octets (lignes 48-54) ---
    uint32_t value = 0xDEADBEEF;
    auto* bytes = reinterpret_cast<unsigned char*>(&value);
    std::print("Octets de 0xDEADBEEF : ");
    for (std::size_t i = 0; i < sizeof(value); ++i) {
        std::print("{:02X} ", bytes[i]);
    }
    std::print("\n"); // Little-endian: EF BE AD DE

    // --- Pointeur <-> uintptr_t (lignes 99-104) ---
    int x = 42;
    uintptr_t addr = reinterpret_cast<uintptr_t>(&x);
    std::print("Adresse de x : 0x{:X}\n", addr);
    int* ptr = reinterpret_cast<int*>(addr);
    std::print("Valeur via ptr : {}\n", *ptr);

    // --- std::memcpy (lignes 140-147) ---
    float f = 3.14f;
    uint32_t bits_memcpy;
    std::memcpy(&bits_memcpy, &f, sizeof(bits_memcpy));
    std::print("Bits de 3.14f (memcpy) : 0x{:08X}\n", bits_memcpy);

    // --- std::bit_cast (C++20) (lignes 155-160) ---
    auto bits_bitcast = std::bit_cast<uint32_t>(f);
    std::print("Bits de 3.14f (bit_cast) : 0x{:08X}\n", bits_bitcast);

    // Verification
    static_assert(std::bit_cast<uint32_t>(3.14f) == 0x4048F5C3);

    return 0;
}
