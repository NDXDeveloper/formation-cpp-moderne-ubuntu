/* ============================================================================
   Section 11.1 : Syntaxe des lambdas et types de captures
   Description : Exemples complets couvrant l'anatomie d'une lambda, le type
                 unique, la conversion en pointeur de fonction, les modes de
                 capture, le mot-clé mutable, constexpr/consteval, et
                 l'inférence de type retour
   Fichier source : 01-syntaxe-captures.md
   ============================================================================ */
#include <print>
#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <cstdlib>
#include <type_traits>

int main() {
    // --- Exemples progressifs de syntaxe (lignes 46-74) ---

    // Lambda minimale
    auto greet = [] { std::print("Hello\n"); };
    greet();  // Hello

    // Avec paramètres
    auto add = [](int a, int b) { return a + b; };
    std::print("{}\n", add(3, 4));  // 7

    // Trailing return type
    auto divide = [](int a, int b) -> double {
        return static_cast<double>(a) / b;
    };
    std::print("{}\n", divide(7, 2));  // 3.5

    // constexpr noexcept
    auto square = [](int x) constexpr noexcept -> int {
        return x * x;
    };
    static_assert(square(5) == 25);

    // --- Type unique d'une lambda (lignes 84-104) ---

    auto f = [](int x) { return x + 1; };
    auto g = [](int x) { return x + 1; };
    static_assert(!std::is_same_v<decltype(f), decltype(g)>);

    auto empty = [] {};
    auto with_int = [x = 42] { (void)x; };
    auto with_str = [s = std::string("hello")] { (void)s; };

    std::print("Taille lambda vide     : {} octets\n", sizeof(empty));     // 1
    std::print("Taille lambda avec int : {} octets\n", sizeof(with_int));  // 4
    std::print("Taille lambda avec str : {} octets\n", sizeof(with_str));  // 32 (typique)

    // --- Conversion en pointeur de fonction (lignes 114-134) ---

    auto lambda = [](int a, int b) { return a + b; };
    int (*fn_ptr)(int, int) = lambda;
    std::print("{}\n", fn_ptr(3, 4));  // 7

    // qsort avec lambda
    int arr[] = {5, 2, 8, 1, 9, 3};
    std::qsort(arr, 6, sizeof(int), [](const void* a, const void* b) -> int {
        int ia = *static_cast<const int*>(a);
        int ib = *static_cast<const int*>(b);
        return (ia > ib) - (ia < ib);
    });
    std::print("qsort: ");
    for (int v : arr) std::print("{} ", v);
    std::print("\n");  // 1 2 3 5 8 9

    // --- Vue d'ensemble des modes de capture (lignes 155-186) ---
    {
        int x = 10, y = 20, z = 30;

        auto b = [x]        { return x; };
        auto c = [&x]       { return x; };
        auto d = [=]        { return x + y + z; };
        auto e = [&]        { return x + y + z; };
        auto ff = [=, &y]   { return x + y + z; };
        auto gg = [&, x]    { return x + y + z; };
        auto h = [val = x * 2] { return val; };

        auto ptr = std::make_unique<int>(42);
        auto i = [p = std::move(ptr)] { return *p; };

        std::print("b={}, c={}, d={}, e={}, f={}, g={}, h={}, i={}\n",
            b(), c(), d(), e(), ff(), gg(), h(), i());
    }

    // --- Moment de la capture (lignes 235-251) ---
    {
        int value = 10;
        auto snapshot = [value] { return value; };
        value = 42;
        std::print("{}\n", snapshot());  // 10 (pas 42)
    }
    {
        int value = 10;
        auto live = [&value] { return value; };
        value = 42;
        std::print("{}\n", live());  // 42
    }

    // --- Mot-clé mutable (lignes 270-278) ---
    {
        int counter = 0;
        auto inc = [counter]() mutable { return ++counter; };
        std::print("{}\n", inc());  // 1
        std::print("{}\n", inc());  // 2
        std::print("{}\n", inc());  // 3
        std::print("{}\n", counter);  // 0
    }

    // --- Compteur sûr (lignes 318-325) ---
    {
        auto create_counter = []() -> std::function<int()> {
            int count = 0;
            return [count]() mutable { return ++count; };
        };
        auto counter = create_counter();
        std::print("{}\n", counter());  // 1
        std::print("{}\n", counter());  // 2
    }

    // --- Safe patterns (lignes 336-344) ---
    {
        std::vector<int> data = {1, 5, 3, 8, 2, 7};
        int threshold = 4;
        std::erase_if(data, [&threshold](int v) { return v < threshold; });
        std::print("Après filtrage (>= 4): ");
        for (int v : data) std::print("{} ", v);
        std::print("\n");  // 5 8 7

        auto deferred = [](int initial) {
            return [initial]() { return initial * 2; };
        };
        std::print("Deferred: {}\n", deferred(21)());  // 42
    }

    // --- constexpr lambda (lignes 354-362) ---
    {
        constexpr auto sq = [](int x) { return x * x; };
        static_assert(sq(5) == 25);
        constexpr int result = sq(7);
        std::print("square(7) = {}\n", result);  // 49

        int runtime_val = 6;
        std::print("{}\n", sq(runtime_val));  // 36
    }

    // --- consteval lambda (lignes 370-373) ---
    {
        auto compile_only = [](int x) consteval { return x * x; };
        constexpr int ok = compile_only(5);
        std::print("consteval: {}\n", ok);  // 25
    }

    // --- Inférence de type retour (lignes 383-415) ---
    {
        auto add2 = [](int a, int b) { return a + b; };
        auto mixed = [](int a, double b) { return a + b; };
        std::print("add(1,2) = {}, mixed(1,2.5) = {}\n", add2(1, 2), mixed(1, 2.5));

        auto fixed = [](bool flag) -> double {
            if (flag) return 42;
            else return 3.14;
        };
        std::print("fixed(true) = {}, fixed(false) = {}\n", fixed(true), fixed(false));

        std::string name = "Alice";
        auto by_value = [&name]() { return name; };
        auto by_ref = [&name]() -> const std::string& { return name; };
        std::print("by_value: {}, by_ref: {}\n", by_value(), by_ref());
    }

    return 0;
}
