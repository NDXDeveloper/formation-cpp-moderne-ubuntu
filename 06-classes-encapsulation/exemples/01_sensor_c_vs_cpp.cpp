/* ============================================================================
   Section 6.1 : Définition de classes — Membres et méthodes
   Description : Comparaison struct C (données/fonctions séparées) vs classe C++
                 (données et fonctions réunies avec encapsulation)
   Fichier source : 01-definition-classes.md
   ============================================================================ */
#include <iostream>

// --- Version C-style : données et fonctions séparées ---
namespace c_style {

struct Sensor {
    int id;
    double value;
};

void sensor_init(Sensor* s, int id) {
    s->id = id;
    s->value = 0.0;
}

void sensor_read(Sensor* s, double new_value) {
    s->value = new_value;
}

} // namespace c_style

// --- Version C++ : données et fonctions réunies ---
class Sensor {
public:
    void init(int id) {
        id_ = id;
        value_ = 0.0;
    }

    void read(double new_value) {
        value_ = new_value;
    }

    double value() const {
        return value_;
    }

private:
    int id_;
    double value_;
};

int main() {
    // Version C-style
    std::cout << "=== C-style Sensor ===\n";
    c_style::Sensor cs;
    c_style::sensor_init(&cs, 1);
    c_style::sensor_read(&cs, 42.5);
    std::cout << "Sensor " << cs.id << ": " << cs.value << "\n";

    // Version C++
    std::cout << "\n=== C++ Sensor ===\n";
    Sensor s;
    s.init(1);
    s.read(42.5);
    std::cout << "Sensor value: " << s.value() << "\n";

    return 0;
}
// Sortie attendue :
// === C-style Sensor ===
// Sensor 1: 42.5
//
// === C++ Sensor ===
// Sensor value: 42.5
