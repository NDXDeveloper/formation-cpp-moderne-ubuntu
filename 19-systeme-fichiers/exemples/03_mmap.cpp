/* ============================================================================
   Section 19.3 : Scénario 7 — mmap() pour fichiers volumineux
   Description : Memory-mapped I/O pour traitement haute performance
   Fichier source : 03-comparaison-api.md
   ============================================================================ */
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <print>
#include <cerrno>
#include <cstring>
#include <cstdint>
#include <fstream>

void process_large_file(const char* path) {
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd == -1) {
        std::println("open: {}", strerror(errno));
        return;
    }

    struct stat st;
    fstat(fd, &st);
    auto size = static_cast<size_t>(st.st_size);

    if (size == 0) {
        std::println("Fichier vide");
        close(fd);
        return;
    }

    // Mapper le fichier en mémoire
    void* addr = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    if (addr == MAP_FAILED) {
        std::println("mmap: {}", strerror(errno));
        return;
    }

    // Conseiller le noyau sur le pattern d'accès
    madvise(addr, size, MADV_SEQUENTIAL);

    // Compter les lignes
    const auto* data = static_cast<const uint8_t*>(addr);
    uint64_t lines = 0;
    for (size_t i = 0; i < size; ++i) {
        if (data[i] == '\n') ++lines;
    }
    std::println("{} lignes dans {} ({} Ko)", lines, path, size / 1024);

    munmap(addr, size);
}

int main() {
    // Créer un fichier de test
    const char* path = "/tmp/large_ex03.txt";
    {
        std::ofstream out(path);
        for (int i = 0; i < 10000; ++i) {
            out << "Ligne " << i << " : données de test pour mmap\n";
        }
    }

    process_large_file(path);

    // Nettoyage
    unlink(path);
}
