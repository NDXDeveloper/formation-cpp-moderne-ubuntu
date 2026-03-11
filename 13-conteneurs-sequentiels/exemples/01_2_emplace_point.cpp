/* ============================================================================
   Section 13.1.2 : emplace à position arbitraire
   Description : Construction en place d'un objet Point à une position
                 spécifique dans le vecteur
   Fichier source : 01.2-methodes-essentielles.md
   ============================================================================ */
#include <vector>
#include <string>
#include <print>

struct Point {
    double x, y;
    Point(double x, double y) : x(x), y(y) {}
};

int main() {
    std::vector<Point> points{{1.0, 2.0}, {5.0, 6.0}};

    // Construit un Point(3.0, 4.0) en place à la position 1
    points.emplace(points.begin() + 1, 3.0, 4.0);

    for (const auto& p : points) {
        std::println("({}, {})", p.x, p.y);
    }
    // (1, 2)
    // (3, 4)
    // (5, 6)
}
