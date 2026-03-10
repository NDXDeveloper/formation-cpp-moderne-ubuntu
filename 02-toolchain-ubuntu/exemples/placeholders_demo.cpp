/* ============================================================================
   Section 2.7 / 2.7.1 : std::print — Syntaxe des placeholders
   Description : Demonstration des differents formats de placeholders
   Fichier source : 07-std-print.md / 07.1-syntaxe-comparaison.md
   Compilation : g++ -std=c++23 -Wall -Wextra placeholders_demo.cpp -o placeholders_demo
   ============================================================================ */
#include <print>
#include <string>

int main() {
    std::string nom = "Alice";

    // Remplacement simple
    std::println("--- Remplacement simple ---");
    std::println("Nom : {}", nom);
    std::println("Entier : {}", 42);
    std::println("Flottant : {}", 3.14);
    std::println("Booléen : {}", true);

    // Largeur et alignement
    std::println("\n--- Largeur et alignement ---");
    std::println("[{:>10}]", "droite");
    std::println("[{:<10}]", "gauche");
    std::println("[{:^10}]", "centre");

    // Formatage numérique
    std::println("\n--- Formatage numérique ---");
    std::println("{:.2f}", 3.14159);
    std::println("{:08d}", 42);
    std::println("{:#b}", 42);
    std::println("{:#x}", 255);
    std::println("{:+d}", 42);

    // Arguments positionnels
    std::println("\n--- Arguments positionnels ---");
    std::println("{1} précède {0}", "B", "A");

    // Bases numériques
    std::println("\n--- Bases numériques ---");
    int val = 255;
    std::println("Décimal     : {:d}", val);
    std::println("Hexadécimal : {:x}", val);
    std::println("Hex (préfixe): {:#x}", val);
    std::println("Hex (majusc.): {:#X}", val);
    std::println("Octal       : {:#o}", val);
    std::println("Binaire     : {:b}", val);
    std::println("Bin (préfixe): {:#b}", val);

    // Caractère de remplissage
    std::println("\n--- Caractère de remplissage ---");
    std::println("ID : {:08d}", 42);
    std::println("Tirets : {:->20}", "FIN");
    std::println("Points : {:.>20}", "FIN");
    std::println("Centré : {:*^20}", "TITRE");

    // Arguments positionnels et répétition
    std::println("\n--- Répétition d'arguments ---");
    std::println("{0} a dit : « Bonjour, je suis {0} »", nom);
    std::println("Coordonnées : ({1}, {0})", 10, 20);

    // Booléens
    std::println("\n--- Booléens ---");
    std::println("Actif : {}", true);
    std::println("Actif (num) : {:d}", true);

    // Accolades littérales
    std::println("\n--- Accolades littérales ---");
    std::println("Un placeholder : {{}}");
    std::println("JSON : {{\"clé\": {}}}", 42);

    return 0;
}
