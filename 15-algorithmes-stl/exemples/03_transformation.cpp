/* ============================================================================
   Section 15.3 : Transformation et accumulation
   Description : Exemples complets couvrant transform (unaire, binaire,
                 in-place, changement de type), ranges::transform avec
                 projection, for_each, for_each_n, accumulate, reduce,
                 transform_reduce, inner_product, partial_sum,
                 inclusive_scan, exclusive_scan, iota, generate, generate_n.
   Fichier source : 03-transformation.md
   ============================================================================ */

#include <algorithm>
#include <numeric>
#include <ranges>
#include <vector>
#include <string>
#include <print>
#include <limits>
#include <cmath>

struct Employee {
    std::string name;
    double salary;
};

struct Stats {
    double sum = 0.0;
    double min = std::numeric_limits<double>::max();
    double max = std::numeric_limits<double>::lowest();
    int count = 0;
};

int main() {
    // === transform unary ===
    {
        std::vector<int> v = {1, 2, 3, 4, 5};
        std::vector<int> squares(v.size());
        std::transform(v.begin(), v.end(), squares.begin(), [](int x) { return x * x; });
        for (int x : squares) std::print("{} ", x);
        std::println("");
        // 1 4 9 16 25
    }

    // === transform with back_inserter ===
    {
        std::vector<int> v = {1, 2, 3, 4, 5};
        std::vector<int> squares;
        std::transform(v.begin(), v.end(), std::back_inserter(squares), [](int x) { return x * x; });
        for (int x : squares) std::print("{} ", x);
        std::println("");
    }

    // === transform in-place ===
    {
        std::vector<int> v = {1, 2, 3, 4, 5};
        std::transform(v.begin(), v.end(), v.begin(), [](int x) { return x * 2; });
        for (int x : v) std::print("{} ", x);
        std::println("");
        // 2 4 6 8 10
    }

    // === transform binary ===
    {
        std::vector<int> a = {1, 2, 3, 4, 5};
        std::vector<int> b = {10, 20, 30, 40, 50};
        std::vector<int> sums(a.size());
        std::transform(a.begin(), a.end(), b.begin(), sums.begin(),
            [](int x, int y) { return x + y; });
        for (int x : sums) std::print("{} ", x);
        std::println("");
        // 11 22 33 44 55
    }

    // === transform binary deltas ===
    {
        std::vector<double> before = {100.0, 200.0, 150.0, 300.0};
        std::vector<double> after  = {110.0, 195.0, 160.0, 310.0};
        std::vector<double> deltas(before.size());
        std::transform(before.begin(), before.end(), after.begin(), deltas.begin(),
            [](double b, double a) { return a - b; });
        for (double d : deltas) std::print("{} ", d);
        std::println("");
        // 10 -5 10 10
    }

    // === transform type change ===
    {
        std::vector<int> codes = {200, 404, 500, 301, 200};
        std::vector<std::string> labels;
        std::transform(codes.begin(), codes.end(), std::back_inserter(labels),
            [](int code) -> std::string {
                switch (code) {
                    case 200: return "OK";
                    case 301: return "Redirect";
                    case 404: return "Not Found";
                    case 500: return "Server Error";
                    default:  return "Unknown";
                }
            });
        for (const auto& l : labels) std::print("{}, ", l);
        std::println("");
        // OK, Not Found, Server Error, Redirect, OK,
    }

    // === ranges transform with projection ===
    {
        std::vector<Employee> team = {
            {"Alice", 75000.0}, {"Bob", 62000.0}, {"Carol", 88000.0}
        };
        std::vector<std::string> names(team.size());
        std::ranges::transform(team, names.begin(), &Employee::name);
        for (const auto& n : names) std::print("{} ", n);
        std::println("");
        // Alice Bob Carol
    }

    // === for_each ===
    {
        std::vector<int> v = {1, 2, 3, 4, 5};
        std::for_each(v.begin(), v.end(), [](int x) { std::print("{} ", x); });
        std::println("");
        std::for_each(v.begin(), v.end(), [](int& x) { x *= 3; });
        for (int x : v) std::print("{} ", x);
        std::println("");
        // 3 6 9 12 15
    }

    // === for_each_n ===
    {
        std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8};
        std::for_each_n(v.begin(), 4, [](int& x) { x *= 10; });
        for (int x : v) std::print("{} ", x);
        std::println("");
        // 10 20 30 40 5 6 7 8
    }

    // === accumulate ===
    {
        std::vector<int> v = {1, 2, 3, 4, 5};
        int sum = std::accumulate(v.begin(), v.end(), 0);
        std::print("sum={}\n", sum);
        // sum=15
        int product = std::accumulate(v.begin(), v.end(), 1, std::multiplies<int>{});
        std::print("product={}\n", product);
        // product=120
    }

    // === accumulate pitfall ===
    {
        std::vector<double> v = {1.5, 2.5, 3.5};
        int bad = std::accumulate(v.begin(), v.end(), 0);
        double good = std::accumulate(v.begin(), v.end(), 0.0);
        std::print("bad={}, good={}\n", bad, good);
        // bad=6, good=7.5
    }

    // === accumulate strings ===
    {
        std::vector<std::string> words = {"Hello", " ", "World", "!"};
        std::string sentence = std::accumulate(words.begin(), words.end(), std::string{});
        std::print("sentence={}\n", sentence);
        // sentence=Hello World!
    }

    // === accumulate Stats ===
    {
        std::vector<double> data = {3.14, 2.71, 1.41, 1.73, 2.23};
        Stats result = std::accumulate(data.begin(), data.end(), Stats{},
            [](Stats acc, double val) {
                acc.sum += val;
                acc.min = std::min(acc.min, val);
                acc.max = std::max(acc.max, val);
                acc.count++;
                return acc;
            });
        std::print("Somme: {:.2f}, Min: {:.2f}, Max: {:.2f}, Moyenne: {:.2f}\n",
                   result.sum, result.min, result.max, result.sum / result.count);
        // Somme: 11.22, Min: 1.41, Max: 3.14, Moyenne: 2.24
    }

    // === reduce ===
    {
        std::vector<int> v = {1, 2, 3, 4, 5};
        int sum = std::reduce(v.begin(), v.end(), 0);
        std::print("reduce sum={}\n", sum);
        int product = std::reduce(v.begin(), v.end(), 1, std::multiplies<int>{});
        std::print("reduce product={}\n", product);
    }

    // === transform_reduce unary ===
    {
        std::vector<int> v = {1, 2, 3, 4, 5};
        int sum_sq = std::transform_reduce(v.begin(), v.end(), 0,
            std::plus<>{}, [](int x) { return x * x; });
        std::print("sum_sq={}\n", sum_sq);
        // sum_sq=55
    }

    // === transform_reduce binary (dot product) ===
    {
        std::vector<double> prices    = {10.0, 25.0, 5.0};
        std::vector<int>    quantities = {3, 2, 10};
        double total = std::transform_reduce(prices.begin(), prices.end(),
            quantities.begin(), 0.0);
        std::print("total={}\n", total);
        // total=130
    }

    // === transform_reduce MAE ===
    {
        std::vector<double> predictions = {2.5, 3.0, 4.5, 5.0};
        std::vector<double> actuals     = {2.7, 2.8, 4.6, 5.2};
        double mae = std::transform_reduce(predictions.begin(), predictions.end(),
            actuals.begin(), 0.0, std::plus<>{},
            [](double pred, double actual) { return std::abs(pred - actual); });
        std::print("mae={:.1f}\n", mae);
        // mae=0.7
    }

    // === inner_product ===
    {
        std::vector<double> a = {1.0, 2.0, 3.0};
        std::vector<double> b = {4.0, 5.0, 6.0};
        double dot = std::inner_product(a.begin(), a.end(), b.begin(), 0.0);
        std::print("dot={}\n", dot);
        // dot=32
    }

    // === partial_sum ===
    {
        std::vector<int> v = {1, 2, 3, 4, 5};
        std::vector<int> cumul(v.size());
        std::partial_sum(v.begin(), v.end(), cumul.begin());
        for (int x : cumul) std::print("{} ", x);
        std::println("");
        // 1 3 6 10 15
    }

    // === partial_sum budget ===
    {
        std::vector<double> monthly_expenses = {1200.0, 1350.0, 980.0, 1500.0, 1100.0, 1450.0};
        std::vector<double> cumulative(monthly_expenses.size());
        std::partial_sum(monthly_expenses.begin(), monthly_expenses.end(), cumulative.begin());
        double budget = 5000.0;
        auto it = std::find_if(cumulative.begin(), cumulative.end(),
            [budget](double total) { return total > budget; });
        if (it != cumulative.end()) {
            auto month = std::distance(cumulative.begin(), it) + 1;
            std::print("Budget dépassé au mois {} (cumul : {:.0f})\n", month, *it);
            // Budget dépassé au mois 4 (cumul : 5030)
        }
    }

    // === inclusive_scan / exclusive_scan ===
    {
        std::vector<int> v = {1, 2, 3, 4, 5};
        std::vector<int> inc(v.size()), exc(v.size());
        std::inclusive_scan(v.begin(), v.end(), inc.begin());
        std::exclusive_scan(v.begin(), v.end(), exc.begin(), 0);
        std::print("inc: "); for (int x : inc) std::print("{} ", x); std::println("");
        std::print("exc: "); for (int x : exc) std::print("{} ", x); std::println("");
        // inc: 1 3 6 10 15
        // exc: 0 1 3 6 10
    }

    // === iota ===
    {
        std::vector<int> indices(10);
        std::iota(indices.begin(), indices.end(), 0);
        for (int x : indices) std::print("{} ", x);
        std::println("");
        // 0 1 2 3 4 5 6 7 8 9
    }

    // === iota indirect sort ===
    {
        std::vector<std::string> names = {"Charlie", "Alice", "Bob", "Dave"};
        std::vector<int> order(names.size());
        std::iota(order.begin(), order.end(), 0);
        std::sort(order.begin(), order.end(), [&names](int a, int b) {
            return names[a] < names[b];
        });
        for (int i : order) std::print("{}\n", names[i]);
        // Alice\nBob\nCharlie\nDave
    }

    // === generate ===
    {
        std::vector<int> v(10);
        int power = 1;
        std::generate(v.begin(), v.end(), [&power]() {
            int result = power;
            power *= 2;
            return result;
        });
        for (int x : v) std::print("{} ", x);
        std::println("");
        // 1 2 4 8 16 32 64 128 256 512
    }

    // === generate_n ===
    {
        std::vector<int> v;
        v.reserve(5);
        std::generate_n(std::back_inserter(v), 5, [n = 0]() mutable {
            int current = n++;
            return current * current;
        });
        for (int x : v) std::print("{} ", x);
        std::println("");
        // 0 1 4 9 16
    }
}
