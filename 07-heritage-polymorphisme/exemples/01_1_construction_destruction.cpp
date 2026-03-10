/* ============================================================================
   Section 7.1.1 : Heritage simple — Ordre de construction et destruction
   Description : Demonstration de l'ordre deterministe construction base->derivee
                 et destruction derivee->base
   Fichier source : 01.1-heritage-simple.md
   ============================================================================ */
#include <print>

class Base {
public:
    Base()  { std::println("Base::Base()"); }
    ~Base() { std::println("Base::~Base()"); }
};

class Derivee : public Base {
public:
    Derivee()  { std::println("Derivee::Derivee()"); }
    ~Derivee() { std::println("Derivee::~Derivee()"); }
};

int main() {
    Derivee d;
}
