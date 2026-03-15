/* ============================================================================
   Section 19.2 : stat() et fstat() — métadonnées de fichiers
   Description : Informations détaillées via struct stat
   Fichier source : 02-appels-posix.md
   ============================================================================ */
#include <sys/stat.h>
#include <unistd.h>
#include <print>
#include <cerrno>
#include <cstring>

int main() {
    struct stat st;

    if (stat("/etc/hostname", &st) == -1) {
        std::println("stat: {}", strerror(errno));
        return 1;
    }

    std::println("=== /etc/hostname ===");
    std::println("Taille        : {} octets", st.st_size);
    std::println("Inode         : {}", st.st_ino);
    std::println("Device        : {}", st.st_dev);
    std::println("Hard links    : {}", st.st_nlink);
    std::println("UID           : {}", st.st_uid);
    std::println("GID           : {}", st.st_gid);
    std::println("Blocs (512o)  : {}", st.st_blocks);
    std::println("Taille bloc   : {}", st.st_blksize);

    // Type de fichier
    if (S_ISREG(st.st_mode))  std::println("Type : fichier régulier");
    if (S_ISDIR(st.st_mode))  std::println("Type : répertoire");
    if (S_ISLNK(st.st_mode))  std::println("Type : lien symbolique");

    // Test sur un répertoire
    std::println("\n=== /tmp ===");
    if (stat("/tmp", &st) == 0) {
        if (S_ISDIR(st.st_mode)) std::println("Type : répertoire");
        std::println("Permissions : {:04o}", st.st_mode & 07777);
    }
}
