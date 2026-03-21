/* ============================================================================
   Section 42.1 : Inline Assembly en C++
   Description : Lecture du Time-Stamp Counter (rdtscp) et do_not_optimize
   Fichier source : 01-inline-assembly.md
   ============================================================================ */

#include <cstdint>
#include <iostream>

struct TscReading {
    uint64_t ticks;
    uint32_t aux;
};

inline TscReading rdtscp() {
    uint32_t lo, hi, aux;
    asm volatile(
        "rdtscp"
        : "=a" (lo),
          "=d" (hi),
          "=c" (aux)
    );
    return {
        (static_cast<uint64_t>(hi) << 32) | lo,
        aux
    };
}

template <typename T>
inline void do_not_optimize(T& val) {
    asm volatile("" : "+r"(val) : : "memory");
}

int main() {
    auto start = rdtscp();

    // Travail a mesurer
    int sum = 0;
    for (int i = 0; i < 1000000; ++i)
        sum += i;
    do_not_optimize(sum);  // Empeche le compilateur d'eliminer la boucle

    auto end = rdtscp();
    uint64_t cycles = end.ticks - start.ticks;

    std::cout << "Cycles:  " << cycles << "\n";
    std::cout << "Core ID: " << start.aux << "\n";
    std::cout << "Sum:     " << sum << "\n";
}
