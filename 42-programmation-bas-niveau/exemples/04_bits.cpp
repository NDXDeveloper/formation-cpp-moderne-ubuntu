/* ============================================================================
   Section 42.2 : Manipulation de Bits et Bitfields
   Description : Operations bit a bit, idiomes classiques, header <bit> (C++20)
   Fichier source : 02-manipulation-bits.md
   ============================================================================ */

#include <bit>
#include <cstdint>
#include <iostream>

bool is_bit_set(uint32_t value, int position) {
    return (value & (1u << position)) != 0;
}

uint32_t set_bit(uint32_t value, int position) {
    return value | (1u << position);
}

uint32_t clear_bit(uint32_t value, int position) {
    return value & ~(1u << position);
}

uint32_t toggle_bit(uint32_t value, int position) {
    return value ^ (1u << position);
}

uint32_t extract_bits(uint32_t value, int start, int width) {
    uint32_t mask = (1u << width) - 1;
    return (value >> start) & mask;
}

bool is_power_of_2(uint32_t n) {
    return n != 0 && (n & (n - 1)) == 0;
}

uint32_t next_power_of_2(uint32_t n) {
    if (n == 0) return 1;
    n--;
    n |= n >> 1; n |= n >> 2; n |= n >> 4; n |= n >> 8; n |= n >> 16;
    return n + 1;
}

int main() {
    // --- Operations de base ---
    uint32_t flags = 0b0000'1010;
    std::cout << "is_bit_set(0b1010, 3) = " << is_bit_set(flags, 3) << "\n";  // 1 (true)
    std::cout << "is_bit_set(0b1010, 2) = " << is_bit_set(flags, 2) << "\n";  // 0 (false)

    flags = set_bit(flags, 2);       // 0b0000'1110
    std::cout << "set_bit(2): 0x" << std::hex << flags << std::dec << "\n";    // 0xe

    flags = clear_bit(flags, 1);     // 0b0000'1100
    std::cout << "clear_bit(1): 0x" << std::hex << flags << std::dec << "\n";  // 0xc

    flags = toggle_bit(flags, 3);    // 0b0000'0100
    std::cout << "toggle_bit(3): 0x" << std::hex << flags << std::dec << "\n"; // 0x4

    // --- Extraction RGB565 ---
    uint16_t pixel = 0b1101011010001011;
    std::cout << "RGB565: R=" << (int)extract_bits(pixel, 11, 5)
              << " G=" << (int)extract_bits(pixel, 5, 6)
              << " B=" << (int)extract_bits(pixel, 0, 5) << "\n";
    // R=26, G=52 (110100=52), B=11

    // --- Puissances de 2 ---
    std::cout << "is_power_of_2(8) = " << is_power_of_2(8) << "\n";          // 1
    std::cout << "is_power_of_2(6) = " << is_power_of_2(6) << "\n";          // 0
    std::cout << "next_power_of_2(100) = " << next_power_of_2(100) << "\n";  // 128

    // --- <bit> C++20 ---
    uint32_t value = 0b0000'0000'0010'1100'0000'0000'0000'0000u;
    std::cout << "popcount = " << std::popcount(value) << "\n";              // 3
    std::cout << "countr_zero = " << std::countr_zero(value) << "\n";        // 18
    std::cout << "countl_zero = " << std::countl_zero(value) << "\n";        // 10
    std::cout << "bit_ceil(100) = " << std::bit_ceil(100u) << "\n";          // 128
    std::cout << "bit_floor(100) = " << std::bit_floor(100u) << "\n";        // 64
    std::cout << "has_single_bit(64) = " << std::has_single_bit(64u) << "\n";// 1

    // --- Endianness ---
    if constexpr (std::endian::native == std::endian::little)
        std::cout << "Endianness: little-endian\n";
    else
        std::cout << "Endianness: big-endian\n";
}
