/* ============================================================================
   Section 24.6.3 : RE2 et PCRE2 — Alternatives runtime performantes
   Description : RE2 FullMatch, PartialMatch, captures typees, pattern pre-compile
   Fichier source : 06.3-re2-pcre2.md
   ============================================================================ */

// Compilation : g++-15 -std=c++23 -O2 -o 06_3_re2 06_3_re2.cpp -lre2

#include <re2/re2.h>
#include <string>
#include <print>

int main() {
    // --- FullMatch ---
    std::string input = "2026-03-21";
    std::println("=== RE2::FullMatch ===");
    if (RE2::FullMatch(input, R"(\d{4}-\d{2}-\d{2})")) {
        std::println("Format de date valide");
    }

    int year, month, day;
    if (RE2::FullMatch(input, R"((\d{4})-(\d{2})-(\d{2}))", &year, &month, &day)) {
        std::println("Date : {}/{}/{}", day, month, year);
    }

    // --- PartialMatch ---
    std::println("\n=== RE2::PartialMatch ===");
    std::string log = "[ERROR] Connection refused on port 5432";
    std::string level;
    int port;
    if (RE2::PartialMatch(log, R"(\[(\w+)\].*port (\d+))", &level, &port)) {
        std::println("Niveau: {}, Port: {}", level, port);
    }

    // --- Pattern pre-compile ---
    std::println("\n=== Pattern pre-compile ===");
    static const RE2 error_pattern(R"(ERROR|FATAL)");
    if (error_pattern.ok()) {
        std::println("Pattern OK, {} groupes", error_pattern.NumberOfCapturingGroups());
    }
    if (RE2::PartialMatch(log, error_pattern)) {
        std::println("Erreur detectee dans le log");
    }
}
