/* ============================================================================
   Section 16.7 : Fold expressions (C++17)
   Description : Les 4 formes de fold, somme/produit, print_all, tous_vrais,
                 au_moins_un, est_parmi, log flux <<, push_all, concepts+fold,
                 somme_carres, appliquer, afficher_indexed, associativité
   Fichier source : 07-fold-expressions.md
   ============================================================================ */
#include <print>
#include <string>
#include <vector>
#include <tuple>
#include <concepts>
#include <utility>
#include <iostream>

using namespace std::string_literals;

// === somme (binary left fold) ===
template <typename... Args>
auto somme(Args... args) {
    return (0 + ... + args);
}

// === produit ===
template <typename... Args>
auto produit(Args... args) {
    return (1 * ... * args);
}

// === print_all (comma fold) ===
template <typename... Args>
void print_all(Args&&... args) {
    (std::print("{} ", args), ...);
    std::print("\n");
}

// === tous_vrais / au_moins_un ===
template <typename... Args>
bool tous_vrais(Args... args) { return (... && args); }

template <typename... Args>
bool au_moins_un(Args... args) { return (... || args); }

// === est_parmi ===
template <typename T, typename... Options>
bool est_parmi(const T& valeur, const Options&... options) {
    return (... || (valeur == options));
}

// === log (left fold with <<) ===
template <typename... Args>
void log_stream(std::ostream& os, Args&&... args) {
    (os << ... << args);
    os << '\n';
}

// === push_all ===
template <typename Container, typename... Args>
void push_all(Container& c, Args&&... args) {
    (c.push_back(std::forward<Args>(args)), ...);
}

// === somme_entiers (concepts + fold) ===
template <typename... Args>
    requires (std::integral<Args> && ...)
auto somme_entiers(Args... args) {
    return (0 + ... + args);
}

// === AllRegular concept ===
template <typename... Ts>
concept AllRegular = (std::regular<Ts> && ...);

template <typename... Ts>
    requires AllRegular<Ts...>
class MultiStore {
    std::tuple<Ts...> data_;
};

// === somme_carres ===
template <typename... Args>
auto somme_carres(Args... args) {
    return (0 + ... + (args * args));
}

// === appliquer ===
template <typename F, typename... Args>
void appliquer(F&& f, Args&&... args) {
    (f(std::forward<Args>(args)), ...);
}

// === afficher_indexed ===
template <typename Tuple, std::size_t... Is>
void afficher_indexed_impl(const Tuple& t, std::index_sequence<Is...>) {
    ((std::print("[{}] = {} ", Is, std::get<Is>(t))), ...);
    std::print("\n");
}

template <typename... Types>
void afficher_indexed(const std::tuple<Types...>& t) {
    afficher_indexed_impl(t, std::index_sequence_for<Types...>{});
}

// === sub_left / sub_right ===
template <typename... Args>
auto sub_left(Args... args) { return (... - args); }

template <typename... Args>
auto sub_right(Args... args) { return (args - ...); }

int main() {
    // somme / produit
    {
        std::print("{}\n", somme(1, 2, 3, 4, 5));   // 15
        std::print("{}\n", produit(2, 3, 4));         // 24
        std::print("{}\n", somme());                   // 0
        std::print("{}\n", produit());                 // 1
    }

    // print_all
    { print_all(1, "hello", 3.14, true); }

    // tous_vrais / au_moins_un
    {
        std::print("{}\n", tous_vrais(true, true, true));    // true
        std::print("{}\n", tous_vrais(true, false, true));   // false
        std::print("{}\n", au_moins_un(false, false, true)); // true
    }

    // est_parmi
    {
        std::print("{}\n", est_parmi(3, 1, 2, 3, 4));  // true
        std::print("{}\n", est_parmi(7, 1, 2, 3));      // false
    }

    // log_stream
    { log_stream(std::cout, "User ", "Alice", " logged in at ", 1709123456); }

    // push_all
    {
        std::vector<int> v;
        push_all(v, 1, 2, 3, 4, 5);
        for (auto x : v) std::print("{} ", x);
        std::print("\n");  // 1 2 3 4 5
    }

    // somme_entiers
    { std::print("somme_entiers = {}\n", somme_entiers(1, 2, 3)); }  // 6

    // MultiStore
    {
        [[maybe_unused]] MultiStore<int, std::string, double> ms;
        std::print("MultiStore OK\n");
    }

    // somme_carres
    { std::print("somme_carres = {}\n", somme_carres(1, 2, 3, 4)); }  // 30

    // appliquer
    {
        appliquer([](auto x) { std::print("{} ", x * 2); }, 1, 2, 3);
        std::print("\n");  // 2 4 6
    }

    // afficher_indexed
    {
        auto t = std::make_tuple(42, 3.14, "hello"s);
        afficher_indexed(t);  // [0] = 42 [1] = 3.14 [2] = hello
    }

    // Associativité
    {
        std::print("sub_left(10,3,2,1) = {}\n", sub_left(10, 3, 2, 1));   // 4
        std::print("sub_right(10,3,2,1) = {}\n", sub_right(10, 3, 2, 1)); // 8
    }
}
