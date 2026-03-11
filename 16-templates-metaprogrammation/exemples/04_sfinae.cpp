/* ============================================================================
   Section 16.4 : SFINAE (Substitution Failure Is Not An Error)
   Description : Exemples SFINAE : extraire, enable_if (retour et paramètre),
                 NumericOps, decltype/declval, void_t, traits has_serialize/
                 has_value_type/is_printable/is_iterable, alias require_
   Fichier source : 04-sfinae.md
   ============================================================================ */
#include <type_traits>
#include <print>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <utility>

// === Premier exemple SFINAE ===
template <typename T>
typename T::value_type extraire(const T& conteneur) {
    return *conteneur.begin();
}

template <typename T>
std::enable_if_t<!std::is_class_v<T>, T> extraire(T valeur) {
    return valeur;
}

// === enable_if dans le type de retour ===
template <typename T>
std::enable_if_t<std::is_integral_v<T>, T>
doubler_v1(T valeur) {
    return valeur * 2;
}

template <typename T>
std::enable_if_t<std::is_floating_point_v<T>, T>
doubler_v1(T valeur) {
    return valeur * 2.0;
}

// === enable_if comme paramètre template par défaut ===
template <typename T,
          std::enable_if_t<std::is_integral_v<T>, int> = 0>
T doubler_v2(T valeur) {
    return valeur * 2;
}

template <typename T,
          std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
T doubler_v2(T valeur) {
    return valeur * 2.0;
}

// === enable_if dans les templates de classes ===
template <typename T, typename Enable = void>
class NumericOps {
public:
    static void info() {
        std::print("Type non numerique\n");
    }
};

template <typename T>
class NumericOps<T, std::enable_if_t<std::is_arithmetic_v<T>>> {
public:
    static void info() {
        std::print("Type arithmetique (taille : {} octets)\n", sizeof(T));
    }
    static T clamped_add(T a, T b, T max_val) {
        T result = a + b;
        return (result > max_val) ? max_val : result;
    }
};

// === Expressions SFINAE: decltype ===
template <typename T>
auto additionner(T a, T b) -> decltype(a + b) {
    return a + b;
}

// === declval: tester .size() ===
template <typename T>
auto taille(const T& obj) -> decltype(static_cast<std::size_t>(obj.size())) {
    return static_cast<std::size_t>(obj.size());
}

template <typename T,
          std::enable_if_t<!std::is_class_v<T>, int> = 0>
std::size_t taille(const T&) {
    return 0;
}

// === has_serialize trait ===
template <typename T, typename = void>
struct has_serialize : std::false_type {};

template <typename T>
struct has_serialize<T,
    std::void_t<decltype(std::declval<T>().serialize())>>
    : std::true_type {};

// === has_value_type trait ===
template <typename T, typename = void>
struct has_value_type : std::false_type {};

template <typename T>
struct has_value_type<T, std::void_t<typename T::value_type>>
    : std::true_type {};

// === is_printable trait ===
template <typename T, typename = void>
struct is_printable : std::false_type {};

template <typename T>
struct is_printable<T,
    std::void_t<decltype(std::declval<std::ostream&>() << std::declval<T>())>>
    : std::true_type {};

// === is_iterable trait ===
template <typename T, typename = void>
struct is_iterable : std::false_type {};

template <typename T>
struct is_iterable<T, std::void_t<
    decltype(std::declval<T>().begin()),
    decltype(std::declval<T>().end())>>
    : std::true_type {};

template <typename T>
constexpr bool is_iterable_v = is_iterable<T>::value;

template <typename T,
          std::enable_if_t<is_iterable_v<T>, int> = 0>
void afficher_elements(const T& conteneur) {
    for (const auto& elem : conteneur) {
        std::print("{} ", elem);
    }
    std::print("\n");
}

// === require_ aliases ===
template <typename T>
using require_integral = std::enable_if_t<std::is_integral_v<T>, int>;

template <typename T, require_integral<T> = 0>
T safe_divide(T a, T b) {
    if (b == 0) throw std::domain_error("Division par zero");
    return a / b;
}

// === Test types ===
struct SerializableType {
    std::string serialize() const { return "serialized"; }
};
struct Opaque {};

int main() {
    // extraire
    {
        std::vector<int> v{10, 20, 30};
        int n = 42;
        std::print("extraire(vector) = {}\n", extraire(v));  // 10
        std::print("extraire(42) = {}\n", extraire(n));       // 42
    }

    // doubler_v1
    {
        std::print("doubler_v1(21) = {}\n", doubler_v1(21));    // 42
        std::print("doubler_v1(1.5) = {}\n", doubler_v1(1.5));  // 3
    }

    // doubler_v2
    {
        std::print("doubler_v2(21) = {}\n", doubler_v2(21));    // 42
        std::print("doubler_v2(1.5) = {}\n", doubler_v2(1.5));  // 3
    }

    // NumericOps
    {
        NumericOps<int>::info();          // Type arithmetique (taille : 4 octets)
        NumericOps<std::string>::info();  // Type non numerique
    }

    // additionner
    {
        std::print("additionner(3, 4) = {}\n", additionner(3, 4));  // 7
    }

    // taille
    {
        std::vector<int> v{1, 2, 3};
        std::print("taille(vector) = {}\n", taille(v));   // 3
        std::print("taille(42) = {}\n", taille(42));       // 0
    }

    // has_serialize
    {
        static_assert(has_serialize<SerializableType>::value);
        static_assert(!has_serialize<int>::value);
        std::print("has_serialize OK\n");
    }

    // has_value_type
    {
        static_assert(has_value_type<std::vector<int>>::value);
        static_assert(has_value_type<std::map<int,int>>::value);
        static_assert(!has_value_type<int>::value);
        static_assert(!has_value_type<double>::value);
        std::print("has_value_type OK\n");
    }

    // is_printable
    {
        static_assert(is_printable<int>::value);
        static_assert(is_printable<std::string>::value);
        static_assert(!is_printable<Opaque>::value);
        std::print("is_printable OK\n");
    }

    // is_iterable + afficher_elements
    {
        std::vector<int> v{1, 2, 3};
        afficher_elements(v);  // 1 2 3
    }

    // safe_divide
    {
        std::print("safe_divide(10, 3) = {}\n", safe_divide(10, 3));  // 3
    }
}
