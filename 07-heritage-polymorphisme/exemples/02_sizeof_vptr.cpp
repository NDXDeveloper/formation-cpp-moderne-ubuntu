/* ============================================================================
   Section 7.2 : Fonctions virtuelles et vtable — Impact du vptr sur sizeof
   Description : L'ajout d'une fonction virtuelle ajoute un vptr (8 octets
                 sur 64 bits) a chaque objet, modifiant sizeof et le layout.
   Fichier source : 02-fonctions-virtuelles-vtable.md
   ============================================================================ */
#include <print>

class SansVirtual {
    int x_;
};

class AvecVirtual {
    int x_;
public:
    virtual void f() {}
};

// Sur architecture 64 bits :
static_assert(sizeof(SansVirtual) == 4);    // juste un int
static_assert(sizeof(AvecVirtual) == 16);   // vptr (8) + int (4) + padding (4)

int main() {
    std::println("sizeof(SansVirtual) = {}", sizeof(SansVirtual));
    std::println("sizeof(AvecVirtual) = {}", sizeof(AvecVirtual));
    std::println("Difference          = {} octets (vptr + padding)",
                 sizeof(AvecVirtual) - sizeof(SansVirtual));
}
