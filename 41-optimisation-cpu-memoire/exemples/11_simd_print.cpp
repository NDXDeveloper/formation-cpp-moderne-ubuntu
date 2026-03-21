/* ============================================================================
   Section 41.3.1 : Intrinsics
   Description : Impression d'un registre __m256 et operations de base AVX
   Fichier source : 03.1-intrinsics.md
   ============================================================================ */

#include <cstdio>
#include <immintrin.h>

void print_m256(__m256 v, const char* label) {
    alignas(32) float tmp[8];
    _mm256_store_ps(tmp, v);
    std::printf("%s: [%.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f]\n",
                label, tmp[0], tmp[1], tmp[2], tmp[3],
                tmp[4], tmp[5], tmp[6], tmp[7]);
}

int main() {
    // set1 : broadcast
    __m256 twos = _mm256_set1_ps(2.0f);
    print_m256(twos, "set1(2)");

    // setr : ordre naturel
    __m256 v = _mm256_setr_ps(1, 2, 3, 4, 5, 6, 7, 8);
    print_m256(v, "setr   ");

    // Arithmetique
    __m256 doubled = _mm256_mul_ps(v, twos);
    print_m256(doubled, "v * 2  ");

    __m256 added = _mm256_add_ps(v, doubled);
    print_m256(added, "v + 2v ");

    // Min / Max
    __m256 a = _mm256_setr_ps(1, 9, 3, 7, 5, 2, 8, 4);
    __m256 b = _mm256_setr_ps(4, 2, 7, 1, 8, 5, 3, 9);
    print_m256(_mm256_min_ps(a, b), "min    ");
    print_m256(_mm256_max_ps(a, b), "max    ");
}
