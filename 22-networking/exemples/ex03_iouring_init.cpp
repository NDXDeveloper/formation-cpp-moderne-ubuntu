/* ============================================================================
   Section 22.3.3.2 : liburing — Interface C/C++ simplifiee
   Description : Initialisation io_uring, obtention de SQE, cleanup
   Fichier source : 03.3.2-liburing.md
   ============================================================================ */

// Compilation : g++-15 -std=c++23 -O2 -o ex03_iouring_init ex03_iouring_init.cpp -luring

#include <liburing.h>
#include <cstring>
#include <print>

int main() {
    // Initialiser une instance io_uring avec 256 entrees SQ
    io_uring ring{};
    int ret = io_uring_queue_init(256, &ring, 0);
    if (ret < 0) {
        std::print(stderr, "io_uring_queue_init: {}\n", strerror(-ret));
        return 1;
    }
    std::print("io_uring initialise OK\n");
    std::print("  SQ entries: {}\n", ring.sq.ring_entries);
    std::print("  CQ entries: {}\n", ring.cq.ring_entries);

    // Obtenir un SQE
    io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    if (sqe) {
        std::print("  SQE obtenu OK\n");
    } else {
        std::print(stderr, "  Erreur: pas de SQE disponible\n");
    }

    // Nettoyage
    io_uring_queue_exit(&ring);
    std::print("io_uring cleanup OK\n");
    return 0;
}
