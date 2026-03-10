/* ============================================================================
   Section 3.5.1 : const - Immutabilite a l'execution
   Description : Variables const, pointeurs const, references const,
                 Sensor avec methodes const, const par defaut
   Fichier source : 05.1-const.md
   ============================================================================ */
#include <print>
#include <string>
#include <vector>

// --- Sensor (lignes 226-243) ---
class Sensor {
    std::string name_;
    double last_reading_;
public:
    Sensor(std::string name, double reading) : name_(std::move(name)), last_reading_(reading) {}
    const std::string& name() const { return name_; }
    double last_reading() const { return last_reading_; }
    void update(double reading) { last_reading_ = reading; }
};

void display(const Sensor& s) {
    std::print("{}: {}\n", s.name(), s.last_reading());
}

void display_text(const std::string& text) {
    std::print("{}\n", text);
}

int main() {
    // --- Variables const (lignes 25-29) ---
    const int max_attempts = 5;
    const double pi = 3.14159265358979;
    std::print("max_attempts={}, pi={}\n", max_attempts, pi);

    // --- Initialisation obligatoire (lignes 39-41) ---
    const int y = 42;
    const int z{};
    std::print("y={}, z={}\n", y, z);

    // --- Pointeur vers donnee constante (lignes 57-66) ---
    int x = 10;
    int yy = 20;
    const int* ptr = &x;
    std::print("{}\n", *ptr); // 10
    ptr = &yy;
    std::print("{}\n", *ptr); // 20

    // --- Pointeur constant vers donnee mutable (lignes 76-82) ---
    int xx = 10;
    int* const ptr2 = &xx;
    *ptr2 = 99;
    std::print("{}\n", xx); // 99

    // --- const pointer const (lignes 94-98) ---
    const int* const ptr3 = &xx;
    std::print("{}\n", *ptr3);

    // --- References const (lignes 128-134) ---
    std::string message = "Bonjour";
    display_text(message);

    // --- const parameter par valeur (lignes 195-198) ---
    auto square = [](const int n) { return n * n; };
    std::print("square(5)={}\n", square(5));

    // --- Sensor (lignes 226-243) ---
    Sensor s("Temp", 22.5);
    display(s);
    s.update(23.1);
    display(s);

    // --- const global linkage (lignes 257-260) ---
    const int max_connections = 100;
    std::print("max_connections={}\n", max_connections);

    // --- const vs #define (lignes 283-293) ---
    const int max_buffer_size = 4096;
    constexpr double pi_cx = 3.14159;
    std::print("buffer={}, pi={}\n", max_buffer_size, pi_cx);

    // --- const ne signifie pas compile-time (lignes 307-312) ---
    constexpr int fixed_size = 100;
    int arr[fixed_size];
    arr[0] = 42;
    std::print("arr[0]={}\n", arr[0]);

    // --- const par defaut (lignes 359-368) ---
    const auto data = std::vector<int>{1, 2, 3, 4, 5};
    const auto size = data.size();
    std::print("size={}\n", size);

    auto accumulator = 0.0;
    for (const auto& value : data) {
        accumulator += value;
    }
    std::print("accumulator={}\n", accumulator);

    return 0;
}
