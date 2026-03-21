/* ============================================================================
   Section 41.2 : Branch prediction et optimisation des conditions
   Description : Remplacement d'un switch par une lookup table (zero branchement)
   Fichier source : 02-branch-prediction.md
   ============================================================================ */

#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

volatile int vsink;

// Version switch
int classify_switch(int val) {
    switch (val % 4) {
        case 0: return 10;
        case 1: return 20;
        case 2: return 30;
        case 3: return 40;
        default: return 0;
    }
}

// Version lookup table — zero branchement
int classify_lut(int val) {
    static constexpr int table[] = {10, 20, 30, 40};
    return table[val % 4];
}

int main() {
    constexpr int N = 10'000'000;
    std::vector<int> data(N);

    std::mt19937 gen(42);
    std::uniform_int_distribution<int> dist(0, 999);
    for (auto& v : data) v = dist(gen);

    // Switch
    auto t1 = std::chrono::high_resolution_clock::now();
    int sum1 = 0;
    for (auto v : data) sum1 += classify_switch(v);
    vsink = sum1;
    auto t2 = std::chrono::high_resolution_clock::now();

    // LUT
    auto t3 = std::chrono::high_resolution_clock::now();
    int sum2 = 0;
    for (auto v : data) sum2 += classify_lut(v);
    vsink = sum2;
    auto t4 = std::chrono::high_resolution_clock::now();

    auto ms1 = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    auto ms2 = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count();

    std::cout << "Switch: " << ms1 << " ms (sum=" << sum1 << ")\n";
    std::cout << "LUT:    " << ms2 << " ms (sum=" << sum2 << ")\n";
    std::cout << "Les deux sommes doivent etre identiques.\n";
}
