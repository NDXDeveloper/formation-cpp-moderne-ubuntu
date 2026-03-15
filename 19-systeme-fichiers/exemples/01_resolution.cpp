/* ============================================================================
   Section 19.1 : Résolution canonique
   Description : canonical(), weakly_canonical() et current_path()
   Fichier source : 01-std-filesystem.md
   ============================================================================ */
#include <filesystem>
#include <print>

namespace fs = std::filesystem;

int main() {
    // canonical() résout les liens symboliques et normalise
    // Le chemin DOIT exister, sinon une exception est levée
    fs::path canon = fs::canonical("/usr/bin/python3");
    std::println("Canonical : {}", canon.string());

    // weakly_canonical() tolère que le chemin n'existe pas entièrement
    fs::path wcanon = fs::weakly_canonical("/home/user/projet/../nouveau/fichier.txt");
    std::println("Weakly canonical : {}", wcanon.string());

    // current_path() retourne le répertoire de travail courant
    std::println("CWD : {}", fs::current_path().string());
}
