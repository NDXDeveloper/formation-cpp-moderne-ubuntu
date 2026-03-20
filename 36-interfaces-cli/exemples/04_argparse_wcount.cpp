/* ============================================================================
   Section 36.2 : argparse — Alternative légère
   Description : Outil wcount (word count) — arguments positionnels,
                 flags booléens, style Python
   Fichier source : 02-argparse.md
   ============================================================================ */

#include <argparse/argparse.hpp>
#include <print>
#include <fstream>
#include <string>

int main(int argc, char* argv[]) {
    argparse::ArgumentParser program("wcount", "1.0.0");
    program.add_description("Compter les lignes, mots ou caractères d'un fichier");

    program.add_argument("file").help("Fichier à analyser");

    program.add_argument("-l", "--lines")
        .help("Compter les lignes").default_value(false).implicit_value(true);
    program.add_argument("-w", "--words")
        .help("Compter les mots").default_value(false).implicit_value(true);
    program.add_argument("-c", "--chars")
        .help("Compter les caractères").default_value(false).implicit_value(true);

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& e) {
        std::println(stderr, "Erreur : {}", e.what());
        std::println(stderr, "{}", program.help().str());
        return 1;
    }

    auto filepath = program.get<std::string>("file");
    bool count_lines = program.get<bool>("--lines");
    bool count_words = program.get<bool>("--words");
    bool count_chars = program.get<bool>("--chars");

    if (!count_lines && !count_words && !count_chars)
        count_lines = count_words = count_chars = true;

    std::ifstream file(filepath);
    if (!file) {
        std::println(stderr, "Erreur : impossible d'ouvrir '{}'", filepath);
        return 1;
    }

    int lines = 0, words = 0, chars = 0;
    std::string line;
    while (std::getline(file, line)) {
        ++lines;
        chars += static_cast<int>(line.size()) + 1;
        bool in_word = false;
        for (char c : line) {
            if (std::isspace(c)) { in_word = false; }
            else if (!in_word) { ++words; in_word = true; }
        }
    }

    if (count_lines) std::print("{:>8}", lines);
    if (count_words) std::print("{:>8}", words);
    if (count_chars) std::print("{:>8}", chars);
    std::println("  {}", filepath);
    return 0;
}
