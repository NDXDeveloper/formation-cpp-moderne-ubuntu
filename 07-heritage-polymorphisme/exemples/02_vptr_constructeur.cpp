/* ============================================================================
   Section 7.2 : Fonctions virtuelles et vtable — vptr dans les constructeurs
   Description : Pendant la construction de Base, le vptr pointe vers la
                 vtable de Base. Un appel virtuel dans le constructeur de Base
                 appelle Base::qui_suis_je(), pas Derivee::qui_suis_je().
   Fichier source : 02-fonctions-virtuelles-vtable.md
   ============================================================================ */
#include <print>

class Base {
public:
    Base() {
        std::println("Base::Base() — type dynamique : Base");
        qui_suis_je();   // appel virtuel dans le constructeur
    }
    virtual void qui_suis_je() const {
        std::println("  → Je suis Base");
    }
    virtual ~Base() = default;
};

class Derivee : public Base {
public:
    Derivee() {
        std::println("Derivee::Derivee() — type dynamique : Derivee");
        qui_suis_je();
    }
    void qui_suis_je() const override {
        std::println("  → Je suis Derivee");
    }
};

int main() {
    Derivee d;
}
