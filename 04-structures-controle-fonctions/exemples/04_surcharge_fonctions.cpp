/* ============================================================================
   Section 4.4 : Surcharge de fonctions (Function Overloading)
   Description : Surcharge de base, résolution (promotions, conversions),
                 conversions utilisateur, explicit, name mangling, portée
                 et masquage, nullptr vs 0
   Fichier source : 04-surcharge-fonctions.md
   ============================================================================ */
#include <iostream>
#include <string>

// --- Surcharge de base (lignes 13-26) ---
void afficher(int x)                 { std::cout << "Entier : " << x << "\n"; }
void afficher(double x)              { std::cout << "Flottant : " << x << "\n"; }
void afficher(const std::string& x)  { std::cout << "Chaîne : " << x << "\n"; }

// --- Résolution de surcharge (lignes 110-119) ---
namespace resolution {
    void f(int)    { std::cout << "  f(int)\n"; }
    void f(double) { std::cout << "  f(double)\n"; }
}

// --- Conversion utilisateur (lignes 240-253 corrigé) ---
struct Distance {
    double metres;
    Distance(double m) : metres(m) {}
};

void mesurer(Distance d)              { std::cout << "  " << d.metres << " m\n"; }
void mesurer(const std::string& label) { std::cout << "  Label : " << label << "\n"; }

// --- explicit (lignes 257-269) ---
struct DistanceExplicit {
    double metres;
    explicit DistanceExplicit(double m) : metres(m) {}
};

void mesurer_explicit(DistanceExplicit d) { std::cout << "  " << d.metres << " m\n"; }

// --- nullptr vs 0 (lignes 224-232) ---
namespace nullptr_demo {
    void f(int x)    { std::cout << "  f(int)\n"; }
    void f(int* ptr) { std::cout << "  f(int*)\n"; }
}

// --- Portée et masquage (lignes 297-309) ---
namespace scope_demo {
    void f(int)    { std::cout << "  int global\n"; }
    void f(double) { std::cout << "  double global\n"; }

    namespace app {
        using scope_demo::f;  // Rend les f visibles
        void f(const std::string&) { std::cout << "  string app\n"; }
    }
}

// --- bool promotion (lignes 200-207) ---
namespace bool_demo {
    void f(bool)         { std::cout << "  f(bool)\n"; }
    void f(int)          { std::cout << "  f(int)\n"; }
    void f(const char*)  { std::cout << "  f(const char*)\n"; }
}

int main() {
    std::cout << "--- Surcharge de base ---\n";
    afficher(42);
    afficher(3.14);
    afficher(std::string("hello"));

    std::cout << "\n--- Résolution (promotions) ---\n";
    resolution::f(42);     // int → f(int)
    resolution::f(3.14);   // double → f(double)
    resolution::f('A');    // char → promotion int → f(int)
    resolution::f(3.14f);  // float → promotion double → f(double)

    std::cout << "\n--- Conversion utilisateur ---\n";
    mesurer(3.14);                     // double → Distance
    mesurer(std::string("parcours"));  // string

    std::cout << "\n--- explicit ---\n";
    // mesurer_explicit(3.14);         // ❌ Erreur — conversion implicite interdite
    mesurer_explicit(DistanceExplicit(3.14));  // ✅ Conversion explicite

    std::cout << "\n--- nullptr vs 0 ---\n";
    nullptr_demo::f(0);        // f(int)
    nullptr_demo::f(nullptr);  // f(int*)

    std::cout << "\n--- Portée et masquage ---\n";
    scope_demo::app::f(42);
    scope_demo::app::f(3.14);
    scope_demo::app::f(std::string("ok"));

    std::cout << "\n--- bool promotion ---\n";
    bool_demo::f(true);    // f(bool) — correspondance exacte
    bool_demo::f(42);      // f(int) — correspondance exacte
    bool_demo::f("hello"); // f(const char*) — correspondance exacte

    return 0;
}
