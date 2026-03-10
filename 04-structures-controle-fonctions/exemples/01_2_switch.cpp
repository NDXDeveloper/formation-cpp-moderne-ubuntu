/* ============================================================================
   Section 4.1.2 : switch et switch avec initialisation (C++17)
   Description : switch de base, fall-through, [[fallthrough]], regroupement
                 de cas, enum class, switch avec initialisation, portée des
                 variables dans un switch
   Fichier source : 01.2-switch.md
   ============================================================================ */
#include <iostream>
#include <string>

// --- switch de base (lignes 15-42) ---
void demo_switch_base() {
    int day = 3;

    switch (day) {
        case 1: std::cout << "Lundi\n"; break;
        case 2: std::cout << "Mardi\n"; break;
        case 3: std::cout << "Mercredi\n"; break;
        case 4: std::cout << "Jeudi\n"; break;
        case 5: std::cout << "Vendredi\n"; break;
        default: std::cout << "Week-end\n"; break;
    }
}

// --- enum class (lignes 50-66) ---
void demo_enum_switch() {
    enum class Color { Red, Green, Blue };
    Color c = Color::Green;

    switch (c) {
        case Color::Red:   std::cout << "Rouge\n"; break;
        case Color::Green: std::cout << "Vert\n"; break;
        case Color::Blue:  std::cout << "Bleu\n"; break;
    }
}

// --- fall-through démonstration (lignes 77-93) ---
void demo_fallthrough() {
    int x = 2;
    switch (x) {
        case 1: std::cout << "un\n";
            [[fallthrough]];
        case 2: std::cout << "deux\n";
            [[fallthrough]];
        case 3: std::cout << "trois\n";
            [[fallthrough]];
        default: std::cout << "autre\n";
    }
    // Sortie attendue : deux / trois / autre
}

// --- [[fallthrough]] intentionnel (lignes 101-125) ---
void demo_fallthrough_intentionnel() {
    int level = 2;

    switch (level) {
        case 3:
            std::cout << "Initialisation avancée\n";
            [[fallthrough]];
        case 2:
            std::cout << "Initialisation standard\n";
            [[fallthrough]];
        case 1:
            std::cout << "Initialisation minimale\n";
            break;
        default:
            std::cout << "Niveau inconnu\n";
            break;
    }
}

// --- Regrouper des cas (lignes 135-150) ---
void demo_regrouper() {
    char c = 'b';

    switch (c) {
        case 'a': case 'e': case 'i': case 'o': case 'u':
            std::cout << "Voyelle\n";
            break;
        default:
            std::cout << "Consonne (ou autre)\n";
            break;
    }
}

// --- switch avec initialisation C++17 (lignes 201-216) ---
void demo_switch_init() {
    std::string token = "404";

    switch (int n = std::stoi(token); n) {
        case 200: std::cout << "OK\n"; break;
        case 404: std::cout << "Not Found\n"; break;
        case 500: std::cout << "Server Error\n"; break;
        default:  std::cout << "Code HTTP : " << n << "\n"; break;
    }
}

// --- Portée des variables dans un switch (lignes 238-251) ---
void demo_scope() {
    int x = 1;

    switch (x) {
        case 1: {
            int val = 42;
            std::cout << "case 1: val=" << val << "\n";
            break;
        }
        case 2: {
            int val = 100;
            std::cout << "case 2: val=" << val << "\n";
            break;
        }
    }
}

int main() {
    std::cout << "--- switch de base ---\n";
    demo_switch_base();

    std::cout << "\n--- enum class ---\n";
    demo_enum_switch();

    std::cout << "\n--- fall-through ---\n";
    demo_fallthrough();

    std::cout << "\n--- [[fallthrough]] intentionnel ---\n";
    demo_fallthrough_intentionnel();

    std::cout << "\n--- regrouper des cas ---\n";
    demo_regrouper();

    std::cout << "\n--- switch avec initialisation ---\n";
    demo_switch_init();

    std::cout << "\n--- portée des variables ---\n";
    demo_scope();

    return 0;
}
