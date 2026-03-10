/* ============================================================================
   Section 6.1 : Définition de classes — Membres et méthodes
   Description : Membres static — compteur d'instances avec Connection,
                 méthode statique avec MathUtils::clamp
   Fichier source : 01-definition-classes.md
   ============================================================================ */
#include <iostream>

// --- Connection : donnée statique (compteur d'instances) ---
class Connection {
public:
    Connection() { ++active_count_; }
    ~Connection() { --active_count_; }

    static int active_count() { return active_count_; }

private:
    inline static int active_count_ = 0;  // C++17 — déclaration + définition
};

// --- MathUtils : méthode statique ---
class MathUtils {
public:
    static double clamp(double val, double lo, double hi) {
        if (val < lo) return lo;
        if (val > hi) return hi;
        return val;
    }
};

int main() {
    // Connection static counter
    std::cout << "Connections: " << Connection::active_count() << "\n";   // 0

    Connection c1;
    std::cout << "Connections: " << Connection::active_count() << "\n";   // 1

    {
        Connection c2;
        Connection c3;
        std::cout << "Connections: " << Connection::active_count() << "\n";   // 3
    }

    std::cout << "Connections: " << Connection::active_count() << "\n";   // 1

    // MathUtils::clamp
    double result = MathUtils::clamp(15.0, 0.0, 10.0);
    std::cout << "clamp(15, 0, 10) = " << result << "\n";   // 10

    return 0;
}
// Sortie attendue :
// Connections: 0
// Connections: 1
// Connections: 3
// Connections: 1
// clamp(15, 0, 10) = 10
