/* ============================================================================
   Section 36.3 : fmt — Formatage avancé
   Description : Démonstration des couleurs et styles terminaux avec {fmt}
   Fichier source : 03-fmt.md, 03.2-couleurs-styles.md
   ============================================================================ */

#include <fmt/core.h>
#include <fmt/color.h>

int main() {
    fmt::print("Hello, {}!\n", "world");
    fmt::print("Pi = {:.4f}\n", 3.14159);

    fmt::print(fg(fmt::color::green), "SUCCESS");
    fmt::print(": opération terminée\n");

    fmt::print(fg(fmt::color::red) | fmt::emphasis::bold, "ERROR");
    fmt::print(": fichier non trouvé\n");

    fmt::print(fg(fmt::color::yellow), "WARNING");
    fmt::print(": configuration par défaut utilisée\n");

    std::string msg = fmt::format("Traité {} éléments en {:.1f}ms", 1000, 42.5);
    fmt::print("{}\n", msg);

    return 0;
}
