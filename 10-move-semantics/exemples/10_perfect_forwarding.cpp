/* ============================================================================
   Section 10.4 : Perfect Forwarding avec std::forward
   Description : Perte de catégorie de valeur, forwarding references,
                 std::forward, mon_make_unique, emplace_back, wrapper avec
                 logging, wrapper chrono, lambdas C++20 avec forwarding,
                 capture par forwarding, piège multi-forward
   Fichier source : 04-perfect-forwarding.md
   ============================================================================ */
#include <print>
#include <string>
#include <memory>
#include <vector>
#include <utility>
#include <functional>
#include <chrono>

// === Widget avec surcharge copie/move (lignes 12-20) ===
class Widget {
public:
    std::string nom_;
    Widget(const std::string& nom) : nom_(nom) {
        std::print("[copie] Widget créé avec '{}'\n", nom);
    }
    Widget(std::string&& nom) : nom_(std::move(nom)) {
        std::print("[move] Widget créé avec '{}'\n", nom_);
    }
};

// === Tentative naïve n°1 et n°2 (lignes 23-32) ===
template <typename T>
Widget creer_v1(const T& arg) {
    return Widget(arg);  // TOUJOURS copie
}

template <typename T>
Widget creer_v2(T&& arg) {
    return Widget(arg);  // arg a un nom → TOUJOURS copie
}

// === Perfect forwarding (lignes 129-132) ===
template <typename T>
Widget creer(T&& arg) {
    return Widget(std::forward<T>(arg));
}

void test_forwarding() {
    std::print("=== Perfect forwarding ===\n");
    std::string nom = "Alice";

    std::print("--- v1 (const T&) ---\n");
    creer_v1(nom);
    creer_v1(std::string("Bob"));

    std::print("--- v2 (T&& sans forward) ---\n");
    creer_v2(nom);
    creer_v2(std::string("Bob"));

    std::print("--- creer (avec forward) ---\n");
    std::string nom2 = "Charlie";
    creer(nom2);                    // lvalue → copie
    creer(std::string("David"));    // rvalue → move
    creer(std::move(nom2));         // rvalue → move
}

// === mon_make_unique (lignes 216-221) ===
template <typename T, typename... Args>
std::unique_ptr<T> mon_make_unique(Args&&... args) {
    return std::unique_ptr<T>(
        new T(std::forward<Args>(args)...)
    );
}

struct Config {
    std::string nom;
    int version;
    Config(std::string n, int v) : nom(std::move(n)), version(v) {
        std::print("Config('{}', {})\n", nom, version);
    }
};

void test_make_unique() {
    std::print("\n=== mon_make_unique ===\n");
    std::string nom = "prod";

    auto c1 = mon_make_unique<Config>(nom, 42);
    std::print("c1: nom='{}', version={}\n", c1->nom, c1->version);

    auto c2 = mon_make_unique<Config>(std::move(nom), 43);
    std::print("c2: nom='{}', version={}\n", c2->nom, c2->version);
    std::print("nom après move: '{}'\n", nom);
}

// === emplace_back (lignes 253-263) ===
void test_emplace() {
    std::print("\n=== emplace_back ===\n");
    std::vector<std::pair<std::string, int>> vec;

    std::string cle = "alpha";

    vec.push_back(std::make_pair(cle, 42));
    vec.emplace_back(std::move(cle), 42);

    std::print("vec.size() = {}\n", vec.size());
    for (const auto& [k, v] : vec) {
        std::print("  '{}' → {}\n", k, v);
    }
    std::print("cle après move: '{}'\n", cle);
}

// === Wrapper avec logging (lignes 287-303) ===
Widget creer_widget(std::string nom) {
    return Widget(std::move(nom));
}

template <typename Func, typename... Args>
decltype(auto) avec_log(const std::string& label, Func&& func, Args&&... args) {
    std::print("[LOG] Début de '{}'\n", label);

    decltype(auto) resultat = std::invoke(
        std::forward<Func>(func),
        std::forward<Args>(args)...
    );

    std::print("[LOG] Fin de '{}'\n", label);
    return resultat;
}

void test_wrapper_log() {
    std::print("\n=== Wrapper avec logging ===\n");
    std::string nom = "Alice";
    auto widget = avec_log("création", creer_widget, std::move(nom));
    std::print("widget.nom_ = '{}'\n", widget.nom_);
}

// === Wrapper avec mesure de temps (lignes 315-329) ===
template <typename Func, typename... Args>
decltype(auto) chrono_mesure(Func&& func, Args&&... args) {
    auto debut = std::chrono::high_resolution_clock::now();

    decltype(auto) resultat = std::invoke(
        std::forward<Func>(func),
        std::forward<Args>(args)...
    );

    auto fin = std::chrono::high_resolution_clock::now();
    auto duree = std::chrono::duration_cast<std::chrono::microseconds>(fin - debut);
    std::print("[PERF] Durée : {} µs\n", duree.count());

    return resultat;
}

void test_chrono() {
    std::print("\n=== Wrapper chrono ===\n");
    auto widget = chrono_mesure(creer_widget, std::string("Chrono"));
    std::print("widget.nom_ = '{}'\n", widget.nom_);
}

// === Lambdas C++20 (lignes 338-347) ===
void destination(std::string s) {
    std::print("destination: '{}'\n", s);
}

void test_lambda_forwarding() {
    std::print("\n=== Lambda forwarding C++20 ===\n");

    // C++14 style
    auto wrapper_14 = [](auto&&... args) {
        return destination(std::forward<decltype(args)>(args)...);
    };

    // C++20 style
    auto wrapper_20 = []<typename... Args>(Args&&... args) {
        return destination(std::forward<Args>(args)...);
    };

    wrapper_14(std::string("C++14"));
    wrapper_20(std::string("C++20"));
}

// === Capture par forwarding C++20 (lignes 357-369) ===
template <typename... Args>
auto creer_differe(Args&&... args) {
    return [...args = std::forward<Args>(args)]() mutable {
        return Widget(std::move(args)...);
    };
}

void test_capture_forward() {
    std::print("\n=== Capture par forwarding ===\n");
    std::string nom = "Alice";
    auto factory = creer_differe(std::move(nom));
    std::print("nom après capture: '{}'\n", nom);

    auto widget = factory();
    std::print("widget.nom_ = '{}'\n", widget.nom_);
}

// === Piège n°3 : forwarder plusieurs fois (lignes 413-418) ===
void g_func(const std::string& s) { std::print("g: '{}'\n", s); }
void h_func(std::string s) { std::print("h: '{}'\n", s); }

template <typename T>
void f_safe(T&& arg) {
    g_func(arg);                        // Première utilisation — lvalue
    h_func(std::forward<T>(arg));       // Dernière utilisation — forward
}

void test_piege_multi_forward() {
    std::print("\n=== Piège : multi forward ===\n");
    f_safe(std::string("test multi forward"));
}

int main() {
    test_forwarding();
    test_make_unique();
    test_emplace();
    test_wrapper_log();
    test_chrono();
    test_lambda_forwarding();
    test_capture_forward();
    test_piege_multi_forward();
    std::print("\n✅ Tous les exemples passés\n");
}
