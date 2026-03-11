/* ============================================================================
   Section 13.5.3 : Interopérabilité conteneurs → span
   Description : Conversions depuis vector, array, tableau C, pointeur+taille ;
                 conversions de mutabilité ; span comme passerelle entre
                 conteneurs (produit scalaire) ; string/string_view
   Fichier source : 05.3-interoperabilite-conteneurs.md
   ============================================================================ */
#include <span>
#include <vector>
#include <array>
#include <string>
#include <string_view>
#include <print>
#include <algorithm>

double produit_scalaire(std::span<const double> a, std::span<const double> b) {
    double resultat = 0.0;
    auto taille = std::min(a.size(), b.size());
    for (std::size_t i = 0; i < taille; ++i) {
        resultat += a[i] * b[i];
    }
    return resultat;
}

int main() {
    // Depuis vector
    {
        std::vector<int> v{10, 20, 30};
        auto lire = [](std::span<const int> s) {
            std::println("Lecture de {} éléments", s.size());
        };
        auto modifier = [](std::span<int> s) {
            for (auto& val : s) val *= 2;
        };
        lire(v);
        modifier(v);
        const std::vector<int> cv{1, 2, 3};
        lire(cv);
    }

    // Depuis array
    {
        std::array<int, 4> arr{10, 20, 30, 40};
        auto lire_dyn = [](std::span<const int> s) {
            std::println("Dynamique : {} éléments", s.size());
        };
        auto lire_stat = [](std::span<const int, 4> s) {
            std::println("Statique  : {} éléments (extent={})", s.size(), s.extent);
        };
        lire_dyn(arr);
        lire_stat(arr);
        std::span s(arr);
        std::println("sizeof = {}", sizeof(s));
        std::span<const int, 3> s3 = std::span(arr).first<3>();
        (void)s3;
    }

    // Depuis tableau C
    {
        int tab[] = {10, 20, 30, 40, 50};
        auto lire = [](std::span<const int> s) {
            for (auto val : s) std::print("{} ", val);
            std::println("");
        };
        lire(tab);
        std::span s(tab);
        std::println("sizeof = {}, extent = {}", sizeof(s), s.extent);
    }

    // Depuis pointeur + taille
    {
        int data[] = {10, 20, 30, 40, 50};
        int* ptr = data;
        std::span s1(ptr, 5);
        std::span<int, 5> s2(ptr, 5);
        std::println("s1.size = {}, s2.size = {}", s1.size(), s2.size());
        std::span s3(ptr, ptr + 3);
        std::println("s3.size = {}", s3.size());
    }

    // Conversions de mutabilité
    {
        std::vector<int> v{1, 2, 3};
        std::span<int> s_mut(v);
        std::span<const int> s_ro = s_mut;
        (void)s_ro;
    }

    // Produit scalaire
    {
        std::vector<double> mesures{1.0, 2.0, 3.0};
        std::array<double, 3> poids{0.5, 0.3, 0.2};
        double coefficients[] = {10.0, 20.0, 30.0};
        std::println("vec × array : {}", produit_scalaire(mesures, poids));
        std::println("vec × C arr : {}", produit_scalaire(mesures, coefficients));
        std::println("array × C   : {}", produit_scalaire(poids, coefficients));
        std::println("partiel     : {}", produit_scalaire(
            std::span(mesures).first(2),
            std::span(poids).last(2)
        ));
    }

    // string / string_view
    {
        std::string s = "Hello, World!";
        auto traiter_texte = [](std::string_view sv) {
            std::println("Texte : {}", sv);
        };
        auto traiter_octets = [](std::span<const char> data) {
            std::println("{} octets bruts", data.size());
        };
        traiter_texte(s);
        traiter_octets(s);
        std::span<const char> sp(s);
        std::string_view sv(sp.data(), sp.size());
        std::println("Via span→sv : {}", sv);
    }
}
