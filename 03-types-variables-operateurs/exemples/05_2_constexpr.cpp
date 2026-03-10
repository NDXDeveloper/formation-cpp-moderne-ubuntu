/* ============================================================================
   Section 3.5.2 : constexpr - Calcul a la compilation
   Description : Variables constexpr, factorial, if constexpr, vector
                 constexpr, Point, lookup table, fonctions utilitaires
   Fichier source : 05.2-constexpr.md
   ============================================================================ */
#include <print>
#include <array>
#include <vector>
#include <cstdint>
#include <type_traits>
#include <string>

// --- factorial (lignes 78-84) ---
constexpr int factorial(int n) {
    int result = 1;
    for (int i = 2; i <= n; ++i) {
        result *= i;
    }
    return result;
}

// --- if constexpr (lignes 156-165) ---
template <typename T>
std::string to_string_custom(const T& value) {
    if constexpr (std::is_arithmetic_v<T>) {
        return std::to_string(value);
    } else if constexpr (std::is_same_v<T, std::string>) {
        return value;
    } else {
        return "unknown";
    }
}

// --- vector constexpr C++20 (lignes 175-188) ---
constexpr int sum_of_squares(int n) {
    std::vector<int> squares;
    for (int i = 1; i <= n; ++i) {
        squares.push_back(i * i);
    }
    int sum = 0;
    for (int s : squares) {
        sum += s;
    }
    return sum;
}

// --- Point constexpr (lignes 205-221) ---
class Point {
    double x_, y_;
public:
    constexpr Point(double x, double y) : x_(x), y_(y) {}
    constexpr double x() const { return x_; }
    constexpr double y() const { return y_; }
    constexpr double distance_to_origin() const {
        return x_ * x_ + y_ * y_;
    }
};

// --- Lookup table (lignes 271-283) ---
constexpr auto generate_squares() {
    std::array<int, 100> table{};
    for (int i = 0; i < 100; ++i) {
        table[i] = i * i;
    }
    return table;
}

// --- Fonctions utilitaires (lignes 312-328) ---
constexpr int clamp(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

constexpr bool is_power_of_two(unsigned int n) {
    return n > 0 && (n & (n - 1)) == 0;
}

constexpr uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return (static_cast<uint32_t>(r) << 16)
         | (static_cast<uint32_t>(g) << 8)
         | static_cast<uint32_t>(b);
}

// --- Detection UB (lignes 346-352) ---
constexpr int divide(int a, int b) {
    return a / b;
}

int main() {
    // --- Variables constexpr (lignes 25-27) ---
    constexpr int max_size = 1024;
    constexpr double pi = 3.14159265358979;
    constexpr int doubled = max_size * 2;
    std::print("max_size={}, pi={}, doubled={}\n", max_size, pi, doubled);

    // --- constexpr vs const (lignes 42-55) ---
    constexpr int compile_val = 42;
    int arr1[compile_val];
    arr1[0] = 1;
    static_assert(compile_val == 42);
    std::print("arr1[0]={}\n", arr1[0]);

    // --- factorial (lignes 78-91) ---
    constexpr int fact10 = factorial(10);
    std::print("factorial(10)={}\n", fact10);
    static_assert(fact10 == 3'628'800);

    // --- if constexpr (lignes 156-165) ---
    std::print("to_string(42)={}\n", to_string_custom(42));
    std::print("to_string(3.14)={}\n", to_string_custom(3.14));
    std::print("to_string(string)={}\n", to_string_custom(std::string{"hello"}));

    // --- sum_of_squares (lignes 175-188) ---
    constexpr int result = sum_of_squares(10);
    std::print("sum_of_squares(10)={}\n", result);
    static_assert(result == 385);

    // --- Point (lignes 205-221) ---
    constexpr Point origin(0.0, 0.0);
    constexpr Point p(3.0, 4.0);
    constexpr double dist_sq = p.distance_to_origin();
    static_assert(dist_sq == 25.0);
    std::print("dist_sq={}\n", dist_sq);

    // --- Constantes (lignes 260-263) ---
    constexpr double pi2 = 3.14159265358979323846;
    constexpr double e  = 2.71828182845904523536;
    constexpr double speed_of_light = 299'792'458.0;
    std::print("pi={}, e={}, c={}\n", pi2, e, speed_of_light);

    // --- Lookup table (lignes 271-283) ---
    constexpr auto squares = generate_squares();
    int sq = squares[42];
    std::print("squares[42]={}\n", sq);
    static_assert(squares[42] == 1764);

    // --- Fonctions utilitaires (lignes 312-328) ---
    static_assert(clamp(5, 0, 10) == 5);
    static_assert(clamp(-1, 0, 10) == 0);
    static_assert(clamp(15, 0, 10) == 10);
    static_assert(is_power_of_two(1));
    static_assert(is_power_of_two(2));
    static_assert(is_power_of_two(4));
    static_assert(!is_power_of_two(3));

    constexpr auto red = rgb(255, 0, 0);
    std::print("red=0x{:06X}\n", red);
    static_assert(red == 0xFF0000);

    // --- Detection UB (lignes 346-352) ---
    constexpr int ok = divide(10, 2);
    std::print("divide(10,2)={}\n", ok);
    static_assert(ok == 5);

    return 0;
}
