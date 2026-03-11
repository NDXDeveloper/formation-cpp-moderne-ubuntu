/* ============================================================================
   Section 16.6.2 : Concepts standard de la STL
   Description : Tous les concepts standard par famille : core, arithmetic,
                 object, comparison, callable, iterator, range. Fonctions
                 compter, trouver_min_max, creer, stocker
   Fichier source : 06.2-concepts-standard.md
   ============================================================================ */
#include <concepts>
#include <print>
#include <string>
#include <vector>
#include <list>
#include <forward_list>
#include <deque>
#include <array>
#include <ranges>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string_view>

// === Core Language ===
static_assert(std::same_as<int, int>);
static_assert(!std::same_as<int, long>);
static_assert(!std::same_as<int, const int>);

struct AnimalBase {};
struct ChienDerived : public AnimalBase {};
static_assert(std::derived_from<ChienDerived, AnimalBase>);
static_assert(std::derived_from<AnimalBase, AnimalBase>);

static_assert(std::convertible_to<int, double>);
static_assert(!std::convertible_to<std::string, int>);
static_assert(std::common_with<int, double>);
static_assert(std::assignable_from<int&, int>);
static_assert(std::swappable<int>);

// === Arithmetic ===
static_assert(std::integral<int>);
static_assert(std::integral<bool>);
static_assert(!std::integral<double>);
static_assert(std::signed_integral<int>);
static_assert(!std::signed_integral<unsigned int>);
static_assert(std::floating_point<double>);
static_assert(!std::floating_point<int>);

template <typename T>
concept Arithmetic = std::integral<T> || std::floating_point<T>;

// === Object ===
static_assert(std::default_initializable<int>);
static_assert(std::constructible_from<std::string, const char*>);
static_assert(std::movable<std::unique_ptr<int>>);
static_assert(!std::copyable<std::unique_ptr<int>>);
static_assert(std::copyable<std::string>);
static_assert(std::semiregular<int>);
static_assert(!std::semiregular<std::unique_ptr<int>>);
static_assert(std::regular<int>);
static_assert(std::regular<std::string>);

// === Comparison ===
static_assert(std::equality_comparable<int>);
static_assert(std::equality_comparable_with<std::string, const char*>);
static_assert(std::totally_ordered<int>);
static_assert(std::three_way_comparable<int>);

// === Iterator ===
static_assert(std::forward_iterator<std::forward_list<int>::iterator>);
static_assert(std::bidirectional_iterator<std::list<int>::iterator>);
static_assert(std::random_access_iterator<std::deque<int>::iterator>);
static_assert(std::contiguous_iterator<std::vector<int>::iterator>);
static_assert(std::contiguous_iterator<int*>);

// === Range ===
static_assert(std::ranges::range<std::vector<int>>);
static_assert(std::ranges::range<std::string>);
static_assert(std::ranges::range<int[10]>);
static_assert(!std::ranges::range<int>);
static_assert(std::ranges::contiguous_range<std::vector<int>>);
static_assert(std::ranges::bidirectional_range<std::list<int>>);
static_assert(std::ranges::sized_range<std::vector<int>>);
static_assert(std::ranges::view<std::string_view>);
static_assert(!std::ranges::view<std::vector<int>>);

// === compter ===
template <std::forward_iterator It, std::sentinel_for<It> Sent>
std::size_t compter(It debut, Sent fin) {
    std::size_t n = 0;
    while (debut != fin) { ++debut; ++n; }
    return n;
}

// === trouver_min_max ===
template <std::ranges::input_range R>
    requires std::totally_ordered<std::ranges::range_value_t<R>>
auto trouver_min_max(R&& range) {
    auto it = std::ranges::begin(range);
    auto sent = std::ranges::end(range);
    if (it == sent) throw std::runtime_error("Range vide");
    auto min_val = *it, max_val = *it;
    while (++it != sent) {
        if (*it < min_val) min_val = *it;
        if (*it > max_val) max_val = *it;
    }
    return std::pair{min_val, max_val};
}

// === creer factory ===
template <typename T, typename... Args>
    requires std::constructible_from<T, Args...>
T creer(Args&&... args) {
    return T(std::forward<Args>(args)...);
}

int main() {
    std::print("static_assert all passed\n");

    // Callable concepts
    {
        auto carre = [](double x) { return x * x; };
        static_assert(std::invocable<decltype(carre), double>);
        auto est_pair = [](int n) { return n % 2 == 0; };
        static_assert(std::predicate<decltype(est_pair), int>);
        auto moins_que = [](int a, int b) { return a < b; };
        static_assert(std::relation<decltype(moins_que), int, int>);
        std::print("Callable concepts OK\n");
    }

    // compter
    {
        std::vector<int> v{1, 2, 3, 4, 5};
        std::print("compter(vector) = {}\n", compter(v.begin(), v.end()));  // 5
    }

    // trouver_min_max
    {
        std::vector v{3, 1, 4, 1, 5, 9};
        auto [min, max] = trouver_min_max(v);
        std::print("Min: {}, Max: {}\n", min, max);  // 1, 9

        auto [a, b] = trouver_min_max(std::array{7, 2, 8});
        std::print("Min: {}, Max: {}\n", a, b);  // 2, 8
    }

    // creer
    {
        auto s = creer<std::string>("hello");
        std::print("creer<string> = {}\n", s);
    }
}
