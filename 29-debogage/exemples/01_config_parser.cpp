/* ============================================================================
   Section 29.1 : GDB — Commandes essentielles et breakpoints
   Description : Programme fil conducteur du chapitre — parseur de config
                 avec bugs subtils à diagnostiquer avec GDB
   Fichier source : 01-gdb-commandes.md
   Compilation : g++-15 -ggdb3 -O0 -std=c++20 -Wall -Wextra -o config_parser 01_config_parser.cpp
   ============================================================================ */

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>

struct ConfigEntry {
    std::string key;
    std::string value;
    int line_number;
};

std::vector<ConfigEntry> parse_config(const std::string& filename) {
    std::vector<ConfigEntry> entries;
    std::ifstream file(filename);
    std::string line;
    int line_num = 0;

    while (std::getline(file, line)) {
        ++line_num;

        if (line.empty() || line[0] == '#') {
            continue;
        }

        auto pos = line.find('=');
        if (pos == std::string::npos) {
            std::cerr << "Ligne " << line_num
                      << " : format invalide (pas de '=')\n";
            continue;
        }

        ConfigEntry entry;
        entry.key = line.substr(0, pos);
        entry.value = line.substr(pos + 1);
        entry.line_number = line_num;
        entries.push_back(entry);
    }

    return entries;
}

std::string find_value(const std::vector<ConfigEntry>& entries,
                       const std::string& key) {
    for (const auto& entry : entries) {
        if (entry.key == key) {
            return entry.value;
        }
    }
    return "";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>\n";
        return 1;
    }

    auto entries = parse_config(argv[1]);
    std::cout << "Entrées parsées : " << entries.size() << "\n";

    for (const auto& e : entries) {
        std::cout << "[L" << e.line_number << "] "
                  << e.key << " = " << e.value << "\n";
    }

    auto db_host = find_value(entries, "db_host");
    if (db_host.empty()) {
        std::cerr << "ERREUR : db_host non trouvé\n";
        return 1;
    }
    std::cout << "Connexion à : " << db_host << "\n";

    return 0;
}
