/* ============================================================================
   Section 41.1.1 : Cache L1, L2, L3
   Description : Multiplication matricielle naive vs tiled (cache blocking)
   Fichier source : 01.1-niveaux-cache.md
   ============================================================================ */

#include <chrono>
#include <cstddef>
#include <cstring>
#include <iostream>

constexpr std::size_t N = 1024;
int A[N][N], B[N][N], C[N][N];

void matmul_naive() {
    for (std::size_t i = 0; i < N; ++i)
        for (std::size_t j = 0; j < N; ++j)
            for (std::size_t k = 0; k < N; ++k)
                C[i][j] += A[i][k] * B[k][j];
}

void matmul_tiled() {
    constexpr std::size_t BLOCK = 64;
    for (std::size_t ii = 0; ii < N; ii += BLOCK)
        for (std::size_t jj = 0; jj < N; jj += BLOCK)
            for (std::size_t kk = 0; kk < N; kk += BLOCK)
                for (std::size_t i = ii; i < ii + BLOCK; ++i)
                    for (std::size_t j = jj; j < jj + BLOCK; ++j)
                        for (std::size_t k = kk; k < kk + BLOCK; ++k)
                            C[i][j] += A[i][k] * B[k][j];
}

int main() {
    for (std::size_t i = 0; i < N; ++i)
        for (std::size_t j = 0; j < N; ++j) {
            A[i][j] = static_cast<int>((i + j) % 100);
            B[i][j] = static_cast<int>((i * j) % 100);
        }

    std::memset(C, 0, sizeof(C));
    auto t1 = std::chrono::high_resolution_clock::now();
    matmul_naive();
    auto t2 = std::chrono::high_resolution_clock::now();
    auto ms1 = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    volatile int c1 = C[0][0];

    std::memset(C, 0, sizeof(C));
    auto t3 = std::chrono::high_resolution_clock::now();
    matmul_tiled();
    auto t4 = std::chrono::high_resolution_clock::now();
    auto ms2 = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count();
    volatile int c2 = C[0][0];

    std::cout << "Naive: " << ms1 << " ms\n";
    std::cout << "Tiled: " << ms2 << " ms\n";
    std::cout << "Speedup: " << (ms2 > 0 ? static_cast<double>(ms1) / ms2 : 0) << "x\n";
}
