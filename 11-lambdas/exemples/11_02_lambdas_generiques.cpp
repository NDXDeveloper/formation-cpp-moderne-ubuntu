/* ============================================================================
   Section 11.2 : Lambdas génériques
   Description : Exemples complets de lambdas génériques — auto, auto&,
                 forwarding references, variadic, lambdas templatées C++20,
                 NTTP, concepts, comparateurs, projections, visiteur variant,
                 déduction de type retour, constexpr
   Fichier source : 02-lambdas-generiques.md
   ============================================================================ */
#include <print>
#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include <functional>
#include <variant>
#include <chrono>
#include <array>
#include <utility>
#include <concepts>
#include <format>

// Helper pour combiner des lambdas en un visiteur (ligne 378-379)
template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

int main() {
    // --- Lambdas génériques avec auto (ligne 36-40) ---
    {
        auto add = [](auto a, auto b) { return a + b; };
        std::print("{}\n", add(3, 4));           // 7
        std::print("{}\n", add(1.5, 2.7));       // 4.2
        std::print("{}\n", add(std::string("Hello, "), std::string("World")));
    }

    // --- Types mixtes (ligne 62-65) ---
    {
        auto multiply = [](auto a, auto b) { return a * b; };
        std::print("{}\n", multiply(3, 2.5));  // 7.5
    }

    // --- auto&, const auto&, auto&& (ligne 75-84) ---
    {
        auto by_value = [](auto x) { return x; };
        auto by_cref = [](const auto& x) { return x.size(); };
        auto by_fwdref = [](auto&& x) {
            return std::forward<decltype(x)>(x);
        };

        std::print("{}\n", by_value(42));
        std::string s = "hello";
        std::print("{}\n", by_cref(s));
        std::print("{}\n", by_fwdref(std::string("world")));
    }

    // --- Forwarding reference wrapper (ligne 90-97) ---
    {
        auto process = [](const auto& x) { std::print("processed: {}\n", x); };
        auto wrapper = [&process](auto&& arg) {
            process(std::forward<decltype(arg)>(arg));
        };
        std::string s = "hello";
        wrapper(s);
        wrapper(std::string("temp"));
    }

    // --- Variadic auto (ligne 105-110) ---
    {
        auto print_all = [](const auto&... args) {
            (std::print("{} ", args), ...);
            std::print("\n");
        };
        print_all(1, 2.5, "hello", 'c');
    }

    // --- Logger générique (ligne 130-137) ---
    {
        auto log = [](std::string_view level, const auto&... args) {
            std::print("[{}] ", level);
            (std::print("{} ", args), ...);
            std::print("\n");
        };
        log("INFO", "User", 42, "connected from", "192.168.1.1");
    }

    // --- Lambdas templatées C++20 — même type (ligne 164-173) ---
    {
        auto add = []<typename T>(T a, T b) { return a + b; };
        std::print("{}\n", add(3, 4));       // 7
        std::print("{}\n", add(1.5, 2.7));   // 4.2
    }

    // --- make_vector auto vs template (ligne 182-197) ---
    {
        auto make_vector_auto = [](auto first, auto... rest) {
            std::vector<decltype(first)> result;
            result.push_back(first);
            (result.push_back(rest), ...);
            return result;
        };

        auto make_vector = []<typename T, typename... Ts>(T first, Ts... rest) {
            std::vector<T> result;
            result.push_back(first);
            (result.push_back(rest), ...);
            return result;
        };

        auto v1 = make_vector_auto(1, 2, 3, 4, 5);
        auto v2 = make_vector(1, 2, 3, 4, 5);
        std::print("v1.size={}, v2.size={}\n", v1.size(), v2.size());
    }

    // --- NTTP — fixed_array (ligne 205-209) ---
    {
        auto fixed_array = []<std::size_t N>() {
            return std::array<int, N>{};
        };
        auto arr = fixed_array.operator()<10>();
        std::print("arr.size = {}\n", arr.size());
    }

    // --- NTTP — repeat (ligne 215-223) ---
    {
        auto repeat = []<std::size_t N>(const char (&str)[N]) {
            for (std::size_t i = 0; i < N - 1; ++i) {
                std::print("{}", str[i]);
            }
            std::print("\n");
        };
        repeat("Hello");
    }

    // --- Combinaison auto + template (ligne 231-242) ---
    {
        auto convert_all = []<typename Target>(const auto& container) {
            std::vector<Target> result;
            result.reserve(container.size());
            for (const auto& elem : container) {
                result.push_back(static_cast<Target>(elem));
            }
            return result;
        };

        std::vector<int> ints = {1, 2, 3, 4, 5};
        auto doubles = convert_all.operator()<double>(ints);
        std::print("doubles: ");
        for (double d : doubles) std::print("{} ", d);
        std::print("\n");
    }

    // --- Concepts — requires (ligne 254-261) ---
    {
        auto add = []<typename T>
            requires std::integral<T>
        (T a, T b) {
            return a + b;
        };
        std::print("{}\n", add(3, 4));
    }

    // --- Concepts — floating_point (ligne 269-274) ---
    {
        auto divide = []<std::floating_point T>(T a, T b) {
            return a / b;
        };
        std::print("{}\n", divide(7.0, 2.0));
    }

    // --- Concepts abrégés (ligne 282-287) ---
    {
        auto process = [](std::integral auto x, std::floating_point auto y) {
            return static_cast<double>(x) + y;
        };
        std::print("{}\n", process(3, 2.5));
    }

    // --- Concept Printable (ligne 303-315) ---
    {
        auto log_item = [](const auto& item) {
            std::print("[LOG] {}\n", item);
        };
        log_item(42);
        log_item("hello");
        log_item(std::string{});
    }

    // --- Comparateurs génériques (ligne 327-335) ---
    {
        auto descending = [](const auto& a, const auto& b) { return a > b; };

        std::vector<int> ints = {3, 1, 4, 1, 5};
        std::sort(ints.begin(), ints.end(), descending);
        for (int v : ints) std::print("{} ", v);
        std::print("\n");

        std::vector<std::string> words = {"banana", "apple", "cherry"};
        std::sort(words.begin(), words.end(), descending);
        for (const auto& w : words) std::print("{} ", w);
        std::print("\n");
    }

    // --- Projections by_field (ligne 345-367) ---
    {
        struct Employee {
            std::string name;
            int age;
            double salary;
        };

        auto by_field = []<typename F>(F field) {
            return [field](const auto& a, const auto& b) {
                return std::invoke(field, a) < std::invoke(field, b);
            };
        };

        std::vector<Employee> team = {
            {"Alice", 30, 85000.0},
            {"Bob", 25, 72000.0},
            {"Carol", 35, 95000.0}
        };

        std::sort(team.begin(), team.end(), by_field(&Employee::age));
        std::print("By age: ");
        for (const auto& e : team) std::print("{}({}) ", e.name, e.age);
        std::print("\n");

        std::sort(team.begin(), team.end(), by_field(&Employee::salary));
        std::print("By salary: ");
        for (const auto& e : team) std::print("{}({}) ", e.name, e.salary);
        std::print("\n");
    }

    // --- Visiteur std::variant (ligne 377-392) ---
    {
        using JsonValue = std::variant<int, double, std::string, bool, std::nullptr_t>;
        JsonValue value = std::string("hello");

        std::visit(overloaded{
            [](int i)                { std::print("int: {}\n", i); },
            [](double d)             { std::print("double: {}\n", d); },
            [](const std::string& s) { std::print("string: {}\n", s); },
            [](bool b)               { std::print("bool: {}\n", b); },
            [](std::nullptr_t)       { std::print("null\n"); }
        }, value);
    }

    // --- Variant catch-all (ligne 398-401) ---
    {
        using JsonValue = std::variant<int, double, std::string, bool, std::nullptr_t>;
        JsonValue value = std::string("hello");

        std::visit(overloaded{
            [](const std::string& s) { std::print("string: {}\n", s); },
            [](const auto&)          { std::print("other type\n"); }
        }, value);
    }

    // --- Déduction de type retour (ligne 458-477) ---
    {
        auto identity = [](auto x) { return x; };
        auto i = identity(42);
        auto d = identity(3.14);
        auto s = identity(std::string("hi"));
        std::print("{}, {}, {}\n", i, d, s);

        auto divide = [](auto a, auto b) { return a / b; };
        std::print("{}\n", divide(7, 2));    // 3 (division entière)
        std::print("{}\n", divide(7.0, 2));  // 3.5

        auto safe_divide = [](auto a, auto b) -> double {
            return static_cast<double>(a) / b;
        };
        std::print("{}\n", safe_divide(7, 2));  // 3.5
    }

    // --- make_pair template (ligne 483-485) ---
    {
        auto make_pair = []<typename T, typename U>(T a, U b) -> std::pair<T, U> {
            return {a, b};
        };
        auto p = make_pair(42, std::string("hello"));
        std::print("{}, {}\n", p.first, p.second);
    }

    // --- decltype merge (ligne 495-506) ---
    {
        auto merge = [](auto&& container1, auto&& container2) {
            using ValueType = typename std::remove_reference_t<decltype(container1)>::value_type;
            std::vector<ValueType> result;
            result.reserve(container1.size() + container2.size());
            result.insert(result.end(), container1.begin(), container1.end());
            result.insert(result.end(), container2.begin(), container2.end());
            return result;
        };

        std::vector<int> a = {1, 2, 3};
        std::list<int> b = {4, 5, 6};
        auto merged = merge(a, b);
        for (int v : merged) std::print("{} ", v);
        std::print("\n");
    }

    // --- Template merge C++20 (ligne 512-518) ---
    {
        auto merge = []<typename T>(const std::vector<T>& a, const std::vector<T>& b) {
            std::vector<T> result;
            result.reserve(a.size() + b.size());
            result.insert(result.end(), a.begin(), a.end());
            result.insert(result.end(), b.begin(), b.end());
            return result;
        };

        std::vector<int> a = {1, 2, 3};
        std::vector<int> b = {4, 5, 6};
        auto merged = merge(a, b);
        for (int v : merged) std::print("{} ", v);
        std::print("\n");
    }

    // --- constexpr lambda générique (ligne 530-536) ---
    {
        constexpr auto max_of = [](auto a, auto b) { return a > b ? a : b; };
        static_assert(max_of(3, 7) == 7);
        static_assert(max_of(2.5, 1.8) == 2.5);
        constexpr int result = max_of(100, 42);
        std::print("max_of(100,42) = {}\n", result);
    }

    // --- constexpr + concepts (ligne 542-550) ---
    {
        constexpr auto safe_add = []<typename T>
            requires std::integral<T>
        (T a, T b) -> T {
            return a + b;
        };
        static_assert(safe_add(10, 20) == 30);
        std::print("safe_add(10,20) = {}\n", safe_add(10, 20));
    }

    return 0;
}
