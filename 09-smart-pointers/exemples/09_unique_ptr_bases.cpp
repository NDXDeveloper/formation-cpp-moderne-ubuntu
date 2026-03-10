/* ============================================================================
   Section 9.1 : std::unique_ptr — Possession exclusive
   Description : Bases de unique_ptr — création, move, sizeof, polymorphisme
   Fichier source : 01-unique-ptr.md
   ============================================================================ */
#include <memory>
#include <print>
#include <string>
#include <array>

// === Exemple basique (lignes 14-21) ===
void exemple_basique() {
    std::print("=== Exemple basique ===\n");
    std::unique_ptr<int> p = std::make_unique<int>(42);
    std::print("{}\n", *p);  // 42
}

// === Copie interdite, move autorisé (lignes 32-46) ===
void exemple_move() {
    std::print("\n=== Move ===\n");
    std::unique_ptr<int> a = std::make_unique<int>(42);
    // std::unique_ptr<int> b = a;  // ❌ Erreur de compilation
    std::unique_ptr<int> b = std::move(a);  // ✅ Transfert de propriété
    std::print("a est nul ? {}\n", (a == nullptr));  // true
    std::print("*b = {}\n", *b);  // 42
}

// === Zéro coût — sizeof (lignes 67-75) ===
void exemple_sizeof() {
    std::print("\n=== sizeof ===\n");
    std::print("sizeof(int*)              = {}\n", sizeof(int*));              // 8
    std::print("sizeof(unique_ptr<int>)   = {}\n", sizeof(std::unique_ptr<int>));  // 8
}

// === Polymorphisme (lignes 103-125) ===
class Animal {
public:
    virtual ~Animal() = default;
    virtual void parler() const = 0;
};

class Chat : public Animal {
public:
    void parler() const override { std::print("Miaou\n"); }
};

class Chien : public Animal {
public:
    void parler() const override { std::print("Wouf\n"); }
};

std::unique_ptr<Animal> creer_animal(const std::string& choix) {
    if (choix == "chat") return std::make_unique<Chat>();
    if (choix == "chien") return std::make_unique<Chien>();
    return nullptr;
}

void exemple_polymorphisme() {
    std::print("\n=== Polymorphisme ===\n");
    auto a1 = creer_animal("chat");
    auto a2 = creer_animal("chien");
    if (a1) a1->parler();  // Miaou
    if (a2) a2->parler();  // Wouf
}

// === GrosBuffer (lignes 130-138) ===
struct GrosBuffer {
    std::array<char, 1024 * 1024> data;  // 1 Mo
};

std::unique_ptr<GrosBuffer> allouer_buffer() {
    return std::make_unique<GrosBuffer>();
}

void exemple_gros_buffer() {
    std::print("\n=== GrosBuffer ===\n");
    auto buf = allouer_buffer();
    std::print("Buffer alloué, taille: {} octets\n", buf->data.size());
}

// === Voiture/Moteur — composition (lignes 143-152) ===
class Moteur {
public:
    void demarrer() const { std::print("Moteur démarre\n"); }
};

class Voiture {
    std::unique_ptr<Moteur> moteur_;
public:
    Voiture() : moteur_(std::make_unique<Moteur>()) {}
    void rouler() const { moteur_->demarrer(); }
};

void exemple_composition() {
    std::print("\n=== Composition Voiture/Moteur ===\n");
    Voiture v;
    v.rouler();
}

int main() {
    exemple_basique();
    exemple_move();
    exemple_sizeof();
    exemple_polymorphisme();
    exemple_gros_buffer();
    exemple_composition();
    std::print("\n✅ Tous les exemples passés\n");
}
