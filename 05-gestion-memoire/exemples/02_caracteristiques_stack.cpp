/* ============================================================================
   Section 5.1.2 : Caractéristiques de la Stack
   Description : Démonstration du scope et des destructeurs automatiques,
                 ordre de destruction LIFO, et récursion sur un arbre binaire
   Fichier source : 01.2-caracteristiques-stack.md
   ============================================================================ */
#include <iostream>
#include <string>

// --- Durée de vie liée au scope (lignes 73-86) ---
void demonstrer_scope() {
    std::string prenom = "Alice";       // construit sur la stack
    std::cout << prenom << "\n";

    {
        std::string nom = "Dupont";     // construit sur la stack (scope interne)
        std::cout << nom << "\n";
    }   // ← destructeur de 'nom' appelé ici — mémoire interne libérée

    // 'nom' n'existe plus, mais 'prenom' est toujours valide
    std::cout << prenom << "\n";

}   // ← destructeur de 'prenom' appelé ici

// --- Ordre de destruction LIFO (lignes 97-115) ---
struct Trace {
    std::string nom;
    Trace(std::string n) : nom(std::move(n)) {
        std::cout << "  Construction de " << nom << "\n";
    }
    ~Trace() {
        std::cout << "  Destruction de " << nom << "\n";
    }
};

void ordre_lifo() {
    std::cout << "Début\n";
    Trace a("A");
    Trace b("B");
    Trace c("C");
    std::cout << "Fin du bloc\n";
}

// --- Récursion sur arbre binaire (lignes 206-216) ---
struct Noeud {
    int valeur;
    Noeud* gauche;
    Noeud* droite;
};

int somme(const Noeud* n) {
    if (n == nullptr) return 0;
    return n->valeur + somme(n->gauche) + somme(n->droite);
}

int main() {
    std::cout << "--- Scope et destructeurs ---\n";
    demonstrer_scope();

    std::cout << "\n--- Ordre LIFO ---\n";
    ordre_lifo();

    std::cout << "\n--- Récursion arbre binaire ---\n";
    // Arbre :      4
    //            /   \.
    //           2     5
    //          / \.
    //         1   3
    Noeud n1{1, nullptr, nullptr};
    Noeud n3{3, nullptr, nullptr};
    Noeud n2{2, &n1, &n3};
    Noeud n5{5, nullptr, nullptr};
    Noeud n4{4, &n2, &n5};
    std::cout << "Somme de l'arbre : " << somme(&n4) << "\n";  // 15

    return 0;
}
