/* ============================================================================
   Section 9.1.1 : Création et utilisation
   Description : Création, accès, nullité, reset(), release(), get()
   Fichier source : 01.1-creation-utilisation.md
   ============================================================================ */
#include <memory>
#include <print>
#include <string>
#include <vector>

struct Point {
    double x, y, z;
    Point(double x, double y, double z) : x(x), y(y), z(z) {}
};

// === Création avec make_unique (lignes 13-33) ===
void exemple_creation() {
    std::print("=== Création ===\n");
    auto p1 = std::make_unique<int>(42);
    auto p2 = std::make_unique<std::string>("Bonjour");
    auto p3 = std::make_unique<int>();
    auto p4 = std::make_unique<Point>(1.0, 2.5, 3.7);

    std::print("p1 = {}\n", *p1);           // 42
    std::print("p2 = {}\n", *p2);           // Bonjour
    std::print("p3 = {}\n", *p3);           // 0
    std::print("p4 = ({}, {}, {})\n", p4->x, p4->y, p4->z);  // (1, 2.5, 3.7)
}

// === Construction directe (ligne 45) ===
void exemple_construction_directe() {
    std::print("\n=== Construction directe ===\n");
    std::unique_ptr<int> p(new int(42));
    std::print("p = {}\n", *p);  // 42
}

// === unique_ptr nul (lignes 55-58) ===
void exemple_nul() {
    std::print("\n=== unique_ptr nul ===\n");
    std::unique_ptr<int> p1;
    std::unique_ptr<int> p2(nullptr);
    std::unique_ptr<int> p3 = nullptr;
    std::print("p1 nul ? {}\n", (p1 == nullptr));  // true
    std::print("p2 nul ? {}\n", (p2 == nullptr));  // true
    std::print("p3 nul ? {}\n", (p3 == nullptr));  // true
}

// === Tableaux dynamiques (lignes 68-74) ===
void exemple_tableau() {
    std::print("\n=== Tableau dynamique ===\n");
    auto tab = std::make_unique<int[]>(100);
    tab[0] = 10;
    tab[99] = 42;
    std::print("tab[0] = {}, tab[99] = {}\n", tab[0], tab[99]);  // 10, 42
}

// === Accéder à la ressource (lignes 97-106) ===
void exemple_acces() {
    std::print("\n=== Accès ===\n");
    auto p = std::make_unique<std::string>("Hello, C++");
    std::string& ref = *p;
    std::print("{}\n", *p);              // Hello, C++
    std::print("Taille : {}\n", p->size());   // 10
    std::print("Vide ? {}\n", p->empty());    // false
    std::print("ref = {}\n", ref);            // Hello, C++
}

// === get() (lignes 117-125) ===
void exemple_get() {
    std::print("\n=== get() ===\n");
    auto p = std::make_unique<int>(42);
    int* raw = p.get();
    std::print("Valeur : {}\n", *raw);   // 42
    std::print("Adresse : {}\n", static_cast<void*>(raw));
    std::print("Même adresse ? {}\n", (raw == p.get()));  // true
}

// === Vérifier la nullité (lignes 156-178) ===
void exemple_nullite() {
    std::print("\n=== Nullité ===\n");
    auto p = std::make_unique<int>(42);

    if (p) {
        std::print("p possède une ressource : {}\n", *p);  // 42
    }
    if (p != nullptr) {
        std::print("p n'est pas nul\n");
    }

    auto q = std::move(p);
    if (!p) {
        std::print("p est maintenant vide\n");
    }
    if (q) {
        std::print("q possède la ressource : {}\n", *q);  // 42
    }
}

// === reset() (lignes 193-203) ===
void exemple_reset() {
    std::print("\n=== reset() ===\n");
    auto p = std::make_unique<std::string>("Premier");
    std::print("Avant reset : {}\n", *p);

    p.reset(new std::string("Deuxième"));
    std::print("Après reset(new) : {}\n", *p);

    p.reset();
    std::print("Après reset() : p nul ? {}\n", (p == nullptr));

    p = nullptr;  // Équivalent à reset()
    std::print("Après p = nullptr : p nul ? {}\n", (p == nullptr));
}

// === release() (lignes 230-239) ===
void exemple_release() {
    std::print("\n=== release() ===\n");
    auto p = std::make_unique<int>(42);

    int* raw = p.release();
    std::print("p nul après release ? {}\n", (p == nullptr));  // true
    std::print("raw = {}\n", *raw);  // 42

    delete raw;  // Obligation de l'appelant
    std::print("raw libéré manuellement\n");
}

// === Interaction avec les fonctions (lignes 336-348) ===
struct Widget {
    int id;
    explicit Widget(int i) : id(i) {}
    std::string nom() const { return "Widget#" + std::to_string(id); }
};

std::unique_ptr<Widget> creer_widget(int id) {
    return std::make_unique<Widget>(id);
}

void afficher_widget(const Widget& w) {
    std::print("Widget: {}\n", w.nom());
}

void exemple_fonctions() {
    std::print("\n=== Interaction avec fonctions ===\n");
    auto widget = creer_widget(1);
    afficher_widget(*widget);  // Widget: Widget#1
}

int main() {
    exemple_creation();
    exemple_construction_directe();
    exemple_nul();
    exemple_tableau();
    exemple_acces();
    exemple_get();
    exemple_nullite();
    exemple_reset();
    exemple_release();
    exemple_fonctions();
    std::print("\n✅ Tous les exemples passés\n");
}
