/* ============================================================================
   Section 29.4.1 : AddressSanitizer (-fsanitize=address)
   Description : Memory leak — allocations jamais libérées (détecté par LSan)
   Fichier source : 04.1-addresssanitizer.md
   Compilation : g++-15 -fsanitize=address -g -O1 -fno-omit-frame-pointer -o memory_leak 06_memory_leak.cpp
   ============================================================================ */

#include <cstdio>
#include <vector>

void process_data() {
    auto* buffer = new std::vector<int>(1000);
    buffer->push_back(42);
    // Oubli du delete — le pointeur sort de la portée
}

int main() {
    for (int i = 0; i < 100; ++i) {
        process_data();    // 100 allocations jamais libérées
    }
    std::printf("Terminé\n");
    return 0;
}
