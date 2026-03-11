/* ============================================================================
   Section 16.5 : Variadic templates (C++11)
   Description : sizeof..., pack expansion, récursion afficher/somme,
                 if constexpr, SimpleTuple, Emetteur, Overload pattern,
                 index_sequence, afficher_tuple
   Fichier source : 05-variadic-templates.md
   ============================================================================ */
#include <print>
#include <string>
#include <tuple>
#include <functional>
#include <vector>
#include <variant>
#include <cmath>
#include <utility>

using namespace std::string_literals;

// === sizeof... ===
template <typename... Types>
void info(Types... args) {
    std::print("Nombre de types : {}\n", sizeof...(Types));
    std::print("Nombre d'arguments : {}\n", sizeof...(args));
}

// === Pack expansion : appeler_print ===
template <typename... Types>
void appeler_print(Types... args) {
    (std::print("{} ", args), ...);
}

// === Pack expansion dans un appel de fonction ===
template <typename... Args>
auto creer_tuple(Args... args) {
    return std::tuple<Args...>(args...);
}

// === Pack expansion avec transformation (pointeurs) ===
template <typename... Args>
auto creer_tuple_pointeurs(Args&... args) {
    return std::make_tuple(&args...);
}

// === Pack expansion avec conversion ===
template <typename... Args>
auto en_doubles(Args... args) {
    return std::make_tuple(static_cast<double>(args)...);
}

// === Récursion : afficher ===
void afficher() { std::print("\n"); }

template <typename T, typename... Rest>
void afficher(T premier, Rest... reste) {
    std::print("{} ", premier);
    afficher(reste...);
}

// === Récursion : somme ===
template <typename T>
T somme(T valeur) { return valeur; }

template <typename T, typename... Rest>
auto somme(T premier, Rest... reste) {
    return premier + somme(reste...);
}

// === if constexpr somme ===
template <typename T, typename... Rest>
auto somme2(T premier, Rest... reste) {
    if constexpr (sizeof...(reste) == 0) {
        return premier;
    } else {
        return premier + somme2(reste...);
    }
}

// === SimpleTuple ===
template <typename... Types>
class SimpleTuple {};

template <typename Head, typename... Tail>
class SimpleTuple<Head, Tail...> : private SimpleTuple<Tail...> {
public:
    SimpleTuple(const Head& head, const Tail&... tail)
        : SimpleTuple<Tail...>(tail...), valeur_{head} {}
    Head& head() { return valeur_; }
    const Head& head() const { return valeur_; }
    SimpleTuple<Tail...>& tail() {
        return static_cast<SimpleTuple<Tail...>&>(*this);
    }
private:
    Head valeur_;
};

// === Emetteur ===
template <typename... Args>
class Emetteur {
public:
    using Callback = std::function<void(Args...)>;
    void on(Callback cb) { callbacks_.push_back(std::move(cb)); }
    void emit(Args... args) const {
        for (const auto& cb : callbacks_) cb(args...);
    }
private:
    std::vector<Callback> callbacks_;
};

// === Overload pattern ===
template <typename... Ts>
struct Overload : Ts... { using Ts::operator()...; };

template <typename... Ts>
Overload(Ts...) -> Overload<Ts...>;

// === afficher_tuple ===
template <typename Tuple, std::size_t... Is>
void afficher_tuple_impl(const Tuple& t, std::index_sequence<Is...>) {
    ((std::print("{}: {} ", Is, std::get<Is>(t))), ...);
    std::print("\n");
}

template <typename... Types>
void afficher_tuple(const std::tuple<Types...>& t) {
    afficher_tuple_impl(t, std::index_sequence_for<Types...>{});
}

int main() {
    // sizeof...
    {
        info(1, 2.0, "trois");  // 3, 3
        info();                   // 0, 0
    }

    // appeler_print
    {
        appeler_print(42, 3.14, "hi");
        std::print("\n");
    }

    // creer_tuple
    {
        auto t = creer_tuple(1, 2.0, "trois"s);
        std::print("{} {} {}\n", std::get<0>(t), std::get<1>(t), std::get<2>(t));
    }

    // creer_tuple_pointeurs
    {
        int a = 1; double b = 2.0; std::string c = "trois";
        auto t = creer_tuple_pointeurs(a, b, c);
        std::print("{} {} {}\n", *std::get<0>(t), *std::get<1>(t), *std::get<2>(t));
    }

    // en_doubles
    {
        auto t = en_doubles(1, 2.0f, 3L, true);
        std::print("{} {} {} {}\n",
            std::get<0>(t), std::get<1>(t), std::get<2>(t), std::get<3>(t));
    }

    // afficher (récursion)
    { afficher(1, 2.5, "hello", true); }

    // somme
    {
        std::print("somme(1..5) = {}\n", somme(1, 2, 3, 4, 5));      // 15
        std::print("somme(1.0,2.5,3.5) = {}\n", somme(1.0, 2.5, 3.5)); // 7
        std::print("somme(a,b,c) = {}\n", somme("a"s, "b"s, "c"s));    // abc
    }

    // somme2
    { std::print("somme2(1..5) = {}\n", somme2(1, 2, 3, 4, 5)); }  // 15

    // SimpleTuple
    {
        SimpleTuple<int, double, char> t{42, 3.14, 'A'};
        std::print("{}\n", t.head());                 // 42
        std::print("{}\n", t.tail().head());          // 3.14
        std::print("{}\n", t.tail().tail().head());   // A
    }

    // Emetteur
    {
        Emetteur<double, double, long> position_changed;
        position_changed.on([](double x, double y, long ts) {
            std::print("Position ({}, {}) at {}\n", x, y, ts);
        });
        position_changed.on([](double x, double y, long) {
            std::print("Distance from origin: {:.2f}\n", std::sqrt(x*x + y*y));
        });
        position_changed.emit(3.0, 4.0, 1709123456L);
    }

    // Overload pattern
    {
        std::variant<int, double, std::string> v = "hello"s;
        std::visit(Overload{
            [](int val)               { std::print("int: {}\n", val); },
            [](double val)            { std::print("double: {}\n", val); },
            [](const std::string& val){ std::print("string: {}\n", val); }
        }, v);
    }

    // afficher_tuple
    {
        auto t = std::make_tuple(42, 3.14, "hello"s);
        afficher_tuple(t);  // 0: 42 1: 3.14 2: hello
    }
}
