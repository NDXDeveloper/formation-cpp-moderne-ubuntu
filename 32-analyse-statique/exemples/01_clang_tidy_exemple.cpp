/* ============================================================================
   Section 32.1 : clang-tidy — Analyse statique moderne
   Description : Programme avec plusieurs défauts détectables par clang-tidy :
                 paramètre copié inutilement, méthode non-const, pointeur
                 vers variable locale, comparaison signée/non-signée
   Fichier source : 01-clang-tidy.md
   Exécution : clang-tidy -checks='bugprone-*,performance-*,readability-*,clang-analyzer-*' 01_clang_tidy_exemple.cpp -- -std=c++23
   ============================================================================ */

#include <vector>
#include <string>

class Config {
    std::string host;
    int port;
public:
    Config(std::string h, int p) : host(h), port(p) {}

    std::string getHost() { return host; }

    int* getPortPtr() {
        int local_port = port;
        return &local_port;  // Retourne un pointeur vers une variable locale
    }
};

void traiter(std::vector<int> donnees) {
    for (int i = 0; i < donnees.size(); ++i) {
        if (donnees[i] == 0) {
            int resultat = 100 / donnees[i];  // Division par zéro
        }
    }
}
