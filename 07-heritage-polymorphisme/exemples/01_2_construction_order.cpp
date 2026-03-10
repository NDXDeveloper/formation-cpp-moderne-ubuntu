/* ============================================================================
   Section 7.1.2 : Heritage multiple — Ordre de construction
   Description : L'ordre de construction suit l'ordre de declaration des bases,
                 PAS l'ordre dans la liste d'initialisation du constructeur.
                 Destruction dans l'ordre inverse.
   Fichier source : 01.2-heritage-multiple.md
   ============================================================================ */
#include <print>

class A {
public:
    A() { std::println("A::A()"); }
    ~A() { std::println("A::~A()"); }
};

class B {
public:
    B() { std::println("B::B()"); }
    ~B() { std::println("B::~B()"); }
};

class C : public B, public A {   // B déclaré AVANT A
public:
    C() : A{}, B{} {             // ordre dans la liste d'init : A, B
        std::println("C::C()");  // mais la construction suit l'ordre de déclaration
    }
    ~C() { std::println("C::~C()"); }
};

int main() {
    C c;
}
