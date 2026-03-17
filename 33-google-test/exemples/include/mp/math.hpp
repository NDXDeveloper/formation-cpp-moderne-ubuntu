/* ============================================================================
   Section 33.2.1 : TEST — Tests simples
   Description : Bibliothèque mathématique utilisée comme cible de test
   Fichier source : 02.1-test-simple.md
   ============================================================================ */

#pragma once
#include <stdexcept>
#include <vector>
#include <numeric>

namespace mp {

inline int add(int a, int b) { return a + b; }

inline int divide(int numerator, int denominator) {
    if (denominator == 0)
        throw std::invalid_argument("Division by zero");
    return numerator / denominator;
}

inline double average(const std::vector<double>& values) {
    if (values.empty())
        throw std::invalid_argument("Empty vector");
    return std::accumulate(values.begin(), values.end(), 0.0)
           / static_cast<double>(values.size());
}

inline int factorial(int n) {
    if (n < 0) throw std::invalid_argument("Negative input");
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

} // namespace mp
