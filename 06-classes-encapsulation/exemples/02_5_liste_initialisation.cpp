/* ============================================================================
   Section 6.2.5 : Liste d'initialisation
   Description : Member initializer list — référence, héritage, piège de
                 l'ordre d'initialisation, interaction avec default member
                 initializers, piège {} vs () pour vector
   Fichier source : 02.5-liste-initialisation.md
   ============================================================================ */
#include <iostream>
#include <string>
#include <vector>
#include <cstdint>

// --- Logger : référence (obligatoire en init list) ---
class Logger {
public:
    explicit Logger(std::ostream& output)
        : output_(output) {}

    void log(const std::string& msg) {
        output_ << msg << "\n";
    }

private:
    std::ostream& output_;
};

// --- Server : default member initializers + init list ---
class Server {
public:
    Server(const std::string& host, uint16_t port, int timeout)
        : host_(host), port_(port), timeout_ms_(timeout) {}

    explicit Server(const std::string& host)
        : host_(host) {}
        // port_ → 8080, timeout_ms_ → 5000 (default member initializers)

    const std::string& host() const { return host_; }
    uint16_t port() const { return port_; }
    int timeout_ms() const { return timeout_ms_; }

private:
    std::string host_;
    uint16_t port_ = 8080;
    int timeout_ms_ = 5000;
};

// --- Container : piège {} vs () ---
class Container {
public:
    Container()
        : v1_(5, 0)      // vector de 5 éléments valant 0
        , v2_{5, 0} {}    // vector initialisé avec {5, 0} → 2 éléments

    std::size_t v1_size() const { return v1_.size(); }
    std::size_t v2_size() const { return v2_.size(); }

private:
    std::vector<int> v1_;
    std::vector<int> v2_;
};

// --- Correct : ordre de déclaration respecté ---
class Correct {
public:
    Correct(int n)
        : unit_size_(64)
        , total_(n * unit_size_) {}

    int total() const { return total_; }
    int unit_size() const { return unit_size_; }

private:
    int unit_size_;    // Déclaré en premier → initialisé en premier
    int total_;        // Déclaré en second → peut utiliser unit_size_
};

// --- Animal/Dog : init list et héritage ---
class Animal {
public:
    Animal(const std::string& name, int legs)
        : name_(name), legs_(legs) {}
    const std::string& name() const { return name_; }
    int legs() const { return legs_; }
protected:
    std::string name_;
    int legs_;
};

class Dog : public Animal {
public:
    Dog(const std::string& name, const std::string& breed)
        : Animal(name, 4)
        , breed_(breed) {}
    const std::string& breed() const { return breed_; }
private:
    std::string breed_;
};

int main() {
    // Logger
    Logger console_logger(std::cout);
    console_logger.log("Application started");

    // Server
    Server s1("api.example.com", 443, 10000);
    Server s2("localhost");
    std::cout << "s1: " << s1.host() << ":" << s1.port()
              << " t=" << s1.timeout_ms() << "\n";
    std::cout << "s2: " << s2.host() << ":" << s2.port()
              << " t=" << s2.timeout_ms() << "\n";

    // Container {} vs ()
    Container c;
    std::cout << "v1 size: " << c.v1_size() << "\n";   // 5
    std::cout << "v2 size: " << c.v2_size() << "\n";   // 2

    // Correct order
    Correct corr(10);
    std::cout << "total: " << corr.total() << "\n";   // 640

    // Dog inheritance
    Dog d("Rex", "German Shepherd");
    std::cout << d.name() << " (" << d.breed() << "), "
              << d.legs() << " legs\n";

    return 0;
}
// Sortie attendue :
// Application started
// s1: api.example.com:443 t=10000
// s2: localhost:8080 t=5000
// v1 size: 5
// v2 size: 2
// total: 640
// Rex (German Shepherd), 4 legs
