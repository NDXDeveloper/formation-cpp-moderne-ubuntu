/* ============================================================================
   Section 19.2 : lseek() — positionner le curseur
   Description : SEEK_SET, SEEK_CUR, SEEK_END et fichiers creux (sparse)
   Fichier source : 02-appels-posix.md
   ============================================================================ */
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <print>
#include <cerrno>
#include <cstring>

int main() {
    int fd = open("/tmp/data_ex02l.bin", O_RDWR | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (fd == -1) return 1;

    // Écrire des données
    const char* msg = "ABCDEFGHIJ";
    write(fd, msg, 10);

    // Revenir au début
    lseek(fd, 0, SEEK_SET);

    // Lire les 5 premiers octets
    char buf[6] = {};
    read(fd, buf, 5);
    std::println("5 premiers : {}", buf);  // ABCDE

    // Se positionner à l'offset 3 depuis le début
    lseek(fd, 3, SEEK_SET);
    read(fd, buf, 5);
    buf[5] = '\0';
    std::println("Depuis offset 3 : {}", buf);  // DEFGH

    // Connaître la position courante
    off_t pos = lseek(fd, 0, SEEK_CUR);
    std::println("Position courante : {}", pos);  // 8

    // Connaître la taille du fichier
    off_t size = lseek(fd, 0, SEEK_END);
    std::println("Taille du fichier : {}", size);  // 10

    close(fd);

    // --- Fichier creux (sparse) ---
    int fd2 = open("/tmp/sparse_ex02.bin", O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (fd2 == -1) return 1;

    write(fd2, "A", 1);
    lseek(fd2, 1L * 1024 * 1024, SEEK_SET);  // 1 Mo
    write(fd2, "B", 1);

    close(fd2);

    // Vérifier taille apparente vs espace réel
    struct stat st;
    stat("/tmp/sparse_ex02.bin", &st);
    std::println("Taille apparente : {} octets", st.st_size);
    std::println("Blocs réels      : {} (× 512 = {} octets)", st.st_blocks, st.st_blocks * 512);

    // Nettoyage
    unlink("/tmp/data_ex02l.bin");
    unlink("/tmp/sparse_ex02.bin");
}
