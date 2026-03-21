/* ============================================================================
   Section 24.6.2 : CTRE — Compile-Time Regular Expressions (C++20)
   Description : search, match, captures, search_all — zero-cost a l'execution
   Fichier source : 06.2-ctre.md
   ============================================================================ */

// Compilation avec FetchContent (voir CMakeLists.txt)
// ou : g++-15 -std=c++23 -O2 -I<chemin_ctre>/include -o 06_2_ctre 06_2_ctre.cpp

#include <ctre.hpp>
#include <string_view>
#include <print>
#include <cassert>

int main() {
    // --- search ---
    std::string_view text = "Date: 2026-03-21";
    if (auto m = ctre::search<R"(\d{4}-\d{2}-\d{2})">(text)) {
        std::println("=== ctre::search ===");
        std::println("Trouvé : {}", m.to_view());
    }

    // --- match (correspondance totale) ---
    constexpr auto valid = ctre::match<R"(\d{4}-\d{2}-\d{2})">(
        std::string_view{"2026-03-21"}
    );
    static_assert(valid);  // Vérifiable à la compilation !

    auto no_match = ctre::match<R"(\d{4}-\d{2}-\d{2})">(
        std::string_view{"Date: 2026-03-21"}
    );
    assert(!no_match);
    std::println("\n=== ctre::match ===");
    std::println("'2026-03-21' match complet : true (static_assert)");
    std::println("'Date: 2026-03-21' match complet : false");

    // --- Groupes de capture ---
    std::string_view log_line = "[2026-03-21T14:32:07] [ERROR] Connection timeout";
    if (auto m = ctre::search<R"((\d{4})-(\d{2})-(\d{2}))">(log_line)) {
        std::println("\n=== Groupes de capture ===");
        std::println("Année: {}, Mois: {}, Jour: {}",
            m.get<1>().to_view(), m.get<2>().to_view(), m.get<3>().to_view());
    }

    // --- search_all (itération) ---
    std::string_view errors = "Codes: E001, E042, E107, E999";
    std::println("\n=== ctre::search_all ===");
    for (auto match : ctre::search_all<R"(E(\d{3}))">(errors)) {
        std::println("  {} (num: {})", match.to_view(), match.get<1>().to_view());
    }
}
