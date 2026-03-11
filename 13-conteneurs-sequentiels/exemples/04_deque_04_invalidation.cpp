/* ============================================================================
   Section 13.4 : Invalidation des itérateurs
   Description : Démonstration que push_back invalide les itérateurs mais
                 préserve les pointeurs et références
   Fichier source : 04-deque.md
   ============================================================================ */
#include <deque>
#include <print>

int main() {
    std::deque<int> dq{10, 20, 30, 40, 50};

    int& ref = dq[2];       // référence vers 30
    int* ptr = &dq[2];      // pointeur vers 30
    // auto it = dq.begin() + 2;  // itérateur vers 30

    dq.push_back(60);

    // ref et ptr sont VALIDES (les blocs n'ont pas bougé)
    std::println("ref={}, *ptr={}", ref, *ptr);  // 30, 30

    // it serait INVALIDE — comportement indéfini
}
