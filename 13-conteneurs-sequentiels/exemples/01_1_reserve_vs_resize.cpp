/* ============================================================================
   Section 13.1.1 : reserve() vs resize()
   Description : Différence fondamentale entre reserve() (alloue la capacité)
                 et resize() (crée des éléments)
   Fichier source : 01.1-fonctionnement-interne.md
   ============================================================================ */
#include <vector>
#include <print>

int main() {
    // reserve() : alloue de la capacité, ne crée PAS d'éléments
    std::vector<int> v1;
    v1.reserve(100);
    std::println("reserve(100) → size={}, capacity={}", v1.size(), v1.capacity());
    // Sortie : reserve(100) → size=0, capacity=100
    // v1[0] serait un comportement indéfini !

    // resize() : change la taille, CRÉE des éléments (value-initialized)
    std::vector<int> v2;
    v2.resize(100);
    std::println("resize(100)  → size={}, capacity={}", v2.size(), v2.capacity());
    // Sortie : resize(100)  → size=100, capacity=100
    // v2[0] est valide et vaut 0

    // resize() avec valeur
    std::vector<int> v3;
    v3.resize(100, -1);
    std::println("v3[50] = {}", v3[50]);
    // Sortie : v3[50] = -1
}
