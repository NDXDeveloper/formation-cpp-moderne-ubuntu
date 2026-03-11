/* ============================================================================
   Section 15.7 : Algorithmes parallèles : std::execution policies
   Description : Exemples complets couvrant les quatre politiques d'exécution
                 (seq, par, par_unseq, unseq), sort parallèle, transform
                 parallèle, reduce parallèle, transform_reduce, count_if
                 parallèle, find_if parallèle, for_each parallèle,
                 inclusive_scan parallèle, policy comme paramètre template,
                 adaptive sort (seuil), validation préalable pour exceptions.
   Fichiers source : 07-algorithmes-paralleles.md, 07.1-politiques-execution.md,
                     07.2-parallelisation-algorithmes.md,
                     07.3-precautions-limitations.md
   Compilation : g++-15 -std=c++23 -Wall -Wextra -Wpedantic -O2 -g
                 Ajouter -ltbb pour activer la parallélisation réelle (GCC).
                 Sans -ltbb, les algorithmes s'exécutent en fallback séquentiel.
   ============================================================================ */

#include <algorithm>
#include <numeric>
#include <execution>
#include <vector>
#include <random>
#include <cmath>
#include <chrono>
#include <limits>
#include <print>

// Timer simple pour benchmarks indicatifs
class Timer {
    std::chrono::high_resolution_clock::time_point start_;
public:
    Timer() : start_(std::chrono::high_resolution_clock::now()) {}

    double elapsed_ms() const {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(now - start_).count();
    }
};

// Génération de données aléatoires
std::vector<int> generate_random(size_t n) {
    std::vector<int> v(n);
    std::mt19937 gen(42);
    std::uniform_int_distribution<int> dist(0, 1'000'000);
    std::generate(v.begin(), v.end(), [&]() { return dist(gen); });
    return v;
}

// Politique comme paramètre template
template<typename ExecutionPolicy>
double compute_rms(ExecutionPolicy policy, const std::vector<double>& data) {
    double sum_sq = std::transform_reduce(policy,
        data.begin(), data.end(),
        0.0,
        std::plus<>{},
        [](double x) { return x * x; }
    );
    return std::sqrt(sum_sq / static_cast<double>(data.size()));
}

// Seuil adaptatif
template<typename Iter, typename Compare = std::less<>>
void adaptive_sort(Iter first, Iter last, Compare comp = {}) {
    auto n = std::distance(first, last);
    if (n < 50'000) {
        std::sort(first, last, comp);
    } else {
        std::sort(std::execution::par, first, last, comp);
    }
}

int main() {
    // === Les quatre politiques d'exécution ===
    {
        std::vector<int> v = {5, 3, 8, 1, 9};

        // seq : séquentiel explicite
        auto v1 = v;
        std::sort(std::execution::seq, v1.begin(), v1.end());
        std::print("seq:       ");
        for (int x : v1) std::print("{} ", x);
        std::println("");

        // par : parallèle
        auto v2 = v;
        std::sort(std::execution::par, v2.begin(), v2.end());
        std::print("par:       ");
        for (int x : v2) std::print("{} ", x);
        std::println("");

        // par_unseq : parallèle + vectorisée
        auto v3 = v;
        std::sort(std::execution::par_unseq, v3.begin(), v3.end());
        std::print("par_unseq: ");
        for (int x : v3) std::print("{} ", x);
        std::println("");

        // unseq (C++20) : vectorisée sans parallélisme
        std::vector<float> data = {3.0f, 1.0f, 4.0f, 1.5f, 2.0f};
        std::for_each(std::execution::unseq, data.begin(), data.end(),
            [](float& x) { x = x * 2.0f + 1.0f; });
        std::print("unseq:     ");
        for (float x : data) std::print("{:.1f} ", x);
        std::println("");
    }

    // === Sort parallèle ===
    {
        auto data = generate_random(1'000'000);

        auto v_seq = data;
        Timer t_seq;
        std::sort(v_seq.begin(), v_seq.end());
        auto seq_ms = t_seq.elapsed_ms();

        auto v_par = data;
        Timer t_par;
        std::sort(std::execution::par, v_par.begin(), v_par.end());
        auto par_ms = t_par.elapsed_ms();

        std::print("Sort 1M : seq={:.1f}ms, par={:.1f}ms\n", seq_ms, par_ms);
        std::print("Résultats identiques : {}\n", v_seq == v_par);
    }

    // === Reduce parallèle ===
    {
        std::vector<double> data(1'000'000);
        std::iota(data.begin(), data.end(), 1.0);

        double sum_acc = std::accumulate(data.begin(), data.end(), 0.0);
        double sum_red = std::reduce(std::execution::par, data.begin(), data.end(), 0.0);
        std::print("accumulate={:.0f}, reduce(par)={:.0f}\n", sum_acc, sum_red);
    }

    // === Transform parallèle ===
    {
        std::vector<double> data(1'000'000);
        std::iota(data.begin(), data.end(), 1.0);
        std::vector<double> result(data.size());

        std::transform(std::execution::par, data.begin(), data.end(), result.begin(),
            [](double x) { return std::sqrt(x) * std::log(x); }
        );
        std::print("transform par: result[0]={:.4f}, result[999999]={:.4f}\n",
                   result[0], result[999999]);
    }

    // === Transform_reduce : somme des carrés ===
    {
        std::vector<int> v = {1, 2, 3, 4, 5};
        int sum_sq = std::transform_reduce(std::execution::par,
            v.begin(), v.end(),
            0,
            std::plus<>{},
            [](int x) { return x * x; }
        );
        std::print("sum_sq={}\n", sum_sq);
        // 55
    }

    // === Transform_reduce : produit scalaire ===
    {
        std::vector<double> a = {1.0, 2.0, 3.0};
        std::vector<double> b = {4.0, 5.0, 6.0};
        double dot = std::transform_reduce(std::execution::par,
            a.begin(), a.end(), b.begin(), 0.0);
        std::print("dot={}\n", dot);
        // 32
    }

    // === Reduce : max parallèle ===
    {
        auto data = generate_random(1'000'000);
        int max_val = std::reduce(std::execution::par,
            data.begin(), data.end(),
            std::numeric_limits<int>::lowest(),
            [](int a, int b) { return std::max(a, b); }
        );
        std::print("max parallèle={}\n", max_val);
    }

    // === count_if parallèle ===
    {
        auto data = generate_random(1'000'000);
        auto n = std::count_if(std::execution::par, data.begin(), data.end(),
            [](int x) { return x > 500'000; }
        );
        std::print("count_if > 500000 : {}\n", n);
    }

    // === find_if parallèle ===
    {
        std::vector<int> data(1'000'000);
        std::iota(data.begin(), data.end(), 0);
        auto it = std::find_if(std::execution::par, data.begin(), data.end(),
            [](int x) { return x == 42; }
        );
        if (it != data.end()) {
            std::print("find_if(42) : trouvé à l'index {}\n", std::distance(data.begin(), it));
        }
    }

    // === for_each parallèle (normalisation) ===
    {
        std::vector<double> data = {10.0, 20.0, 30.0, 40.0, 50.0};
        double max_val = std::reduce(std::execution::par,
            data.begin(), data.end(),
            std::numeric_limits<double>::lowest(),
            [](double a, double b) { return std::max(a, b); }
        );
        std::for_each(std::execution::par, data.begin(), data.end(),
            [max_val](double& x) { x /= max_val; }
        );
        for (double x : data) std::print("{:.2f} ", x);
        std::println("");
        // 0.20 0.40 0.60 0.80 1.00
    }

    // === inclusive_scan parallèle ===
    {
        std::vector<double> data = {1.0, 2.0, 3.0, 4.0, 5.0};
        std::vector<double> cumulative(data.size());
        std::inclusive_scan(std::execution::par,
            data.begin(), data.end(), cumulative.begin());
        for (double x : cumulative) std::print("{:.0f} ", x);
        std::println("");
        // 1 3 6 10 15
    }

    // === Policy comme paramètre template ===
    {
        std::vector<double> small_data = {3.0, 4.0, 5.0};
        std::vector<double> large_data(100'000);
        std::iota(large_data.begin(), large_data.end(), 1.0);

        double rms_small = compute_rms(std::execution::seq, small_data);
        double rms_large = compute_rms(std::execution::par, large_data);
        std::print("RMS small={:.4f}, RMS large={:.4f}\n", rms_small, rms_large);
    }

    // === Adaptive sort ===
    {
        auto small = generate_random(1'000);
        auto large = generate_random(100'000);
        adaptive_sort(small.begin(), small.end());
        adaptive_sort(large.begin(), large.end());
        std::print("small sorted: {}\n", std::is_sorted(small.begin(), small.end()));
        std::print("large sorted: {}\n", std::is_sorted(large.begin(), large.end()));
    }

    // === Validation préalable (pas d'exception) ===
    {
        std::vector<int> data = {1, 4, 9, 16, 25};
        bool all_valid = std::all_of(std::execution::par,
            data.begin(), data.end(),
            [](int x) { return x >= 0; }
        );
        if (all_valid) {
            std::vector<double> result(data.size());
            std::transform(std::execution::par,
                data.begin(), data.end(), result.begin(),
                [](int x) { return std::sqrt(static_cast<double>(x)); }
            );
            for (double x : result) std::print("{:.1f} ", x);
            std::println("");
            // 1.0 2.0 3.0 4.0 5.0
        }
    }

    // === Data race : le bon et le mauvais pattern ===
    {
        std::vector<int> data(100'000);
        std::iota(data.begin(), data.end(), 0);

        // ✅ Correct : utiliser std::reduce au lieu de for_each + accumulation
        double sum = std::reduce(std::execution::par, data.begin(), data.end(), 0.0);
        std::print("reduce sum={:.0f}\n", sum);

        // ✅ Correct : utiliser copy_if au lieu de for_each + push_back
        int threshold = 50'000;
        std::vector<int> buffer(data.size());
        auto it = std::copy_if(std::execution::par,
            data.begin(), data.end(), buffer.begin(),
            [threshold](int x) { return x > threshold; }
        );
        buffer.erase(it, buffer.end());
        std::print("copy_if > {} : {} éléments\n", threshold, buffer.size());

        // ✅ Correct : utiliser minmax_element
        auto [min_it, max_it] = std::minmax_element(std::execution::par,
            data.begin(), data.end());
        std::print("min={}, max={}\n", *min_it, *max_it);
    }
}
