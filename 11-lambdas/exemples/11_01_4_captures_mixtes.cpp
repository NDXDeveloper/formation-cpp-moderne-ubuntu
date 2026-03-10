/* ============================================================================
   Section 11.1.4 : Captures mixtes et init captures
   Description : Exemples complets de captures mixtes — combinaison valeur/
                 référence, init captures (renommer, expression, move),
                 captures constexpr, boucle factory, masquage de variable
   Fichier source : 01.4-captures-mixtes.md
   ============================================================================ */
#include <print>
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <format>
#include <algorithm>
#include <chrono>
#include <map>
#include <utility>

int main() {
    // --- Défaut valeur, exception référence (lignes 24-40) ---
    {
        auto process_batch = [](const std::vector<int>& data, int threshold) {
            int match_count = 0;
            int total_sum = 0;

            std::for_each(data.begin(), data.end(),
                [=, &match_count, &total_sum](int value) {
                    if (value > threshold) {
                        ++match_count;
                        total_sum += value;
                    }
                });

            std::print("Matches: {}, Sum: {}\n", match_count, total_sum);
        };

        process_batch({1, 5, 10, 15, 20, 25}, 12);
        // Matches: 3, Sum: 60
    }

    // --- Règles de composition (lignes 105-116) ---
    {
        int a = 1, b = 2, c = 3;
        auto ok1 = [=, &a]() { return a + b + c; };
        auto ok2 = [&, a]() { return a + b + c; };
        auto ok3 = [=, &a, &b]() { return a + b + c; };
        auto ok4 = [&, a, b]() { return a + b + c; };
        std::print("ok1={}, ok2={}, ok3={}, ok4={}\n", ok1(), ok2(), ok3(), ok4());
    }

    // --- Combinaison avec this (lignes 74-85) ---
    {
        struct Pipeline {
            std::string name_ = "default";
            int stage_ = 0;
            auto create_step(int step_id) {
                return [=, this]() {
                    std::print("[{}] Executing step {} at stage {}\n",
                               name_, step_id, stage_);
                };
            }
        };

        Pipeline p;
        auto step = p.create_step(42);
        step();
    }

    // --- Init capture: renommer (lignes 170-179) ---
    {
        struct DatabasePool {
            std::string connection_string_ = "postgres://localhost/mydb";
            auto create_query_runner() {
                return [conn_str = connection_string_]() {
                    std::print("Connecting to: {}\n", conn_str);
                };
            }
        };

        DatabasePool db;
        auto runner = db.create_query_runner();
        runner();
    }

    // --- Init capture: expression (lignes 185-192) ---
    {
        int width = 800;
        int height = 600;

        auto report = [area = width * height, ratio = static_cast<double>(width) / height]() {
            std::print("Area: {}, Aspect ratio: {:.2f}\n", area, ratio);
        };

        report();  // Area: 480000, Aspect ratio: 1.33
    }

    // --- Init capture: boucle factory (lignes 200-211) ---
    {
        std::vector<std::function<std::string()>> generators;

        for (int i = 0; i < 5; ++i) {
            generators.push_back([label = "item_" + std::to_string(i)]() {
                return label;
            });
        }

        for (auto& gen : generators) {
            std::print("{}\n", gen());
        }
    }

    // --- Init capture avec std::move — unique_ptr (lignes 225-235) ---
    {
        auto ptr = std::make_unique<std::string>("critical data");
        auto handler = [p = std::move(ptr)]() {
            std::print("Data: {}\n", *p);
        };
        std::print("ptr is null: {}\n", ptr == nullptr);
        handler();  // Data: critical data
    }

    // --- Init capture avec std::move — vector (lignes 245-257) ---
    {
        std::vector<double> measurements(1'000'000);
        auto analyzer = [data = std::move(measurements)]() {
            double sum = 0.0;
            for (double v : data) sum += v;
            return sum / static_cast<double>(data.size());
        };
        std::print("Size after move: {}\n", measurements.size());  // 0
        std::print("Average: {}\n", analyzer());
    }

    // --- Init capture par référence (lignes 300-309) ---
    {
        std::vector<int> data = {1, 2, 3, 4, 5};
        auto appender = [&ref = data](int value) {
            ref.push_back(value);
        };
        appender(6);
        appender(7);
        std::print("data: ");
        for (int v : data) std::print("{} ", v);
        std::print("\n");
        // 1 2 3 4 5 6 7
    }

    // --- Init capture ref const via as_const (lignes 315-323) ---
    {
        std::map<std::string, int> scores = {{"Alice", 95}, {"Bob", 87}};
        auto reader = [&cscores = std::as_const(scores)](const std::string& name) {
            auto it = cscores.find(name);
            return it != cscores.end() ? it->second : -1;
        };
        std::print("Alice: {}\n", reader("Alice"));  // 95
    }

    // --- Boucle factory avec init captures (lignes 367-385) ---
    {
        std::vector<std::function<std::string()>> formatters;

        std::vector<std::pair<std::string, double>> metrics = {
            {"cpu", 78.5}, {"memory", 62.3}, {"disk", 91.0}
        };

        for (const auto& [name, value] : metrics) {
            formatters.push_back(
                [label = name, val = value, severity = (value > 80.0 ? "HIGH" : "OK")]() {
                    return std::format("[{}] {}: {:.1f}%", severity, label, val);
                });
        }

        for (auto& fmt : formatters) {
            std::print("{}\n", fmt());
        }
    }

    // --- Indépendance de la copie (lignes 399-408) ---
    {
        std::string message = "original";
        auto lambda = [message = message]() { return message; };
        message = "modified";
        std::print("{}\n", lambda());   // original
        std::print("{}\n", message);    // modified
    }

    // --- Expression transformée (lignes 414-420) ---
    {
        std::string message = "hello";
        auto lambda = [msg = message + " world"]() { return msg; };
        std::print("{}\n", lambda());  // hello world
    }

    // --- Évaluation unique (lignes 428-439) ---
    {
        int call_count = 0;
        auto get_next_id = [&call_count]() { return ++call_count; };
        auto lambda = [id = get_next_id()]() { return id; };
        std::print("{}\n", lambda());  // 1
        std::print("{}\n", lambda());  // 1
        std::print("{}\n", lambda());  // 1
    }

    // --- Init capture constexpr (lignes 449-454) ---
    {
        constexpr auto make_multiplier = [](int factor) {
            return [f = factor](int x) { return x * f; };
        };
        constexpr auto double_it = make_multiplier(2);
        static_assert(double_it(21) == 42);
        std::print("double_it(21) = {}\n", double_it(21));
    }

    // --- Anti-pattern: this + init capture (lignes 468-485) ---
    {
        struct Server {
            int port_ = 8080;
            auto make_handler() {
                return [this, port = port_]() {
                    std::print("Snapshot port: {}, Current port: {}\n", port, port_);
                };
            }
            void set_port(int p) { port_ = p; }
        };

        Server s;
        auto h = s.make_handler();
        s.set_port(9090);
        h();  // Snapshot port: 8080, Current port: 9090
    }

    // --- Masquage de variable (lignes 514-518) ---
    {
        int x = 10;
        auto lambda = [x = x * 2]() { return x; };
        std::print("{}\n", x);         // 10
        std::print("{}\n", lambda());  // 20
    }

    return 0;
}
