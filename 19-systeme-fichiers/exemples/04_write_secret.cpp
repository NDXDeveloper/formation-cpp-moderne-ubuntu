/* ============================================================================
   Section 19.4 : Création sécurisée de fichiers sensibles
   Description : O_EXCL + mode 0600 + fchmod pour fichiers confidentiels
   Fichier source : 04-permissions-droits.md
   ============================================================================ */
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <filesystem>
#include <print>
#include <cerrno>
#include <cstring>

namespace fs = std::filesystem;

bool write_secret_file(const fs::path& path, const std::string& content) {
    // O_EXCL : échoue si le fichier existe
    // Mode 0600 : rw------- (seul le propriétaire peut lire/écrire)
    int fd = open(path.c_str(),
        O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);

    if (fd == -1) {
        std::println("Impossible de créer {} : {}", path.string(), strerror(errno));
        return false;
    }

    // Renforcer les permissions (contrer un umask inattendu)
    fchmod(fd, 0600);

    // Écriture complète
    const char* ptr = content.data();
    size_t remaining = content.size();
    while (remaining > 0) {
        ssize_t n = write(fd, ptr, remaining);
        if (n == -1) {
            if (errno == EINTR) continue;
            std::println("write: {}", strerror(errno));
            close(fd);
            fs::remove(path);  // Nettoyage en cas d'échec
            return false;
        }
        ptr += n;
        remaining -= static_cast<size_t>(n);
    }

    fsync(fd);
    close(fd);
    return true;
}

int main() {
    fs::path secret = "/tmp/secret_ex04.key";
    fs::remove(secret);  // Nettoyer d'éventuels résidus

    bool ok = write_secret_file(secret, "ma-cle-secrete-1234567890");
    std::println("Création : {}", ok ? "succès" : "échec");

    // Vérifier les permissions
    struct stat st;
    stat(secret.c_str(), &st);
    std::println("Permissions : {:04o}", st.st_mode & 07777);
    std::println("Taille : {} octets", st.st_size);

    // Tenter de recréer (doit échouer avec O_EXCL)
    bool ok2 = write_secret_file(secret, "autre contenu");
    std::println("Recréation : {} (attendu: échec)", ok2 ? "succès" : "échec");

    // Nettoyage
    fs::remove(secret);
}
