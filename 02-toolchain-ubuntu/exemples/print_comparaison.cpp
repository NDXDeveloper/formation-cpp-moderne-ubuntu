/* ============================================================================
   Section 2.7 / 2.7.1 : std::print — Comparaison printf vs cout vs print
   Description : Meme affichage avec les trois approches (printf, cout, println)
   Fichier source : 07-std-print.md / 07.1-syntaxe-comparaison.md
   Compilation : g++ -std=c++23 -Wall -Wextra print_comparaison.cpp -o print_comparaison
   ============================================================================ */
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <print>
#include <string>

int main() {
    std::string nom = "Ubuntu";
    int version = 24;
    double pi = 3.14159265358979;

    // --- printf ---
    std::puts("=== printf ===");
    printf("Bienvenue sur %s %d !\n", nom.c_str(), version);
    printf("Pi vaut approximativement %.4f\n", pi);
    printf("En hexadécimal : %#x\n", 255);

    // --- std::cout ---
    std::cout << "\n=== std::cout ===" << std::endl;
    std::cout << "Bienvenue sur " << nom << " " << version << " !" << std::endl;
    std::cout << "Pi vaut approximativement " << std::fixed << std::setprecision(4)
              << pi << std::endl;
    std::cout << "En hexadécimal : 0x" << std::hex << 255 << std::endl;

    // Restaurer l'état de cout
    std::cout << std::dec << std::defaultfloat;

    // --- std::println ---
    std::println("\n=== std::println ===");
    std::println("Bienvenue sur {} {} !", nom, version);
    std::println("Pi vaut approximativement {:.4f}", pi);
    std::println("En hexadécimal : {:#x}", 255);

    return 0;
}
