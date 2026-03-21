/* ============================================================================
   Section 41.1.2 : Cache lines et false sharing
   Description : Verification manuelle de la disposition en cache line
   Fichier source : 01.2-cache-lines.md
   ============================================================================ */

#include <cstddef>
#include <cstdint>
#include <iostream>

struct SuspectStruct {
    int counter_a;
    int counter_b;
};

int main() {
    SuspectStruct s;

    auto addr_a = reinterpret_cast<std::uintptr_t>(&s.counter_a);
    auto addr_b = reinterpret_cast<std::uintptr_t>(&s.counter_b);

    bool meme_ligne = (addr_a / 64) == (addr_b / 64);

    std::cout << "counter_a @ 0x" << std::hex << addr_a
              << " -> cache line " << std::dec << (addr_a / 64) << "\n";
    std::cout << "counter_b @ 0x" << std::hex << addr_b
              << " -> cache line " << std::dec << (addr_b / 64) << "\n";
    std::cout << "Meme cache line : " << (meme_ligne ? "OUI" : "NON") << "\n";
}
