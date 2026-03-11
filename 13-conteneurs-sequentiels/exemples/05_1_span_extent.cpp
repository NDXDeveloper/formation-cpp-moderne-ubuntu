/* ============================================================================
   Section 13.5.1 : extent et cas d'usage statiques
   Description : Inspection de extent avec if constexpr, span statique pour
                 cryptographie (clé AES 32 octets) et algèbre linéaire 3D
   Fichier source : 05.1-span-statique-dynamique.md
   ============================================================================ */
#include <span>
#include <vector>
#include <array>
#include <cstdint>
#include <cmath>
#include <print>

template <typename T, std::size_t E>
void info(std::span<T, E> s) {
    if constexpr (E == std::dynamic_extent) {
        std::println("Span DYNAMIQUE : size={}", s.size());
    } else {
        std::println("Span STATIQUE  : extent={}, size={}", E, s.size());
    }
}

double norme_3d(std::span<const double, 3> v) {
    return std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

double produit_scalaire_3d(std::span<const double, 3> a,
                           std::span<const double, 3> b) {
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

int main() {
    // info()
    {
        std::array<int, 4> arr{1, 2, 3, 4};
        std::vector<int> vec{1, 2, 3};
        info(std::span(arr));
        info(std::span(vec));
    }

    // Cryptographie
    {
        auto chiffrer = [](std::span<const std::uint8_t, 32> cle,
                           std::span<const std::uint8_t> donnees) {
            std::println("Clé de {} octets, {} octets à chiffrer",
                          cle.size(), donnees.size());
        };
        std::array<std::uint8_t, 32> ma_cle{};
        std::vector<std::uint8_t> message{0x01, 0x02, 0x03};
        chiffrer(ma_cle, message);
    }

    // Algèbre linéaire
    {
        std::array<double, 3> pos{3.0, 4.0, 0.0};
        std::array<double, 3> dir{1.0, 0.0, 0.0};
        std::println("Norme    : {}", norme_3d(pos));
        std::println("Scalaire : {}", produit_scalaire_3d(pos, dir));
    }
}
