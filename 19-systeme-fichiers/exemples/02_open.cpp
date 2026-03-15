/* ============================================================================
   Section 19.2 : open() — ouvrir ou créer un fichier
   Description : Flags d'accès, O_CREAT, O_EXCL, O_APPEND, O_CLOEXEC
   Fichier source : 02-appels-posix.md
   ============================================================================ */
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <print>
#include <cerrno>
#include <cstring>

int main() {
    // Lecture seule d'un fichier existant
    int fd1 = open("/etc/hostname", O_RDONLY);
    if (fd1 == -1) {
        std::println("Erreur : {}", strerror(errno));
        return 1;
    }
    std::println("fd1 (lecture) : {}", fd1);

    // Création d'un fichier en écriture (tronque s'il existe)
    int fd2 = open("/tmp/output_ex02.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd2 == -1) {
        std::println("Erreur : {}", strerror(errno));
        close(fd1);
        return 1;
    }
    std::println("fd2 (écriture) : {}", fd2);

    // Création exclusive — échoue si le fichier existe déjà
    // D'abord s'assurer que le fichier existe
    int fd3 = open("/tmp/output_ex02.txt", O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (fd3 == -1 && errno == EEXIST) {
        std::println("Création exclusive : fichier déjà existant (attendu)");
    }

    // Ouverture en ajout (append)
    int fd4 = open("/tmp/append_ex02.log", O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0644);
    if (fd4 != -1) {
        std::println("fd4 (append) : {}", fd4);
    }

    // Nettoyage
    close(fd1);
    close(fd2);
    if (fd3 != -1) close(fd3);
    if (fd4 != -1) close(fd4);
    unlink("/tmp/output_ex02.txt");
    unlink("/tmp/append_ex02.log");
}
