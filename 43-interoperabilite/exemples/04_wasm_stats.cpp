/* ============================================================================
   Section 43.4.2 : Compilation et integration JavaScript
   Description : Programme de statistiques — compilable en natif et en Wasm
   Fichier source : 04.2-compilation-js.md
   ============================================================================ */

#include <cstdio>
#include <vector>
#include <numeric>
#include <cmath>

int main() {
    std::vector<double> data = {1.0, 2.0, 3.0, 4.0, 5.0};
    double mean = std::accumulate(data.begin(), data.end(), 0.0) / data.size();

    double variance = 0.0;
    for (double x : data) {
        variance += (x - mean) * (x - mean);
    }
    variance /= data.size();

    std::printf("Mean: %.2f\n", mean);
    std::printf("Variance: %.2f\n", variance);
    std::printf("Std Dev: %.2f\n", std::sqrt(variance));

    return 0;
}
