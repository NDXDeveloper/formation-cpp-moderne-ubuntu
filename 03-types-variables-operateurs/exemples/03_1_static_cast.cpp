/* ============================================================================
   Section 3.3.1 : static_cast - Conversions sures
   Description : Conversions numeriques, division entiere vs flottante,
                 enum, enum class, to_underlying
   Fichier source : 03.1-static-cast.md
   ============================================================================ */
#include <print>
#include <cstdint>
#include <string>
#include <utility>

int main() {
    // --- Flottant vers entier (lignes 34-36) ---
    double pi = 3.14159;
    int n = static_cast<int>(pi); // 3
    std::print("static_cast<int>(3.14159) = {}\n", n);

    // --- Division entiere vs flottante (lignes 43-51) ---
    int numerator = 7;
    int denominator = 2;
    double bad = numerator / denominator;
    double good = static_cast<double>(numerator) / denominator;
    std::print("7 / 2 (entier)   = {}\n", bad);   // 3
    std::print("7 / 2 (flottant) = {}\n", good);  // 3.5

    // --- Entre types entiers (lignes 58-60) ---
    int64_t large = 42;
    int32_t small = static_cast<int32_t>(large);
    std::print("int64_t {} -> int32_t {}\n", large, small);

    // --- Signe vers non signe (lignes 73-76) ---
    int signed_val = -1;
    unsigned int unsigned_val = static_cast<unsigned int>(signed_val);
    std::print("static_cast<unsigned int>(-1) = {}\n", unsigned_val);

    // --- enum non-scoped (lignes 175-178) ---
    enum Color { Red, Green, Blue };
    int color_n = Red;
    Color c = static_cast<Color>(1);
    std::print("Red = {}, static_cast<Color>(1) = {}\n", color_n, static_cast<int>(c));

    // --- enum class (lignes 186-192) ---
    enum class Direction : uint8_t { North = 0, East = 1, South = 2, West = 3 };
    Direction d = static_cast<Direction>(1);
    int dir_n = static_cast<int>(d);
    std::print("Direction::East = {}\n", dir_n);

    // --- enum class avec to_underlying (C++23) (ligne 210) ---
    enum class LogLevel : int { Debug = 0, Info = 1, Warning = 2, Error = 3 };
    const char* names[] = {"DEBUG", "INFO", "WARNING", "ERROR"};
    LogLevel level = LogLevel::Warning;
    auto index = std::to_underlying(level);
    std::print("[{}] Message de test\n", names[index]);

    return 0;
}
