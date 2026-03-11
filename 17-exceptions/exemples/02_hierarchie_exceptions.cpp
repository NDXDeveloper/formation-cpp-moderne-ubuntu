/* ============================================================================
   Section 17.2 : Hiérarchie des exceptions standard (std::exception)
   Description : Démonstration des familles d'exceptions standard —
                 logic_error, runtime_error, exceptions autonomes,
                 capture à différents niveaux de granularité
   Fichier source : 02-hierarchie-exceptions.md
   ============================================================================ */

#include <stdexcept>
#include <print>
#include <string>
#include <vector>
#include <bitset>
#include <functional>
#include <optional>
#include <variant>
#include <new>
#include <typeinfo>
#include <cmath>
#include <future>
#include <filesystem>

// === Famille 1 : logic_error ===

int parse_port(const std::string& str) {
    int port = std::stoi(str);
    if (port < 0 || port > 65535) {
        throw std::invalid_argument("Port hors limites [0, 65535] : " + str);
    }
    return port;
}

double logarithme(double x) {
    if (x <= 0.0) {
        throw std::domain_error("logarithme() : argument non positif");
    }
    return std::log(x);
}

void demo_logic_errors() {
    std::print("--- std::invalid_argument ---\n");
    try {
        parse_port("99999");
    } catch (const std::invalid_argument& e) {
        std::print("  invalid_argument : {}\n", e.what());
    }

    try {
        parse_port("abc");
    } catch (const std::invalid_argument& e) {
        std::print("  invalid_argument (stoi) : {}\n", e.what());
    }

    std::print("--- std::domain_error ---\n");
    try {
        logarithme(-5.0);
    } catch (const std::domain_error& e) {
        std::print("  domain_error : {}\n", e.what());
    }

    std::print("--- std::out_of_range ---\n");
    try {
        std::vector<int> v{10, 20, 30};
        int val = v.at(10);
        (void)val;
    } catch (const std::out_of_range& e) {
        std::print("  out_of_range : {}\n", e.what());
    }

    std::print("--- std::length_error ---\n");
    try {
        std::string s;
        s.resize(s.max_size() + 1);
    } catch (const std::length_error& e) {
        std::print("  length_error : {}\n", e.what());
    }

    std::print("--- std::future_error ---\n");
    try {
        std::promise<int> p;
        auto f = p.get_future();
        p.set_value(42);
        [[maybe_unused]] int r1 = f.get();
        f.get();  // deuxième extraction → future_error
    } catch (const std::future_error& e) {
        std::print("  future_error : {} (code : {})\n", e.what(), e.code().value());
    }
}

// === Famille 2 : runtime_error ===

void demo_runtime_errors() {
    std::print("--- std::overflow_error ---\n");
    try {
        std::bitset<128> bits;
        bits.set();  // tous les bits à 1
        [[maybe_unused]] unsigned long val = bits.to_ulong();
    } catch (const std::overflow_error& e) {
        std::print("  overflow_error : {}\n", e.what());
    }

    std::print("--- std::system_error ---\n");
    try {
        throw std::system_error(
            std::make_error_code(std::errc::no_such_file_or_directory),
            "Échec d'ouverture de /chemin/inexistant"
        );
    } catch (const std::system_error& e) {
        std::print("  system_error : {} (code : {}, catégorie : {})\n",
                   e.what(), e.code().value(), e.code().category().name());
    }

    std::print("--- std::filesystem::filesystem_error ---\n");
    try {
        std::filesystem::copy("/fichier/inexistant", "/dest/inexistant");
    } catch (const std::filesystem::filesystem_error& e) {
        std::print("  filesystem_error : {}\n", e.what());
    }
}

// === Famille 3 : exceptions autonomes ===

void demo_autonomous_errors() {
    std::print("--- std::bad_alloc ---\n");
    try {
        auto* p = new double[1'000'000'000'000ULL];
        delete[] p;
    } catch (const std::bad_alloc& e) {
        std::print("  bad_alloc : {}\n", e.what());
    }

    std::print("--- std::bad_cast ---\n");
    struct Base { virtual ~Base() = default; };
    struct Derivee : Base {};
    struct Autre : Base {};
    try {
        Autre a;
        Base& b = a;
        [[maybe_unused]] Derivee& d = dynamic_cast<Derivee&>(b);
    } catch (const std::bad_cast& e) {
        std::print("  bad_cast : {}\n", e.what());
    }

    std::print("--- std::bad_function_call ---\n");
    try {
        std::function<void()> fn;
        fn();
    } catch (const std::bad_function_call& e) {
        std::print("  bad_function_call : {}\n", e.what());
    }

    std::print("--- std::bad_optional_access ---\n");
    try {
        std::optional<int> vide;
        [[maybe_unused]] int v = vide.value();
    } catch (const std::bad_optional_access& e) {
        std::print("  bad_optional_access : {}\n", e.what());
    }

    std::print("--- std::bad_variant_access ---\n");
    try {
        std::variant<int, std::string> var = 42;
        [[maybe_unused]] auto s = std::get<std::string>(var);
    } catch (const std::bad_variant_access& e) {
        std::print("  bad_variant_access : {}\n", e.what());
    }
}

// === Capture à différents niveaux ===

void demo_capture_granulaire() {
    auto tester = [](auto lanceur, const char* desc) {
        try {
            lanceur();
        }
        catch (const std::filesystem::filesystem_error& e) {
            std::print("  [{}] filesystem_error : {}\n", desc, e.what());
        }
        catch (const std::system_error& e) {
            std::print("  [{}] system_error : {}\n", desc, e.what());
        }
        catch (const std::runtime_error& e) {
            std::print("  [{}] runtime_error : {}\n", desc, e.what());
        }
        catch (const std::logic_error& e) {
            std::print("  [{}] logic_error : {}\n", desc, e.what());
        }
        catch (const std::exception& e) {
            std::print("  [{}] exception : {}\n", desc, e.what());
        }
    };

    tester([] { throw std::invalid_argument("arg invalide"); }, "logic");
    tester([] { throw std::runtime_error("erreur runtime"); }, "runtime");
    tester([] {
        throw std::system_error(
            std::make_error_code(std::errc::permission_denied), "refusé");
    }, "system");
}

int main() {
    std::print("=== 1. Famille logic_error ===\n");
    demo_logic_errors();

    std::print("\n=== 2. Famille runtime_error ===\n");
    demo_runtime_errors();

    std::print("\n=== 3. Exceptions autonomes ===\n");
    demo_autonomous_errors();

    std::print("\n=== 4. Capture à différents niveaux ===\n");
    demo_capture_granulaire();

    return 0;
}
