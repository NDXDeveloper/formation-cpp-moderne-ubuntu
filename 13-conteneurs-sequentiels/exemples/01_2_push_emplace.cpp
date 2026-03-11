/* ============================================================================
   Section 13.1.2 : push_back et emplace_back
   Description : Ajout d'éléments en fin de vecteur avec push_back (copie/move)
                 et emplace_back (construction en place)
   Fichier source : 01.2-methodes-essentielles.md
   ============================================================================ */
#include <vector>
#include <string>
#include <print>

int main() {
    std::vector<std::string> mots;
    mots.reserve(4);

    // push_back : prend un objet existant (copie ou move)
    std::string salut = "bonjour";
    mots.push_back(salut);              // copie de salut
    mots.push_back(std::move(salut));   // move — salut est maintenant vide

    // emplace_back : construit en place à partir des arguments du constructeur
    mots.emplace_back("monde");         // construit directement, pas de temporaire
    mots.emplace_back(5, 'x');          // construit std::string(5, 'x') → "xxxxx"

    for (const auto& m : mots) {
        std::print("[{}] ", m);
    }
    // Sortie : [bonjour] [bonjour] [monde] [xxxxx]
}
