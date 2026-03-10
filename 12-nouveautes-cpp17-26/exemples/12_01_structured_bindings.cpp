/* ============================================================================
   Section 12.1 : Structured Bindings (C++17)
   Description : Exemples complets de liaisons structurees - paires, tuples,
                 structures, tableaux, qualificateurs, cas d'usage courants
                 et decomposition personnalisee
   Fichier source : 01-structured-bindings.md
   ============================================================================ */
#include <map>
#include <string>
#include <tuple>
#include <print>
#include <vector>
#include <ranges>

using namespace std::string_literals;

// === Section "Le probleme" (lignes 13-24, 39-49) ===
// Avant C++17 - illustratif seulement, teste via la version moderne

// === Section "Paires et tuples" (lignes 77-104) ===
std::tuple<std::string, int, double> get_student_info() {
    return {"Alice", 95, 17.5};
}

// === Section "Structures" (lignes 113-127) ===
struct Point {
    double x;
    double y;
    double z;
};

// === Section "Qualificateurs - reference mutable" (lignes 179-187) ===
struct Config {
    std::string host;
    int port;
};

// === Section "Resultats de fonctions" (lignes 207-221) ===
std::tuple<bool, std::string, int> parse_config(const std::string& /*path*/) {
    return {true, "localhost", 8080};
}

// === Section "Decomposition personnalisee" (lignes 329-355) ===
class Person {
    std::string name_;
    int age_;
public:
    Person(std::string n, int a) : name_(std::move(n)), age_(a) {}

    template <std::size_t I>
    auto get() const {
        if constexpr (I == 0) return name_;
        else if constexpr (I == 1) return age_;
    }
};

namespace std {
    template <> struct tuple_size<Person> : integral_constant<size_t, 2> {};
    template <> struct tuple_element<0, Person> { using type = std::string; };
    template <> struct tuple_element<1, Person> { using type = int; };
}

int main() {
    // --- Paires et map (lignes 77-86) ---
    std::print("=== Map iteration ===\n");
    std::map<std::string, int> scores = {{"Alice", 95}, {"Bob", 87}};
    for (const auto& [name, score] : scores) {
        std::print("{} a obtenu {}/100\n", name, score);
    }

    // --- Tuple (lignes 92-104) ---
    std::print("\n=== Tuple decomposition ===\n");
    auto [name, score, average] = get_student_info();
    std::print("{} — score: {}, moyenne: {}\n", name, score, average);

    // --- Struct (lignes 113-127) ---
    std::print("\n=== Struct decomposition ===\n");
    Point origin{0.0, 0.0, 0.0};
    auto [x, y, z] = origin;
    std::print("x={}, y={}, z={}\n", x, y, z);

    auto [latitude, longitude, altitude] = origin;
    std::print("lat={}, lon={}, alt={}\n", latitude, longitude, altitude);

    // --- Tableaux C-style (lignes 137-139) ---
    std::print("\n=== C-style array ===\n");
    int rgb[3] = {255, 128, 0};
    auto [r, g, b] = rgb;
    std::print("r={}, g={}, b={}\n", r, g, b);

    // --- Reference mutable (lignes 179-187) ---
    std::print("\n=== Mutable reference ===\n");
    Config cfg{"localhost", 8080};
    auto& [host, port] = cfg;
    port = 9090;
    std::print("cfg.port = {}\n", cfg.port);

    // --- parse_config (lignes 207-221) ---
    std::print("\n=== parse_config ===\n");
    auto [success, cfg_host, cfg_port] = parse_config("/etc/app/config.yaml");
    if (success) {
        std::print("Connexion a {}:{}\n", cfg_host, cfg_port);
    }

    // --- insert / try_emplace (lignes 242-254) ---
    std::print("\n=== insert / try_emplace ===\n");
    std::map<std::string, int> my_map;
    auto [it, inserted] = my_map.insert({"key", 42});
    if (inserted) {
        std::print("Insere : {}\n", it->second);
    }

    auto [it2, inserted2] = my_map.try_emplace("key", 42);
    if (!inserted2) {
        std::print("La cle existait deja, valeur actuelle : {}\n", it2->second);
    }

    // --- if with initializer (lignes 262-264) ---
    std::print("\n=== if with initializer ===\n");
    std::map<std::string, int> cache;
    std::string key = "test";
    if (auto [cit, cinserted] = cache.try_emplace(key, 99); !cinserted) {
        std::print("Cache hit pour '{}'\n", key);
    } else {
        std::print("Cache miss, insere {}={}\n", key, cit->second);
    }

    // --- enumerate (C++23) (lignes 274-287) ---
    std::print("\n=== enumerate ===\n");
    std::vector<std::string> names = {"Alice", "Bob", "Clara"};
    for (auto [index, ename] : std::views::enumerate(names)) {
        std::print("[{}] {}\n", index, ename);
    }

    // --- Decomposition personnalisee (lignes 329-355) ---
    std::print("\n=== Custom decomposition ===\n");
    Person alice("Alice", 30);
    auto [pname, page] = alice;
    std::print("name={}, age={}\n", pname, page);
}
