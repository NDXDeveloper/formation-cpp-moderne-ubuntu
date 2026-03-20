/* ============================================================================
   Section 36.4 : Gestion des couleurs et du TTY
   Description : Détection du contexte terminal avec isatty()
   Fichier source : 04-couleurs-tty.md
   ============================================================================ */

#include <unistd.h>
#include <print>

int main() {
    bool stdin_is_tty  = isatty(STDIN_FILENO);
    bool stdout_is_tty = isatty(STDOUT_FILENO);
    bool stderr_is_tty = isatty(STDERR_FILENO);

    std::println("stdin  : {}", stdin_is_tty ? "TTY" : "pipe/fichier");
    std::println("stdout : {}", stdout_is_tty ? "TTY" : "pipe/fichier");
    std::println("stderr : {}", stderr_is_tty ? "TTY" : "pipe/fichier");
    return 0;
}
