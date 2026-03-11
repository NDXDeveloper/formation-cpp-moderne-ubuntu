/* ============================================================================
   Section 13.1 : Accès aux éléments
   Description : Différentes méthodes d'accès aux éléments d'un std::vector
                 (operator[], at(), front(), back(), data())
   Fichier source : 01-vector.md
   ============================================================================ */
#include <vector>
#include <print>

int main() {
    std::vector<int> v{10, 20, 30, 40, 50};

    // Accès par index — pas de vérification de bornes
    std::println("v[2] = {}", v[2]);       // 30

    // Accès avec vérification — lance std::out_of_range si hors bornes
    std::println("v.at(2) = {}", v.at(2)); // 30

    // Premier et dernier éléments
    std::println("front = {}", v.front()); // 10
    std::println("back = {}", v.back());   // 50

    // Pointeur brut vers les données (interopérabilité C)
    int* raw = v.data();
    std::println("*(raw+3) = {}", *(raw + 3)); // 40
}
