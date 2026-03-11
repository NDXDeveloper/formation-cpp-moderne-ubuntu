/* ============================================================================
   Section 13.5 : const et mutabilité
   Description : Distinction entre span<int> (modifiable), span<const int>
                 (lecture seule) et const span<int> (span non réassignable)
   Fichier source : 05-span.md
   ============================================================================ */
#include <span>
#include <vector>
#include <print>

int main() {
    std::vector<int> v{10, 20, 30};

    // span<int> : éléments MODIFIABLES
    std::span<int> s_mut(v);
    s_mut[0] = 99;
    std::println("v[0] = {}", v[0]);  // 99

    // span<const int> : éléments en LECTURE SEULE
    std::span<const int> s_ro(v);
    std::println("s_ro[0] = {}", s_ro[0]);  // 99

    // const span<int> : le span est const, mais les éléments restent modifiables
    const std::span<int> s_const(v);
    s_const[1] = 77;
}
