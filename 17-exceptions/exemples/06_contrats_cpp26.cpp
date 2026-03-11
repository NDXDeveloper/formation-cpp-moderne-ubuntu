/* ============================================================================
   Section 17.6 : Contrats (C++26) — Préconditions et postconditions
   Description : Simulation des contrats C++26 (pre, post, contract_assert)
                 avec des macros, car GCC 15 et Clang 20 ne supportent pas
                 encore les contrats (P2900). Démonstration des concepts :
                 préconditions, postconditions, contract_assert, comparaison
                 avec assert et exceptions.
   Fichier source : 06-contrats-cpp26.md
   Note : Les contrats C++26 ne sont pas encore supportés par les compilateurs
          mainline (mars 2026). Ce fichier utilise des macros pour simuler
          le comportement attendu. La vraie syntaxe est montrée en commentaire.
   ============================================================================ */

#include <print>
#include <string>
#include <cmath>
#include <cassert>
#include <stdexcept>
#include <expected>
#include <source_location>
#include <span>
#include <algorithm>

// === Simulation de contrats via macros ===
// En attendant le support compilateur, on simule avec des macros.
// La vraie syntaxe C++26 est montrée en commentaire à côté.

#define CONTRACT_PRE(cond) \
    do { \
        if (!(cond)) { \
            std::print(stderr, "Violation de précondition : {} ({}:{})\n", \
                       #cond, __FILE__, __LINE__); \
            std::abort(); \
        } \
    } while (false)

#define CONTRACT_POST(cond) \
    do { \
        if (!(cond)) { \
            std::print(stderr, "Violation de postcondition : {} ({}:{})\n", \
                       #cond, __FILE__, __LINE__); \
            std::abort(); \
        } \
    } while (false)

#define CONTRACT_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            std::print(stderr, "Violation de contract_assert : {} ({}:{})\n", \
                       #cond, __FILE__, __LINE__); \
            std::abort(); \
        } \
    } while (false)

// === Exemple 1 : Précondition simple ===
// Syntaxe C++26 réelle :
//   double racine_carree(double x)
//       pre(x >= 0.0)
//       post(r : r >= 0.0)
//   { return std::sqrt(x); }

double racine_carree(double x) {
    CONTRACT_PRE(x >= 0.0);  // pre(x >= 0.0)
    double r = std::sqrt(x);
    CONTRACT_POST(r >= 0.0); // post(r : r >= 0.0)
    return r;
}

// === Exemple 2 : Préconditions multiples ===
// Syntaxe C++26 réelle :
//   int recherche_binaire(std::span<const int> donnees, int cible)
//       pre(!donnees.empty())
//       pre(std::is_sorted(donnees.begin(), donnees.end()))
//       post(r : r == -1 || (r >= 0 && r < static_cast<int>(donnees.size())))
//       post(r : r == -1 || donnees[r] == cible)

int recherche_binaire(std::span<const int> donnees, int cible) {
    CONTRACT_PRE(!donnees.empty());
    CONTRACT_PRE(std::is_sorted(donnees.begin(), donnees.end()));

    auto it = std::lower_bound(donnees.begin(), donnees.end(), cible);
    int r;
    if (it != donnees.end() && *it == cible) {
        r = static_cast<int>(std::distance(donnees.begin(), it));
    } else {
        r = -1;
    }

    CONTRACT_POST(r == -1 || (r >= 0 && r < static_cast<int>(donnees.size())));
    CONTRACT_POST(r == -1 || donnees[r] == cible);
    return r;
}

// === Exemple 3 : Contrat vs Exception — deux rôles distincts ===
// Syntaxe C++26 réelle :
//   int diviser(int a, int b)
//       pre(b != 0)  // Bug si violé, pas une erreur d'exécution
//   { return a / b; }

int diviser_contrat(int a, int b) {
    CONTRACT_PRE(b != 0);  // pre(b != 0)
    return a / b;
}

// Exception : le fichier peut ne pas exister (erreur d'exécution légitime)
std::expected<std::string, std::string> charger_fichier(const std::string& chemin) {
    // Ici on utilise expected, pas un contrat — c'est une erreur d'exécution
    if (chemin.empty()) {
        return std::unexpected(std::string("chemin vide"));
    }
    return std::string("contenu de ") + chemin;
}

// === Exemple 4 : Combinaison contrat + expected ===
// Syntaxe C++26 réelle :
//   std::expected<int, std::string> trouver_element(
//       std::span<const int> donnees, int cible)
//       pre(!donnees.empty())         // Contrat : bug si violé
//       post(r : !r.has_value() || *r >= 0)  // Contrat sur le résultat

std::expected<int, std::string> trouver_element(
    std::span<const int> donnees, int cible)
{
    CONTRACT_PRE(!donnees.empty());  // pre(!donnees.empty())

    auto it = std::find(donnees.begin(), donnees.end(), cible);
    if (it == donnees.end()) {
        return std::unexpected(std::string("élément non trouvé"));
    }
    int r = static_cast<int>(std::distance(donnees.begin(), it));

    CONTRACT_POST(r >= 0);  // post(r : !r.has_value() || *r >= 0)
    return r;
}

// === Exemple 5 : constexpr avec contrat ===
// Syntaxe C++26 réelle :
//   constexpr int factorielle(int n)
//       pre(n >= 0)
//       pre(n <= 20)
//   { ... }

constexpr int factorielle(int n) {
    // CONTRACT_PRE ne peut pas être constexpr avec notre macro,
    // mais en C++26, pre(n >= 0) serait vérifié à la compilation
    if (n < 0 || n > 20) {
        throw std::invalid_argument("n hors limites pour factorielle");
    }
    if (n <= 1) return 1;
    return n * factorielle(n - 1);
}

// Vérification à la compilation
static_assert(factorielle(5) == 120);
static_assert(factorielle(0) == 1);

int main() {
    std::print("=== 1. Précondition et postcondition (racine_carree) ===\n");
    std::print("  racine_carree(25.0) = {}\n", racine_carree(25.0));
    std::print("  racine_carree(2.0) = {}\n", racine_carree(2.0));
    // racine_carree(-1.0) déclencherait : Violation de précondition : x >= 0.0

    std::print("\n=== 2. Préconditions multiples (recherche_binaire) ===\n");
    {
        int arr[] = {10, 20, 30, 40, 50};
        std::span<const int> donnees(arr);
        std::print("  recherche_binaire(30) = {}\n", recherche_binaire(donnees, 30));
        std::print("  recherche_binaire(35) = {}\n", recherche_binaire(donnees, 35));
        std::print("  recherche_binaire(10) = {}\n", recherche_binaire(donnees, 10));
    }

    std::print("\n=== 3. Contrat vs Exception — deux rôles distincts ===\n");
    {
        std::print("  diviser_contrat(10, 3) = {}\n", diviser_contrat(10, 3));
        // diviser_contrat(10, 0) → Violation de précondition (bug de l'appelant)

        auto r = charger_fichier("config.yaml");
        if (r) std::print("  charger_fichier → {}\n", *r);

        auto r2 = charger_fichier("");
        if (!r2) std::print("  charger_fichier(\"\") → erreur : {}\n", r2.error());
    }

    std::print("\n=== 4. Combinaison contrat + expected ===\n");
    {
        int arr[] = {1, 2, 3, 4, 5};
        std::span<const int> donnees(arr);

        auto r1 = trouver_element(donnees, 3);
        if (r1) std::print("  trouver_element(3) → index {}\n", *r1);

        auto r2 = trouver_element(donnees, 99);
        if (!r2) std::print("  trouver_element(99) → {}\n", r2.error());
    }

    std::print("\n=== 5. constexpr avec vérification (simule contrat compile-time) ===\n");
    constexpr auto f5 = factorielle(5);
    constexpr auto f10 = factorielle(10);
    std::print("  factorielle(5) = {} (compile-time)\n", f5);
    std::print("  factorielle(10) = {} (compile-time)\n", f10);

    std::print("\n=== 6. Tableau récapitulatif des mécanismes ===\n");
    std::print("  Contrats (pre/post)    → Erreurs de programmation (bugs)\n");
    std::print("  Exceptions (throw)     → Erreurs d'exécution rares\n");
    std::print("  std::expected          → Erreurs d'exécution fréquentes\n");
    std::print("  noexcept               → Garantie d'absence d'exception\n");
    std::print("  assert / static_assert → Vérifications héritées (C)\n");

    std::print("\n=== Note sur le support compilateur ===\n");
    std::print("  Les contrats C++26 (P2900) ne sont pas encore supportés\n");
    std::print("  par GCC 15 ni Clang 20 (mars 2026).\n");
    std::print("  Ce fichier utilise des macros pour simuler le comportement.\n");
    std::print("  Des forks expérimentaux sont disponibles sur Compiler Explorer.\n");

    std::print("\nProgramme terminé.\n");
    return 0;
}
