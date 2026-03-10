/* ============================================================================
   Section 3.1.2 : decltype - Extraction du type d'une expression
   Description : Regles de decltype, parentheses, decltype(auto), templates
   Fichier source : 01.2-decltype.md
   ============================================================================ */
#include <print>
#include <vector>
#include <string>
#include <type_traits>
#include <utility>

// Trailing return type avec decltype (lignes 146-149)
template <typename T, typename U>
auto add(T a, U b) -> decltype(a + b) {
    return a + b;
}

// decltype(auto) pour retourner exactement le type (lignes 178-180)
std::vector<int> global_values = {10, 20, 30};

auto get_first_copy() {
    return global_values[0]; // int (copie)
}

decltype(auto) get_first_ref() {
    return global_values[0]; // int& (reference)
}

// execute_and_log (lignes 263-274)
template <typename Func>
void execute_and_log(Func func) {
    using ReturnType = decltype(func());
    if constexpr (std::is_void_v<ReturnType>) {
        func();
        std::print("Fonction executee (void)\n");
    } else {
        ReturnType result = func();
        std::print("Resultat : {}\n", result);
    }
}

int main() {
    // --- Syntaxe de base (lignes 22-28) ---
    int x = 42;
    decltype(x) y = 10;    // int
    static_assert(std::is_same_v<decltype(y), int>);

    const int cx = 100;
    decltype(cx) z = 50;   // const int
    static_assert(std::is_same_v<decltype(z), const int>);
    std::print("y={}, z={}\n", y, z);

    // --- Expressions complexes (lignes 34-39) ---
    int a = 1, b = 2;
    decltype(a + b) sum = 0; // int
    static_assert(std::is_same_v<decltype(sum), int>);

    std::vector<double> v = {1.0, 2.0, 3.0};
    decltype(v.size()) n = 0; // std::size_t
    static_assert(std::is_same_v<decltype(n), std::size_t>);
    std::print("sum={}, n={}\n", sum, n);

    // --- Regle 1 vs Regle 2 (lignes 60-67, 82-94) ---
    int& rx = x;
    static_assert(std::is_same_v<decltype(x), int>);         // identifiant -> type declare
    static_assert(std::is_same_v<decltype(cx), const int>);   // identifiant -> type declare
    static_assert(std::is_same_v<decltype(rx), int&>);        // identifiant -> type declare

    // Expressions (regle 2)
    static_assert(std::is_same_v<decltype((x)), int&>);       // parentheses -> lvalue -> T&
    static_assert(std::is_same_v<decltype(42), int>);          // prvalue -> T
    static_assert(std::is_same_v<decltype(x + 1), int>);      // prvalue -> T
    static_assert(std::is_same_v<decltype(std::move(x)), int&&>); // xvalue -> T&&
    std::print("Toutes les static_assert passees!\n");

    // --- Trailing return type (lignes 146-152) ---
    std::print("add(1, 2.5)={}\n", add(1, 2.5));   // double
    std::print("add(1, 2)={}\n", add(1, 2));         // int

    // --- decltype(auto) (lignes 170-180) ---
    auto copy = get_first_copy();
    decltype(auto) ref = get_first_ref();
    std::print("copy={}, ref={}\n", copy, ref);
    ref = 999;  // modifie global_values[0] via la reference
    std::print("global_values[0] apres modif={}\n", global_values[0]);

    // --- Variable decltype(auto) (lignes 189-194) ---
    const int& crx = x;
    auto a2 = crx;            // int (auto retire const et &)
    decltype(auto) b2 = crx;  // const int& (preserve tout)
    static_assert(std::is_same_v<decltype(a2), int>);
    static_assert(std::is_same_v<decltype(b2), const int&>);
    std::print("a2={}, b2={}\n", a2, b2);

    // --- Declarer une variable du meme type (lignes 241-242) ---
    std::vector<int> data = {1, 2, 3, 4, 5};
    decltype(data) backup = data;
    std::print("backup.size()={}\n", backup.size());

    // --- execute_and_log (lignes 263-274) ---
    execute_and_log([]{ return 42; });
    execute_and_log([]{ std::print("  [side effect] "); });

    return 0;
}
