/* ============================================================================
   Section 19.4 : Le umask
   Description : Lecture du umask, impact sur les permissions des fichiers créés
   Fichier source : 04-permissions-droits.md
   ============================================================================ */
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <print>

int main() {
    // Lire le umask actuel
    mode_t old_mask = umask(0);
    umask(old_mask);  // Restaurer immédiatement
    std::println("umask actuel : {:04o}", old_mask);

    // Démonstration : créer un fichier avec mode 0666
    int fd1 = open("/tmp/umask_test_ex04.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd1 >= 0) {
        struct stat st;
        fstat(fd1, &st);
        std::println("Mode demandé 0666, obtenu {:04o} (umask {:04o})",
            st.st_mode & 07777, old_mask);
        close(fd1);
        unlink("/tmp/umask_test_ex04.txt");
    }

    // Temporairement restreindre le umask
    mode_t prev = umask(0077);  // Seul le propriétaire aura accès
    int fd2 = open("/tmp/secret_umask_ex04.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    umask(prev);  // Restaurer

    if (fd2 >= 0) {
        struct stat st;
        fstat(fd2, &st);
        std::println("Mode demandé 0666 avec umask 0077, obtenu {:04o}",
            st.st_mode & 07777);
        close(fd2);
        unlink("/tmp/secret_umask_ex04.txt");
    }
}
