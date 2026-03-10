/* ============================================================================
   Section 11.3 : Lambdas et algorithmes STL
   Description : Exemples complets d'utilisation des lambdas avec les algorithmes
                 STL — find_if, count_if, all_of/any_of/none_of, sort, transform,
                 for_each, erase/remove, copy_if, partition, accumulate, generate,
                 inner_product, transform_reduce, conteneurs ordonnés
   Fichier source : 03-lambdas-stl.md
   ============================================================================ */
#include <print>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <numeric>
#include <set>
#include <queue>
#include <limits>
#include <format>
#include <chrono>
#include <random>
#include <iterator>
#include <cmath>
#include <cstdint>
#include <tuple>
#include <cctype>

int main() {
    // --- find_if (ligne 22-31) ---
    {
        std::vector<int> values = {12, 7, 25, 3, 18, 42, 9};
        auto it = std::find_if(values.begin(), values.end(), [](int v) {
            return v > 20;
        });
        if (it != values.end()) {
            std::print("Premier > 20 : {}\n", *it);  // 25
        }
    }

    // --- find_if_not (ligne 37-41) ---
    {
        std::vector<int> values = {12, 7, 25, 3, 18, 42, 9};
        auto it = std::find_if_not(values.begin(), values.end(), [](int v) {
            return v % 2 == 0;
        });
        std::print("Premier impair : {}\n", *it);  // 7
    }

    // --- count_if (ligne 47-53) ---
    {
        std::vector<std::string> words = {"apple", "avocado", "banana", "apricot", "cherry"};
        auto count = std::count_if(words.begin(), words.end(), [](const std::string& w) {
            return w.starts_with('a');
        });
        std::print("Mots commençant par 'a' : {}\n", count);  // 3
    }

    // --- all_of / any_of / none_of (ligne 61-73) ---
    {
        std::vector<int> scores = {75, 82, 91, 68, 88};
        bool all_passing = std::all_of(scores.begin(), scores.end(),
            [](int s) { return s >= 60; });
        bool any_perfect = std::any_of(scores.begin(), scores.end(),
            [](int s) { return s == 100; });
        bool none_negative = std::none_of(scores.begin(), scores.end(),
            [](int s) { return s < 0; });
        std::print("all_passing={}, any_perfect={}, none_negative={}\n",
                   all_passing, any_perfect, none_negative);
    }

    // --- Prédicats avec capture (ligne 83-100) ---
    {
        struct Product {
            std::string name;
            double price;
            std::string category;
        };

        std::vector<Product> catalog = {
            {"Widget", 15.0, "gadget"},
            {"Doohickey", 5.0, "gadget"},
            {"Thingamajig", 25.0, "tool"}
        };

        auto find_affordable = [](const std::vector<Product>& catalog,
                                  double budget,
                                  const std::string& target_category) {
            auto it = std::find_if(catalog.begin(), catalog.end(),
                [&budget, &target_category](const Product& p) {
                    return p.category == target_category && p.price <= budget;
                });
            if (it != catalog.end()) {
                std::print("Trouvé : {} à {:.2f}€\n", it->name, it->price);
            }
        };
        find_affordable(catalog, 20.0, "gadget");
    }

    // --- sort avec comparateur (ligne 114-130) ---
    {
        struct Employee {
            std::string name;
            int age;
            double salary;
        };

        std::vector<Employee> team = {
            {"Alice", 30, 85000},
            {"Bob", 25, 72000},
            {"Carol", 35, 95000},
            {"Dave", 28, 85000}
        };

        std::sort(team.begin(), team.end(), [](const Employee& a, const Employee& b) {
            return a.salary > b.salary;
        });
        std::print("Par salaire décroissant: ");
        for (const auto& e : team) std::print("{}({}) ", e.name, e.salary);
        std::print("\n");

        // Tri multi-critères (ligne 139-144)
        std::sort(team.begin(), team.end(), [](const Employee& a, const Employee& b) {
            if (a.salary != b.salary) return a.salary > b.salary;
            return a.name < b.name;
        });
        std::print("Multi-critères: ");
        for (const auto& e : team) std::print("{}({}) ", e.name, e.salary);
        std::print("\n");

        // std::tie (ligne 150-153)
        std::sort(team.begin(), team.end(), [](const Employee& a, const Employee& b) {
            return std::tie(b.salary, a.name) < std::tie(a.salary, b.name);
        });
        std::print("std::tie: ");
        for (const auto& e : team) std::print("{}({}) ", e.name, e.salary);
        std::print("\n");
    }

    // --- stable_sort (ligne 161-183) ---
    {
        struct Employee2 {
            std::string name;
            std::string department;
            double salary;
        };

        std::vector<Employee2> staff = {
            {"Alice", "Engineering", 85000},
            {"Bob", "Marketing", 72000},
            {"Carol", "Engineering", 95000},
            {"Dave", "Marketing", 85000}
        };

        std::stable_sort(staff.begin(), staff.end(), [](const Employee2& a, const Employee2& b) {
            return a.department < b.department;
        });
        std::stable_sort(staff.begin(), staff.end(), [](const Employee2& a, const Employee2& b) {
            return a.salary > b.salary;
        });
        std::print("stable_sort: ");
        for (const auto& e : staff) std::print("{}({},{}) ", e.name, e.department, e.salary);
        std::print("\n");
    }

    // --- partial_sort (ligne 191-196) ---
    {
        std::vector<int> scores = {72, 95, 88, 61, 43, 99, 77, 84};
        std::partial_sort(scores.begin(), scores.begin() + 3, scores.end(),
            [](int a, int b) { return a > b; });
        std::print("Top 3: {} {} {}\n", scores[0], scores[1], scores[2]);
    }

    // --- transform — toupper (ligne 208-216) ---
    {
        std::vector<std::string> names = {"alice", "bob", "carol"};
        std::vector<std::string> upper_names(names.size());
        std::transform(names.begin(), names.end(), upper_names.begin(),
            [](std::string s) {
                std::transform(s.begin(), s.end(), s.begin(), ::toupper);
                return s;
            });
        for (const auto& n : upper_names) std::print("{} ", n);
        std::print("\n");
    }

    // --- transform in-place (ligne 222-227) ---
    {
        std::vector<double> prices = {9.99, 24.50, 3.75, 19.90};
        std::transform(prices.begin(), prices.end(), prices.begin(),
            [](double price) { return price * 0.9; });
        for (double p : prices) std::print("{:.3f} ", p);
        std::print("\n");
    }

    // --- transform binaire (ligne 235-244) ---
    {
        std::vector<std::string> first_names = {"Alice", "Bob", "Carol"};
        std::vector<std::string> last_names = {"Smith", "Jones", "Lee"};
        std::vector<std::string> full_names(first_names.size());
        std::transform(first_names.begin(), first_names.end(),
                       last_names.begin(), full_names.begin(),
            [](const std::string& first, const std::string& last) {
                return first + " " + last;
            });
        for (const auto& n : full_names) std::print("{}, ", n);
        std::print("\n");
    }

    // --- for_each Stats (ligne 252-272) ---
    {
        struct Stats {
            int count = 0;
            double sum = 0.0;
            double min = std::numeric_limits<double>::max();
            double max = std::numeric_limits<double>::lowest();
        };

        std::vector<double> measurements = {23.5, 19.8, 27.3, 22.1, 25.6};
        Stats stats;
        std::for_each(measurements.begin(), measurements.end(),
            [&stats](double v) {
                ++stats.count;
                stats.sum += v;
                stats.min = std::min(stats.min, v);
                stats.max = std::max(stats.max, v);
            });
        std::print("Count: {}, Avg: {:.1f}, Range: [{:.1f}, {:.1f}]\n",
                   stats.count, stats.sum / stats.count, stats.min, stats.max);
    }

    // --- erase-remove (ligne 286-293) ---
    {
        std::vector<int> data = {1, -3, 5, -7, 2, -1, 8, -4};
        data.erase(
            std::remove_if(data.begin(), data.end(), [](int v) { return v < 0; }),
            data.end()
        );
        for (int v : data) std::print("{} ", v);
        std::print("\n");
    }

    // --- erase_if C++20 (ligne 301-306) ---
    {
        std::vector<int> data = {1, -3, 5, -7, 2, -1, 8, -4};
        auto removed = std::erase_if(data, [](int v) { return v < 0; });
        std::print("Supprimés : {}, Restants : {}\n", removed, data.size());
    }

    // --- copy_if (ligne 316-322) ---
    {
        std::vector<int> source = {1, -3, 5, -7, 2, -1, 8, -4};
        std::vector<int> positives;
        std::copy_if(source.begin(), source.end(), std::back_inserter(positives),
            [](int v) { return v > 0; });
        for (int v : positives) std::print("{} ", v);
        std::print("\n");
    }

    // --- partition (ligne 330-339) ---
    {
        std::vector<int> data = {1, -3, 5, -7, 2, -1, 8, -4};
        auto boundary = std::partition(data.begin(), data.end(),
            [](int v) { return v >= 0; });
        std::print("Positifs : ");
        for (auto it = data.begin(); it != boundary; ++it) std::print("{} ", *it);
        std::print("\nNégatifs : ");
        for (auto it = boundary; it != data.end(); ++it) std::print("{} ", *it);
        std::print("\n");
    }

    // --- accumulate (ligne 351-367) ---
    {
        std::vector<int> values = {1, 2, 3, 4, 5};
        int sum = std::accumulate(values.begin(), values.end(), 0);
        int product = std::accumulate(values.begin(), values.end(), 1,
            [](int acc, int val) { return acc * val; });
        std::print("sum={}, product={}\n", sum, product);

        std::vector<std::string> words = {"C++", "is", "powerful"};
        auto sentence = std::accumulate(words.begin(), words.end(), std::string{},
            [](const std::string& acc, const std::string& word) {
                return acc.empty() ? word : acc + " " + word;
            });
        std::print("{}\n", sentence);
    }

    // --- accumulate type change (ligne 375-391) ---
    {
        struct Order {
            std::string product;
            int quantity;
            double unit_price;
        };

        std::vector<Order> orders = {
            {"Widget", 5, 12.50},
            {"Gadget", 2, 45.00},
            {"Doohickey", 10, 3.75}
        };

        double total = std::accumulate(orders.begin(), orders.end(), 0.0,
            [](double acc, const Order& order) {
                return acc + order.quantity * order.unit_price;
            });
        std::print("total = {}\n", total);  // 190.0
    }

    // --- generate fibonacci (ligne 420-430) ---
    {
        std::vector<int> fibonacci(10);
        std::generate(fibonacci.begin(), fibonacci.end(),
            [a = 0, b = 1]() mutable {
                int current = a;
                a = b;
                b = current + b;
                return current;
            });
        for (int v : fibonacci) std::print("{} ", v);
        std::print("\n");
    }

    // --- generate IDs (ligne 437-443) ---
    {
        std::vector<std::string> ids(5);
        std::generate(ids.begin(), ids.end(),
            [n = 0]() mutable {
                return std::format("ID-{:04d}", ++n);
            });
        for (const auto& id : ids) std::print("{}\n", id);
    }

    // --- generate_n random (ligne 449-457) ---
    {
        std::vector<double> samples;
        samples.reserve(100);
        std::mt19937 rng(42);
        std::normal_distribution<double> dist(0.0, 1.0);
        std::generate_n(std::back_inserter(samples), 100,
            [&rng, &dist]() { return dist(rng); });
        std::print("samples.size = {}\n", samples.size());
    }

    // --- analyze_logs (ligne 469-499) ---
    {
        struct LogEntry {
            std::string level;
            std::string message;
            std::chrono::system_clock::time_point timestamp;
        };

        auto now = std::chrono::system_clock::now();
        std::vector<LogEntry> logs = {
            {"ERROR", "disk full", now},
            {"DEBUG", "trace", now + std::chrono::seconds(1)},
            {"INFO", "started", now + std::chrono::seconds(2)},
            {"ERROR", "timeout", now + std::chrono::seconds(3)},
            {"DEBUG", "cleanup", now + std::chrono::seconds(4)}
        };

        std::sort(logs.begin(), logs.end(),
            [](const LogEntry& a, const LogEntry& b) {
                return a.timestamp < b.timestamp;
            });

        auto error_count = std::count_if(logs.begin(), logs.end(),
            [](const LogEntry& e) { return e.level == "ERROR"; });

        std::vector<std::string> error_messages;
        std::for_each(logs.begin(), logs.end(),
            [&error_messages](const LogEntry& e) {
                if (e.level == "ERROR") {
                    error_messages.push_back(e.message);
                }
            });

        std::erase_if(logs, [](const LogEntry& e) { return e.level == "DEBUG"; });

        std::print("Erreurs : {}, Logs restants : {}\n", error_count, logs.size());
    }

    // --- set with lambda comparator (ligne 537-547) ---
    {
        auto cmp = [](const std::string& a, const std::string& b) {
            return a.size() < b.size();
        };
        std::set<std::string, decltype(cmp)> words_by_length(cmp);
        words_by_length.insert("apple");
        words_by_length.insert("hi");
        words_by_length.insert("banana");
        for (const auto& w : words_by_length) std::print("{} ", w);
        std::print("\n");
    }

    // --- C++20 default-constructible lambda (ligne 553-559) ---
    {
        auto cmp = [](const std::string& a, const std::string& b) {
            return a.size() < b.size();
        };
        std::set<std::string, decltype(cmp)> words_by_length;
        words_by_length.insert("apple");
        words_by_length.insert("hi");
        words_by_length.insert("banana");
        for (const auto& w : words_by_length) std::print("{} ", w);
        std::print("\n");
    }

    // --- inner_product (ligne 581-596) ---
    {
        std::vector<double> prices = {10.0, 25.0, 5.0};
        std::vector<int> quantities = {3, 1, 8};
        double total = std::inner_product(
            prices.begin(), prices.end(),
            quantities.begin(), 0.0);
        std::print("total = {}\n", total);  // 95.0

        double max_diff = std::inner_product(
            prices.begin(), prices.end(),
            quantities.begin(), 0.0,
            [](double acc, double diff) { return std::max(acc, diff); },
            [](double price, int qty) { return std::abs(price - qty); }
        );
        std::print("max_diff = {}\n", max_diff);
    }

    // --- adjacent_difference (ligne 602-607) ---
    {
        std::vector<int> readings = {100, 103, 98, 105, 102};
        std::vector<int> changes(readings.size());
        std::adjacent_difference(readings.begin(), readings.end(), changes.begin(),
            [](int current, int previous) { return current - previous; });
        for (int v : changes) std::print("{} ", v);
        std::print("\n");
    }

    // --- transform_reduce (ligne 615-628) ---
    {
        struct Pixel {
            uint8_t r, g, b;
        };

        std::vector<Pixel> image = {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}};
        double total_lum = std::transform_reduce(
            image.begin(), image.end(),
            0.0,
            std::plus<>{},
            [](const Pixel& p) { return 0.299*p.r + 0.587*p.g + 0.114*p.b; }
        );
        double avg_lum = total_lum / static_cast<double>(image.size());
        std::print("avg luminosity = {:.1f}\n", avg_lum);
    }

    // --- Comparateurs génériques (ligne 675-684) ---
    {
        auto descending = [](const auto& a, const auto& b) { return a > b; };

        std::vector<int> ints = {3, 1, 4, 1, 5};
        std::vector<double> doubles = {2.7, 1.4, 3.1};
        std::vector<std::string> words = {"banana", "apple", "cherry"};

        std::sort(ints.begin(), ints.end(), descending);
        std::sort(doubles.begin(), doubles.end(), descending);
        std::sort(words.begin(), words.end(), descending);

        for (int v : ints) std::print("{} ", v);
        std::print("| ");
        for (double v : doubles) std::print("{} ", v);
        std::print("| ");
        for (const auto& w : words) std::print("{} ", w);
        std::print("\n");
    }

    // --- field_comparator (ligne 690-698) ---
    {
        struct Employee {
            std::string name;
            int age;
            double salary;
        };

        auto field_comparator = [](auto member_ptr) {
            return [member_ptr](const auto& a, const auto& b) {
                return std::invoke(member_ptr, a) < std::invoke(member_ptr, b);
            };
        };

        std::vector<Employee> team = {
            {"Alice", 30, 85000.0},
            {"Bob", 25, 72000.0},
            {"Carol", 35, 95000.0}
        };

        std::sort(team.begin(), team.end(), field_comparator(&Employee::salary));
        for (const auto& e : team) std::print("{}({}) ", e.name, e.salary);
        std::print("\n");

        std::sort(team.begin(), team.end(), field_comparator(&Employee::name));
        for (const auto& e : team) std::print("{} ", e.name);
        std::print("\n");

        std::sort(team.begin(), team.end(), field_comparator(&Employee::age));
        for (const auto& e : team) std::print("{}({}) ", e.name, e.age);
        std::print("\n");
    }

    // --- Avant/Après analyze (ligne 714-767) ---
    {
        // C++03 style
        struct AboveThreshold {
            double threshold;
            AboveThreshold(double t) : threshold(t) {}
            bool operator()(double v) const { return v > threshold; }
        };

        struct DescOrder {
            bool operator()(double a, double b) const { return a > b; }
        };

        struct Summer {
            double& sum;
            int& count;
            Summer(double& s, int& c) : sum(s), count(c) {}
            void operator()(double v) { sum += v; ++count; }
        };

        auto analyze_old = [](const std::vector<double>& data, double threshold) {
            std::vector<double> above;
            std::copy_if(data.begin(), data.end(), std::back_inserter(above),
                         AboveThreshold(threshold));
            std::sort(above.begin(), above.end(), DescOrder());
            double sum = 0;
            int count = 0;
            std::for_each(above.begin(), above.end(), Summer(sum, count));
            if (count > 0) {
                std::print("Old: Average of {} values above {}: {:.2f}\n",
                           count, threshold, sum / count);
            }
        };

        // C++14 style
        auto analyze_new = [](const std::vector<double>& data, double threshold) {
            std::vector<double> above;
            std::copy_if(data.begin(), data.end(), std::back_inserter(above),
                [threshold](double v) { return v > threshold; });
            std::sort(above.begin(), above.end(),
                [](double a, double b) { return a > b; });
            double sum = 0;
            std::for_each(above.begin(), above.end(), [&sum](double v) { sum += v; });
            if (!above.empty()) {
                std::print("New: Average of {} values above {}: {:.2f}\n",
                           above.size(), threshold, sum / above.size());
            }
        };

        std::vector<double> data = {10.5, 25.3, 7.8, 30.1, 15.9, 42.0, 3.2};
        analyze_old(data, 15.0);
        analyze_new(data, 15.0);
    }

    return 0;
}
