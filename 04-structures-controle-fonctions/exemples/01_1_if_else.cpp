/* ============================================================================
   Section 4.1.1 : if, else if, else et if constexpr
   Description : Conditionnelle de base, chaînage else if, if avec
                 initialisation (C++17), if constexpr avec templates
   Fichier source : 01.1-if-else.md
   ============================================================================ */
#include <iostream>
#include <string>
#include <type_traits>

// --- if / else (lignes 14-25) ---
void demo_if_else() {
    int temperature = 38;

    if (temperature > 37) {
        std::cout << "Fièvre détectée\n";
    } else {
        std::cout << "Température normale\n";
    }
}

// --- else if (lignes 44-60) ---
void demo_else_if() {
    int score = 73;

    if (score >= 90) {
        std::cout << "Mention : Excellent\n";
    } else if (score >= 75) {
        std::cout << "Mention : Bien\n";
    } else if (score >= 60) {
        std::cout << "Mention : Assez bien\n";
    } else {
        std::cout << "Mention : Insuffisant\n";
    }
}

// --- if constexpr (lignes 156-177) ---
template <typename T>
void afficher(const T& val) {
    if constexpr (std::is_integral_v<T>) {
        std::cout << "Entier : " << val << "\n";
    } else if constexpr (std::is_floating_point_v<T>) {
        std::cout << "Flottant : " << val << "\n";
    } else {
        std::cout << "Autre (taille : " << val.size() << ")\n";
    }
}

// --- if constexpr avec sizeof (lignes 201-211) ---
void demo_sizeof_constexpr() {
    if constexpr (sizeof(int) == 4) {
        std::cout << "sizeof(int) == 4 sur cette plateforme\n";
    }
}

int main() {
    std::cout << "--- if / else ---\n";
    demo_if_else();

    std::cout << "\n--- else if ---\n";
    demo_else_if();

    std::cout << "\n--- if constexpr ---\n";
    afficher(42);
    afficher(3.14);
    afficher(std::string("hello"));

    std::cout << "\n--- if constexpr sizeof ---\n";
    demo_sizeof_constexpr();

    return 0;
}
