/* ============================================================================
   Section 13.1 : Ajout d'éléments
   Description : Utilisation de push_back et emplace_back pour ajouter des
                 éléments à un std::vector
   Fichier source : 01-vector.md
   ============================================================================ */
#include <vector>
#include <string>
#include <print>

int main() {
    std::vector<std::string> logs;

    // push_back : ajoute une copie ou déplace en fin
    logs.push_back("Démarrage du service");

    // emplace_back : construit l'élément directement en place (pas de copie)
    logs.emplace_back("Connexion établie");

    // Depuis C++17, emplace_back retourne une référence vers l'élément créé
    auto& dernier = logs.emplace_back("Traitement en cours");
    std::println("Dernier log : {}", dernier);

    std::println("Nombre de logs : {}", logs.size());
    // Sortie : Nombre de logs : 3
}
