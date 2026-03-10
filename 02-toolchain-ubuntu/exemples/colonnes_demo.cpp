/* ============================================================================
   Section 2.7.1 : Syntaxe et comparaison — Affichage en colonnes
   Description : Comparaison printf / cout / println pour l'alignement en colonnes
   Fichier source : 07.1-syntaxe-comparaison.md
   Compilation : g++ -std=c++23 -Wall -Wextra colonnes_demo.cpp -o colonnes_demo
   ============================================================================ */
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <print>

int main() {
    // --- printf ---
    std::puts("=== printf ===");
    printf("%-15s %8s %10s\n", "Produit", "Qté", "Prix");
    printf("%-15s %8d %10.2f\n", "Clavier", 12, 49.99);
    printf("%-15s %8d %10.2f\n", "Souris", 25, 29.99);
    printf("%-15s %8d %10.2f\n", "Écran 27\"", 3, 399.00);

    // --- std::cout ---
    std::cout << "\n=== std::cout ===" << std::endl;
    std::cout << std::left << std::setw(15) << "Produit"
              << std::right << std::setw(8) << "Qté"
              << std::setw(10) << "Prix" << "\n";
    std::cout << std::left << std::setw(15) << "Clavier"
              << std::right << std::setw(8) << 12
              << std::fixed << std::setprecision(2) << std::setw(10) << 49.99 << "\n";
    std::cout << std::left << std::setw(15) << "Souris"
              << std::right << std::setw(8) << 25
              << std::setw(10) << 29.99 << "\n";
    std::cout << std::left << std::setw(15) << "Écran 27\""
              << std::right << std::setw(8) << 3
              << std::setw(10) << 399.00 << "\n";

    // Restaurer l'état
    std::cout << std::defaultfloat;

    // --- std::println ---
    std::println("\n=== std::println ===");
    std::println("{:<15} {:>8} {:>10}", "Produit", "Qté", "Prix");
    std::println("{:<15} {:>8} {:>10.2f}", "Clavier", 12, 49.99);
    std::println("{:<15} {:>8} {:>10.2f}", "Souris", 25, 29.99);
    std::println("{:<15} {:>8} {:>10.2f}", "Écran 27\"", 3, 399.00);

    return 0;
}
