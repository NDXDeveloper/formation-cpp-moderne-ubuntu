/* ============================================================================
   Section 4.3.1 : Passage par valeur
   Description : Copie indépendante, sink parameter (normaliser), copy elision
                 (RVO), slicing polymorphique, const sur paramètre par valeur
   Fichier source : 03.1-par-valeur.md
   ============================================================================ */
#include <iostream>
#include <string>
#include <algorithm>

// --- Principe (lignes 13-26) ---
void doubler(int x) {
    x *= 2;
    std::cout << "Dans la fonction : x = " << x << "\n";
}

// --- Sink parameter (lignes 104-109) ---
std::string normaliser(std::string input) {
    std::transform(input.begin(), input.end(), input.begin(), ::tolower);
    return input;
}

// --- Copy elision / RVO (lignes 167-177) ---
std::string creer_message() {
    std::string msg = "Hello, World!";
    return msg;  // Le compilateur peut éliminer la copie
}

// --- Slicing (lignes 241-267) ---
struct Animal {
    virtual void parler() const { std::cout << "...\n"; }
    virtual ~Animal() = default;
};

struct Chien : Animal {
    void parler() const override { std::cout << "Woof!\n"; }
};

void faire_parler(Animal a) {
    a.parler();  // Appelle toujours Animal::parler() — slicing
}

void faire_parler_ok(const Animal& a) {
    a.parler();  // Appelle la bonne version selon le type réel
}

// --- const sur paramètre par valeur (lignes 192-195) ---
int carre(const int x) {
    // x = 10;  // ❌ Erreur de compilation
    return x * x;
}

int main() {
    std::cout << "--- Passage par valeur ---\n";
    int n = 5;
    doubler(n);
    std::cout << "Après l'appel    : n = " << n << "\n";

    std::cout << "\n--- Sink parameter (normaliser) ---\n";
    std::string s = "HELLO WORLD";
    std::cout << normaliser(s) << "\n";
    std::cout << "Original: " << s << "\n";

    std::cout << "\n--- Copy elision (RVO) ---\n";
    std::string resultat = creer_message();
    std::cout << resultat << "\n";

    std::cout << "\n--- Slicing ---\n";
    Chien rex;
    faire_parler(rex);      // "..." — slicing !
    faire_parler_ok(rex);   // "Woof!" — polymorphisme correct

    std::cout << "\n--- const sur paramètre ---\n";
    std::cout << "carre(7) = " << carre(7) << "\n";

    return 0;
}
