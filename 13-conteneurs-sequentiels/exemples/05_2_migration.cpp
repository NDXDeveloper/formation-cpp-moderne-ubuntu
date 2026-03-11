/* ============================================================================
   Section 13.5.2 : Migration de base pointeur+taille vers span
   Description : Remplacement de la paire (T*, size_t) par std::span,
                 avec lecture seule, moyenne, normalisation et recherche
   Fichier source : 05.2-remplacement-pointeur-taille.md
   ============================================================================ */
#include <span>
#include <vector>
#include <array>
#include <numeric>
#include <algorithm>
#include <print>

void remplir_zeros(std::span<int> data) {
    for (auto& val : data) val = 0;
}

int somme(std::span<const int> donnees) {
    return std::accumulate(donnees.begin(), donnees.end(), 0);
}

double moyenne(std::span<const double> valeurs) {
    if (valeurs.empty()) return 0.0;
    double total = 0.0;
    for (auto v : valeurs) total += v;
    return total / static_cast<double>(valeurs.size());
}

void normaliser(std::span<double> valeurs) {
    double moy = moyenne(valeurs);
    for (auto& v : valeurs) v -= moy;
}

std::size_t trouver_index(std::span<const int> data, int valeur) {
    for (std::size_t i = 0; i < data.size(); ++i) {
        if (data[i] == valeur) return i;
    }
    return data.size();
}

bool contient(std::span<const int> data, int valeur) {
    return std::find(data.begin(), data.end(), valeur) != data.end();
}

int main() {
    // Migration de base
    {
        std::vector<int> v{1, 2, 3, 4, 5};
        std::array<int, 3> a{10, 20, 30};
        int tab[] = {100, 200};
        remplir_zeros(v);
        remplir_zeros(a);
        remplir_zeros(tab);
    }

    // Moyenne + normaliser
    {
        std::vector<double> data{10.0, 20.0, 30.0, 40.0};
        std::println("Moyenne : {}", moyenne(data));
        normaliser(data);
        for (auto v : data) std::print("{} ", v);
        std::println("");
    }

    // Recherche
    {
        int tab[] = {10, 20, 30, 40, 50};
        std::println("Index de 30 : {}", trouver_index(tab, 30));
        std::println("Contient 99 : {}", contient(tab, 99));
    }
}
