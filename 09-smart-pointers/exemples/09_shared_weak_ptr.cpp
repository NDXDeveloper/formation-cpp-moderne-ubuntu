/* ============================================================================
   Section 9.2 : std::shared_ptr et std::weak_ptr
   Description : Possession partagée, compteur de références, cycles, weak_ptr
   Fichier source : 02-shared-weak-ptr.md
   ============================================================================ */
#include <memory>
#include <print>
#include <string>

// === Possession partagée (lignes 15-27) ===
void exemple_shared_basique() {
    std::print("=== Shared basique ===\n");
    auto p1 = std::make_shared<std::string>("Bonjour");
    auto p2 = p1;  // Copie autorisée — possession partagée

    std::print("{}\n", *p1);  // "Bonjour"
    std::print("{}\n", *p2);  // "Bonjour"
    std::print("Même adresse ? {}\n", p1.get() == p2.get());  // true
}

// === Compteur de références (lignes 36-42) ===
void exemple_compteur() {
    std::print("\n=== Compteur de références ===\n");
    auto a = std::make_shared<int>(42);  // 1 propriétaire
    std::print("use_count après a : {}\n", a.use_count());

    auto b = a;                          // 2 propriétaires
    std::print("use_count après b=a : {}\n", a.use_count());

    auto c = a;                          // 3 propriétaires
    std::print("use_count après c=a : {}\n", a.use_count());
}

// === Surcoût mémoire — sizeof (lignes 83-90) ===
void exemple_sizeof() {
    std::print("\n=== sizeof ===\n");
    std::print("sizeof(int*)              = {}\n", sizeof(int*));                    // 8
    std::print("sizeof(unique_ptr<int>)   = {}\n", sizeof(std::unique_ptr<int>));    // 8
    std::print("sizeof(shared_ptr<int>)   = {}\n", sizeof(std::shared_ptr<int>));    // 16
}

// === Cycle de références (lignes 119-140) ===
struct NoeudCycle {
    std::string nom;
    std::shared_ptr<NoeudCycle> voisin;

    explicit NoeudCycle(std::string n) : nom(std::move(n)) {}
    ~NoeudCycle() { std::print("Destruction de {}\n", nom); }
};

void exemple_cycle() {
    std::print("\n=== Cycle de références (MEMORY LEAK) ===\n");
    auto a = std::make_shared<NoeudCycle>("A");
    auto b = std::make_shared<NoeudCycle>("B");

    a->voisin = b;  // A → B
    b->voisin = a;  // B → A  → CYCLE !

    std::print("a.use_count = {}\n", a.use_count());  // 2
    std::print("b.use_count = {}\n", b.use_count());  // 2

    // En sortant du scope : aucun compteur n'atteint 0
    // "Destruction de A" et "Destruction de B" ne sont JAMAIS affichés
}

// === weak_ptr — arbre sans cycle (lignes 155-175) ===
struct Noeud {
    std::string nom;
    std::shared_ptr<Noeud> enfant;
    std::weak_ptr<Noeud> parent;  // ✅ Observation — pas de cycle

    explicit Noeud(std::string n) : nom(std::move(n)) {}
    ~Noeud() { std::print("Destruction de {}\n", nom); }
};

void exemple_arbre() {
    std::print("\n=== Arbre avec weak_ptr (pas de cycle) ===\n");
    auto racine = std::make_shared<Noeud>("Racine");
    auto feuille = std::make_shared<Noeud>("Feuille");

    racine->enfant = feuille;
    feuille->parent = racine;

    std::print("racine.use_count = {}\n", racine.use_count());    // 1
    std::print("feuille.use_count = {}\n", feuille.use_count());  // 2
    // ✅ Les deux seront détruits correctement
}

// === lock() et expired() (lignes 182-205) ===
void exemple_lock() {
    std::print("\n=== lock() et expired() ===\n");
    auto shared = std::make_shared<std::string>("Hello");
    std::weak_ptr<std::string> weak = shared;

    // Tentative d'accès
    if (auto locked = weak.lock()) {
        std::print("Valeur : {}\n", *locked);
    }

    std::print("expired avant reset : {}\n", weak.expired());  // false

    shared.reset();  // Détruire la ressource

    std::print("expired après reset : {}\n", weak.expired());  // true

    if (auto locked = weak.lock()) {
        std::print("Valeur : {}\n", *locked);
    } else {
        std::print("Ressource expirée\n");
    }
}

int main() {
    exemple_shared_basique();
    exemple_compteur();
    exemple_sizeof();
    exemple_cycle();
    exemple_arbre();
    exemple_lock();
    std::print("\n✅ Tous les exemples passés\n");
}
