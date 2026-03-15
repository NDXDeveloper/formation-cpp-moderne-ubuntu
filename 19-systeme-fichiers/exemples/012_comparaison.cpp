/* ============================================================================
   Section 19.1.2 : Comparaison de chemins
   Description : Comparaison syntaxique et pièges (trailing slash, dotfiles)
   Fichier source : 01.2-manipulation-chemins.md
   ============================================================================ */
#include <filesystem>
#include <print>

namespace fs = std::filesystem;

int main() {
    // Comparaison syntaxique
    fs::path a = "/home/user/projet";
    fs::path b = "/home/user/projet";
    fs::path c = "/home/user/projet/";  // Trailing slash
    fs::path d = "/home/user/./projet"; // Composante . superflue

    std::println("a == b : {}", a == b);   // true
    std::println("a == c : {}", a == c);   // false
    std::println("a == d : {}", a == d);   // false

    // Dotfiles : stem() et extension()
    fs::path dotfile = "/home/user/.bashrc";
    std::println("\nfilename()  : {}", dotfile.filename().string());   // .bashrc
    std::println("stem()      : {}", dotfile.stem().string());       // .bashrc
    std::println("extension() : '{}'", dotfile.extension().string()); // "" (vide !)

    fs::path hidden = "/home/user/.app.log";
    std::println("stem()      : {}", hidden.stem().string());        // .app
    std::println("extension() : {}", hidden.extension().string());   // .log

    // Trailing slash piège
    fs::path e = "/home/user/projet";
    fs::path f = "/home/user/projet/";
    std::println("\na.filename() : '{}'", e.filename().string());   // projet
    std::println("b.filename() : '{}'", f.filename().string());   // "" (vide !)
}
