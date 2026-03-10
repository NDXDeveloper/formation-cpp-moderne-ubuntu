/* ============================================================================
   Section 3.2.1 : Entiers - int, long, short, int32_t, int64_t
   Description : Types a largeur fixe, promotions, wrap-around, comparaison
                 signee/non-signee, troncation, std::ssize
   Fichier source : 02.1-entiers.md
   ============================================================================ */
#include <print>
#include <cstdint>
#include <vector>
#include <type_traits>

int main() {
    // --- Types signes (lignes 27-31) ---
    signed char        sc = -128;
    short              s  = -32'000;
    int                i  = -2'000'000;
    long               l  = -1'000'000;
    long long          ll = -9'000'000'000'000'000'000LL;
    std::print("sc={}, s={}, i={}, l={}, ll={}\n", sc, s, i, l, ll);

    // --- Types non signes (lignes 42-47) ---
    unsigned char        uc  = 255;
    unsigned short       us  = 65'535;
    unsigned int         ui  = 4'000'000'000U;
    unsigned long        ul  = 4'000'000'000UL;
    unsigned long long   ull = 18'000'000'000'000'000'000ULL;
    std::print("uc={}, us={}, ui={}, ul={}, ull={}\n", uc, us, ui, ul, ull);

    // --- Types a largeur fixe (lignes 91-101) ---
    int8_t    a8 = -100;
    int16_t   a16 = -30'000;
    int32_t   a32 = -2'000'000;
    int64_t   a64 = -9'000'000'000'000'000'000LL;
    uint8_t   u8 = 200;
    uint16_t  u16 = 60'000;
    uint32_t  u32 = 4'000'000'000U;
    uint64_t  u64 = 18'000'000'000'000'000'000ULL;
    std::print("int8={}, int16={}, int32={}, int64={}\n", a8, a16, a32, a64);
    std::print("uint8={}, uint16={}, uint32={}, uint64={}\n", u8, u16, u32, u64);

    // --- Promotion integrale (lignes 192-195) ---
    short sa = 10;
    short sb = 20;
    auto result = sa + sb;
    static_assert(std::is_same_v<decltype(result), int>);
    std::print("short + short = {} (type int)\n", result);

    // --- Conversion arithmetique (lignes 210-212) ---
    int ii = 10;
    double d = 3.14;
    auto result2 = ii + d;
    static_assert(std::is_same_v<decltype(result2), double>);
    std::print("int + double = {} (type double)\n", result2);

    // --- Wrap-around non signe (lignes 238-240) ---
    uint32_t x = 0;
    x = x - 1;
    std::print("uint32_t(0) - 1 = {} (UINT32_MAX)\n", x);

    // --- Comparaison signee/non-signee (lignes 250-257) ---
    int ai = -1;
    unsigned int bi = 0;
    if (ai < static_cast<int>(bi)) {
        std::print("-1 < 0 (comparaison correcte avec cast)\n");
    }
    // Sans cast : -1 converti en unsigned = 4294967295 > 0
    // Ceci est le piege documente dans le .md

    // --- std::ssize (lignes 278-279) ---
    std::vector<int> v = {10, 20, 30};
    for (auto idx = 0; idx < std::ssize(v); ++idx) {
        std::print("v[{}]={} ", idx, v[idx]);
    }
    std::print("\n");

    // --- Troncation (lignes 290-291) ---
    int64_t big = 5'000'000'000LL;
    int32_t small_val = static_cast<int32_t>(big);
    std::print("Troncation: {} -> {}\n", big, small_val);

    // --- Litteraux (lignes 157-161) ---
    constexpr int decimal     = 42;
    constexpr int octal       = 052;
    constexpr int hexadecimal = 0x2A;
    constexpr int binary      = 0b101010;
    std::print("decimal={}, octal={}, hex={}, binary={}\n",
               decimal, octal, hexadecimal, binary);
    static_assert(decimal == octal && octal == hexadecimal && hexadecimal == binary);

    return 0;
}
