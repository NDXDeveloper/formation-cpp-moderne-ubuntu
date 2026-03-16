/* ============================================================================
   Section 30.1.1 : memcheck — Détection de fuites
   Description : Fuites multiples — classe sans destructeur + buffer non libéré
   Fichier source : 01.1-memcheck.md
   Compilation : g++-15 -std=c++23 -g -O0 -o fuites_multiples 02_fuites_multiples.cpp
   Exécution  : valgrind --leak-check=full --show-leak-kinds=all ./fuites_multiples
   ============================================================================ */

#include <cstring>
#include <string>

struct Config {
    char* hostname;
    int port;

    Config(const char* h, int p) : port(p) {
        hostname = new char[std::strlen(h) + 1];
        std::strcpy(hostname, h);
    }
    // Pas de destructeur : hostname ne sera jamais libéré
};

Config* charger_config() {
    return new Config("serveur.example.com", 8080);
}

void traiter_requete() {
    int* buffer = new int[256];
    // Traitement simulé...
    // Oubli : pas de delete[] buffer;
}

int main() {
    Config* cfg = charger_config();

    for (int i = 0; i < 3; ++i) {
        traiter_requete();
    }

    delete cfg;  // hostname n'est pas libéré dans le destructeur (implicite)
    return 0;
}
