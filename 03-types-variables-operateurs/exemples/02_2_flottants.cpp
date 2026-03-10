/* ============================================================================
   Section 3.2.2 : Flottants - float, double, long double
   Description : IEEE 754, limites, NaN, infini, erreurs d'arrondi,
                 absorption, comparaison avec epsilon
   Fichier source : 02.2-flottants.md
   ============================================================================ */
#include <print>
#include <limits>
#include <cmath>

bool nearly_equal(double a, double b, double epsilon = 1e-9) {
    return std::abs(a - b) < epsilon;
}

bool nearly_equal_relative(double a, double b, double rel_epsilon = 1e-9) {
    double diff = std::abs(a - b);
    double largest = std::max(std::abs(a), std::abs(b));
    return diff <= largest * rel_epsilon;
}

int main() {
    // --- Limites (lignes 63-76) ---
    std::print("float  — max : {}, epsilon : {}\n",
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::epsilon());
    std::print("double — max : {}, epsilon : {}\n",
        std::numeric_limits<double>::max(),
        std::numeric_limits<double>::epsilon());
    std::print("double — digits10 : {}\n",
        std::numeric_limits<double>::digits10);

    // --- Litteraux (lignes 87-90) ---
    auto a = 3.14;      // double
    auto b = 1.0;       // double
    auto c = 2.5e10;    // double
    auto d = 1e-3;      // double
    std::print("a={}, b={}, c={}, d={}\n", a, b, c, d);

    // --- Infini (lignes 131-135) ---
    double inf = 1.0 / 0.0;
    double neg_inf = -1.0 / 0.0;
    std::print("inf={}, neg_inf={}\n", inf, neg_inf);
    std::print("isinf(inf)={}\n", std::isinf(inf));

    // --- NaN (lignes 145-149) ---
    double nan = 0.0 / 0.0;
    double nan2 = std::sqrt(-1.0);
    std::print("nan={}\n", nan);
    std::print("isnan(nan)={}\n", std::isnan(nan));
    std::print("isnan(nan2)={}\n", std::isnan(nan2));

    // --- NaN != NaN (lignes 155-161) ---
    double qnan = std::numeric_limits<double>::quiet_NaN();
    if (qnan == qnan) {
        std::print("egal\n");
    } else {
        std::print("pas egal (NaN != NaN)\n");
    }

    // --- Zero positif et negatif (lignes 171-176) ---
    double pos_zero = +0.0;
    double neg_zero = -0.0;
    std::print("+0 == -0 : {}\n", pos_zero == neg_zero);
    std::print("1/+0 = {}\n", 1.0 / pos_zero);
    std::print("1/-0 = {}\n", 1.0 / neg_zero);

    // --- Erreurs d'arrondi (lignes 190-192) ---
    double x = 0.1;
    std::print("0.1 = {:.20f}\n", x);

    // --- Accumulation (lignes 197-202) ---
    double sum = 0.0;
    for (int i = 0; i < 10; ++i) {
        sum += 0.1;
    }
    std::print("sum(0.1 x 10) = {:.20f}\n", sum);

    // --- 0.1 + 0.2 != 0.3 (lignes 212-219) ---
    double fa = 0.1 + 0.2;
    double fb = 0.3;
    if (fa == fb) {
        std::print("egaux\n");
    } else {
        std::print("differents (0.1+0.2 != 0.3)\n");
    }

    // --- Comparaison avec epsilon (lignes 229-231) ---
    std::print("nearly_equal(0.1+0.2, 0.3) = {}\n", nearly_equal(fa, fb));

    // --- Absorption float (lignes 253-257) ---
    float big_f   = 1'000'000.0f;
    float small_f = 0.001f;
    float result_f = big_f + small_f;
    std::print("float: 1000000 + 0.001 = {:.6f}\n", result_f);

    // --- Absorption double (lignes 265-269) ---
    double big   = 1e16;
    double small_d = 1.0;
    double result = big + small_d;
    std::print("double: 1e16 + 1.0 = {:.1f}\n", result);

    return 0;
}
