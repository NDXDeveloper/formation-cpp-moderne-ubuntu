/* ============================================================================
   Section 9.4 : Ne jamais utiliser new/delete dans du code moderne
   Description : Alternatives modernes, Singleton, placement new, Engine RAII
   Fichier source : 04-jamais-new-delete.md
   ============================================================================ */
#include <memory>
#include <print>
#include <string>
#include <vector>
#include <cstdio>
#include <new>

// === Stack vs heap (lignes 33-41) ===
struct Config {
    std::string filename;
    explicit Config(const std::string& f) : filename(f) {}
};

void traiter(const Config& c) {
    std::print("Config: {}\n", c.filename);
}

void exemple_stack_vs_heap() {
    std::print("=== Stack vs heap ===\n");
    // ❌ Allocation dynamique inutile
    auto config = std::make_unique<Config>("app.yaml");
    traiter(*config);

    // ✅ Variable locale
    Config config2("app.yaml");
    traiter(config2);
}

// === unique_ptr vs new (lignes 50-57) ===
struct Widget {
    int value;
    explicit Widget(int v) : value(v) {
        std::print("Widget({}) créé\n", v);
    }
    ~Widget() { std::print("Widget({}) détruit\n", value); }
};

void exemple_unique_ptr() {
    std::print("\n=== unique_ptr vs new ===\n");
    auto w = std::make_unique<Widget>(42);
    std::print("w->value = {}\n", w->value);
}

// === shared_ptr — propriété partagée (lignes 64-76) ===
void exemple_shared_ptr() {
    std::print("\n=== shared_ptr ===\n");
    auto shared_w = std::make_shared<Widget>(99);
    std::print("use_count: {}\n", shared_w.use_count());
    {
        auto copy = shared_w;
        std::print("use_count après copie: {}\n", shared_w.use_count());
    }
    std::print("use_count après destruction copie: {}\n", shared_w.use_count());
}

// === vector vs new[] (lignes 83-91) ===
void exemple_vector() {
    std::print("\n=== vector vs new[] ===\n");
    std::vector<int> tab(1000, 0);
    std::print("tab.size() = {}\n", tab.size());
}

// === Collection polymorphique (lignes 97-110) ===
class Animal {
public:
    virtual ~Animal() = default;
    virtual void parler() const = 0;
};
class Chat : public Animal {
public: void parler() const override { std::print("Miaou\n"); }
};
class Chien : public Animal {
public: void parler() const override { std::print("Wouf\n"); }
};

void exemple_polymorphisme() {
    std::print("\n=== Collection polymorphique ===\n");
    std::vector<std::unique_ptr<Animal>> zoo;
    zoo.push_back(std::make_unique<Chat>());
    zoo.push_back(std::make_unique<Chien>());
    for (const auto& a : zoo) {
        a->parler();
    }
}

// === Custom deleter FILE (lignes 116-129) ===
void exemple_file() {
    std::print("\n=== Custom deleter FILE ===\n");
    auto f = std::unique_ptr<FILE, decltype(&fclose)>(
        fopen("/tmp/test_09_jamais.txt", "w"), fclose
    );
    if (f) {
        std::fputs("Hello RAII\n", f.get());
        std::print("Fichier écrit avec succès\n");
    }
}

// === Singleton — constructeur privé (lignes 167-175) ===
class Singleton {
    Singleton() = default;
public:
    static std::unique_ptr<Singleton> creer() {
        return std::unique_ptr<Singleton>(new Singleton());
    }
    void hello() const { std::print("Singleton actif\n"); }
};

void exemple_singleton() {
    std::print("\n=== Singleton ===\n");
    auto s = Singleton::creer();
    s->hello();
}

// === Placement new (lignes 182-186) ===
void exemple_placement_new() {
    std::print("\n=== Placement new ===\n");
    alignas(Widget) char buffer[sizeof(Widget)];
    Widget* w = new (buffer) Widget(42);
    std::print("w->value = {}\n", w->value);
    w->~Widget();  // Destruction explicite obligatoire
}

// === Engine — lisibilité avec smart pointers (lignes 310-318) ===
struct Renderer {
    ~Renderer() { std::print("Renderer détruit\n"); }
};
struct Logger {
    ~Logger() { std::print("Logger détruit\n"); }
};

class Engine {
    std::unique_ptr<Renderer> renderer_;
    std::shared_ptr<Logger> logger_;
    Config& config_;
public:
    Engine(std::unique_ptr<Renderer> r, std::shared_ptr<Logger> l, Config& c)
        : renderer_(std::move(r)), logger_(std::move(l)), config_(c) {}
    void info() const {
        std::print("Engine OK, config: {}\n", config_.filename);
    }
};

void exemple_engine() {
    std::print("\n=== Engine RAII ===\n");
    Config config{"MonApp"};
    auto logger = std::make_shared<Logger>();
    {
        Engine engine(std::make_unique<Renderer>(), logger, config);
        engine.info();
        std::print("Logger use_count: {}\n", logger.use_count());
    }
    std::print("Après destruction Engine, Logger use_count: {}\n", logger.use_count());
}

int main() {
    exemple_stack_vs_heap();
    exemple_unique_ptr();
    exemple_shared_ptr();
    exemple_vector();
    exemple_polymorphisme();
    exemple_file();
    exemple_singleton();
    exemple_placement_new();
    exemple_engine();
    std::print("\n✅ Tous les exemples passés\n");
}
