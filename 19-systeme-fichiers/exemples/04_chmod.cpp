/* ============================================================================
   Section 19.4 : chmod() et fchmod() POSIX
   Description : Modification des permissions par chemin et par descripteur
   Fichier source : 04-permissions-droits.md
   ============================================================================ */
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <print>
#include <cerrno>
#include <cstring>
#include <fstream>

int main() {
    const char* path = "/tmp/test_chmod_ex04.txt";

    // Créer le fichier
    std::ofstream{path} << "test chmod";

    // chmod par chemin
    if (chmod(path, 0644) == -1) {
        std::println("chmod: {}", strerror(errno));
    } else {
        std::println("chmod 0644 : OK");
    }

    // Vérifier
    struct stat st;
    stat(path, &st);
    std::println("Permissions : {:04o}", st.st_mode & 07777);

    // fchmod par descripteur (plus sûr)
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd >= 0) {
        if (fchmod(fd, 0600) == -1) {
            std::println("fchmod: {}", strerror(errno));
        } else {
            std::println("fchmod 0600 : OK");
        }
        close(fd);
    }

    // Vérifier
    stat(path, &st);
    std::println("Permissions : {:04o}", st.st_mode & 07777);

    // Nettoyage
    unlink(path);
}
