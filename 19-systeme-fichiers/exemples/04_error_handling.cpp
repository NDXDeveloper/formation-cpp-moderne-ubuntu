/* ============================================================================
   Section 19.4 : Gestion d'erreurs POSIX
   Description : errno, strerror, diagnostic par code, conversion error_code
   Fichier source : 04-permissions-droits.md
   ============================================================================ */
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <print>
#include <system_error>

void posix_error_handling(const char* path) {
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd == -1) {
        int err = errno;  // Sauvegarder immédiatement

        // 1. Message lisible avec strerror
        std::println("Erreur pour {} : {}", path, strerror(err));

        // 2. Diagnostic programmatique
        switch (err) {
            case ENOENT:
                std::println("  → Le fichier n'existe pas");
                break;
            case EACCES:
                std::println("  → Permission refusée");
                break;
            case EMFILE:
                std::println("  → Trop de fichiers ouverts par ce processus");
                break;
            default:
                std::println("  → Erreur inattendue (errno={})", err);
        }

        // 3. Conversion vers std::error_code
        std::error_code ec(err, std::generic_category());
        std::println("  error_code : {}", ec.message());

        return;
    }

    std::println("{} ouvert avec succès (fd={})", path, fd);
    close(fd);
}

int main() {
    posix_error_handling("/etc/hostname");           // Succès
    posix_error_handling("/tmp/inexistant_ex04");     // ENOENT
    posix_error_handling("/etc/shadow");              // EACCES (probablement)
}
