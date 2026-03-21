/* ============================================================================
   Section 41.1.1 : Cache L1, L2, L3
   Description : Acces aleatoire — observe les transitions L1, L2, L3, RAM
   Fichier source : 01.1-niveaux-cache.md
   ============================================================================ */

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

volatile std::uint64_t sink;

void random_access(const std::vector<int>& data,
                   const std::vector<std::size_t>& indices) {
    std::uint64_t sum = 0;
    for (auto idx : indices)
        sum += data[idx];
    sink = sum;
}

int main() {
    constexpr std::size_t NUM_ACCESSES = 10'000'000;

    for (std::size_t ARRAY_SIZE : {4096ul, 8192ul, 65536ul, 262144ul, 4000000ul, 16000000ul}) {
        std::vector<int> data(ARRAY_SIZE);
        std::iota(data.begin(), data.end(), 0);

        std::mt19937 gen(42);
        std::uniform_int_distribution<std::size_t> dist(0, ARRAY_SIZE - 1);
        std::vector<std::size_t> indices(NUM_ACCESSES);
        std::generate(indices.begin(), indices.end(), [&] { return dist(gen); });

        auto t1 = std::chrono::high_resolution_clock::now();
        random_access(data, indices);
        auto t2 = std::chrono::high_resolution_clock::now();
        double ns = std::chrono::duration<double, std::nano>(t2 - t1).count() / NUM_ACCESSES;

        std::cout << "Size: " << (ARRAY_SIZE * sizeof(int) / 1024)
                  << " KiB -> " << ns << " ns/access\n";
    }
}
