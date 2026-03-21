/* ============================================================================
   Section 46.3 : Namespaces et eviter la pollution globale
   Description : Namespaces imbriques C++17, namespace anonyme, using local
   Fichier source : 03-namespaces.md
   ============================================================================ */

#include <iostream>
#include <string>

// C++17 : syntaxe compacte pour namespaces imbriques
namespace monprojet::core::detail {
    void helper() { std::cout << "monprojet::core::detail::helper()\n"; }
}

namespace monprojet {
    class Config {
    public:
        void load() { std::cout << "Config loaded\n"; }
    };

    void initialize() { std::cout << "monprojet::initialize()\n"; }
}

// Namespace anonyme — liaison interne (visible uniquement ici)
namespace {
    int internal_counter = 0;
    void internal_helper() { ++internal_counter; }
}

int main() {
    monprojet::initialize();
    monprojet::Config cfg;
    cfg.load();
    monprojet::core::detail::helper();

    internal_helper();
    std::cout << "Internal counter: " << internal_counter << "\n";
}
