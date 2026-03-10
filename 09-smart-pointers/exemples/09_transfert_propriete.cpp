/* ============================================================================
   Section 9.1.2 : Transfert de propriété avec std::move
   Description : Logger, factory, conteneurs, polymorphisme, état après move
   Fichier source : 01.2-transfert-propriete.md
   ============================================================================ */
#include <memory>
#include <print>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <numbers>

// === Logger — passage par valeur (lignes 36-66) ===
class Logger {
    std::string nom_;
public:
    explicit Logger(std::string nom) : nom_(std::move(nom)) {}
    void log(const std::string& msg) {
        std::print("[{}] {}\n", nom_, msg);
    }
};

void enregistrer_logger(std::unique_ptr<Logger> logger) {
    logger->log("Enregistré avec succès");
}

void exemple_logger() {
    std::print("=== Logger (transfert par valeur) ===\n");
    auto logger = std::make_unique<Logger>("App");
    logger->log("Avant transfert");
    enregistrer_logger(std::move(logger));
    if (!logger) {
        std::print("logger a été transféré\n");
    }
}

// === Factory pattern (lignes 146-175) ===
class Animal {
public:
    virtual ~Animal() = default;
    virtual std::string cri() const = 0;
};

class Chat : public Animal {
public:
    std::string cri() const override { return "Miaou"; }
};

class Chien : public Animal {
public:
    std::string cri() const override { return "Wouf"; }
};

class Perroquet : public Animal {
public:
    std::string cri() const override { return "Coco"; }
};

std::unique_ptr<Animal> creer_animal(std::string_view type) {
    if (type == "chat")  return std::make_unique<Chat>();
    if (type == "chien") return std::make_unique<Chien>();
    return nullptr;
}

void exemple_factory() {
    std::print("\n=== Factory pattern ===\n");
    auto animal = creer_animal("chat");
    if (animal) {
        std::print("{}\n", animal->cri());  // Miaou
    }

    auto inconnu = creer_animal("poisson");
    std::print("inconnu nul ? {}\n", (inconnu == nullptr));  // true
}

// === Conteneurs (lignes 188-221) ===
void exemple_vector() {
    std::print("\n=== Vector de unique_ptr ===\n");
    std::vector<std::unique_ptr<Animal>> zoo;

    zoo.push_back(std::make_unique<Chat>());
    zoo.push_back(std::make_unique<Chien>());

    auto perroquet = std::make_unique<Perroquet>();
    zoo.push_back(std::move(perroquet));

    for (const auto& animal : zoo) {
        std::print("{}\n", animal->cri());
    }

    // Extraction du dernier élément
    auto dernier = std::move(zoo.back());
    zoo.pop_back();
    std::print("Extrait : {}\n", dernier->cri());  // Coco
    std::print("zoo.size() = {}\n", zoo.size());    // 2
}

// === Map (lignes 227-246) ===
struct Config {
    std::string filename;
    int port;
    explicit Config(std::string f, int p = 8080) : filename(std::move(f)), port(p) {}
};

void exemple_map() {
    std::print("\n=== Map de unique_ptr ===\n");
    std::map<std::string, std::unique_ptr<Config>> configurations;

    configurations["dev"]  = std::make_unique<Config>("dev.yaml", 3000);
    configurations["prod"] = std::make_unique<Config>("prod.yaml", 443);

    const auto& config_dev = configurations["dev"];
    if (config_dev) {
        std::print("Port dev: {}\n", config_dev->port);
    }

    // Extraction avec extract() (C++17)
    auto node = configurations.extract("dev");
    if (!node.empty()) {
        auto config = std::move(node.mapped());
        std::print("Extrait: {} (port {})\n", config->filename, config->port);
    }
    std::print("configurations.size() = {}\n", configurations.size());  // 1
}

// === Polymorphisme — Shape (lignes 255-288) ===
class Shape {
public:
    virtual ~Shape() = default;
    virtual double aire() const = 0;
};

class Cercle : public Shape {
    double rayon_;
public:
    explicit Cercle(double r) : rayon_(r) {}
    double aire() const override { return std::numbers::pi * rayon_ * rayon_; }
};

class Rectangle : public Shape {
    double largeur_, hauteur_;
public:
    Rectangle(double l, double h) : largeur_(l), hauteur_(h) {}
    double aire() const override { return largeur_ * hauteur_; }
};

void exemple_polymorphisme() {
    std::print("\n=== Polymorphisme Shape ===\n");
    std::unique_ptr<Shape> s = std::make_unique<Cercle>(5.0);
    std::print("Aire cercle(5) : {:.2f}\n", s->aire());

    std::vector<std::unique_ptr<Shape>> formes;
    formes.push_back(std::make_unique<Cercle>(3.0));
    formes.push_back(std::make_unique<Rectangle>(4.0, 5.0));

    double total = 0.0;
    for (const auto& forme : formes) {
        total += forme->aire();
    }
    std::print("Aire totale : {:.2f}\n", total);
}

// === État après déplacement (lignes 301-312) ===
void exemple_apres_move() {
    std::print("\n=== État après move ===\n");
    auto p = std::make_unique<int>(42);
    auto q = std::move(p);

    if (p == nullptr) {
        std::print("p est nul après move\n");
    }

    p = std::make_unique<int>(100);  // Réaffectation
    std::print("p réaffecté : {}\n", *p);  // 100
    std::print("q : {}\n", *q);            // 42
}

int main() {
    exemple_logger();
    exemple_factory();
    exemple_vector();
    exemple_map();
    exemple_polymorphisme();
    exemple_apres_move();
    std::print("\n✅ Tous les exemples passés\n");
}
