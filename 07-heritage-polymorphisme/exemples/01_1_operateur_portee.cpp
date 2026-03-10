/* ============================================================================
   Section 7.1.1 : Heritage simple — Acces aux membres de la base
   Description : Appel explicite a la methode de la classe de base via
                 l'operateur de portee :: pour etendre le comportement
   Fichier source : 01.1-heritage-simple.md
   ============================================================================ */
#include <print>

class Fichier {
public:
    void ouvrir() {
        std::println("Ouverture fichier standard");
    }
};

class FichierChiffre : public Fichier {
public:
    void ouvrir() {
        Fichier::ouvrir();  // appel explicite à la version de base
        std::println("Initialisation du déchiffrement");
    }
};

int main() {
    FichierChiffre fc;
    fc.ouvrir();
}
