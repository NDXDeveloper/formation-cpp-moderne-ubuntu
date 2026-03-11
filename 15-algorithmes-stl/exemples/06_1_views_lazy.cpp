/* ============================================================================
   Section 15.6.1 : Views et lazy evaluation
   Description : Exemples complets couvrant filter, transform, take, take_while,
                 drop, drop_while, reverse, keys/values, elements<N>, iota,
                 enumerate (C++23), zip (C++23), split, join, common,
                 matérialisation avec ranges::to (C++23), views mutables.
   Fichier source : 06.1-views-lazy.md
   ============================================================================ */

#include <ranges>
#include <algorithm>
#include <numeric>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <tuple>
#include <print>

struct Employee {
    std::string name;
    double salary;
};

int main() {
    // === filter ===
    {
        std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        auto positives_odd = std::views::filter(v, [](int x) {
            return x % 2 != 0;
        });
        for (int x : positives_odd) std::print("{} ", x);
        std::println("");
        // 1 3 5 7 9
    }

    // === transform ===
    {
        std::vector<int> v = {1, 2, 3, 4, 5};
        auto squared = std::views::transform(v, [](int x) { return x * x; });
        for (int x : squared) std::print("{} ", x);
        std::println("");
        // 1 4 9 16 25
    }

    // === transform type change ===
    {
        std::vector<Employee> team = {
            {"Alice", 75000.0}, {"Bob", 62000.0}, {"Carol", 88000.0}
        };
        auto names = std::views::transform(team, &Employee::name);
        for (const auto& name : names) std::println("{}", name);
        // Alice / Bob / Carol
    }

    // === take ===
    {
        std::vector<int> v = {10, 20, 30, 40, 50, 60, 70};
        auto first_three = std::views::take(v, 3);
        for (int x : first_three) std::print("{} ", x);
        std::println("");
        // 10 20 30
    }

    // === take_while ===
    {
        std::vector<int> v = {2, 4, 6, 7, 8, 10};
        auto leading_evens = std::views::take_while(v, [](int x) {
            return x % 2 == 0;
        });
        for (int x : leading_evens) std::print("{} ", x);
        std::println("");
        // 2 4 6
    }

    // === drop ===
    {
        std::vector<int> v = {10, 20, 30, 40, 50, 60, 70};
        auto after_three = std::views::drop(v, 3);
        for (int x : after_three) std::print("{} ", x);
        std::println("");
        // 40 50 60 70
    }

    // === drop_while ===
    {
        std::vector<int> v = {1, 3, 5, 6, 7, 8};
        auto from_first_even = std::views::drop_while(v, [](int x) {
            return x % 2 != 0;
        });
        for (int x : from_first_even) std::print("{} ", x);
        std::println("");
        // 6 7 8
    }

    // === reverse ===
    {
        std::vector<int> v = {1, 2, 3, 4, 5};
        for (int x : std::views::reverse(v)) std::print("{} ", x);
        std::println("");
        // 5 4 3 2 1
    }

    // === keys / values ===
    {
        std::map<std::string, int> scores = {
            {"Alice", 95}, {"Bob", 82}, {"Carol", 91}
        };
        for (const auto& name : std::views::keys(scores)) std::print("{} ", name);
        std::println("");
        // Alice Bob Carol
        for (int score : std::views::values(scores)) std::print("{} ", score);
        std::println("");
        // 95 82 91
    }

    // === elements<N> ===
    {
        std::vector<std::tuple<std::string, int, double>> data = {
            {"web-01", 72, 4.5},
            {"db-01", 91, 12.3},
            {"cache-01", 23, 1.8}
        };
        for (const auto& name : std::views::elements<0>(data)) std::print("{} ", name);
        std::println("");
        // web-01 db-01 cache-01
    }

    // === iota (infini + borné) ===
    {
        for (int x : std::views::iota(0) | std::views::take(5)) std::print("{} ", x);
        std::println("");
        // 0 1 2 3 4

        for (int x : std::views::iota(1, 11)) std::print("{} ", x);
        std::println("");
        // 1 2 3 4 5 6 7 8 9 10
    }

    // === enumerate (C++23) ===
    {
        std::vector<std::string> names = {"Alice", "Bob", "Carol"};
        for (auto [i, name] : std::views::enumerate(names)) {
            std::print("[{}] {}\n", i, name);
        }
        // [0] Alice / [1] Bob / [2] Carol
    }

    // === zip (C++23) ===
    {
        std::vector<std::string> names = {"Alice", "Bob", "Carol"};
        std::vector<int> scores = {95, 82, 91};
        for (auto [name, score] : std::views::zip(names, scores)) {
            std::print("{}: {}\n", name, score);
        }
        // Alice: 95 / Bob: 82 / Carol: 91
    }

    // === split ===
    {
        std::string csv = "Alice,Bob,Carol,Dave";
        for (auto word : std::views::split(csv, ',')) {
            std::print("{}\n", std::string_view(word.begin(), word.end()));
        }
    }

    // === join ===
    {
        std::vector<std::vector<int>> matrix = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
        for (int x : std::views::join(matrix)) std::print("{} ", x);
        std::println("");
        // 1 2 3 4 5 6 7 8 9
    }

    // === common ===
    {
        auto view = std::views::iota(0, 10)
                  | std::views::filter([](int x) { return x % 2 == 0; });
        auto common = std::views::common(view);
        int sum = std::accumulate(common.begin(), common.end(), 0);
        std::print("sum={}\n", sum);
        // sum=20
    }

    // === Matérialisation C++23 : ranges::to ===
    {
        auto result = std::views::iota(1, 6)
                    | std::views::transform([](int x) { return x * x; })
                    | std::ranges::to<std::vector>();
        for (int x : result) std::print("{} ", x);
        std::println("");
        // 1 4 9 16 25

        auto as_set = std::views::iota(1, 6)
                    | std::ranges::to<std::set>();
        for (int x : as_set) std::print("{} ", x);
        std::println("");
        // 1 2 3 4 5
    }

    // === Views sur données mutables ===
    {
        std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        for (int& x : std::views::filter(v, [](int x) { return x % 2 == 0; })) {
            x *= 2;
        }
        for (int x : v) std::print("{} ", x);
        std::println("");
        // 1 4 3 8 5 12 7 16 9 20
    }
}
