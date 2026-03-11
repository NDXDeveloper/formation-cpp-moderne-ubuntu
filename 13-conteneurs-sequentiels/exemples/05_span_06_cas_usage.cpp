/* ============================================================================
   Section 13.5 : Cas d'usage typiques
   Description : Interface unifiée (est_trie, moyenne), travail sur tranches
                 (analyse de paquet réseau avec header/payload)
   Fichier source : 05-span.md
   ============================================================================ */
#include <span>
#include <vector>
#include <array>
#include <algorithm>
#include <print>
#include <cstdint>

bool est_trie(std::span<const int> donnees) {
    return std::is_sorted(donnees.begin(), donnees.end());
}

double moyenne(std::span<const double> donnees) {
    if (donnees.empty()) return 0.0;
    double total = 0.0;
    for (auto val : donnees) total += val;
    return total / static_cast<double>(donnees.size());
}

void analyser_paquet(std::span<const std::uint8_t> paquet) {
    if (paquet.size() < 4) {
        std::println("Paquet trop court");
        return;
    }
    auto header  = paquet.first(4);
    auto payload = paquet.subspan(4);
    std::println("Header  : {} octets", header.size());
    std::println("Payload : {} octets", payload.size());
}

int main() {
    // est_trie + moyenne
    {
        std::vector<int> v{1, 3, 5, 7};
        std::array<int, 3> a{10, 5, 1};
        int tab[] = {2, 4, 6};
        std::println("v trié   : {}", est_trie(v));
        std::println("a trié   : {}", est_trie(a));
        std::println("tab trié : {}", est_trie(tab));

        std::vector<double> notes{14.5, 16.0, 12.5, 18.0};
        std::println("Moyenne : {:.1f}", moyenne(notes));
        std::println("Top 2   : {:.1f}", moyenne(std::span(notes).last(2)));
    }

    // Paquet réseau
    {
        std::vector<std::uint8_t> data{0x01, 0x02, 0x03, 0x04,
                                        0xAA, 0xBB, 0xCC};
        analyser_paquet(data);
    }
}
