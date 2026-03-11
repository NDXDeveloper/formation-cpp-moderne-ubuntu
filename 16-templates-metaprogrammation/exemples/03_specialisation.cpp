/* ============================================================================
   Section 16.3 : Spécialisation partielle et totale
   Description : Serializer avec spécialisations totales (bool, const char*) et
                 partielles (T*, pair, vector), convertir, traiter, FixedBuffer,
                 traits is_pointer/remove_const, Formatter membre spécialisé
   Fichier source : 03-specialisation.md
   ============================================================================ */
#include <string>
#include <format>
#include <print>
#include <vector>
#include <bitset>
#include <type_traits>
#include <utility>

// === Serializer: template principal + spécialisations ===
template <typename T>
class Serializer {
public:
    static std::string to_string(const T& valeur) {
        return std::format("{}", valeur);
    }
};

template <>
class Serializer<bool> {
public:
    static std::string to_string(bool valeur) {
        return valeur ? "true" : "false";
    }
};

template <>
class Serializer<const char*> {
public:
    static std::string to_string(const char* valeur) {
        if (valeur == nullptr) return "null";
        return std::format("\"{}\"", valeur);
    }
};

template <typename T>
class Serializer<T*> {
public:
    static std::string to_string(T* ptr) {
        if (ptr == nullptr) return "null";
        return std::format("*{}", *ptr);
    }
};

template <typename K, typename V>
class Serializer<std::pair<K, V>> {
public:
    static std::string to_string(const std::pair<K, V>& p) {
        return std::format("({}, {})",
            Serializer<K>::to_string(p.first),
            Serializer<V>::to_string(p.second));
    }
};

template <typename T>
class Serializer<std::vector<T>> {
public:
    static std::string to_string(const std::vector<T>& vec) {
        std::string result = "[";
        for (std::size_t i = 0; i < vec.size(); ++i) {
            if (i > 0) result += ", ";
            result += Serializer<T>::to_string(vec[i]);
        }
        return result + "]";
    }
};

// === convertir (spécialisation de fonction) ===
template <typename T>
std::string convertir(const T& valeur) {
    return std::format("{}", valeur);
}

template <>
std::string convertir<bool>(const bool& valeur) {
    return valeur ? "vrai" : "faux";
}

// === traiter : surcharge vs spécialisation ===
template <typename T>
void traiter(T val) {
    std::print("template general: {}\n", val);
}

template <typename T>
void traiter(T* val) {
    std::print("template pointeur: {}\n", *val);
}

template <>
void traiter<int*>(int* val) {
    std::print("specialisation int*: {}\n", *val);
}

// === FixedBuffer avec spécialisations ===
template <typename T, std::size_t N>
class FixedBuffer {
public:
    void info() const { std::print("FixedBuffer<T, {}> generique\n", N); }
private:
    T data_[N];
};

template <typename T>
class FixedBuffer<T, 1> {
public:
    void info() const { std::print("FixedBuffer<T, 1> optimise (element unique)\n"); }
    void set(const T& val) { element_ = val; }
    const T& get() const { return element_; }
private:
    T element_;
};

template <std::size_t N>
class FixedBuffer<bool, N> {
public:
    void info() const { std::print("FixedBuffer<bool, {}> compact (bitset)\n", N); }
private:
    std::bitset<N> bits_;
};

template <>
class FixedBuffer<bool, 1> {
public:
    void info() const { std::print("FixedBuffer<bool, 1> cas special\n"); }
private:
    bool flag_;
};

// === Traits is_pointer et remove_const ===
template <typename T>
struct my_is_pointer { static constexpr bool value = false; };

template <typename T>
struct my_is_pointer<T*> { static constexpr bool value = true; };

template <typename T>
constexpr bool my_is_pointer_v = my_is_pointer<T>::value;

template <typename T>
struct my_remove_const { using type = T; };

template <typename T>
struct my_remove_const<const T> { using type = T; };

template <typename T>
using my_remove_const_t = typename my_remove_const<T>::type;

// === Formatter avec spécialisation de membre ===
template <typename T>
class Formatter {
public:
    void format(const T& valeur) const { std::print("[generic] {}\n", valeur); }
    void banner() const { std::print("=== Formatter ===\n"); }
};

template <>
void Formatter<bool>::format(const bool& valeur) const {
    std::print("[bool] {}\n", valeur ? "OUI" : "NON");
}

int main() {
    // Serializer
    {
        std::print("{}\n", Serializer<int>::to_string(42));
        std::print("{}\n", Serializer<bool>::to_string(true));
        std::print("{}\n", Serializer<const char*>::to_string("hi"));
        std::print("{}\n", Serializer<const char*>::to_string(nullptr));

        int x = 42; double y = 3.14;
        std::print("{}\n", Serializer<int*>::to_string(&x));
        std::print("{}\n", Serializer<double*>::to_string(&y));
        std::print("{}\n", Serializer<double*>::to_string(nullptr));

        std::pair<std::string, int> p{"Alice", 30};
        std::print("{}\n", Serializer<decltype(p)>::to_string(p));

        std::vector<int> v{1, 2, 3};
        std::print("{}\n", Serializer<decltype(v)>::to_string(v));
    }

    // convertir
    {
        std::print("{}\n", convertir(42));    // 42
        std::print("{}\n", convertir(true));  // vrai
    }

    // traiter
    {
        int x = 42;
        traiter(&x);  // template pointeur (PAS specialisation int*)
    }

    // FixedBuffer
    {
        FixedBuffer<int, 100> a; a.info();
        FixedBuffer<double, 1> b; b.info();
        FixedBuffer<bool, 64> c; c.info();
        FixedBuffer<bool, 1> d; d.info();
    }

    // Traits
    {
        static_assert(!my_is_pointer_v<int>);
        static_assert(my_is_pointer_v<int*>);
        static_assert(my_is_pointer_v<double*>);
        static_assert(!my_is_pointer_v<int&>);
        std::print("is_pointer traits OK\n");

        static_assert(std::is_same_v<my_remove_const_t<const int>, int>);
        static_assert(std::is_same_v<my_remove_const_t<int>, int>);
        static_assert(std::is_same_v<my_remove_const_t<const double*>, const double*>);
        std::print("remove_const traits OK\n");
    }

    // Formatter
    {
        Formatter<int> fi;
        fi.banner();     // === Formatter ===
        fi.format(42);   // [generic] 42

        Formatter<bool> fb;
        fb.banner();     // === Formatter ===
        fb.format(true); // [bool] OUI
    }
}
