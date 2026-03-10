/* ============================================================================
   Section 12.5 : Ranges (C++20)
   Description : Pipelines fonctionnels - ranges::sort, filter, transform,
                 take/drop, iota, reverse/split/join, enumerate/zip/chunk/slide,
                 ranges::to, pipeline de mesures
   Fichier source : 05-ranges.md
   ============================================================================ */
#include <algorithm>
#include <ranges>
#include <vector>
#include <string>
#include <string_view>
#include <print>

int main() {
    // === ranges::sort (lignes 89-96) ===
    std::print("=== ranges::sort ===\n");
    std::vector<int> data = {5, 3, 8, 1, 9, 2, 7};
    std::ranges::sort(data);
    for (int n : data) std::print("{} ", n);
    std::print("\n");

    // === filter view (lignes 111-119) ===
    std::print("\n=== filter view ===\n");
    std::vector<int> data2 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto evens = data2 | std::views::filter([](int n) { return n % 2 == 0; });
    for (int n : evens) std::print("{} ", n);
    std::print("\n");

    // === pipeline filter+transform+take (lignes 143-152) ===
    std::print("\n=== pipeline filter+transform+take ===\n");
    std::vector<int> data3 = {5, 3, 8, 1, 6, 2, 7, 4, 9};
    auto result = data3
        | std::views::filter([](int n) { return n % 2 == 0; })
        | std::views::transform([](int n) { return n * n; })
        | std::views::take(3);
    for (int n : result) std::print("{} ", n);
    std::print("\n");
    // 64 36 4

    // === filter + transform (lignes 183-191) ===
    std::print("\n=== filter + transform ===\n");
    std::vector<int> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto odds = nums | std::views::filter([](int n) { return n % 2 != 0; });
    std::print("odds: ");
    for (int n : odds) std::print("{} ", n);
    std::print("\n");

    auto squares = nums | std::views::transform([](int n) { return n * n; });
    std::print("squares: ");
    for (int n : squares) std::print("{} ", n);
    std::print("\n");

    // === take / drop / take_while / drop_while (lignes 197-211) ===
    std::print("\n=== take/drop ===\n");
    auto first_five = nums | std::views::take(5);
    std::print("take(5): ");
    for (int n : first_five) std::print("{} ", n);
    std::print("\n");

    auto after_three = nums | std::views::drop(3);
    std::print("drop(3): ");
    for (int n : after_three) std::print("{} ", n);
    std::print("\n");

    auto small = nums | std::views::take_while([](int n) { return n < 6; });
    std::print("take_while(<6): ");
    for (int n : small) std::print("{} ", n);
    std::print("\n");

    auto big = nums | std::views::drop_while([](int n) { return n < 6; });
    std::print("drop_while(<6): ");
    for (int n : big) std::print("{} ", n);
    std::print("\n");

    // === iota (lignes 217-225) ===
    std::print("\n=== iota ===\n");
    auto range_10 = std::views::iota(1, 11);
    std::print("iota(1,11): ");
    for (int n : range_10) std::print("{} ", n);
    std::print("\n");

    auto perfect_squares = std::views::iota(1)
        | std::views::transform([](int n) { return n * n; })
        | std::views::take(5);
    std::print("perfect_squares: ");
    for (int n : perfect_squares) std::print("{} ", n);
    std::print("\n");

    // === reverse, split, join (lignes 238-254) ===
    std::print("\n=== reverse/split/join ===\n");
    std::vector<int> v = {1, 2, 3, 4, 5};
    auto reversed = v | std::views::reverse;
    std::print("reverse: ");
    for (int n : reversed) std::print("{} ", n);
    std::print("\n");

    std::string csv = "Alice,Bob,Clara,Dave";
    std::print("split: ");
    for (auto word : csv | std::views::split(',')) {
        auto sv = std::string_view(word.begin(), word.end());
        std::print("'{}' ", sv);
    }
    std::print("\n");

    std::vector<std::vector<int>> nested = {{1, 2}, {3, 4}, {5}};
    auto flat = nested | std::views::join;
    std::print("join: ");
    for (int n : flat) std::print("{} ", n);
    std::print("\n");

    // === enumerate, zip, chunk, slide (C++23) (lignes 261-293) ===
    std::print("\n=== enumerate/zip/chunk/slide (C++23) ===\n");
    std::vector<std::string> names = {"Alice", "Bob", "Clara"};

    std::print("enumerate:\n");
    for (auto [i, name] : std::views::enumerate(names)) {
        std::print("  [{}] {}\n", i, name);
    }

    std::vector<int> scores = {95, 87, 92};
    std::print("zip:\n");
    for (auto [name, score] : std::views::zip(names, scores)) {
        std::print("  {} : {}\n", name, score);
    }

    std::print("chunk(3): ");
    for (auto chunk : std::views::iota(1, 11) | std::views::chunk(3)) {
        std::print("{{");
        for (int n : chunk) std::print("{} ", n);
        std::print("}} ");
    }
    std::print("\n");

    std::print("slide(3): ");
    for (auto window : std::views::iota(1, 6) | std::views::slide(3)) {
        std::print("{{");
        for (int n : window) std::print("{} ", n);
        std::print("}} ");
    }
    std::print("\n");

    // === ranges::to (C++23) (lignes 306-312) ===
    std::print("\n=== ranges::to ===\n");
    auto sq_vec = std::views::iota(1, 11)
        | std::views::filter([](int n) { return n % 2 == 0; })
        | std::views::transform([](int n) { return n * n; })
        | std::ranges::to<std::vector>();
    std::print("squares of evens: ");
    for (int n : sq_vec) std::print("{} ", n);
    std::print("\n");
    // {4, 16, 36, 64, 100}

    // === Measurement pipeline (lignes 338-368) ===
    std::print("\n=== measurement pipeline ===\n");
    struct Measurement {
        std::string sensor_id;
        double temperature;
        bool valid;
    };

    std::vector<Measurement> readings = {
        {"sensor-01", 22.5, true},
        {"sensor-02", -99.0, false},
        {"sensor-01", 23.1, true},
        {"sensor-03", 18.7, true},
        {"sensor-02", 21.0, true},
        {"sensor-01", 24.0, true},
        {"sensor-03", -99.0, false},
        {"sensor-02", 22.3, true},
    };

    auto top_temps = readings
        | std::views::filter([](const Measurement& m) { return m.valid; })
        | std::views::transform([](const Measurement& m) { return m.temperature; })
        | std::ranges::to<std::vector>();

    std::ranges::sort(top_temps, std::greater{});
    auto top_5 = top_temps | std::views::take(5);

    std::print("Top 5 temperatures :\n");
    for (auto [i, temp] : std::views::enumerate(top_5)) {
        std::print("  {}. {:.1f} C\n", i + 1, temp);
    }
}
