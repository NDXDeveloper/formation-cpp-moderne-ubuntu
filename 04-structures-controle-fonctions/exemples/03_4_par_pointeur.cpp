/* ============================================================================
   Section 4.3.4 : Passage par pointeur (*)
   Description : Déréférencement, paramètre optionnel (nullptr), interop C,
                 tableaux/buffers, arbre binaire, pointeur const (4 formes),
                 nullptr vs surcharge, std::span (C++20)
   Fichier source : 03.4-par-pointeur.md
   ============================================================================ */
#include <iostream>
#include <string>
#include <memory>
#include <span>
#include <cstring>

// --- Principe (lignes 13-25) ---
void doubler(int* ptr) {
    *ptr *= 2;
}

// --- Paramètre optionnel (lignes 70-87) ---
void enregistrer(const std::string& nom, const std::string* commentaire) {
    std::cout << "Enregistrement de " << nom;
    if (commentaire != nullptr) {
        std::cout << " (commentaire : " << *commentaire << ")";
    }
    std::cout << "\n";
}

// --- Tableaux/buffers (lignes 137-147) ---
void remplir(int* buffer, std::size_t taille) {
    for (std::size_t i = 0; i < taille; ++i) {
        buffer[i] = static_cast<int>(i * i);
    }
}

// --- std::span C++20 (lignes 152-160) ---
void remplir_span(std::span<int> buffer) {
    for (std::size_t i = 0; i < buffer.size(); ++i) {
        buffer[i] = static_cast<int>(i * i);
    }
}

// --- Arbre binaire (lignes 167-179) ---
struct TreeNode {
    int value;
    TreeNode* left;
    TreeNode* right;
};

void parcours_infixe(const TreeNode* node) {
    if (node == nullptr) return;
    parcours_infixe(node->left);
    std::cout << node->value << " ";
    parcours_infixe(node->right);
}

// --- nullptr vs surcharge (lignes 274-282) ---
void f(int x)    { std::cout << "  f(int) : " << x << "\n"; }
void f(int* ptr) { std::cout << "  f(int*) : " << (ptr ? "non-null" : "nullptr") << "\n"; }

// --- Vérification nullptr (lignes 259-269) ---
struct Config {
    int timeout = 30;
};

void traiter(const Config* config) {
    if (config == nullptr) {
        std::cout << "  Erreur : configuration absente\n";
        return;
    }
    std::cout << "  Timeout : " << config->timeout << "\n";
}

// --- unique_ptr (lignes 336-342) ---
std::unique_ptr<int> creer() {
    return std::make_unique<int>(42);
}

int main() {
    std::cout << "--- Déréférencement ---\n";
    int n = 5;
    doubler(&n);
    std::cout << "n = " << n << "\n";  // n = 10

    std::cout << "\n--- Paramètre optionnel ---\n";
    std::string note = "Urgent";
    enregistrer("Alice", &note);
    enregistrer("Bob", nullptr);

    std::cout << "\n--- Tableau/buffer ---\n";
    int tableau[5];
    remplir(tableau, 5);
    for (int v : tableau) std::cout << v << " ";
    std::cout << "\n";  // 0 1 4 9 16

    std::cout << "\n--- std::span (C++20) ---\n";
    int tableau2[5];
    remplir_span(tableau2);
    for (int v : tableau2) std::cout << v << " ";
    std::cout << "\n";  // 0 1 4 9 16

    std::cout << "\n--- Arbre binaire ---\n";
    TreeNode n1{1, nullptr, nullptr};
    TreeNode n3{3, nullptr, nullptr};
    TreeNode n2{2, &n1, &n3};
    TreeNode n5{5, nullptr, nullptr};
    TreeNode n4{4, &n2, &n5};
    parcours_infixe(&n4);
    std::cout << "\n";  // 1 2 3 4 5

    std::cout << "\n--- nullptr vs surcharge ---\n";
    f(0);        // f(int)
    f(nullptr);  // f(int*)

    std::cout << "\n--- Vérification nullptr ---\n";
    Config cfg{60};
    traiter(&cfg);
    traiter(nullptr);

    std::cout << "\n--- unique_ptr ---\n";
    auto ptr = creer();
    std::cout << "Valeur : " << *ptr << "\n";  // 42

    std::cout << "\n--- Pointeur const (4 formes) ---\n";
    int valeur = 42;
    const int* p1 = &valeur;       // pointeur vers donnée constante
    int* const p2 = &valeur;       // pointeur constant vers donnée modifiable
    const int* const p3 = &valeur; // pointeur constant vers donnée constante
    std::cout << "*p1=" << *p1 << " *p2=" << *p2 << " *p3=" << *p3 << "\n";

    return 0;
}
