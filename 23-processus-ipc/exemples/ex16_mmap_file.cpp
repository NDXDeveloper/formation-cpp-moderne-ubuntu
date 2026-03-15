/* ============================================================================
   Section 23.3 : Fichiers mappés en mémoire
   Description : Lecture d'un fichier via mmap (accès par pointeur, pas de read)
   Fichier source : 03-shared-memory.md
   ============================================================================ */
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <print>
#include <string_view>

void read_file_with_mmap(const char* path) {
    // Ouvrir le fichier
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        throw std::system_error(errno, std::system_category(), "open()");
    }

    // Obtenir la taille
    struct stat st;
    fstat(fd, &st);
    size_t size = static_cast<size_t>(st.st_size);

    if (size == 0) {
        close(fd);
        std::println("Fichier vide");
        return;
    }

    // Mapper le fichier entier en lecture
    void* ptr = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);  // Le fd n'est plus nécessaire après mmap

    if (ptr == MAP_FAILED) {
        throw std::system_error(errno, std::system_category(), "mmap()");
    }

    // Le fichier entier est accessible via un pointeur
    std::string_view content(static_cast<const char*>(ptr), size);
    std::println("Fichier: {} octets", size);
    std::println("Premières lignes:");

    size_t pos = 0;
    int lines = 0;
    while (pos < content.size() && lines < 5) {
        size_t end = content.find('\n', pos);
        if (end == std::string_view::npos) end = content.size();
        std::println("  {}", content.substr(pos, end - pos));
        pos = end + 1;
        lines++;
    }

    munmap(ptr, size);
}

int main() {
    read_file_with_mmap("/etc/passwd");
}
