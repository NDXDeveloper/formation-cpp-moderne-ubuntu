/* ============================================================================
   Section 30.1.1 : memcheck — Détection de fuites
   Description : Fuite par cycle de std::shared_ptr — les compteurs ne
                 descendent jamais à zéro
   Fichier source : 01.1-memcheck.md
   Compilation : g++-15 -std=c++23 -g -O0 -o cycle_shared 03_cycle_shared_ptr.cpp
   Exécution  : valgrind --leak-check=full --show-leak-kinds=all ./cycle_shared
   ============================================================================ */

#include <memory>
#include <string>

struct Noeud {
    std::string nom;
    std::shared_ptr<Noeud> voisin;

    Noeud(std::string n) : nom(std::move(n)) {}
    ~Noeud() { /* jamais appelé en cas de cycle */ }
};

void creer_cycle() {
    auto a = std::make_shared<Noeud>("A");
    auto b = std::make_shared<Noeud>("B");

    a->voisin = b;
    b->voisin = a;  // Cycle : compteurs ne descendront jamais à 0
}

int main() {
    creer_cycle();
    return 0;
}
