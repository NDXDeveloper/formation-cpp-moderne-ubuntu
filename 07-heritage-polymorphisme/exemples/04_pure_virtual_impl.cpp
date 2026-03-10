/* ============================================================================
   Section 7.4 : Classes abstraites — Pure virtual avec implementation
   Description : Une fonction virtuelle pure PEUT avoir une implementation
                 dans la base. La classe reste abstraite, mais l'implementation
                 est accessible via Base::methode() depuis les derivees.
   Fichier source : 04-classes-abstraites.md
   ============================================================================ */
#include <print>

class Connexion {
public:
    virtual void fermer() = 0;   // pure — mais avec implémentation
    virtual ~Connexion() = default;
};

// Implémentation fournie en dehors de la déclaration de la classe
void Connexion::fermer() {
    std::println("Nettoyage générique de la connexion");
}

class ConnexionTCP : public Connexion {
public:
    void fermer() override {
        std::println("Fermeture du socket TCP");
        Connexion::fermer();   // appel explicite à l'implémentation de base
    }
};

class ConnexionUDP : public Connexion {
public:
    void fermer() override {
        std::println("Fermeture du socket UDP");
        Connexion::fermer();   // réutilise le nettoyage commun
    }
};

int main() {
    std::println("=== ConnexionTCP ===");
    ConnexionTCP tcp;
    tcp.fermer();

    std::println("\n=== ConnexionUDP ===");
    ConnexionUDP udp;
    udp.fermer();
}
