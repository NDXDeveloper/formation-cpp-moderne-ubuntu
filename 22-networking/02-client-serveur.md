🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 22.2 — Client/Serveur basique en C++

## Chapitre 22 : Networking et Communication

---

## Introduction

La section 22.1 a détaillé chaque appel système individuellement — `socket`, `bind`, `listen`, `accept`, `connect`, `send`, `recv`. Cette section les assemble en programmes complets et fonctionnels. Vous allez construire un serveur TCP et son client, puis explorer les différentes stratégies pour gérer plusieurs clients simultanément.

L'objectif n'est pas de produire du code prêt pour la production (les librairies comme Asio, section 22.4, sont faites pour ça), mais de **comprendre les architectures fondamentales** des serveurs réseau — les mêmes architectures qui sous-tendent Nginx, Redis, PostgreSQL et tous les serveurs que vous utilisez quotidiennement.

---

## Boîte à outils : les utilitaires réutilisables

Avant de construire les exemples, rassemblons les utilitaires développés en section 22.1 dans un header commun. Tout le code de cette section s'appuie sur ces briques :

```cpp
// net_utils.hpp
#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>

#include <cerrno>
#include <cstring>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <system_error>
#include <print>
#include <utility>

// --- RAII Socket (section 22.1.1) ---
class Socket {  
public:  
    Socket(int domain, int type, int protocol = 0)
        : fd_{socket(domain, type | SOCK_CLOEXEC, protocol)} {
        if (fd_ == -1)
            throw std::system_error(errno, std::system_category(), "socket()");
    }
    explicit Socket(int fd) noexcept : fd_{fd} {}
    ~Socket() { if (fd_ != -1) close(fd_); }

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket(Socket&& o) noexcept : fd_{std::exchange(o.fd_, -1)} {}
    Socket& operator=(Socket&& o) noexcept {
        if (this != &o) { if (fd_ != -1) close(fd_); fd_ = std::exchange(o.fd_, -1); }
        return *this;
    }

    [[nodiscard]] int fd() const noexcept { return fd_; }

    void set_option(int level, int optname, int value) {
        if (setsockopt(fd_, level, optname, &value, sizeof(value)) == -1)
            throw std::system_error(errno, std::system_category(), "setsockopt()");
    }
    void enable_reuse_addr() { set_option(SOL_SOCKET, SO_REUSEADDR, 1); }
    void enable_tcp_nodelay() { set_option(IPPROTO_TCP, TCP_NODELAY, 1); }

private:
    int fd_ = -1;
};

// --- RAII addrinfo (section 22.1.1) ---
struct AddrInfoDeleter {
    void operator()(addrinfo* p) const noexcept { if (p) freeaddrinfo(p); }
};
using AddrInfoPtr = std::unique_ptr<addrinfo, AddrInfoDeleter>;

inline AddrInfoPtr resolve(const char* host, const char* service,
                           int family = AF_UNSPEC, int socktype = SOCK_STREAM) {
    addrinfo hints{};
    hints.ai_family = family;
    hints.ai_socktype = socktype;
    hints.ai_flags = AI_PASSIVE;
    addrinfo* result = nullptr;
    if (int s = getaddrinfo(host, service, &hints, &result); s != 0)
        throw std::runtime_error(std::string("getaddrinfo: ") + gai_strerror(s));
    return AddrInfoPtr{result};
}

// --- send_all (section 22.1.3) ---
inline void send_all(int sockfd, const void* buf, size_t len) {
    auto* ptr = static_cast<const std::byte*>(buf);
    size_t remaining = len;
    while (remaining > 0) {
        ssize_t sent = send(sockfd, ptr, remaining, MSG_NOSIGNAL);
        if (sent == -1) {
            if (errno == EINTR) continue;
            throw std::system_error(errno, std::system_category(), "send()");
        }
        ptr += sent;
        remaining -= static_cast<size_t>(sent);
    }
}

// --- recv_exact (section 22.1.3) ---
inline bool recv_exact(int sockfd, void* buf, size_t len) {
    auto* ptr = static_cast<std::byte*>(buf);
    size_t remaining = len;
    while (remaining > 0) {
        ssize_t n = recv(sockfd, ptr, remaining, 0);
        if (n == -1) {
            if (errno == EINTR) continue;
            throw std::system_error(errno, std::system_category(), "recv()");
        }
        if (n == 0) return false;
        ptr += n;
        remaining -= static_cast<size_t>(n);
    }
    return true;
}

// --- Affichage d'adresse (section 22.1.2) ---
inline std::string format_address(const sockaddr_storage& addr) {
    char ip[INET6_ADDRSTRLEN];
    uint16_t port;
    if (addr.ss_family == AF_INET) {
        auto* v4 = reinterpret_cast<const sockaddr_in*>(&addr);
        inet_ntop(AF_INET, &v4->sin_addr, ip, sizeof(ip));
        port = ntohs(v4->sin_port);
    } else {
        auto* v6 = reinterpret_cast<const sockaddr_in6*>(&addr);
        inet_ntop(AF_INET6, &v6->sin6_addr, ip, sizeof(ip));
        port = ntohs(v6->sin6_port);
    }
    return std::string(ip) + ":" + std::to_string(port);
}
```

Ce header sera inclus par tous les exemples qui suivent. Dans un vrai projet, ces utilitaires vivraient dans un namespace dédié et seraient répartis en plusieurs fichiers — ici la concision prime.

---

## Serveur séquentiel : un client à la fois

Commençons par l'architecture la plus simple possible : un serveur qui accepte une connexion, la traite entièrement, puis passe à la suivante. C'est un serveur **séquentiel** — un seul client est servi à la fois.

### Un serveur echo TCP complet

Le serveur echo est le « Hello World » du réseau : il renvoie au client exactement ce qu'il reçoit.

```cpp
// echo_server_sequential.cpp
#include "net_utils.hpp"
#include <csignal>

// Flag global pour l'arrêt propre
static volatile sig_atomic_t running = 1;

void signal_handler(int) { running = 0; }

void handle_client(Socket client, const std::string& client_info) {
    std::println("[{}] Connexion établie", client_info);

    char buffer[4096];

    while (true) {
        ssize_t n = recv(client.fd(), buffer, sizeof(buffer), 0);

        if (n == -1) {
            if (errno == EINTR) continue;
            std::println(stderr, "[{}] Erreur recv: {}", client_info, strerror(errno));
            break;
        }

        if (n == 0) {
            std::println("[{}] Déconnexion", client_info);
            break;
        }

        // Echo : renvoyer les données reçues
        try {
            send_all(client.fd(), buffer, static_cast<size_t>(n));
        } catch (const std::system_error& e) {
            std::println(stderr, "[{}] Erreur send: {}", client_info, e.what());
            break;
        }
    }
    // client est fermé automatiquement (RAII)
}

int main() {
    // Ignorer SIGPIPE globalement (alternative à MSG_NOSIGNAL)
    signal(SIGPIPE, SIG_IGN);

    // Arrêt propre sur SIGINT (Ctrl+C) et SIGTERM
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    auto addr = resolve(nullptr, "8080", AF_INET6, SOCK_STREAM);
    Socket server{addr->ai_family, addr->ai_socktype, addr->ai_protocol};
    server.enable_reuse_addr();

    // Dual-stack : accepter IPv4 et IPv6
    server.set_option(IPPROTO_IPV6, IPV6_V6ONLY, 0);

    if (bind(server.fd(), addr->ai_addr, addr->ai_addrlen) == -1)
        throw std::system_error(errno, std::system_category(), "bind()");

    if (listen(server.fd(), SOMAXCONN) == -1)
        throw std::system_error(errno, std::system_category(), "listen()");

    std::println("Serveur echo en écoute sur le port 8080 (séquentiel)");

    while (running) {
        sockaddr_storage client_addr{};
        socklen_t addr_len = sizeof(client_addr);

        int client_fd = accept4(server.fd(),
                                reinterpret_cast<sockaddr*>(&client_addr),
                                &addr_len, SOCK_CLOEXEC);

        if (client_fd == -1) {
            if (errno == EINTR) continue;  // Signal reçu — vérifier running
            throw std::system_error(errno, std::system_category(), "accept4()");
        }

        auto info = format_address(client_addr);
        handle_client(Socket{client_fd}, info);
    }

    std::println("Arrêt du serveur.");
}
```

### Le client correspondant

```cpp
// echo_client.cpp
#include "net_utils.hpp"

Socket connect_to(const char* host, const char* port) {
    auto addr = resolve(host, port, AF_UNSPEC, SOCK_STREAM);

    for (auto* rp = addr.get(); rp != nullptr; rp = rp->ai_next) {
        int fd = socket(rp->ai_family, rp->ai_socktype | SOCK_CLOEXEC,
                        rp->ai_protocol);
        if (fd == -1) continue;

        if (connect(fd, rp->ai_addr, rp->ai_addrlen) == 0) {
            return Socket{fd};
        }
        close(fd);
    }
    throw std::runtime_error("Impossible de se connecter à "
                             + std::string(host) + ":" + port);
}

int main() {
    auto conn = connect_to("localhost", "8080");
    std::println("Connecté au serveur");

    std::string line;
    while (std::print("> "), std::getline(std::cin, line)) {
        if (line.empty()) continue;
        if (line == "quit") break;

        // Envoyer
        send_all(conn.fd(), line.data(), line.size());

        // Recevoir la réponse (echo)
        char buffer[4096];
        ssize_t n = recv(conn.fd(), buffer, sizeof(buffer), 0);

        if (n <= 0) {
            std::println("Connexion fermée par le serveur");
            break;
        }

        std::println("Echo: {}", std::string_view(buffer, static_cast<size_t>(n)));
    }
}
```

### Test

```bash
# Terminal 1
$ g++ -std=c++23 -o echo_server echo_server_sequential.cpp
$ ./echo_server
Serveur echo en écoute sur le port 8080 (séquentiel)

# Terminal 2
$ g++ -std=c++23 -o echo_client echo_client.cpp
$ ./echo_client
Connecté au serveur
> hello
Echo: hello
> bonjour le monde
Echo: bonjour le monde
> quit
```

### Limites du serveur séquentiel

Ce serveur fonctionne, mais il souffre d'un défaut rédhibitoire : **pendant qu'il traite un client, tous les autres attendent**. Si le client A envoie des données lentement (ou ne fait rien pendant 30 secondes), les clients B, C et D sont bloqués dans la file du backlog. Et si plus de `SOMAXCONN` clients s'accumulent, les nouvelles connexions sont rejetées.

Pour un outil de diagnostic local ou un prototype, c'est acceptable. Pour tout le reste, il faut gérer la concurrence.

---

## Serveur multi-threadé : un thread par client

L'approche la plus intuitive pour servir plusieurs clients simultanément est de dédier un **thread par connexion**. Le thread principal accepte les connexions et les dispatche immédiatement à des threads de traitement.

```cpp
// echo_server_threaded.cpp
#include "net_utils.hpp"
#include <thread>
#include <vector>
#include <csignal>

static volatile sig_atomic_t running = 1;  
void signal_handler(int) { running = 0; }  

void handle_client(Socket client, std::string client_info) {
    std::println("[{}] Connexion établie (thread {})",
                 client_info, std::this_thread::get_id());

    char buffer[4096];

    while (true) {
        ssize_t n = recv(client.fd(), buffer, sizeof(buffer), 0);
        if (n == -1) {
            if (errno == EINTR) continue;
            std::println(stderr, "[{}] Erreur: {}", client_info, strerror(errno));
            break;
        }
        if (n == 0) {
            std::println("[{}] Déconnexion", client_info);
            break;
        }

        try {
            send_all(client.fd(), buffer, static_cast<size_t>(n));
        } catch (const std::system_error&) {
            break;
        }
    }
}

int main() {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    auto addr = resolve(nullptr, "8080", AF_INET6, SOCK_STREAM);
    Socket server{addr->ai_family, addr->ai_socktype, addr->ai_protocol};
    server.enable_reuse_addr();
    server.set_option(IPPROTO_IPV6, IPV6_V6ONLY, 0);

    if (bind(server.fd(), addr->ai_addr, addr->ai_addrlen) == -1)
        throw std::system_error(errno, std::system_category(), "bind()");
    if (listen(server.fd(), SOMAXCONN) == -1)
        throw std::system_error(errno, std::system_category(), "listen()");

    std::println("Serveur echo en écoute sur le port 8080 (multi-threadé)");

    std::vector<std::jthread> threads;

    while (running) {
        sockaddr_storage client_addr{};
        socklen_t addr_len = sizeof(client_addr);

        int client_fd = accept4(server.fd(),
                                reinterpret_cast<sockaddr*>(&client_addr),
                                &addr_len, SOCK_CLOEXEC);
        if (client_fd == -1) {
            if (errno == EINTR) continue;
            throw std::system_error(errno, std::system_category(), "accept4()");
        }

        auto info = format_address(client_addr);

        // Lancer un thread dédié pour ce client
        // std::jthread (C++20) se join automatiquement à la destruction
        threads.emplace_back(handle_client, Socket{client_fd}, std::move(info));
    }

    std::println("Arrêt du serveur — attente des threads...");
    // Les jthreads sont automatiquement rejoints quand le vector est détruit
}
```

### Transfert du socket au thread

Un point subtil : le `Socket` est **déplacé** vers le thread, pas copié (la copie est interdite). C'est la sémantique de déplacement de C++11 (chapitre 10) qui rend ce transfert de propriété propre :

```cpp
threads.emplace_back(handle_client, Socket{client_fd}, std::move(info));
//                                  ^^^^^^^^^^^^^^^^^
//                                  Le Socket est déplacé vers le thread
//                                  Le thread principal n'a plus accès au fd
```

Après le déplacement, c'est le thread client qui possède le socket et qui le fermera automatiquement quand `handle_client` retourne.

### Avantages et limites

**Avantages :**

- Simple à comprendre et à implémenter.
- Chaque client est traité indépendamment — un client lent ne bloque pas les autres.
- Le code de `handle_client` est strictement linéaire, facile à raisonner.

**Limites :**

- **Coût des threads** — Chaque thread consomme environ 8 MiB de stack par défaut sur Linux. À 1 000 clients simultanés, c'est 8 GiB rien que pour les stacks. À 10 000 clients, c'est intenable.
- **Overhead de création** — Créer et détruire un thread par connexion a un coût non négligeable (appel système `clone`, allocation de stack, structures noyau). Pour des connexions de courte durée (requêtes HTTP), ce coût peut dépasser le temps de traitement.
- **Complexité de synchronisation** — Si les threads partagent des ressources (compteurs, caches, état global), il faut des mutex (chapitre 21), ce qui ajoute de la complexité et des risques de deadlock.
- **Gestion du cycle de vie** — Le `vector<jthread>` croît indéfiniment dans cet exemple. Un vrai serveur devrait nettoyer les threads terminés.

Le modèle un-thread-par-client est adapté pour des serveurs gérant **quelques centaines** de connexions simultanées avec des traitements substantiels (calculs, accès base de données). Au-delà, il faut un thread pool ou un modèle événementiel (section 22.3).

---

## Serveur avec thread pool

Le thread pool résout les deux principaux problèmes du modèle précédent : le coût de création des threads et la consommation mémoire illimitée. Un nombre fixe de threads est créé au démarrage, et les connexions leur sont distribuées via une file d'attente partagée.

```cpp
// echo_server_pool.cpp
#include "net_utils.hpp"
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <csignal>

static volatile sig_atomic_t running = 1;  
void signal_handler(int) { running = 0; }  

// --- File d'attente thread-safe ---
class ConnectionQueue {  
public:  
    void push(Socket sock, std::string info) {
        {
            std::lock_guard lock{mutex_};
            queue_.emplace(std::move(sock), std::move(info));
        }
        cv_.notify_one();
    }

    // Retourne false si le shutdown est demandé
    bool pop(Socket& sock, std::string& info) {
        std::unique_lock lock{mutex_};
        cv_.wait(lock, [this] { return !queue_.empty() || shutdown_; });

        if (shutdown_ && queue_.empty()) return false;

        auto& [s, i] = queue_.front();
        sock = std::move(s);
        info = std::move(i);
        queue_.pop();
        return true;
    }

    void shutdown() {
        {
            std::lock_guard lock{mutex_};
            shutdown_ = true;
        }
        cv_.notify_all();
    }

private:
    struct Entry {
        Socket socket;
        std::string info;
        Entry(Socket s, std::string i)
            : socket{std::move(s)}, info{std::move(i)} {}
    };

    std::queue<Entry> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool shutdown_ = false;
};

void handle_client(Socket client, const std::string& client_info) {
    std::println("[{}] Traitement (thread {})",
                 client_info, std::this_thread::get_id());

    char buffer[4096];
    while (true) {
        ssize_t n = recv(client.fd(), buffer, sizeof(buffer), 0);
        if (n == -1) {
            if (errno == EINTR) continue;
            break;
        }
        if (n == 0) break;

        try {
            send_all(client.fd(), buffer, static_cast<size_t>(n));
        } catch (const std::system_error&) {
            break;
        }
    }
    std::println("[{}] Déconnexion", client_info);
}

void worker(ConnectionQueue& queue) {
    while (true) {
        Socket sock{-1};
        std::string info;

        if (!queue.pop(sock, info)) {
            break;  // Shutdown demandé
        }
        handle_client(std::move(sock), info);
    }
}

int main() {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    constexpr int num_workers = 8;

    auto addr = resolve(nullptr, "8080", AF_INET6, SOCK_STREAM);
    Socket server{addr->ai_family, addr->ai_socktype, addr->ai_protocol};
    server.enable_reuse_addr();
    server.set_option(IPPROTO_IPV6, IPV6_V6ONLY, 0);

    if (bind(server.fd(), addr->ai_addr, addr->ai_addrlen) == -1)
        throw std::system_error(errno, std::system_category(), "bind()");
    if (listen(server.fd(), SOMAXCONN) == -1)
        throw std::system_error(errno, std::system_category(), "listen()");

    std::println("Serveur echo en écoute sur le port 8080 ({} workers)", num_workers);

    // Démarrer le pool de workers
    ConnectionQueue queue;
    std::vector<std::jthread> workers;
    workers.reserve(num_workers);
    for (int i = 0; i < num_workers; ++i) {
        workers.emplace_back(worker, std::ref(queue));
    }

    // Boucle d'accept
    while (running) {
        sockaddr_storage client_addr{};
        socklen_t addr_len = sizeof(client_addr);

        int client_fd = accept4(server.fd(),
                                reinterpret_cast<sockaddr*>(&client_addr),
                                &addr_len, SOCK_CLOEXEC);
        if (client_fd == -1) {
            if (errno == EINTR) continue;
            throw std::system_error(errno, std::system_category(), "accept4()");
        }

        queue.push(Socket{client_fd}, format_address(client_addr));
    }

    // Shutdown propre
    std::println("Arrêt du serveur...");
    queue.shutdown();
    // Les jthreads sont rejoints automatiquement
}
```

### Analyse de l'architecture

```
                    ┌──────────┐
                    │  accept  │ ◄── Thread principal
                    │  loop    │
                    └────┬─────┘
                         │ push(Socket)
                         ▼
                 ┌───────────────┐
                 │  Connection   │
                 │    Queue      │ ◄── std::queue protégée par mutex
                 └───┬───┬───┬───┘
                     │   │   │  pop(Socket)
                     ▼   ▼   ▼
                   ┌──┐ ┌──┐ ┌──┐
                   │W1│ │W2│ │W3│ ... ◄── Workers (threads fixes)
                   └──┘ └──┘ └──┘
```

Le thread principal ne fait que `accept()` et pousser les sockets dans la queue — une opération rapide. Les workers bloquent sur `pop()` en attendant du travail. Un worker qui termine avec un client retourne immédiatement à la queue pour en prendre un autre — pas de création/destruction de thread.

### Dimensionnement du pool

Le nombre de workers dépend de la nature du traitement :

- **I/O bound** (lecture réseau, accès disque, requêtes base de données) — Plus de workers que de cœurs CPU. Les threads passent l'essentiel de leur temps à attendre, donc un ratio de 2x à 4x le nombre de cœurs est raisonnable.
- **CPU bound** (calculs, compression, chiffrement) — Un worker par cœur CPU. Au-delà, les threads se disputent le CPU et les context switches dégradent les performances.
- **Mixte** — Commencez avec le nombre de cœurs et mesurez avec un profiler.

`std::thread::hardware_concurrency()` retourne le nombre de cœurs logiques de la machine — c'est un bon point de départ :

```cpp
int num_workers = std::max(4u, std::thread::hardware_concurrency());
```

### Limite du thread pool avec des sockets bloquants

Ce thread pool a une limitation subtile mais importante : chaque worker **bloque** sur `recv()` pendant toute la durée de vie de sa connexion. Un worker occupé par un client inactif (qui n'envoie rien pendant des minutes) est un worker indisponible pour les autres clients.

Avec 8 workers et 8 connexions inactives, le serveur est paralysé — les nouveaux clients s'accumulent dans la queue sans être servis.

C'est précisément le problème que résout le multiplexage I/O (`epoll`, section 22.3) : au lieu de bloquer un thread par connexion, un seul thread (ou un petit nombre) surveille **toutes les connexions** et ne traite que celles qui ont des données prêtes.

---

## Un protocole applicatif : au-delà de l'echo

L'echo est pédagogique, mais les vrais serveurs implémentent des **protocoles applicatifs** structurés. Construisons un micro-protocole requête/réponse utilisant le framing par longueur préfixée (section 22.1.3) et une sérialisation JSON simplifiée.

### Définition du protocole

```
Chaque message est composé de :
┌──────────────────┬──────────────────────────────────────┐
│  uint32_t (4 B)  │  payload JSON (len octets, UTF-8)    │
│  network order   │                                      │
└──────────────────┴──────────────────────────────────────┘

Requêtes (client → serveur) :
  {"cmd": "echo", "data": "..."}
  {"cmd": "time"}
  {"cmd": "stats"}

Réponses (serveur → client) :
  {"status": "ok", "data": "..."}
  {"status": "error", "message": "..."}
```

### Fonctions d'envoi/réception de messages

```cpp
// protocol.hpp
#pragma once

#include "net_utils.hpp"
#include <string>
#include <cstdint>
#include <optional>

constexpr uint32_t MAX_MSG_SIZE = 1024 * 1024;  // 1 MiB

// Envoyer un message framé
void send_message(int sockfd, std::string_view payload) {
    uint32_t len_net = htonl(static_cast<uint32_t>(payload.size()));
    send_all(sockfd, &len_net, sizeof(len_net));
    send_all(sockfd, payload.data(), payload.size());
}

// Recevoir un message framé
// Retourne std::nullopt si le pair a fermé la connexion
std::optional<std::string> recv_message(int sockfd) {
    // Lire le header
    uint32_t len_net = 0;
    if (!recv_exact(sockfd, &len_net, sizeof(len_net))) {
        return std::nullopt;  // Connexion fermée
    }

    uint32_t len = ntohl(len_net);
    if (len > MAX_MSG_SIZE) {
        throw std::runtime_error("Message trop grand: " + std::to_string(len));
    }

    // Lire le payload
    std::string payload(len, '\0');
    if (!recv_exact(sockfd, payload.data(), len)) {
        throw std::runtime_error("Connexion fermée pendant lecture du payload");
    }

    return payload;
}
```

### Le serveur avec protocole

Pour garder l'exemple autonome (sans dépendance à une librairie JSON), nous utilisons un parsing simple par recherche de sous-chaînes. Dans un vrai projet, vous utiliseriez `nlohmann/json` (chapitre 24).

```cpp
// protocol_server.cpp
#include "protocol.hpp"
#include <chrono>
#include <atomic>
#include <thread>

struct ServerStats {
    std::atomic<uint64_t> total_requests{0};
    std::atomic<uint64_t> active_connections{0};
};

ServerStats stats;

std::string process_request(std::string_view request) {
    stats.total_requests++;

    if (request.find("\"echo\"") != std::string_view::npos) {
        // Extraire le champ "data" (parsing simplifié)
        auto pos = request.find("\"data\"");
        if (pos != std::string_view::npos) {
            auto start = request.find('"', pos + 6);
            auto end = request.find('"', start + 1);
            if (start != std::string_view::npos && end != std::string_view::npos) {
                auto data = request.substr(start + 1, end - start - 1);
                return "{\"status\":\"ok\",\"data\":\"" + std::string(data) + "\"}";
            }
        }
        return R"({"status":"error","message":"missing data field"})";
    }

    if (request.find("\"time\"") != std::string_view::npos) {
        auto now = std::chrono::system_clock::now();
        auto epoch = std::chrono::duration_cast<std::chrono::seconds>(
            now.time_since_epoch()).count();
        return "{\"status\":\"ok\",\"data\":\"" + std::to_string(epoch) + "\"}";
    }

    if (request.find("\"stats\"") != std::string_view::npos) {
        return "{\"status\":\"ok\","
               "\"requests\":" + std::to_string(stats.total_requests.load()) + ","
               "\"connections\":" + std::to_string(stats.active_connections.load()) + "}";
    }

    return R"({"status":"error","message":"unknown command"})";
}

void handle_client(Socket client, std::string client_info) {
    stats.active_connections++;
    std::println("[{}] Connecté", client_info);

    try {
        while (true) {
            auto msg = recv_message(client.fd());
            if (!msg) break;  // Connexion fermée

            std::println("[{}] Requête: {}", client_info, *msg);

            auto response = process_request(*msg);
            send_message(client.fd(), response);
        }
    } catch (const std::exception& e) {
        std::println(stderr, "[{}] Erreur: {}", client_info, e.what());
    }

    stats.active_connections--;
    std::println("[{}] Déconnecté", client_info);
}

int main() {
    signal(SIGPIPE, SIG_IGN);

    auto addr = resolve(nullptr, "9000", AF_INET6, SOCK_STREAM);
    Socket server{addr->ai_family, addr->ai_socktype, addr->ai_protocol};
    server.enable_reuse_addr();
    server.set_option(IPPROTO_IPV6, IPV6_V6ONLY, 0);

    if (bind(server.fd(), addr->ai_addr, addr->ai_addrlen) == -1)
        throw std::system_error(errno, std::system_category(), "bind()");
    if (listen(server.fd(), SOMAXCONN) == -1)
        throw std::system_error(errno, std::system_category(), "listen()");

    std::println("Serveur protocole en écoute sur le port 9000");

    while (true) {
        sockaddr_storage client_addr{};
        socklen_t addr_len = sizeof(client_addr);

        int client_fd = accept4(server.fd(),
                                reinterpret_cast<sockaddr*>(&client_addr),
                                &addr_len, SOCK_CLOEXEC);
        if (client_fd == -1) {
            if (errno == EINTR) continue;
            throw std::system_error(errno, std::system_category(), "accept4()");
        }

        auto info = format_address(client_addr);
        std::jthread(handle_client, Socket{client_fd}, std::move(info)).detach();
    }
}
```

### Le client correspondant

```cpp
// protocol_client.cpp
#include "protocol.hpp"

int main() {
    auto conn = connect_to("localhost", "9000");  // Voir echo_client.cpp
    std::println("Connecté au serveur");

    // Requête echo
    send_message(conn.fd(), R"({"cmd":"echo","data":"Hello, protocol!"})");
    if (auto resp = recv_message(conn.fd())) {
        std::println("Echo: {}", *resp);
    }

    // Requête time
    send_message(conn.fd(), R"({"cmd":"time"})");
    if (auto resp = recv_message(conn.fd())) {
        std::println("Time: {}", *resp);
    }

    // Requête stats
    send_message(conn.fd(), R"({"cmd":"stats"})");
    if (auto resp = recv_message(conn.fd())) {
        std::println("Stats: {}", *resp);
    }

    // Commande inconnue
    send_message(conn.fd(), R"({"cmd":"unknown"})");
    if (auto resp = recv_message(conn.fd())) {
        std::println("Unknown: {}", *resp);
    }
}
```

```
$ ./protocol_client
Connecté au serveur  
Echo: {"status":"ok","data":"Hello, protocol!"}  
Time: {"status":"ok","data":"1742558400"}  
Stats: {"status":"ok","requests":3,"connections":1}  
Unknown: {"status":"error","message":"unknown command"}  
```

Ce protocole simple illustre les mécanismes fondamentaux qu'on retrouve dans tous les protocoles réseau réels : framing, routage de commandes, sérialisation structurée et gestion d'erreurs au niveau applicatif.

---

## Arrêt propre (Graceful Shutdown)

Un serveur de production ne peut pas être brutalement tué par `kill -9` sans conséquences — les connexions en cours sont coupées, les données en transit sont perdues, les clients voient des erreurs. Un arrêt propre est nécessaire.

### Le mécanisme

L'arrêt propre suit un protocole en trois phases :

```
1. Recevoir le signal (SIGINT ou SIGTERM)
   └─► Positionner le flag running = false

2. Arrêter d'accepter de nouvelles connexions
   └─► Sortir de la boucle accept

3. Attendre que les connexions en cours se terminent
   └─► Avec un timeout pour ne pas attendre indéfiniment
```

### Le problème de `accept()` bloquant

Si le thread principal est bloqué dans `accept()` quand le signal arrive, `accept()` retourne `-1` avec `errno = EINTR` — ce qui nous permet de vérifier `running` et de sortir de la boucle. Cela fonctionne grâce au handler de signal que nous avons installé.

Mais il y a un cas problématique : si `accept()` est appelé **après** que le signal ait été traité mais **avant** la vérification de `running`, il bloquera indéfiniment — le signal a déjà été consommé.

La solution robuste utilise un mécanisme supplémentaire. La plus courante est le **self-pipe trick** : le handler de signal écrit un octet dans un pipe, et le thread principal utilise `poll()` pour surveiller à la fois le socket d'écoute et le pipe :

```cpp
// Self-pipe pour l'arrêt propre
int shutdown_pipe[2];

void signal_handler(int) {
    // Écrire un octet dans le pipe pour réveiller poll()
    // write() est async-signal-safe
    char c = 1;
    write(shutdown_pipe[1], &c, 1);
}

void accept_loop(int server_fd) {
    pipe2(shutdown_pipe, O_CLOEXEC | O_NONBLOCK);

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    pollfd fds[2];
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;       // Nouvelle connexion disponible
    fds[1].fd = shutdown_pipe[0];
    fds[1].events = POLLIN;       // Signal de shutdown

    while (true) {
        int ret = poll(fds, 2, -1);  // Attente infinie
        if (ret == -1 && errno == EINTR) continue;

        // Signal de shutdown reçu ?
        if (fds[1].revents & POLLIN) {
            std::println("Signal d'arrêt reçu");
            break;
        }

        // Nouvelle connexion ?
        if (fds[0].revents & POLLIN) {
            sockaddr_storage addr{};
            socklen_t len = sizeof(addr);
            int fd = accept4(server_fd, reinterpret_cast<sockaddr*>(&addr),
                             &len, SOCK_CLOEXEC);
            if (fd != -1) {
                // Dispatcher au pool de workers...
            }
        }
    }

    close(shutdown_pipe[0]);
    close(shutdown_pipe[1]);
}
```

> 💡 Le self-pipe trick est un pattern classique de la programmation système Unix. Depuis Linux 2.6.22, `signalfd()` offre une alternative plus élégante qui expose les signaux comme un file descriptor, directement utilisable avec `poll()` ou `epoll`.

---

## Comparaison des architectures

Voici un récapitulatif des architectures couvertes dans cette section, avec leurs caractéristiques et cas d'usage :

```
Architecture        Clients max    Complexité    Cas d'usage
────────────────    ───────────    ──────────    ─────────────────────────────
Séquentiel          1 à la fois    Très faible   Prototypes, outils CLI locaux,
                                                 scripts de test

Multi-threadé       ~500-1000      Faible        Serveurs d'applications avec
(1 thread/client)                                traitements longs (calcul, DB)

Thread pool         ~500-1000      Moyenne       Serveurs d'applications avec
                    (par pool)                   charge prévisible

Événementiel        ~10 000+       Élevée        Serveurs haute performance,
(epoll, sect 22.3)                               proxies, serveurs de chat
```

La vérité est que ces architectures ne sont pas mutuellement exclusives. Les serveurs de production les combinent souvent : un event loop `epoll` pour le multiplexage I/O, combiné à un thread pool pour les traitements CPU-bound. C'est le modèle de Nginx (event loop mono-thread pour l'I/O, workers pour le traitement), et c'est ce que les librairies comme Asio (section 22.4) facilitent.

---

## Tester avec les outils standard

Avant d'écrire un client dédié, vous pouvez tester vos serveurs avec les outils en ligne de commande de Linux :

### `nc` (netcat) — Le couteau suisse

```bash
# Se connecter à un serveur TCP
nc localhost 8080

# Envoyer un message et afficher la réponse
echo "Hello" | nc localhost 8080

# Écouter sur un port (serveur TCP improvisé)
nc -l 8080
```

### `curl` — Pour les serveurs HTTP

```bash
# Pas directement applicable à notre protocole binaire,
# mais indispensable dès qu'on touche à HTTP (section 22.5)
curl http://localhost:8080/
```

### `ss` — Inspecter l'état des sockets

```bash
# Voir les sockets en écoute
ss -tlnp

# Voir toutes les connexions TCP
ss -tnp

# Voir les connexions sur le port 8080
ss -tnp sport = :8080
```

### `tcpdump` — Voir les paquets sur le réseau

```bash
# Capturer le trafic sur le port 8080
sudo tcpdump -i lo port 8080 -X

# Voir le three-way handshake
sudo tcpdump -i lo port 8080 -c 10
```

Ces outils sont vos compagnons quotidiens pour le développement et le diagnostic réseau. `ss` remplace `netstat` (obsolète), et `tcpdump` est la première chose à lancer quand « ça ne marche pas » en réseau.

---

## Résumé

Cette section a assemblé les appels système de la section 22.1 en serveurs et clients fonctionnels, explorant trois architectures fondamentales :

- **Séquentiel** — Un client à la fois. Simple mais inutilisable dès qu'on a plusieurs clients. Bon pour comprendre le flux de base.
- **Multi-threadé** — Un thread par client. Facile à raisonner mais ne passe pas à l'échelle au-delà de quelques centaines de connexions.
- **Thread pool** — Nombre fixe de threads avec file d'attente partagée. Meilleur contrôle des ressources, mais chaque worker bloque encore sur les I/O de sa connexion.

Vous avez aussi vu comment implémenter un protocole applicatif avec framing par longueur préfixée, comment gérer l'arrêt propre avec le self-pipe trick, et comment utiliser les outils Linux pour le diagnostic.

La limite commune à toutes ces architectures est le **blocage sur les I/O** : un thread qui attend des données sur un socket ne peut rien faire d'autre. La section suivante résout ce problème fondamentalement, en introduisant le multiplexage I/O avec `epoll`.

---

> **Prochaine étape** → Section 22.3 : Multiplexage I/O — select, poll, epoll — où un seul thread surveille des milliers de connexions simultanément.

⏭️ [Multiplexage I/O : select, poll, epoll](/22-networking/03-multiplexage-io.md)
