/* ============================================================================
   Section 4.3.2 : Passage par référence (&)
   Description : Alias vers l'original, modification in-out, swap, retour
                 de valeurs multiples (ancien style et structured bindings),
                 propriétés des références, retour de référence sûr
   Fichier source : 03.2-par-reference.md
   ============================================================================ */
#include <iostream>
#include <vector>
#include <string>

// --- Principe (lignes 13-25) ---
void doubler(int& x) {
    x *= 2;
}

// --- Incrémenter (lignes 67-79) ---
void incrementer(int& compteur) {
    ++compteur;
}

// --- Modifier un conteneur (lignes 83-95) ---
void ajouter_defauts(std::vector<std::string>& config) {
    config.push_back("timeout=30");
    config.push_back("retries=3");
    config.push_back("verbose=false");
}

// --- Swap (lignes 103-115) ---
void my_swap(int& a, int& b) {
    int tmp = a;
    a = b;
    b = tmp;
}

// --- Retour multiple ancien style (lignes 123-135) ---
void decomposer(double valeur, int& partie_entiere, double& partie_decimale) {
    partie_entiere = static_cast<int>(valeur);
    partie_decimale = valeur - partie_entiere;
}

// --- Retour multiple moderne (lignes 140-157) ---
struct Decomposition {
    int partie_entiere;
    double partie_decimale;
};

Decomposition decomposer_moderne(double valeur) {
    return {
        static_cast<int>(valeur),
        valeur - static_cast<int>(valeur)
    };
}

// --- Retourner une référence sûre (lignes 242-252) ---
int& premier_element(std::vector<int>& v) {
    return v[0];
}

int& compteur_global() {
    static int count = 0;
    return count;
}

int main() {
    std::cout << "--- Passage par référence ---\n";
    int n = 5;
    doubler(n);
    std::cout << "n = " << n << "\n";  // n = 10

    std::cout << "\n--- Incrémenter ---\n";
    int visites = 0;
    incrementer(visites);
    incrementer(visites);
    incrementer(visites);
    std::cout << "visites = " << visites << "\n";  // 3

    std::cout << "\n--- Modifier un conteneur ---\n";
    std::vector<std::string> params = {"host=localhost"};
    ajouter_defauts(params);
    std::cout << "params contient " << params.size() << " éléments\n";  // 4

    std::cout << "\n--- Swap ---\n";
    int x = 1, y = 2;
    my_swap(x, y);
    std::cout << "x=" << x << " y=" << y << "\n";  // x=2 y=1

    std::cout << "\n--- Décomposer (ancien style) ---\n";
    int entier;
    double decimale;
    decomposer(3.14, entier, decimale);
    std::cout << entier << " + " << decimale << "\n";  // 3 + 0.14

    std::cout << "\n--- Décomposer (moderne) ---\n";
    auto [e, d] = decomposer_moderne(3.14);
    std::cout << e << " + " << d << "\n";  // 3 + 0.14

    std::cout << "\n--- Référence vers élément de conteneur ---\n";
    std::vector<int> vec = {10, 20, 30};
    premier_element(vec) = 99;
    std::cout << "vec[0] = " << vec[0] << "\n";  // 99

    std::cout << "\n--- Compteur global (static) ---\n";
    compteur_global()++;
    compteur_global()++;
    compteur_global()++;
    std::cout << "count = " << compteur_global() << "\n";  // 3

    return 0;
}
