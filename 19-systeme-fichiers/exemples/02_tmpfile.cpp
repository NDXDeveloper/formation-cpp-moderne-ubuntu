/* ============================================================================
   Section 19.2 : O_TMPFILE — fichiers temporaires anonymes
   Description : Création de fichier sans nom avec nettoyage garanti
   Fichier source : 02-appels-posix.md
   ============================================================================ */
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <print>
#include <cerrno>
#include <cstring>

int main() {
    // Crée un fichier temporaire anonyme dans /tmp
    int fd = open("/tmp", O_TMPFILE | O_RDWR | O_CLOEXEC, 0644);
    if (fd == -1) {
        std::println("O_TMPFILE: {}", strerror(errno));
        return 1;
    }

    std::println("Fichier temporaire anonyme créé (fd={})", fd);

    // Écrire des données — le fichier n'est visible nulle part dans /tmp
    const char* data = "données temporaires sécurisées\n";
    write(fd, data, strlen(data));

    // Relire
    lseek(fd, 0, SEEK_SET);
    char buf[256] = {};
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        std::println("Contenu : {}", buf);
    }

    // Taille via fstat
    struct stat st;
    fstat(fd, &st);
    std::println("Taille : {} octets", st.st_size);

    close(fd);  // Le fichier est automatiquement supprimé
    std::println("Fichier temporaire supprimé automatiquement");
}
