/* ============================================================================
   Section 41.2 : Branch prediction et optimisation des conditions
   Description : Somme conditionnelle branchless via masque arithmetique
   Fichier source : 02-branch-prediction.md
   ============================================================================ */

#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

volatile std::int64_t sink;

// Version avec branchement
void sum_branched(const std::vector<int>& data) {
    std::int64_t sum = 0;
    for (auto val : data) {
        if (val >= 128)
            sum += val;
    }
    sink = sum;
}

// Version branchless — pas de if
void sum_branchless(const std::vector<int>& data) {
    std::int64_t sum = 0;
    for (auto val : data) {
        int mask = -(val >= 128);  // 0xFFFFFFFF si vrai, 0 sinon
        sum += (val & mask);
    }
    sink = sum;
}

int main() {
    constexpr int N = 10'000'000;
    std::vector<int> data(N);

    std::mt19937 gen(42);
    std::uniform_int_distribution<int> dist(0, 255);
    for (auto& v : data) v = dist(gen);

    auto t1 = std::chrono::high_resolution_clock::now();
    sum_branched(data);
    auto t2 = std::chrono::high_resolution_clock::now();
    auto ms1 = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    auto saved_sum = static_cast<std::int64_t>(sink);

    auto t3 = std::chrono::high_resolution_clock::now();
    sum_branchless(data);
    auto t4 = std::chrono::high_resolution_clock::now();
    auto ms2 = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count();

    std::cout << "Branched:   " << ms1 << " ms (sum=" << saved_sum << ")\n";
    std::cout << "Branchless: " << ms2 << " ms (sum=" << static_cast<std::int64_t>(sink) << ")\n";
    std::cout << "Les deux sommes doivent etre identiques.\n";
    std::cout << "Note: avec -O2 le compilateur peut rendre les deux equivalents.\n";
    std::cout << "      Compiler avec -fno-if-conversion -fno-tree-vectorize\n";
    std::cout << "      pour observer la difference brute.\n";
}
