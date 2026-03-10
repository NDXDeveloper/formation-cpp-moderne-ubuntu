/* ============================================================================
   Section 7.1.3 : Heritage virtuel — Ordre de construction
   Description : Les bases virtuelles sont construites en premier, dans l'ordre
                 de parcours en profondeur gauche-droite. Puis les bases non
                 virtuelles, puis les membres, puis le corps du constructeur.
   Fichier source : 01.3-heritage-virtuel.md
   ============================================================================ */
#include <print>

class V1 {
public:
    V1() { std::println("V1"); }
};

class V2 {
public:
    V2() { std::println("V2"); }
};

class A : virtual public V1, virtual public V2 {
public:
    A() { std::println("A"); }
};

class B : virtual public V2, virtual public V1 {
public:
    B() { std::println("B"); }
};

class C : public A, public B {
public:
    C() { std::println("C"); }
};

int main() {
    C c;
}
