/* ============================================================================
   Section 6.2 : Constructeurs
   Description : Introduction aux constructeurs — Timestamp, génération
                 implicite, = default, = delete, explicit, syntaxes
                 d'initialisation
   Fichier source : 02-constructeurs.md
   ============================================================================ */
#include <iostream>
#include <string>
#include <vector>

// --- Timestamp : constructeur par défaut + paramétré ---
class Timestamp {
public:
    Timestamp() {
        seconds_since_epoch_ = 0;
    }

    Timestamp(long seconds) {
        seconds_since_epoch_ = seconds;
    }

    long seconds() const { return seconds_since_epoch_; }

private:
    long seconds_since_epoch_;
};

// --- Flexible : = default pour rétablir le constructeur par défaut ---
class Flexible {
public:
    Flexible() = default;
    Flexible(int value) : value_(value) {}
    int value() const { return value_; }
private:
    int value_ = 0;
};

// --- Distance : explicit bloque la conversion implicite ---
class Distance {
public:
    explicit Distance(double meters) : meters_(meters) {}
    double meters() const { return meters_; }
private:
    double meters_;
};

void print_distance(Distance d) {
    std::cout << d.meters() << " m\n";
}

// --- Point : syntaxes d'initialisation ---
class Point {
public:
    Point() : x_(0.0), y_(0.0) {}
    Point(double x, double y) : x_(x), y_(y) {}
    double x() const { return x_; }
    double y() const { return y_; }
private:
    double x_, y_;
};

int main() {
    // Timestamp
    Timestamp t1;
    Timestamp t2(1709078400);
    Timestamp t3{1709078400};
    std::cout << "t1: " << t1.seconds() << "\n";
    std::cout << "t2: " << t2.seconds() << "\n";
    std::cout << "t3: " << t3.seconds() << "\n";

    // Flexible = default
    Flexible a;
    Flexible b(42);
    std::cout << "a: " << a.value() << "\n";
    std::cout << "b: " << b.value() << "\n";

    // explicit
    // print_distance(3.5);          // ERREUR — conversion implicite interdite
    print_distance(Distance(3.5));   // OK

    // Syntaxes d'initialisation
    Point p1(3.0, 4.0);
    Point p2{3.0, 4.0};
    Point p4;
    std::cout << "p1: " << p1.x() << ", " << p1.y() << "\n";
    std::cout << "p2: " << p2.x() << ", " << p2.y() << "\n";
    std::cout << "p4: " << p4.x() << ", " << p4.y() << "\n";

    // Piège vector {} vs ()
    std::vector<int> v1(5, 0);    // 5 éléments valant 0
    std::vector<int> v2{5, 0};    // 2 éléments : 5 et 0
    std::cout << "v1 size: " << v1.size() << "\n";
    std::cout << "v2 size: " << v2.size() << "\n";

    return 0;
}
// Sortie attendue :
// t1: 0
// t2: 1709078400
// t3: 1709078400
// a: 0
// b: 42
// 3.5 m
// p1: 3, 4
// p2: 3, 4
// p4: 0, 0
// v1 size: 5
// v2 size: 2
