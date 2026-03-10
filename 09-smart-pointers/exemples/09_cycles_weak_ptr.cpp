/* ============================================================================
   Section 9.2.2 : Cycles de références et std::weak_ptr
   Description : Cycles, résolution avec weak_ptr, lock(), expired(), patterns
   Fichier source : 02.2-cycles-weak-ptr.md
   ============================================================================ */
#include <memory>
#include <print>
#include <string>
#include <vector>
#include <functional>

// === Cycle simple — Personne (lignes 18-54) ===
struct PersonneCycle {
    std::string nom;
    std::shared_ptr<PersonneCycle> ami;

    PersonneCycle(std::string n) : nom(std::move(n)) {
        std::print("[+] {} créé\n", nom);
    }
    ~PersonneCycle() {
        std::print("[-] {} détruit\n", nom);
    }
};

void exemple_cycle() {
    std::print("=== Cycle (MEMORY LEAK attendu) ===\n");
    auto alice = std::make_shared<PersonneCycle>("Alice");
    auto bob = std::make_shared<PersonneCycle>("Bob");
    alice->ami = bob;
    bob->ami = alice;
    std::print("alice.use_count = {}\n", alice.use_count());  // 2
    std::print("bob.use_count = {}\n", bob.use_count());      // 2
    // Aucun destructeur ne sera appelé → leak
}

// === Résolution avec weak_ptr (lignes 144-174) ===
struct PersonneWeak {
    std::string nom;
    std::weak_ptr<PersonneWeak> ami;  // ✅ Référence faible

    PersonneWeak(std::string n) : nom(std::move(n)) {
        std::print("[+] {} créé\n", nom);
    }
    ~PersonneWeak() {
        std::print("[-] {} détruit\n", nom);
    }
};

void exemple_sans_cycle() {
    std::print("\n=== Sans cycle (weak_ptr) ===\n");
    auto alice = std::make_shared<PersonneWeak>("Alice");
    auto bob = std::make_shared<PersonneWeak>("Bob");
    alice->ami = bob;
    bob->ami = alice;
    std::print("alice.use_count = {}\n", alice.use_count());  // 1
    std::print("bob.use_count = {}\n", bob.use_count());      // 1
    // Les deux seront détruits correctement
}

// === Cycle triangulaire (lignes 92-109) ===
struct Noeud {
    std::string id;
    std::shared_ptr<Noeud> suivant;
    explicit Noeud(std::string i) : id(std::move(i)) {}
    ~Noeud() { std::print("[-] {} détruit\n", id); }
};

void exemple_cycle_triangulaire() {
    std::print("\n=== Cycle triangulaire (LEAK) ===\n");
    auto a = std::make_shared<Noeud>("A");
    auto b = std::make_shared<Noeud>("B");
    auto c = std::make_shared<Noeud>("C");
    a->suivant = b;
    b->suivant = c;
    c->suivant = a;
    // Aucun destructeur appelé
}

// === Auto-référence (lignes 116-122) ===
void exemple_auto_reference() {
    std::print("\n=== Auto-référence (LEAK) ===\n");
    auto noeud = std::make_shared<Noeud>("X");
    noeud->suivant = noeud;
    std::print("X.use_count = {}\n", noeud.use_count());  // 2
    // X se maintient en vie tout seul
}

// === API weak_ptr : lock(), expired() (lignes 204-263) ===
void exemple_lock_expired() {
    std::print("\n=== lock() et expired() ===\n");

    // Création
    auto shared = std::make_shared<std::string>("Hello");
    std::weak_ptr<std::string> w1 = shared;
    std::weak_ptr<std::string> w2(shared);
    std::weak_ptr<std::string> w3 = w1;
    std::print("strong_count = {}\n", shared.use_count());  // 1

    // lock() — ressource vivante
    if (auto locked = w1.lock()) {
        std::print("Valeur : {}\n", *locked);  // Hello
        std::print("strong_count = {}\n", shared.use_count());  // 2
    }
    // locked détruit → strong_count revient à 1

    // expired()
    std::print("Expiré ? {}\n", w1.expired());  // false

    // Destruction de la ressource
    shared.reset();
    std::print("Expiré ? {}\n", w1.expired());  // true

    // lock() — ressource expirée
    if (auto locked = w1.lock()) {
        std::print("Ceci ne sera jamais affiché\n");
    } else {
        std::print("Ressource expirée\n");
    }

    // use_count() via weak_ptr
    auto shared2 = std::make_shared<int>(42);
    std::weak_ptr<int> weak2 = shared2;
    std::print("Propriétaires forts : {}\n", weak2.use_count());  // 1
}

// === Pattern 1 : TreeNode avec enable_shared_from_this (lignes 337-356) ===
struct TreeNode : public std::enable_shared_from_this<TreeNode> {
    std::string valeur;
    std::weak_ptr<TreeNode> parent;
    std::vector<std::shared_ptr<TreeNode>> enfants;

    explicit TreeNode(std::string v) : valeur(std::move(v)) {}

    void ajouter_enfant(std::shared_ptr<TreeNode> enfant) {
        enfant->parent = shared_from_this();
        enfants.push_back(std::move(enfant));
    }

    std::string chemin() const {
        std::string resultat = valeur;
        auto p = parent.lock();
        while (p) {
            resultat = p->valeur + "/" + resultat;
            p = p->parent.lock();
        }
        return resultat;
    }
};

void exemple_tree() {
    std::print("\n=== TreeNode (enable_shared_from_this) ===\n");
    auto racine = std::make_shared<TreeNode>("racine");
    auto fils = std::make_shared<TreeNode>("fils");
    auto petit_fils = std::make_shared<TreeNode>("petit-fils");

    racine->ajouter_enfant(fils);
    fils->ajouter_enfant(petit_fils);

    std::print("{}\n", fils->chemin());        // racine/fils
    std::print("{}\n", petit_fils->chemin());  // racine/fils/petit-fils
}

// === Pattern 4 : Worker avec weak_from_this (lignes 433-455) ===
class Worker : public std::enable_shared_from_this<Worker> {
    std::function<void()> callback_;
    std::string nom_;

public:
    explicit Worker(std::string n) : nom_(std::move(n)) {
        std::print("[+] Worker '{}' créé\n", nom_);
    }
    ~Worker() {
        std::print("[-] Worker '{}' détruit\n", nom_);
    }

    void demarrer_safe() {
        callback_ = [weak_self = weak_from_this(), nom = nom_]() {
            if (auto self = weak_self.lock()) {
                self->traiter();
            } else {
                std::print("Worker '{}' déjà détruit\n", nom);
            }
        };
    }

    void traiter() { std::print("Traitement par '{}'\n", nom_); }
    void executer_callback() { if (callback_) callback_(); }
};

void exemple_worker() {
    std::print("\n=== Worker (weak_from_this) ===\n");
    std::function<void()> saved_callback;

    {
        auto w = std::make_shared<Worker>("W1");
        w->demarrer_safe();
        w->executer_callback();  // Traitement par 'W1'
        saved_callback = [w]() { w->executer_callback(); };
    }
    // Worker détruit ici

    // Le callback sauvé ne devrait plus fonctionner si weak
    std::print("Après destruction du Worker\n");
}

int main() {
    exemple_cycle();
    exemple_sans_cycle();
    exemple_cycle_triangulaire();
    exemple_auto_reference();
    exemple_lock_expired();
    exemple_tree();
    exemple_worker();
    std::print("\n✅ Tous les exemples passés\n");
}
