/* ============================================================================
   Section 41.2 : Branch prediction et optimisation des conditions
   Description : Impact du tri sur la prediction de branchement (sorted vs unsorted)
   Fichier source : 02-branch-prediction.md
   ============================================================================ */

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

volatile std::int64_t sink;

void conditional_sum(const std::vector<int>& data) {
    std::int64_t sum = 0;
    for (auto val : data) {
        if (val >= 128)
            sum += val;
    }
    sink = sum;
}

int main() {
    constexpr int N = 10'000'000;
    std::vector<int> data(N);

    std::mt19937 gen(42);
    std::uniform_int_distribution<int> dist(0, 255);
    for (auto& v : data) v = dist(gen);

    // Test 1 : donnees NON triees
    auto t1 = std::chrono::high_resolution_clock::now();
    conditional_sum(data);
    auto t2 = std::chrono::high_resolution_clock::now();
    auto ms1 = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

    // Test 2 : donnees TRIEES
    std::sort(data.begin(), data.end());
    auto t3 = std::chrono::high_resolution_clock::now();
    conditional_sum(data);
    auto t4 = std::chrono::high_resolution_clock::now();
    auto ms2 = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count();

    std::cout << "Non triees: " << ms1 << " ms\n";
    std::cout << "Triees:     " << ms2 << " ms\n";
    std::cout << "Note: avec -O2 le compilateur peut transformer le if en cmov\n";
    std::cout << "      (branchless), reduisant ou eliminant la difference.\n";
    std::cout << "      Compiler avec -fno-if-conversion pour observer l'effet pur.\n";
}
