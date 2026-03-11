/* ============================================================================
   Section 13.1.1 : emplace_back vs push_back
   Description : Construction en place avec emplace_back vs création d'un
                 temporaire avec push_back — trace des appels constructeurs
   Fichier source : 01.1-fonctionnement-interne.md
   ============================================================================ */
#include <vector>
#include <string>
#include <print>

struct Connexion {
    std::string host;
    int port;

    Connexion(std::string h, int p) : host(std::move(h)), port(p) {
        std::println("  Constructeur({}, {})", host, port);
    }

    Connexion(const Connexion& other) : host(other.host), port(other.port) {
        std::println("  Copie({}, {})", host, port);
    }

    Connexion(Connexion&& other) noexcept
        : host(std::move(other.host)), port(other.port) {
        std::println("  Move({}, {})", host, port);
    }
};

int main() {
    std::vector<Connexion> conns;
    conns.reserve(4);  // éviter les réallocations pour l'exemple

    std::println("--- push_back avec temporaire ---");
    conns.push_back(Connexion{"localhost", 8080});
    // Constructeur + Move

    std::println("\n--- emplace_back ---");
    conns.emplace_back("127.0.0.1", 3306);
    // Constructeur uniquement — pas de temporaire, pas de move
}
