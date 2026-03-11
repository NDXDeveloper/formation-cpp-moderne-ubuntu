/* ============================================================================
   Section 16.6.1 : Syntaxe requires
   Description : Clause requires (positions), expression requires, les 4 types
                 d'exigences, composition, subsomption Animal/Oiseau,
                 SmartContainer, formater avec requires + if constexpr
   Fichier source : 06.1-requires.md
   ============================================================================ */
#include <print>
#include <concepts>
#include <type_traits>
#include <string>
#include <vector>
#include <algorithm>
#include <format>
#include <cstring>
#include <functional>

// === Concepts de base ===
template <typename T>
concept Incrementable = requires(T a) {
    a++; ++a; a + 1;
};

template <typename T>
concept HasSize = requires(T t) { t.size(); };

template <typename T>
concept HasValueType = requires { typename T::value_type; };

template <typename T>
concept Comparable = requires(T a, T b) {
    { a == b } -> std::convertible_to<bool>;
    { a != b } -> std::convertible_to<bool>;
    { a < b }  -> std::convertible_to<bool>;
};

template <typename T>
concept NothrowMovable = requires(T a) {
    { T(std::move(a)) } noexcept;
};

template <typename T>
concept SmallTrivial = requires {
    requires sizeof(T) <= 16;
    requires std::is_trivially_copyable_v<T>;
};

// === Subsomption Animal/Oiseau ===
template <typename T>
concept Animal = requires(T a) { a.respirer(); a.manger(); };

template <typename T>
concept Oiseau = Animal<T> && requires(T a) { a.voler(); };

template <Animal T>
std::string decrire(T) { return "Un animal"; }

template <Oiseau T>
std::string decrire(T) { return "Un oiseau"; }

struct Chien  { void respirer() {} void manger() {} };
struct Aigle  { void respirer() {} void manger() {} void voler() {} };

// === SmartContainer ===
template <typename T>
class SmartContainer {
public:
    void push(const T& val) { data_.push_back(val); }

    void sort() requires std::totally_ordered<T> {
        std::sort(data_.begin(), data_.end());
    }

    void print_all() const requires std::formattable<T, char> {
        for (const auto& elem : data_) std::print("{} ", elem);
        std::print("\n");
    }

    auto sum() const requires requires(T a, T b) {
        { a + b } -> std::convertible_to<T>;
        { T{0} };
    } {
        T total{0};
        for (const auto& elem : data_) total = total + elem;
        return total;
    }

    std::size_t size() const { return data_.size(); }
private:
    std::vector<T> data_;
};

// === Numeric + if constexpr ===
template <typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

template <typename T>
    requires Numeric<T>
std::string formater(T val) {
    if constexpr (std::integral<T>) {
        return std::format("{}", val);
    } else {
        return std::format("{:.2f}", val);
    }
}

// === Clause requires avec sizeof ===
template <typename T>
    requires (sizeof(T) <= 8)
void traiter_petit(T val) {
    std::print("Type petit ({} octets): {}\n", sizeof(T), val);
}

int main() {
    // Concepts basiques
    {
        static_assert(Incrementable<int>);
        static_assert(!Incrementable<std::string>);
        static_assert(HasSize<std::vector<int>>);
        static_assert(!HasSize<int>);
        static_assert(HasValueType<std::vector<int>>);
        static_assert(!HasValueType<int>);
        static_assert(Comparable<int>);
        static_assert(NothrowMovable<int>);
        static_assert(SmallTrivial<int>);
        static_assert(!SmallTrivial<std::string>);
        std::print("Concepts basiques OK\n");
    }

    // traiter_petit
    {
        traiter_petit(42);    // 4 octets
        traiter_petit(3.14);  // 8 octets
    }

    // Subsomption
    {
        std::print("decrire(Chien) = {}\n", decrire(Chien{}));  // Un animal
        std::print("decrire(Aigle) = {}\n", decrire(Aigle{}));  // Un oiseau
    }

    // SmartContainer
    {
        SmartContainer<int> ci;
        ci.push(3); ci.push(1); ci.push(2);
        ci.sort();
        ci.print_all();  // 1 2 3
        std::print("sum = {}\n", ci.sum());  // 6
    }

    // formater
    {
        std::print("{}\n", formater(42));      // 42
        std::print("{}\n", formater(3.14159)); // 3.14
    }
}
