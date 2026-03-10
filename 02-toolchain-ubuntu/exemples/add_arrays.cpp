/* ============================================================================
   Section 2.1.2 : Comparaison GCC vs Clang — Auto-vectorisation
   Description : Boucle vectorisable pour comparer l'assembleur GCC vs Clang
   Fichier source : 01.2-gcc-vs-clang.md
   Compilation : g++ -O2 -march=native -Wall -Wextra add_arrays.cpp -o add_arrays
   Assembleur  : g++ -O2 -march=native -S -o add_gcc.s add_arrays.cpp
   ============================================================================ */
#include <iostream>
#include <vector>

void add_arrays(float* __restrict__ dst,
                const float* __restrict__ a,
                const float* __restrict__ b,
                int n) {
    for (int i = 0; i < n; ++i)
        dst[i] = a[i] + b[i];
}

int main() {
    const int N = 8;
    std::vector<float> a(N), b(N), dst(N);

    for (int i = 0; i < N; ++i) {
        a[i] = static_cast<float>(i);
        b[i] = static_cast<float>(i * 10);
    }

    add_arrays(dst.data(), a.data(), b.data(), N);

    std::cout << "Résultat : ";
    for (int i = 0; i < N; ++i) {
        std::cout << dst[i] << " ";
    }
    std::cout << std::endl;

    return 0;
}
