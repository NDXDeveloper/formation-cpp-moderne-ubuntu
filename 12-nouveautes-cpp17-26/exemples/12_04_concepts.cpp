/* ============================================================================
   Section 12.4 : Concepts (C++20)
   Description : Contraintes sur les templates - sort avec concept, quatre
                 syntaxes, multiply avec requires, concept Numeric personnalise,
                 Printable, Hashable, resolution par contraintes (overload)
   Fichier source : 04-concepts.md
   ============================================================================ */
#include <algorithm>
#include <concepts>
#include <iterator>
#include <vector>
#include <print>
#include <string>
#include <format>
#include <functional>
#include <type_traits>

using namespace std::string_literals;

// === sort_container avec concept (lignes 82-85) ===
template <std::ranges::random_access_range Container>
void sort_container(Container& c) {
    std::sort(c.begin(), c.end());
}

// === Quatre syntaxes (lignes 103-139) ===
// 1. requires clause
template <typename T>
    requires std::integral<T>
T add_v1(T a, T b) { return a + b; }

// 2. concept en lieu de typename
template <std::integral T>
T add_v2(T a, T b) { return a + b; }

// 3. trailing requires
template <typename T>
T add_v3(T a, T b) requires std::integral<T> { return a + b; }

// 4. auto contraint
auto add_v4(std::integral auto a, std::integral auto b) { return a + b; }

// === multiply avec requires (lignes 159-163) ===
template <typename T>
    requires std::is_arithmetic_v<T> && (!std::same_as<T, bool>)
T multiply(T a, T b) { return a * b; }

// === Concept Numeric personnalise (lignes 178-179) ===
template <typename T>
concept Numeric = std::is_arithmetic_v<T> && !std::is_same_v<T, bool>;

template <Numeric T>
T multiply_v2(T a, T b) { return a * b; }

// === Concept Printable (lignes 201-204) ===
template <typename T>
concept Printable = requires(T value) {
    { std::format("{}", value) } -> std::convertible_to<std::string>;
};

// === Concept Hashable (lignes 215-217) ===
template <typename T>
concept Hashable = requires(T a) {
    { std::hash<T>{}(a) } -> std::convertible_to<std::size_t>;
};

template <typename T>
concept HashableEquality = Hashable<T> && std::equality_comparable<T>;

// === Resolution par contraintes (lignes 305-325) ===
template <typename T>
void describe(const T&) {
    std::print("Valeur quelconque\n");
}

template <std::integral T>
void describe(const T& value) {
    std::print("Entier : {}\n", value);
}

template <std::floating_point T>
void describe(const T& value) {
    std::print("Flottant : {}\n", value);
}

int main() {
    // --- sort avec concept ---
    std::print("=== sort_container ===\n");
    std::vector<int> vec = {5, 3, 1, 4, 2};
    sort_container(vec);
    for (int v : vec) std::print("{} ", v);
    std::print("\n");

    // --- Quatre syntaxes ---
    std::print("\n=== quatre syntaxes add ===\n");
    std::print("v1: {}\n", add_v1(3, 4));
    std::print("v2: {}\n", add_v2(3, 4));
    std::print("v3: {}\n", add_v3(3, 4));
    std::print("v4: {}\n", add_v4(3, 4));

    // --- multiply ---
    std::print("\n=== multiply ===\n");
    std::print("multiply(3,4)={}\n", multiply(3, 4));
    std::print("multiply(2.5,1.5)={}\n", multiply(2.5, 1.5));
    // multiply(true, false);  // Erreur : bool ne satisfait pas la contrainte

    // --- Numeric concept ---
    std::print("multiply_v2(3,4)={}\n", multiply_v2(3, 4));
    std::print("multiply_v2(2.5,1.5)={}\n", multiply_v2(2.5, 1.5));

    // --- Printable concept ---
    std::print("\n=== Printable concept ===\n");
    std::print("int is Printable: {}\n", Printable<int>);
    std::print("string is Printable: {}\n", Printable<std::string>);

    // --- Hashable concept ---
    std::print("\n=== Hashable concept ===\n");
    std::print("int is Hashable: {}\n", Hashable<int>);
    std::print("string is Hashable: {}\n", Hashable<std::string>);
    std::print("int is HashableEquality: {}\n", HashableEquality<int>);

    // --- Overload resolution ---
    std::print("\n=== overload resolution ===\n");
    describe(42);
    describe(3.14);
    describe("hello"s);
}
