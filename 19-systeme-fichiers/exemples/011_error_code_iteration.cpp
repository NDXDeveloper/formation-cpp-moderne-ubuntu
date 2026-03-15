/* ============================================================================
   Section 19.1.1 : directory_options et itération avec error_code
   Description : Parcours tolérant aux erreurs avec increment(ec)
   Fichier source : 01.1-parcours-repertoires.md
   ============================================================================ */
#include <filesystem>
#include <print>
#include <system_error>

namespace fs = std::filesystem;

int main() {
    std::error_code ec;
    auto it = fs::recursive_directory_iterator("/var", ec);

    if (ec) {
        std::println("Erreur à l'ouverture : {}", ec.message());
        return 1;
    }

    int count = 0;
    int errors = 0;
    for (; it != fs::recursive_directory_iterator(); it.increment(ec)) {
        if (ec) {
            ++errors;
            ec.clear();
            continue;
        }
        ++count;
    }

    std::println("Parcouru {} entrées dans /var ({} erreurs ignorées)", count, errors);
}
