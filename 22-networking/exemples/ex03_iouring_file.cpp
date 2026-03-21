/* ============================================================================
   Section 22.3.3.3 : Cas d'usage — networking, fichiers, timeouts
   Description : Lecture asynchrone d'un fichier par blocs avec io_uring
   Fichier source : 03.3.3-cas-usage.md
   ============================================================================ */

// Compilation : g++-15 -std=c++23 -O2 -o ex03_iouring_file ex03_iouring_file.cpp -luring
// Execution  : ./ex03_iouring_file <fichier>

#include <liburing.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <print>
#include <algorithm>

constexpr size_t block_size = 64 * 1024;  // 64 Ko par lecture

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::print(stderr, "Usage: {} <fichier>\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        std::print(stderr, "open: {}\n", strerror(errno));
        return 1;
    }

    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size <= 0) {
        std::print(stderr, "Fichier vide ou erreur lseek\n");
        close(fd);
        return 1;
    }

    io_uring ring{};
    if (int ret = io_uring_queue_init(64, &ring, 0); ret < 0) {
        std::print(stderr, "io_uring_queue_init: {}\n", strerror(-ret));
        close(fd);
        return 1;
    }

    // Soumettre toutes les lectures en parallele par blocs
    size_t remaining = file_size;
    size_t offset    = 0;
    int    pending   = 0;
    auto*  file_buf  = new char[file_size];

    while (remaining > 0) {
        size_t chunk = std::min(remaining, block_size);
        auto* sqe = io_uring_get_sqe(&ring);
        if (!sqe) {
            io_uring_submit(&ring);
            sqe = io_uring_get_sqe(&ring);
        }
        io_uring_prep_read(sqe, fd, file_buf + offset, chunk, offset);
        io_uring_sqe_set_data64(sqe, offset);
        offset    += chunk;
        remaining -= chunk;
        ++pending;
    }

    std::print("Soumission de {} lectures asynchrones ({} octets)\n",
               pending, file_size);
    io_uring_submit(&ring);

    // Recolter toutes les completions
    size_t total_read = 0;
    int    errors     = 0;

    while (pending > 0) {
        io_uring_cqe* cqe = nullptr;
        int ret = io_uring_wait_cqe(&ring, &cqe);
        if (ret < 0) {
            std::print(stderr, "wait_cqe: {}\n", strerror(-ret));
            break;
        }
        if (cqe->res < 0) {
            std::print(stderr, "read a l'offset {}: {}\n",
                       io_uring_cqe_get_data64(cqe), strerror(-cqe->res));
            ++errors;
        } else {
            total_read += cqe->res;
        }
        io_uring_cqe_seen(&ring, cqe);
        --pending;
    }

    std::print("Lecture terminee : {} octets lus, {} erreurs\n",
               total_read, errors);

    delete[] file_buf;
    io_uring_queue_exit(&ring);
    close(fd);
    return 0;
}
