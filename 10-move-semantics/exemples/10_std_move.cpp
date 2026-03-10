/* ============================================================================
   Section 10.2 : std::move — Transfert de propriété sans copie
   Description : Modèle mental de std::move, transfert unique_ptr, Service
                 pass-by-value+move, push_back, move sur const, move sur
                 primitifs, conteneurs, std::swap, algorithme std::move,
                 emplace_back, classe Connection
   Fichier source : 02-std-move.md
   ============================================================================ */
#include <print>
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <algorithm>
#include <utility>

// === Modèle mental (lignes 29-38) ===
void test_modele_mental() {
    std::print("=== Modèle mental ===\n");
    std::string s = "Hello";

    std::string&& ref = std::move(s);
    // s est toujours "Hello" — personne n'a encore pris les ressources
    std::print("Après move sans récepteur: s = '{}'\n", s);

    std::string t = std::move(s);
    // MAINTENANT s est vidé
    std::print("Après move avec récepteur: s = '{}', t = '{}'\n", s, t);
}

// === Transfert unique_ptr (lignes 52-58) ===
struct Config {
    std::string filename;
    explicit Config(const std::string& f) : filename(f) {}
};

void enregistrer(std::unique_ptr<Config> config) {
    std::print("Config enregistrée: {}\n", config->filename);
}

void test_unique_ptr_transfer() {
    std::print("\n=== Transfert unique_ptr ===\n");
    auto config = std::make_unique<Config>("prod.yaml");
    enregistrer(std::move(config));
    std::print("config est nul: {}\n", config == nullptr);
}

// === Service: pass by value + move (lignes 66-76) ===
struct Logger {
    void log(const std::string& msg) const { std::print("[LOG] {}\n", msg); }
};

class Service {
    std::string nom_;
    std::unique_ptr<Logger> logger_;

public:
    Service(std::string nom, std::unique_ptr<Logger> logger)
        : nom_(std::move(nom))
        , logger_(std::move(logger))
    {}
    void info() const { std::print("Service: {}\n", nom_); }
};

void test_service() {
    std::print("\n=== Service ===\n");
    Service svc("MonService", std::make_unique<Logger>());
    svc.info();
}

// === push_back avec move (lignes 84-88) ===
void test_push_back() {
    std::print("\n=== push_back avec move ===\n");
    std::vector<std::string> noms;
    std::string s = "un nom très long qui dépasse le small string optimization";

    noms.push_back(s);              // Copie
    std::print("Après copie: s = '{}'\n", s);
    noms.push_back(std::move(s));   // Déplacement
    std::print("Après move: s = '{}'\n", s);
    std::print("noms.size() = {}\n", noms.size());
}

// === Move sur const → copie silencieuse (lignes 137-141) ===
void test_move_const() {
    std::print("\n=== Move sur const ===\n");
    const std::string s = "Hello";
    std::string t = std::move(s);  // COPIE, pas déplacement
    std::print("s après move const = '{}'\n", s);  // s toujours "Hello"
    std::print("t = '{}'\n", t);
}

// === Move sur types primitifs (lignes 180-182) ===
void test_move_primitif() {
    std::print("\n=== Move sur primitif ===\n");
    int x = 42;
    int y = std::move(x);
    std::print("x = {}, y = {}\n", x, y);  // x vaut toujours 42
}

// === Conteneurs (lignes 207-216) ===
void test_conteneurs() {
    std::print("\n=== Conteneurs ===\n");
    std::vector<int> v1(1'000'000, 42);
    std::vector<int> v2 = std::move(v1);
    std::print("v2.size() = {}\n", v2.size());
    std::print("v1.size() = {}\n", v1.size());

    std::map<std::string, int> m1 = {{"a", 1}, {"b", 2}};
    std::map<std::string, int> m2 = std::move(m1);
    std::print("m2.size() = {}\n", m2.size());
    std::print("m1.size() = {}\n", m1.size());
}

// === std::swap (lignes 224-232) ===
void test_swap() {
    std::print("\n=== std::swap ===\n");
    std::vector<int> a(1'000'000);
    std::vector<int> b(2'000'000);

    std::swap(a, b);
    std::print("a.size() = {}\n", a.size());  // 2'000'000
    std::print("b.size() = {}\n", b.size());  // 1'000'000
}

// === Algorithme std::move (lignes 240-246) ===
void test_algo_move() {
    std::print("\n=== Algorithme std::move ===\n");
    std::vector<std::string> source = {"alpha", "beta", "gamma"};
    std::vector<std::string> destination(3);

    std::move(source.begin(), source.end(), destination.begin());
    std::print("destination: ");
    for (const auto& s : destination) std::print("'{}' ", s);
    std::print("\n");
    std::print("source: ");
    for (const auto& s : source) std::print("'{}' ", s);
    std::print("\n");
}

// === push_back vs emplace_back (lignes 256-264) ===
void test_emplace() {
    std::print("\n=== push_back vs emplace_back ===\n");
    std::vector<std::string> vec;
    std::string s = "une chaîne longue pour éviter le SSO";

    vec.push_back(s);              // Copie
    vec.push_back(std::move(s));   // Déplacement
    vec.emplace_back("construction in-place");

    std::print("vec.size() = {}\n", vec.size());
    for (const auto& v : vec) std::print("  '{}'\n", v);
}

// === Connection (lignes 297-316) ===
class Connection {
    int socket_fd_;
    std::string host_;

public:
    Connection(int fd, std::string host) : socket_fd_(fd), host_(std::move(host)) {}

    Connection(Connection&& other) noexcept
        : socket_fd_(other.socket_fd_)
        , host_(std::move(other.host_))
    {
        other.socket_fd_ = -1;
    }

    ~Connection() {
        if (socket_fd_ >= 0) {
            std::print("  Fermeture connexion fd={}\n", socket_fd_);
        }
    }

    void info() const {
        std::print("  Connection(fd={}, host='{}')\n", socket_fd_, host_);
    }
};

void test_connection() {
    std::print("\n=== Connection ===\n");
    Connection c1(42, "localhost");
    c1.info();

    Connection c2 = std::move(c1);
    std::print("Après move:\n");
    c1.info();  // fd=-1, host vide
    c2.info();  // fd=42, host="localhost"
}

int main() {
    test_modele_mental();
    test_unique_ptr_transfer();
    test_service();
    test_push_back();
    test_move_const();
    test_move_primitif();
    test_conteneurs();
    test_swap();
    test_algo_move();
    test_emplace();
    test_connection();
    std::print("\n✅ Tous les exemples passés\n");
}
