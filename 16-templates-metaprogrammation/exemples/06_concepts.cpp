/* ============================================================================
   Section 16.6 : Concepts (C++20) pour contraindre les templates
   Description : Concept Numeric/Addable, trois syntaxes d'utilisation,
                 résolution de surcharge avec subsomption, Counter/Wrapper
                 contraints, concepts + variadic templates
   Fichier source : 06-concepts.md
   ============================================================================ */
#include <print>
#include <concepts>
#include <type_traits>
#include <string>
#include <vector>

// === Concept Numeric ===
template <typename T>
concept Numeric = std::is_arithmetic_v<T>;

static_assert(Numeric<int>);
static_assert(Numeric<double>);
static_assert(!Numeric<std::string>);

// === Concept Addable ===
template <typename T>
concept Addable = requires(T a, T b) {
    { a + b } -> std::convertible_to<T>;
};

// === Syntaxe 1 ===
template <std::integral T>
T doubler_s1(T valeur) { return valeur * 2; }

// === Syntaxe 2 ===
template <typename T>
    requires std::integral<T>
T doubler_s2(T valeur) { return valeur * 2; }

// === Syntaxe 2 composée ===
template <typename T, typename U>
    requires std::convertible_to<U, T> && std::integral<T>
T ajouter(T a, U b) { return a + static_cast<T>(b); }

// === Syntaxe 3 ===
std::integral auto doubler_s3(std::integral auto valeur) { return valeur * 2; }
auto tripler = [](std::integral auto val) { return val * 3; };

// === Résolution de surcharge ===
template <typename T>
std::string traiter(T) { return "type quelconque"; }

template <std::integral T>
std::string traiter(T) { return "type entier"; }

template <std::signed_integral T>
std::string traiter(T) { return "type entier signe"; }

// === Counter ===
template <std::integral T>
class Counter {
public:
    explicit Counter(T initial = T{0}) : value_{initial} {}
    void increment() { ++value_; }
    T get() const { return value_; }
private:
    T value_;
};

// === Wrapper contraint ===
template <typename T>
class WrapperGen {
public:
    void info() const { std::print("Wrapper generique\n"); }
};

template <typename T>
    requires std::is_arithmetic_v<T>
class WrapperGen<T> {
public:
    void info() const { std::print("Wrapper numerique\n"); }
};

// === Concepts + variadic ===
template <typename... Args>
    requires (std::formattable<Args, char> && ...)
void print_all(Args&&... args) {
    (std::print("{} ", args), ...);
    std::print("\n");
}

int main() {
    // Trois syntaxes
    {
        std::print("s1: {}\n", doubler_s1(21));   // 42
        std::print("s2: {}\n", doubler_s2(21));   // 42
        std::print("s3: {}\n", doubler_s3(21));   // 42
        std::print("tripler: {}\n", tripler(14));  // 42
    }

    // ajouter
    { std::print("ajouter(10, 5) = {}\n", ajouter(10, 5)); }  // 15

    // Résolution de surcharge
    {
        std::print("traiter(3.14) = {}\n", traiter(3.14));  // type quelconque
        std::print("traiter(42u) = {}\n", traiter(42u));     // type entier
        std::print("traiter(42) = {}\n", traiter(42));        // type entier signe
    }

    // Counter
    {
        Counter<int> c1{10};
        c1.increment();
        std::print("Counter<int>: {}\n", c1.get());  // 11
    }

    // Wrapper
    {
        WrapperGen<int> w1; w1.info();             // Wrapper numerique
        WrapperGen<std::string> w2; w2.info();     // Wrapper generique
    }

    // print_all
    { print_all(1, 2.0, "hello"); }

    // Addable
    {
        static_assert(Addable<int>);
        static_assert(Addable<std::string>);
        std::print("Addable OK\n");
    }
}
