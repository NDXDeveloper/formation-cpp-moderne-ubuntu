/* ============================================================================
   Section 8.4 : Opérateur d'appel de fonction (operator())
   Description : Surcharges multiples de operator() — Formateur par type
                 et visiteur pour std::variant avec std::visit
   Fichier source : 04-operateur-appel.md
   ============================================================================ */
#include <string>
#include <format>
#include <print>
#include <variant>

class Formateur {
public:
    std::string operator()(int valeur) const {
        return std::format("{}", valeur);
    }

    std::string operator()(double valeur) const {
        return std::format("{:.2f}", valeur);
    }

    std::string operator()(std::string const& valeur) const {
        return std::format("\"{}\"", valeur);
    }
};

struct Afficheur {
    void operator()(int i) const    { std::println("int: {}", i); }
    void operator()(double d) const { std::println("double: {:.2f}", d); }
    void operator()(std::string const& s) const { std::println("string: {}", s); }
};

int main() {
    Formateur fmt;
    std::println("{}", fmt(42));         // 42
    std::println("{}", fmt(3.14159));    // 3.14
    std::println("{}", fmt("hello"));   // "hello"

    std::variant<int, double, std::string> v = 3.14;
    std::visit(Afficheur{}, v);   // double: 3.14

    v = 42;
    std::visit(Afficheur{}, v);   // int: 42

    v = std::string{"world"};
    std::visit(Afficheur{}, v);   // string: world
}
