/* ============================================================================
   Section 9.2.1 : Comptage de références
   Description : use_count(), opérations incrémentant/décrémentant, cycle de vie
   Fichier source : 02.1-comptage-references.md
   ============================================================================ */
#include <memory>
#include <print>
#include <string>

// === Visualiser use_count() (lignes 22-40) ===
void exemple_use_count() {
    std::print("=== use_count() ===\n");
    auto a = std::make_shared<std::string>("Bonjour");
    std::print("Après création :      use_count = {}\n", a.use_count());  // 1

    {
        auto b = a;
        std::print("Après copie (b) :     use_count = {}\n", a.use_count());  // 2

        auto c = a;
        std::print("Après copie (c) :     use_count = {}\n", a.use_count());  // 3
    }

    std::print("Après destruction b,c: use_count = {}\n", a.use_count());  // 1

    a.reset();
    std::print("Après reset :         a nul ? {}\n", (a == nullptr));  // true
}

// === Opérations qui incrémentent (lignes 117-128) ===
void exemple_increment() {
    std::print("\n=== Incrémentations ===\n");
    auto a = std::make_shared<int>(42);
    std::print("Création :            use_count = {}\n", a.use_count());  // 1

    auto b = a;
    std::print("Copie b=a :           use_count = {}\n", a.use_count());  // 2

    auto c(a);
    std::print("Copie c(a) :          use_count = {}\n", a.use_count());  // 3

    std::shared_ptr<int> d;
    d = a;
    std::print("Affectation d=a :     use_count = {}\n", a.use_count());  // 4

    // Promotion weak → shared
    std::weak_ptr<int> weak = a;
    auto locked = weak.lock();
    std::print("Après lock() :        use_count = {}\n", a.use_count());  // 5
}

// === Opérations qui décrémentent (lignes 132-143) ===
void exemple_decrement() {
    std::print("\n=== Décrémentations ===\n");
    auto a = std::make_shared<int>(42);
    auto b = a;
    auto c = a;
    auto d = a;
    std::print("Départ :              use_count = {}\n", a.use_count());  // 4

    b.reset();
    std::print("Après b.reset() :     use_count = {}\n", a.use_count());  // 3

    c = nullptr;
    std::print("Après c=nullptr :     use_count = {}\n", a.use_count());  // 2

    {
        auto temp = a;
        std::print("Dans scope temp :     use_count = {}\n", a.use_count());  // 3
    }
    std::print("Après scope temp :    use_count = {}\n", a.use_count());  // 2
}

// === Opérations neutres (lignes 147-157) ===
void exemple_neutre() {
    std::print("\n=== Opérations neutres ===\n");
    auto a = std::make_shared<int>(42);
    std::print("Départ :              use_count = {}\n", a.use_count());  // 1

    auto e = std::move(a);
    std::print("Après move :          e.use_count = {}\n", e.use_count());  // 1
    std::print("a nul ? {}\n", (a == nullptr));  // true

    int* raw = e.get();
    std::print("Après get() :         use_count = {}\n", e.use_count());  // 1
    std::print("*raw = {}\n", *raw);

    std::weak_ptr<int> w = e;
    std::print("Après weak_ptr :      use_count = {}\n", e.use_count());  // 1
}

// === Cycle de vie complet — Sensor (lignes 317-367) ===
struct Sensor {
    std::string nom;
    Sensor(std::string n) : nom(std::move(n)) {
        std::print("[+] Sensor '{}' créé\n", nom);
    }
    ~Sensor() {
        std::print("[-] Sensor '{}' détruit\n", nom);
    }
};

void exemple_sensor() {
    std::print("\n=== Cycle de vie Sensor ===\n");

    std::print("--- Étape 1 : Création ---\n");
    auto s1 = std::make_shared<Sensor>("Température");
    std::print("use_count = {}\n\n", s1.use_count());  // 1

    std::print("--- Étape 2 : Copie ---\n");
    auto s2 = s1;
    std::print("use_count = {}\n\n", s1.use_count());  // 2

    std::print("--- Étape 3 : Scope interne ---\n");
    {
        auto s3 = s1;
        std::print("use_count (dans le scope) = {}\n", s1.use_count());  // 3
    }
    std::print("use_count (après le scope) = {}\n\n", s1.use_count());  // 2

    std::print("--- Étape 4 : Move ---\n");
    auto s4 = std::move(s2);
    std::print("s2 est nul ? {}\n", (s2 == nullptr));  // true
    std::print("use_count = {}\n\n", s1.use_count());  // 2

    std::print("--- Étape 5 : Reset de s1 ---\n");
    s1.reset();
    std::print("s1 est nul ? {}\n", (s1 == nullptr));  // true
    std::print("use_count via s4 = {}\n\n", s4.use_count());  // 1

    std::print("--- Étape 6 : Fin ---\n");
}

int main() {
    exemple_use_count();
    exemple_increment();
    exemple_decrement();
    exemple_neutre();
    exemple_sensor();
    std::print("\n✅ Tous les exemples passés\n");
}
