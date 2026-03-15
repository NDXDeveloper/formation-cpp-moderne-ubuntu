/* ============================================================================
   Section 23.4 : Message queues POSIX — Envoi et réception
   Description : Création de queue, envoi/réception, test des priorités
   Fichier source : 04-message-queues.md
   ============================================================================ */
#include <mqueue.h>
#include <fcntl.h>
#include <print>
#include <cstring>
#include <vector>
#include <string_view>

int main() {
    const char* name = "/test_mq_basic";

    // Cleanup from previous runs
    mq_unlink(name);

    // Créer la queue
    mq_attr attr{};
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 256;

    mqd_t mq = mq_open(name, O_CREAT | O_RDWR, 0666, &attr);
    if (mq == static_cast<mqd_t>(-1)) {
        throw std::system_error(errno, std::system_category(), "mq_open()");
    }

    // Envoyer un message
    const char* message = "Hello, queue!";
    if (mq_send(mq, message, strlen(message), 0) == -1) {
        throw std::system_error(errno, std::system_category(), "mq_send()");
    }
    std::println("Envoyé: {}", message);

    // Recevoir le message
    mq_attr qattr;
    mq_getattr(mq, &qattr);

    std::vector<char> buffer(static_cast<size_t>(qattr.mq_msgsize));
    unsigned int priority;

    ssize_t n = mq_receive(mq, buffer.data(), buffer.size(), &priority);
    if (n == -1) {
        throw std::system_error(errno, std::system_category(), "mq_receive()");
    }

    std::string_view msg_view(buffer.data(), static_cast<size_t>(n));
    std::println("Reçu (priorité {}): {}", priority, msg_view);

    // Test des priorités
    std::println("\n=== Test des priorités ===");
    mq_send(mq, "urgence critique", 16, 10);
    mq_send(mq, "tâche normale", 14, 1);
    mq_send(mq, "alerte importante", 17, 5);
    mq_send(mq, "routine", 7, 0);

    // Réception : ordre par priorité décroissante
    for (int i = 0; i < 4; ++i) {
        unsigned int prio;
        ssize_t len = mq_receive(mq, buffer.data(), buffer.size(), &prio);
        std::println("  [prio={}] {}", prio,
                     std::string_view(buffer.data(), static_cast<size_t>(len)));
    }

    mq_close(mq);
    mq_unlink(name);
    std::println("\nQueue nettoyée");
}
