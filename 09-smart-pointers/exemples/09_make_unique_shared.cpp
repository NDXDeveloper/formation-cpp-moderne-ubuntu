/* ============================================================================
   Section 9.3 : std::make_unique et std::make_shared
   Description : Factory functions, exception safety, allocation unique, limites
   Fichier source : 03-make-unique-shared.md
   ============================================================================ */
#include <memory>
#include <print>
#include <string>
#include <vector>

// === Syntaxe de base (lignes 17-32) ===
void exemple_syntaxe() {
    std::print("=== Syntaxe de base ===\n");

    auto u1 = std::make_unique<int>(42);
    auto u2 = std::make_unique<std::string>("Hello");
    auto u3 = std::make_unique<std::vector<int>>(10, 0);

    auto s1 = std::make_shared<int>(42);
    auto s2 = std::make_shared<std::string>("Hello");
    auto s3 = std::make_shared<std::vector<int>>(10, 0);

    auto arr = std::make_unique<int[]>(100);

    std::print("u1 = {}\n", *u1);           // 42
    std::print("u2 = {}\n", *u2);           // Hello
    std::print("u3.size() = {}\n", u3->size());  // 10
    std::print("s1 = {}\n", *s1);           // 42
    std::print("s2 = {}\n", *s2);           // Hello
    std::print("s3.size() = {}\n", s3->size());  // 10
    std::print("arr[0] = {}\n", arr[0]);    // 0
}

// === Lisibilité : pas de répétition (lignes 197-203) ===
void exemple_lisibilite() {
    std::print("\n=== Lisibilité ===\n");

    // Type mentionné une seule fois
    auto p = std::make_shared<std::string>("prod.yaml");
    std::print("config = {}\n", *p);
}

// === Symétrie unique/shared (lignes 210-215) ===
struct Config {
    std::string filename;
    explicit Config(std::string f) : filename(std::move(f)) {}
};

void exemple_symetrie() {
    std::print("\n=== Symétrie unique/shared ===\n");

    auto config_u = std::make_unique<Config>("prod.yaml");
    std::print("unique: {}\n", config_u->filename);

    auto config_s = std::make_shared<Config>("prod.yaml");
    std::print("shared: {}\n", config_s->filename);
}

// === Constructeur privé — Singleton (lignes 242-255) ===
class Singleton {
    Singleton() = default;
public:
    static std::unique_ptr<Singleton> creer() {
        return std::unique_ptr<Singleton>(new Singleton());
    }
    void info() const { std::print("Singleton actif\n"); }
};

void exemple_singleton() {
    std::print("\n=== Constructeur privé ===\n");
    auto s = Singleton::creer();
    s->info();
}

// === initializer_list (lignes 264-284) ===
void exemple_initializer_list() {
    std::print("\n=== initializer_list ===\n");

    // Parenthèses : vector(size_t count, int value)
    auto v1 = std::make_shared<std::vector<int>>(10, 0);
    std::print("v1.size() = {}\n", v1->size());  // 10

    // initializer_list explicite
    auto init = std::initializer_list<int>{1, 2, 3, 4, 5};
    auto v = std::make_shared<std::vector<int>>(init);
    std::print("v = ");
    for (int x : *v) std::print("{} ", x);
    std::print("\n");  // 1 2 3 4 5

    // Construction directe avec {}
    auto v3 = std::shared_ptr<std::vector<int>>(
        new std::vector<int>{10, 20, 30}
    );
    std::print("v3 = ");
    for (int x : *v3) std::print("{} ", x);
    std::print("\n");  // 10 20 30
}

// === Compromis make_shared et weak_ptr (lignes 156-166) ===
void exemple_compromis() {
    std::print("\n=== Compromis make_shared + weak_ptr ===\n");

    auto shared = std::make_shared<std::string>("Ressource volumineuse");
    std::weak_ptr<std::string> weak = shared;

    std::print("Avant reset: use_count = {}\n", shared.use_count());  // 1
    std::print("expired ? {}\n", weak.expired());  // false

    shared.reset();
    std::print("Après reset: expired ? {}\n", weak.expired());  // true

    weak.reset();
    std::print("weak aussi reset\n");
}

int main() {
    exemple_syntaxe();
    exemple_lisibilite();
    exemple_symetrie();
    exemple_singleton();
    exemple_initializer_list();
    exemple_compromis();
    std::print("\n✅ Tous les exemples passés\n");
}
