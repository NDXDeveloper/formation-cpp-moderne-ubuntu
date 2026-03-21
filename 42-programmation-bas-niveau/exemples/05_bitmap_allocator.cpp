/* ============================================================================
   Section 42.2 : Manipulation de Bits et Bitfields
   Description : Allocateur bitmap — gestion de 256 blocs via 4 mots de 64 bits
   Fichier source : 02-manipulation-bits.md
   ============================================================================ */

#include <bit>
#include <cstdint>
#include <array>
#include <optional>
#include <iostream>

class BitmapAllocator {
    static constexpr size_t NumBlocks = 256;
    static constexpr size_t WordBits = 64;
    static constexpr size_t NumWords = NumBlocks / WordBits;

    std::array<uint64_t, NumWords> bitmap_;

public:
    BitmapAllocator() {
        bitmap_.fill(~uint64_t{0});  // Tous les blocs sont libres
    }

    std::optional<size_t> allocate() {
        for (size_t w = 0; w < NumWords; ++w) {
            if (bitmap_[w] == 0) continue;
            int bit = std::countr_zero(bitmap_[w]);
            bitmap_[w] &= ~(uint64_t{1} << bit);
            return w * WordBits + bit;
        }
        return std::nullopt;
    }

    void deallocate(size_t index) {
        size_t w = index / WordBits;
        size_t bit = index % WordBits;
        bitmap_[w] |= (uint64_t{1} << bit);
    }

    size_t free_count() const {
        size_t count = 0;
        for (auto word : bitmap_)
            count += std::popcount(word);
        return count;
    }
};

int main() {
    BitmapAllocator alloc;
    std::cout << "Initial free: " << alloc.free_count() << "\n";  // 256

    auto b0 = alloc.allocate();
    auto b1 = alloc.allocate();
    auto b2 = alloc.allocate();
    std::cout << "Allocated: " << *b0 << ", " << *b1 << ", " << *b2 << "\n";  // 0, 1, 2
    std::cout << "Free: " << alloc.free_count() << "\n";  // 253

    alloc.deallocate(*b1);
    std::cout << "After freeing " << *b1 << ": " << alloc.free_count() << " free\n";  // 254

    auto b3 = alloc.allocate();
    std::cout << "Re-allocated: " << *b3 << "\n";  // 1 (reused)
    std::cout << "Free: " << alloc.free_count() << "\n";  // 253
}
