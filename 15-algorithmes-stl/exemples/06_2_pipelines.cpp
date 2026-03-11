/* ============================================================================
   Section 15.6.2 : Pipelines avec l'opérateur |
   Description : Exemples complets couvrant pipelines filter+transform+take,
                 anatomie d'un pipeline avec données, fizz numbers, map prices
                 en majuscules, zip+filter+enumerate (C++23), matérialisation
                 ranges::to (C++23), count_if sur view, adaptateur personnalisé,
                 max_element sur view filtrée, pipeline de logs.
   Fichier source : 06.2-pipelines.md
   ============================================================================ */

#include <ranges>
#include <algorithm>
#include <numeric>
#include <vector>
#include <string>
#include <map>
#include <print>

struct LogEntry {
    std::string timestamp;
    int level;
    std::string message;
    std::string service;
};

struct Employee {
    std::string name;
    std::string department;
    double salary;
};

// Adaptateur personnalisé : garder les N premiers éléments pairs
auto take_evens(int n) {
    return std::views::filter([](int x) { return x % 2 == 0; })
         | std::views::take(n);
}

int main() {
    // === Pipeline filter + transform + take ===
    {
        std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        auto result = v
            | std::views::filter([](int x) { return x % 2 == 0; })
            | std::views::transform([](int x) { return x * x; })
            | std::views::take(3);
        for (int x : result) std::print("{} ", x);
        std::println("");
        // 4 16 36
    }

    // === Pipeline anatomie ===
    {
        std::vector<int> data = {15, 3, 8, 22, 1, 47, 6, 10, 33, 2};
        auto pipeline = data
            | std::views::filter([](int x) { return x >= 10; })
            | std::views::transform([](int x) { return x * 2; })
            | std::views::take(4);
        for (int x : pipeline) std::print("{} ", x);
        std::println("");
        // 30 44 94 20
    }

    // === Fizz only (séquence infinie) ===
    {
        auto fizz_only = std::views::iota(1)
            | std::views::filter([](int x) { return x % 3 == 0 && x % 5 != 0; })
            | std::views::take(10);
        for (int x : fizz_only) std::print("{} ", x);
        std::println("");
        // 3 6 9 12 18 21 24 27 33 36
    }

    // === Map prices en majuscules ===
    {
        std::map<std::string, double> prices = {
            {"apple", 1.20}, {"banana", 0.80}, {"cherry", 3.50},
            {"date", 5.00}, {"elderberry", 7.50}, {"fig", 2.30}
        };
        auto expensive_names = prices
            | std::views::filter([](const auto& pair) { return pair.second > 2.0; })
            | std::views::keys
            | std::views::transform([](const std::string& name) {
                  std::string upper = name;
                  std::ranges::transform(upper, upper.begin(), ::toupper);
                  return upper;
              });
        for (const auto& name : expensive_names) std::println("{}", name);
        // CHERRY / DATE / ELDERBERRY / FIG
    }

    // === Zip + filter + enumerate (C++23) ===
    {
        std::vector<std::string> names = {"Alice", "Bob", "Carol", "Dave", "Eve"};
        std::vector<int> scores = {95, 78, 88, 62, 91};
        auto ranking = std::views::zip(names, scores)
            | std::views::filter([](const auto& pair) {
                  auto [name, score] = pair;
                  return score >= 80;
              })
            | std::views::enumerate;
        for (auto [rank, entry] : ranking) {
            auto [name, score] = entry;
            std::print("#{} {} (score: {})\n", rank + 1, name, score);
        }
        // #1 Alice (score: 95)
        // #2 Carol (score: 88)
        // #3 Eve (score: 91)
    }

    // === Matérialisation C++23 ===
    {
        std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        auto result = data
            | std::views::filter([](int x) { return x % 2 == 0; })
            | std::views::transform([](int x) { return x * x; })
            | std::ranges::to<std::vector>();
        for (int x : result) std::print("{} ", x);
        std::println("");
        // 4 16 36 64 100
    }

    // === count_if sur view ===
    {
        std::vector<int> v = {5, 3, 8, 1, 9, 2, 7, 4, 6};
        auto big_squares = v
            | std::views::filter([](int x) { return x % 2 == 0; })
            | std::views::transform([](int x) { return x * x; });
        auto count = std::ranges::count_if(big_squares, [](int x) { return x > 20; });
        std::print("Carrés de pairs > 20 : {}\n", count);
        // 64 et 36 → count == 2
    }

    // === Adaptateur personnalisé take_evens ===
    {
        std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        for (int x : v | take_evens(3)) std::print("{} ", x);
        std::println("");
        // 2 4 6
    }

    // === max_element sur view filtrée ===
    {
        std::vector<Employee> company = {
            {"Alice", "Engineering", 90000},
            {"Bob", "Marketing", 70000},
            {"Carol", "Engineering", 95000},
            {"Dave", "Engineering", 80000},
            {"Eve", "Marketing", 75000}
        };
        auto eng = company
            | std::views::filter([](const Employee& e) {
                  return e.department == "Engineering";
              });
        auto it = std::ranges::max_element(eng, {}, &Employee::salary);
        if (it != eng.end()) {
            std::print("Top eng : {} ({:.0f}€)\n", it->name, it->salary);
        }
        // Top eng : Carol (95000€)
    }

    // === Pipeline de logs ===
    {
        std::vector<LogEntry> logs = {
            {"2024-01-01 10:00", 1, "Request received", "api-gateway"},
            {"2024-01-01 10:01", 3, "Connection timeout", "api-gateway"},
            {"2024-01-01 10:02", 2, "Slow query", "database"},
            {"2024-01-01 10:03", 4, "Out of memory", "api-gateway"},
            {"2024-01-01 10:04", 3, "Auth failed", "auth-service"},
            {"2024-01-01 10:05", 3, "Rate limit exceeded", "api-gateway"},
        };
        auto critical_api_logs = logs
            | std::views::filter([](const LogEntry& e) {
                  return e.level >= 3 && e.service == "api-gateway";
              })
            | std::views::reverse
            | std::views::take(10)
            | std::views::transform([](const LogEntry& e) {
                  return e.timestamp + " [" + e.message + "]";
              });
        for (const auto& msg : critical_api_logs) std::println("{}", msg);
        // Logs critiques en ordre inversé
    }
}
