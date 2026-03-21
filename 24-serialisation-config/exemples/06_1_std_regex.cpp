/* ============================================================================
   Section 24.6.1 : std::regex — API standard et limites de performance
   Description : regex_search, regex_match, regex_replace, sregex_iterator
   Fichier source : 06.1-std-regex.md
   ============================================================================ */

#include <regex>
#include <string>
#include <print>

int main() {
    // --- regex_search avec groupes de capture ---
    std::string text = "Date: 2026-03-21, Revision: 42";
    std::regex pattern(R"((\d{4})-(\d{2})-(\d{2}))");
    std::smatch match;

    if (std::regex_search(text, match, pattern)) {
        std::println("=== regex_search ===");
        std::println("Correspondance : {}", match[0].str());
        std::println("Année  : {}", match[1].str());
        std::println("Mois   : {}", match[2].str());
        std::println("Jour   : {}", match[3].str());
        std::println("Préfixe : '{}'", match.prefix().str());
        std::println("Suffixe : '{}'", match.suffix().str());
    }

    // --- regex_match (correspondance totale) ---
    std::regex ipv4_pattern(R"((\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3}))");
    std::string ip = "192.168.1.1";
    std::smatch ip_match;
    if (std::regex_match(ip, ip_match, ipv4_pattern)) {
        std::println("\n=== regex_match ===");
        std::println("IPv4 valide: {}.{}.{}.{}",
            ip_match[1].str(), ip_match[2].str(),
            ip_match[3].str(), ip_match[4].str());
    }

    // --- regex_replace ---
    std::string input = "nom: Dupont, prenom: Jean, age: 42";
    std::regex field_pattern(R"((\w+): (\w+))");
    std::string result = std::regex_replace(input, field_pattern, "$2 ($1)");
    std::println("\n=== regex_replace ===");
    std::println("{}", result);

    // --- sregex_iterator ---
    std::string errors = "Erreurs: E001, E042, E107, E999";
    std::regex error_code(R"(E(\d{3}))");
    auto begin = std::sregex_iterator(errors.begin(), errors.end(), error_code);
    auto end = std::sregex_iterator();

    std::println("\n=== sregex_iterator ===");
    for (auto it = begin; it != end; ++it) {
        std::println("  {} (num: {})", (*it)[0].str(), (*it)[1].str());
    }

    // --- regex_error ---
    std::println("\n=== regex_error ===");
    try {
        std::regex bad_pattern(R"([invalid)");
    } catch (const std::regex_error& e) {
        std::println("Erreur regex : {}", e.what());
    }
}
