/* ============================================================================
   Section 3.1.1 : auto - Deduction automatique du type
   Description : Regles de deduction auto, references, lambdas generiques,
                 structured bindings
   Fichier source : 01.1-auto.md
   ============================================================================ */
#include <print>
#include <string>
#include <vector>
#include <map>
#include <type_traits>

using namespace std::string_literals;

int main() {
    // --- Syntaxe de base (lignes 30-36) ---
    auto count = 0;              // int
    auto ratio = 1.5;            // double
    auto flag = true;            // bool
    auto letter = 'A';           // char
    auto message = "bonjour";    // const char*
    std::print("count={}, ratio={}, flag={}, letter={}, message={}\n",
               count, ratio, flag, letter, message);

    // --- Deduction de base : qualificateurs retires (lignes 57-64) ---
    const int cx = 10;
    auto a = cx;          // int, pas const int
    static_assert(std::is_same_v<decltype(a), int>);
    a = 99;  // modifiable
    std::print("a={} (modifie, cx={})\n", a, cx);

    // --- Deduction avec reference auto& (lignes 72-77) ---
    auto& ref = cx;       // const int& (const preserve)
    static_assert(std::is_same_v<decltype(ref), const int&>);
    std::print("ref={}\n", ref);

    // --- Deduction avec reference universelle auto&& (lignes 90-97) ---
    int x = 10;
    const int cx2 = 20;
    auto&& r1 = x;       // int&
    auto&& r2 = cx2;     // const int&
    auto&& r3 = 42;      // int&&
    static_assert(std::is_same_v<decltype(r1), int&>);
    static_assert(std::is_same_v<decltype(r2), const int&>);
    static_assert(std::is_same_v<decltype(r3), int&&>);
    std::print("r1={}, r2={}, r3={}\n", r1, r2, r3);

    // --- Initialiser par accolades C++17 (lignes 124-129) ---
    auto a2 = {1, 2, 3};  // std::initializer_list<int>
    auto b2{42};           // int
    static_assert(std::is_same_v<decltype(b2), int>);
    std::print("b2={}, a2.size()={}\n", b2, a2.size());

    // --- Lambda generique (lignes 224-228) ---
    auto square = [](auto x) { return x * x; };
    std::print("square(5)={}\n", square(5));       // 25
    std::print("square(2.5)={}\n", square(2.5));   // 6.25

    // --- Structured bindings (lignes 312-316) ---
    std::map<std::string, int> scores = {{"Alice", 95}, {"Bob", 87}};
    for (const auto& [name, score] : scores) {
        std::print("{} : {}\n", name, score);
    }

    // --- Range-based for avec const auto& (lignes 166-177) ---
    std::vector<std::string> names = {"Alice", "Bob", "Charlie"};
    for (const auto& name : names) {
        std::print("  {}\n", name);
    }

    // --- Type de retour auto (lignes 186-188) ---
    auto add = [](int a, int b) { return a + b; };
    std::print("add(3,4)={}\n", add(3, 4));

    // --- Trailing return type (lignes 211-214) ---
    auto multiply = [](auto a, auto b) -> decltype(a * b) {
        return a * b;
    };
    std::print("multiply(3, 2.5)={}\n", multiply(3, 2.5));

    return 0;
}
