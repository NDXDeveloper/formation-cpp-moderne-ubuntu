/* ============================================================================
   Section 41.1 : Comprendre le cache CPU et la localite des donnees
   Description : Parcours row-major vs column-major d'une matrice 4096x4096
   Fichier source : 01-cache-cpu.md
   ============================================================================ */

#include <chrono>
#include <cstddef>
#include <iostream>

constexpr std::size_t N = 4096;
int matrix[N][N];

long long sum_row_major() {
    long long sum = 0;
    for (std::size_t i = 0; i < N; ++i)
        for (std::size_t j = 0; j < N; ++j)
            sum += matrix[i][j];
    return sum;
}

long long sum_col_major() {
    long long sum = 0;
    for (std::size_t j = 0; j < N; ++j)
        for (std::size_t i = 0; i < N; ++i)
            sum += matrix[i][j];
    return sum;
}

int main() {
    for (std::size_t i = 0; i < N; ++i)
        for (std::size_t j = 0; j < N; ++j)
            matrix[i][j] = static_cast<int>((i + j) % 256);

    auto t1 = std::chrono::high_resolution_clock::now();
    volatile long long r1 = sum_row_major();
    auto t2 = std::chrono::high_resolution_clock::now();
    auto ms1 = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

    auto t3 = std::chrono::high_resolution_clock::now();
    volatile long long r2 = sum_col_major();
    auto t4 = std::chrono::high_resolution_clock::now();
    auto ms2 = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count();

    std::cout << "Row-major:    " << ms1 << " ms\n";
    std::cout << "Column-major: " << ms2 << " ms\n";
    std::cout << "Ratio col/row: " << (ms1 > 0 ? static_cast<double>(ms2) / ms1 : 0) << "x\n";
}
