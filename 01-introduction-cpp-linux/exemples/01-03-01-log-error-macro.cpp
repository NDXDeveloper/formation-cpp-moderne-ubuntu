/* ============================================================================
   Section 1.3.1 : Le préprocesseur — #include, #define, macros
   Description : Macro multi-ligne avec l'idiome do { ... } while(0)
                 Montre LOG_ERROR avec __FILE__ et __LINE__
   Fichier source : 03.1-preprocesseur.md
   Compilation : g++ -std=c++23 -o 01-03-01-log-error-macro 01-03-01-log-error-macro.cpp
   Sortie attendue (stderr) :
     [ERROR] 01-03-01-log-error-macro.cpp:<N> - Connexion refusée
     [ERROR] 01-03-01-log-error-macro.cpp:<N> - Fichier introuvable
   (les numéros de ligne <N> varient)
   ============================================================================ */
#include <iostream>

#define LOG_ERROR(msg) \
    do { \
        std::cerr << "[ERROR] " << __FILE__ << ":" << __LINE__ \
                  << " - " << msg << std::endl; \
    } while(0)

int main() {
    LOG_ERROR("Connexion refusée");
    LOG_ERROR("Fichier introuvable");

    return 0;
}
