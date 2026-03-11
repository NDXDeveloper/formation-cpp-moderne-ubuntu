/* ============================================================================
   Section 13.2 : Structured bindings (C++17)
   Description : Décomposition d'un std::array en variables nommées avec
                 les structured bindings
   Fichier source : 02-array.md
   ============================================================================ */
#include <array>
#include <print>

std::array<double, 3> coordonnees_gps() {
    return {48.8566, 2.3522, 35.0};  // lat, lon, altitude
}

int main() {
    auto [lat, lon, alt] = coordonnees_gps();

    std::println("Latitude  : {}", lat);
    std::println("Longitude : {}", lon);
    std::println("Altitude  : {} m", alt);
}
