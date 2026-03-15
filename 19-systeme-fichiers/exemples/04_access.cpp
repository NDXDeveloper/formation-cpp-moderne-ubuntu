/* ============================================================================
   Section 19.4 : access() — vérification POSIX des permissions
   Description : Test R_OK, W_OK, X_OK, F_OK sur des fichiers
   Fichier source : 04-permissions-droits.md
   ============================================================================ */
#include <unistd.h>
#include <print>

int main() {
    const char* path = "/etc/shadow";

    if (access(path, R_OK) == 0) {
        std::println("{} : lecture autorisée", path);
    } else {
        std::println("{} : lecture refusée", path);
    }

    if (access(path, W_OK) == 0) {
        std::println("{} : écriture autorisée", path);
    } else {
        std::println("{} : écriture refusée", path);
    }

    // Test sur /etc/passwd (lisible par tous)
    const char* passwd = "/etc/passwd";
    std::println("\n{} :", passwd);
    std::println("  Existe   : {}", access(passwd, F_OK) == 0);
    std::println("  Lecture  : {}", access(passwd, R_OK) == 0);
    std::println("  Écriture : {}", access(passwd, W_OK) == 0);

    // Test sur un exécutable
    const char* bash = "/usr/bin/bash";
    std::println("\n{} :", bash);
    std::println("  Exécution : {}", access(bash, X_OK) == 0);
}
