/* ============================================================================
   Section 13.1.1 : Idiome swap (pré-C++11)
   Description : Technique du swap trick pour libérer la mémoire excédentaire
                 d'un vecteur avant C++11
   Fichier source : 01.1-fonctionnement-interne.md
   ============================================================================ */
#include <vector>
#include <print>

int main() {
    std::vector<int> v(10'000);
    v.clear();

    // Idiome pré-C++11 : swap avec un vecteur temporaire
    std::vector<int>(v).swap(v);
    // Ou de manière équivalente :
    // std::vector<int>().swap(v);  // pour vider complètement

    std::println("size={}, capacity={}", v.size(), v.capacity());
}
