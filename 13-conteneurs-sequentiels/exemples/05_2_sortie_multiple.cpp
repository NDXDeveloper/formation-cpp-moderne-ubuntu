/* ============================================================================
   Section 13.5.2 : Fonctions à sorties multiples
   Description : Utilisation de span comme buffer de sortie, retournant un
                 sous-span sur la partie effectivement remplie
   Fichier source : 05.2-remplacement-pointeur-taille.md
   ============================================================================ */
#include <span>
#include <vector>
#include <print>

std::span<int> lire_donnees(std::span<int> buffer) {
    std::size_t nb_lus = std::min(buffer.size(), std::size_t{3});
    for (std::size_t i = 0; i < nb_lus; ++i) {
        buffer[i] = static_cast<int>((i + 1) * 100);
    }
    return buffer.first(nb_lus);
}

int main() {
    std::vector<int> buf(10, 0);
    auto resultat = lire_donnees(buf);
    std::println("Lus : {} éléments", resultat.size());
    for (auto val : resultat) std::print("{} ", val);
    std::println("");
    // Sortie : 100 200 300
}
