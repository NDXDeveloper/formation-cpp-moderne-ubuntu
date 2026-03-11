/* ============================================================================
   Section 17.1 : Syntaxe : try, catch, throw
   Description : Démonstration complète du mécanisme d'exceptions —
                 throw, try/catch, stack unwinding, ordre de capture,
                 catch-all, function-try-block, filet de sécurité main
   Fichier source : 01-syntaxe-try-catch.md
   ============================================================================ */

#include <stdexcept>
#include <print>
#include <string>
#include <memory>
#include <queue>

// === throw et diviser ===
double diviser(double numerateur, double denominateur) {
    if (denominateur == 0.0) {
        throw std::invalid_argument("Division par zéro");
    }
    return numerateur / denominateur;
}

// === Stack unwinding avec Trace ===
struct Trace {
    std::string nom;
    Trace(std::string n) : nom(std::move(n)) {
        std::print("  Construction de {}\n", nom);
    }
    ~Trace() {
        std::print("  Destruction de {}\n", nom);
    }
};

void fonction_c() {
    Trace t3("t3 (fonction_c)");
    std::print("→ throw dans fonction_c\n");
    throw std::runtime_error("Erreur dans fonction_c");
}

void fonction_b() {
    Trace t2("t2 (fonction_b)");
    fonction_c();
    std::print("Après appel à fonction_c\n");  // jamais exécuté
}

void fonction_a() {
    Trace t1("t1 (fonction_a)");
    try {
        fonction_b();
    }
    catch (const std::runtime_error& e) {
        std::print("Exception capturée dans fonction_a : {}\n", e.what());
    }
}

// === Relance avec throw; vs throw e; ===
void demo_relance() {
    try {
        throw std::runtime_error("erreur originale");
    } catch (const std::exception& e) {
        std::print("Capturé : {}\n", e.what());
        // throw; préserve le type dynamique
        // throw e; ferait du slicing
    }
}

// === Ordre des catch : du plus spécifique au plus général ===
void demo_ordre_catch() {
    try {
        throw std::invalid_argument("argument invalide");
    }
    catch (const std::invalid_argument& e) {
        std::print("Catch invalid_argument : {}\n", e.what());
    }
    catch (const std::logic_error& e) {
        std::print("Catch logic_error : {}\n", e.what());
    }
    catch (const std::exception& e) {
        std::print("Catch exception : {}\n", e.what());
    }
}

// === Catch-all ===
void demo_catch_all() {
    try {
        throw 42;  // lancer un int (déconseillé, mais légal)
    }
    catch (const std::exception& e) {
        std::print("Exception standard : {}\n", e.what());
    }
    catch (...) {
        std::print("Exception inconnue capturée (catch-all)\n");
    }
}

// === RAII vs allocation brute ===
void demo_raii_vs_brut() {
    // Avec RAII (pas de fuite)
    try {
        auto p = std::make_unique<int>(42);
        throw std::runtime_error("oops");
    } catch (const std::exception& e) {
        std::print("RAII : exception capturée, pas de fuite mémoire\n");
    }
}

// === Function-try-block ===
class Connexion {
public:
    Connexion(const std::string& url) {
        if (url.empty()) throw std::invalid_argument("URL vide");
        std::print("Connexion établie vers {}\n", url);
    }
};

class Service {
    Connexion conn_;
public:
    Service(const std::string& url)
    try : conn_(url)
    {
        std::print("Service créé\n");
    }
    catch (const std::invalid_argument& e) {
        std::print("Échec initialisation Service : {}\n", e.what());
        // l'exception est automatiquement relancée
    }
};

int main() {
    std::print("=== 1. throw / try / catch basique ===\n");
    try {
        auto r = diviser(10.0, 0.0);
        std::print("Résultat : {}\n", r);
    } catch (const std::invalid_argument& e) {
        std::print("Erreur : {}\n", e.what());
    }

    std::print("\n=== 2. Stack unwinding ===\n");
    fonction_a();

    std::print("\n=== 3. Relance d'exception ===\n");
    demo_relance();

    std::print("\n=== 4. Ordre des catch ===\n");
    demo_ordre_catch();

    std::print("\n=== 5. Catch-all ===\n");
    demo_catch_all();

    std::print("\n=== 6. RAII vs allocation brute ===\n");
    demo_raii_vs_brut();

    std::print("\n=== 7. Function-try-block ===\n");
    try {
        Service s("");  // URL vide → exception dans le constructeur de Connexion
    } catch (const std::invalid_argument& e) {
        std::print("Main : exception relancée par Service : {}\n", e.what());
    }

    std::print("\n=== 8. Filet de sécurité main ===\n");
    std::print("Programme terminé normalement.\n");

    return 0;
}
