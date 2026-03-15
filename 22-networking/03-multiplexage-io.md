🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 22.3 — Multiplexage I/O : select, poll, epoll

## Chapitre 22 : Networking et Communication

---

## Introduction

La section 22.2 a exposé la limite fondamentale des architectures basées sur des threads bloquants : chaque thread ne peut surveiller **qu'un seul socket** à la fois. Un thread bloqué dans `recv()` attend passivement qu'un client spécifique envoie des données — il ne peut pas vérifier si un autre client est prêt, ni accepter de nouvelles connexions.

Le multiplexage I/O résout ce problème de manière radicale. Au lieu de bloquer sur un socket individuel, vous bloquez sur un **ensemble de sockets**, et le noyau vous dit lesquels sont prêts. Un seul thread peut ainsi surveiller des milliers de connexions simultanément, ne traitant que celles qui ont effectivement des données à lire ou de la place pour écrire.

```
Thread pool (section 22.2)              Multiplexage I/O
──────────────────────────              ─────────────────

  Thread 1 ──► recv(client_A) 💤        Un seul thread :
  Thread 2 ──► recv(client_B) 💤          │
  Thread 3 ──► recv(client_C) 💤          ├── epoll_wait() 💤
  Thread 4 ──► recv(client_D) 💤          │   "Dis-moi quand A, B, C ou D est prêt"
                                          │
  4 threads pour 4 clients                ├── client_B est prêt → recv(client_B)
  1000 clients = 1000 threads             ├── client_D est prêt → recv(client_D)
                                          │
                                          1 thread pour N clients
```

C'est le modèle qui a permis le **C10K problem** (servir 10 000 connexions simultanées sur une seule machine) d'être résolu dans les années 2000, et qui sous-tend aujourd'hui les serveurs capables de gérer des centaines de milliers de connexions — Nginx, Redis, Node.js, HAProxy.

Linux offre trois mécanismes de multiplexage, par ordre chronologique : `select`, `poll` et `epoll`. Cette section couvre les trois, en expliquant pourquoi chacun a remplacé le précédent, et pourquoi `epoll` est le seul choix raisonnable en production.

---

## Le concept : readiness notification

Tous les mécanismes de multiplexage reposent sur le même principe fondamental : **la notification de disponibilité** (readiness notification). Vous fournissez au noyau une liste de file descriptors et les événements qui vous intéressent (données disponibles en lecture, place disponible en écriture, erreur), puis vous bloquez. Le noyau vous réveille dès qu'au moins un fd est prêt.

Les événements surveillables sont :

| Événement | Signification |
|-----------|---------------|
| **Lisible** (readable) | Des données sont disponibles dans le buffer de réception, ou une nouvelle connexion est en attente (`accept` possible), ou le pair a fermé la connexion (EOF) |
| **Écrivable** (writable) | De la place est disponible dans le buffer d'envoi pour `send()`, ou la connexion non-bloquante est établie |
| **Erreur** | Une erreur est survenue sur le socket |
| **Hangup** | Le pair a fermé la connexion |

Un socket d'écoute est considéré « lisible » quand une connexion attend dans la queue d'accept — c'est ce qui permet de multiplexer l'accept avec les I/O des clients existants.

---

## `select()` — Le doyen (1983)

### Signature

```cpp
#include <sys/select.h>

int select(int nfds, fd_set* readfds, fd_set* writefds,
           fd_set* exceptfds, struct timeval* timeout);
```

### Fonctionnement

`select()` surveille trois ensembles de file descriptors : ceux à surveiller en lecture, en écriture, et pour les conditions exceptionnelles. Chaque ensemble est représenté par un `fd_set` — un bitmap de taille fixe.

Vous remplissez les ensembles avec les fd qui vous intéressent, appelez `select()`, et il modifie les ensembles **en place** pour n'y laisser que les fd prêts. Le paramètre `nfds` doit être le plus grand fd + 1.

```cpp
fd_set read_fds;  
FD_ZERO(&read_fds);                    // Vider l'ensemble  
FD_SET(server_fd, &read_fds);          // Surveiller le socket d'écoute  
FD_SET(client1_fd, &read_fds);         // Surveiller le client 1  
FD_SET(client2_fd, &read_fds);         // Surveiller le client 2  

int max_fd = std::max({server_fd, client1_fd, client2_fd});

timeval timeout{};  
timeout.tv_sec = 30;  // Timeout de 30 secondes  

int ready = select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout);

if (ready > 0) {
    if (FD_ISSET(server_fd, &read_fds)) {
        // Nouvelle connexion en attente → accept()
    }
    if (FD_ISSET(client1_fd, &read_fds)) {
        // Client 1 a des données à lire → recv()
    }
    if (FD_ISSET(client2_fd, &read_fds)) {
        // Client 2 a des données à lire → recv()
    }
}
```

### Exemple : serveur echo avec `select()`

```cpp
#include "net_utils.hpp"
#include <sys/select.h>
#include <algorithm>
#include <vector>

int main() {
    signal(SIGPIPE, SIG_IGN);

    auto addr = resolve(nullptr, "8080", AF_INET6, SOCK_STREAM);
    Socket server{addr->ai_family, addr->ai_socktype, addr->ai_protocol};
    server.enable_reuse_addr();
    server.set_option(IPPROTO_IPV6, IPV6_V6ONLY, 0);

    if (bind(server.fd(), addr->ai_addr, addr->ai_addrlen) == -1)
        throw std::system_error(errno, std::system_category(), "bind()");
    if (listen(server.fd(), SOMAXCONN) == -1)
        throw std::system_error(errno, std::system_category(), "listen()");

    std::println("Serveur echo (select) sur le port 8080");

    std::vector<int> client_fds;  // File descriptors des clients connectés

    while (true) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server.fd(), &read_fds);

        int max_fd = server.fd();
        for (int fd : client_fds) {
            FD_SET(fd, &read_fds);
            max_fd = std::max(max_fd, fd);
        }

        int ready = select(max_fd + 1, &read_fds, nullptr, nullptr, nullptr);
        if (ready == -1) {
            if (errno == EINTR) continue;
            throw std::system_error(errno, std::system_category(), "select()");
        }

        // Nouvelle connexion ?
        if (FD_ISSET(server.fd(), &read_fds)) {
            int client_fd = accept4(server.fd(), nullptr, nullptr, SOCK_CLOEXEC);
            if (client_fd != -1) {
                std::println("Nouveau client: fd={}", client_fd);
                client_fds.push_back(client_fd);
            }
        }

        // Données des clients existants ?
        // Itération en sens inverse pour pouvoir supprimer pendant le parcours
        for (auto it = client_fds.begin(); it != client_fds.end(); ) {
            int fd = *it;
            if (!FD_ISSET(fd, &read_fds)) {
                ++it;
                continue;
            }

            char buffer[4096];
            ssize_t n = recv(fd, buffer, sizeof(buffer), 0);

            if (n <= 0) {
                // Déconnexion ou erreur
                std::println("Client fd={} déconnecté", fd);
                close(fd);
                it = client_fds.erase(it);
            } else {
                send(fd, buffer, static_cast<size_t>(n), MSG_NOSIGNAL);
                ++it;
            }
        }
    }
}
```

### Pourquoi `select()` est obsolète

`select()` souffre de limitations structurelles qui le rendent inadapté aux serveurs modernes :

**Limite `FD_SETSIZE`** — Le bitmap `fd_set` a une taille fixe, définie à la compilation. Sur Linux, `FD_SETSIZE` vaut typiquement **1024**. Si un file descriptor a une valeur ≥ 1024, vous ne pouvez pas le surveiller avec `select()`. Modifier `FD_SETSIZE` nécessite de recompiler la libc — ce n'est pas une option en pratique.

**Complexité O(n) à chaque appel** — À chaque appel, vous devez reconstruire les `fd_set` (puisque `select` les modifie), et le noyau doit scanner **tout** le bitmap pour trouver les fd positionnés, même s'il n'y en a que 3 sur 1000. Côté userspace, vous devez aussi itérer sur tous vos fd pour vérifier `FD_ISSET`.

**Modification en place** — `select` modifie les ensembles passés en paramètre, ce qui oblige à les reconstruire avant chaque appel. C'est une source de bugs et d'inefficacité.

**`nfds` doit être calculé** — Vous devez traquer le fd maximum en permanence, ce qui ajoute de la logique inutile.

> ⚠️ **N'utilisez `select()` dans aucun nouveau code.** Son seul avantage est la portabilité maximale (POSIX, Windows, tout Unix) — mais si vous développez sur Linux, `epoll` est strictement supérieur. Si vous avez besoin de portabilité, utilisez une librairie d'abstraction comme Asio (section 22.4).

---

## `poll()` — L'amélioration (1986)

### Signature

```cpp
#include <poll.h>

int poll(struct pollfd* fds, nfds_t nfds, int timeout);

struct pollfd {
    int   fd;       // File descriptor à surveiller
    short events;   // Événements demandés (entrée)
    short revents;  // Événements survenus (sortie, rempli par le noyau)
};
```

### Amélioration par rapport à `select()`

`poll()` corrige les deux défauts les plus gênants de `select()` :

**Pas de limite de fd** — `poll()` utilise un tableau de structures `pollfd` au lieu d'un bitmap. Vous pouvez surveiller n'importe quel file descriptor, quelle que soit sa valeur numérique.

**Séparation entrée/sortie** — Les événements demandés (`events`) et les événements survenus (`revents`) sont dans des champs séparés. Plus besoin de reconstruire la structure à chaque appel — seul `revents` est modifié par le noyau.

### Événements

| Constante | `events` | `revents` | Signification |
|-----------|----------|-----------|---------------|
| `POLLIN` | ✓ | ✓ | Données disponibles en lecture |
| `POLLOUT` | ✓ | ✓ | Écriture possible sans blocage |
| `POLLERR` | | ✓ | Erreur sur le fd (toujours surveillé) |
| `POLLHUP` | | ✓ | Déconnexion (hangup) |
| `POLLNVAL` | | ✓ | Fd invalide (pas un fd ouvert) |

`POLLERR`, `POLLHUP` et `POLLNVAL` sont toujours reportés dans `revents`, même si vous ne les demandez pas dans `events`.

### Exemple : serveur echo avec `poll()`

```cpp
#include "net_utils.hpp"
#include <poll.h>
#include <vector>

int main() {
    signal(SIGPIPE, SIG_IGN);

    auto addr = resolve(nullptr, "8080", AF_INET6, SOCK_STREAM);
    Socket server{addr->ai_family, addr->ai_socktype, addr->ai_protocol};
    server.enable_reuse_addr();
    server.set_option(IPPROTO_IPV6, IPV6_V6ONLY, 0);

    if (bind(server.fd(), addr->ai_addr, addr->ai_addrlen) == -1)
        throw std::system_error(errno, std::system_category(), "bind()");
    if (listen(server.fd(), SOMAXCONN) == -1)
        throw std::system_error(errno, std::system_category(), "listen()");

    std::println("Serveur echo (poll) sur le port 8080");

    // Le premier élément est toujours le socket d'écoute
    std::vector<pollfd> poll_fds;
    poll_fds.push_back({server.fd(), POLLIN, 0});

    while (true) {
        int ready = poll(poll_fds.data(),
                         static_cast<nfds_t>(poll_fds.size()),
                         -1);  // -1 = attente infinie
        if (ready == -1) {
            if (errno == EINTR) continue;
            throw std::system_error(errno, std::system_category(), "poll()");
        }

        // Vérifier le socket d'écoute (index 0)
        if (poll_fds[0].revents & POLLIN) {
            int client_fd = accept4(server.fd(), nullptr, nullptr, SOCK_CLOEXEC);
            if (client_fd != -1) {
                std::println("Nouveau client: fd={}", client_fd);
                poll_fds.push_back({client_fd, POLLIN, 0});
            }
        }

        // Vérifier les clients (index 1 à N)
        for (size_t i = 1; i < poll_fds.size(); ) {
            if (poll_fds[i].revents == 0) {
                ++i;
                continue;
            }

            int fd = poll_fds[i].fd;

            if (poll_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                // Erreur ou déconnexion
                std::println("Client fd={} : erreur/déconnexion", fd);
                close(fd);
                poll_fds.erase(poll_fds.begin() + static_cast<ptrdiff_t>(i));
                continue;
            }

            if (poll_fds[i].revents & POLLIN) {
                char buffer[4096];
                ssize_t n = recv(fd, buffer, sizeof(buffer), 0);

                if (n <= 0) {
                    std::println("Client fd={} déconnecté", fd);
                    close(fd);
                    poll_fds.erase(poll_fds.begin() + static_cast<ptrdiff_t>(i));
                    continue;
                }

                send(fd, buffer, static_cast<size_t>(n), MSG_NOSIGNAL);
            }

            ++i;
        }
    }
}
```

### Pourquoi `poll()` ne suffit pas non plus

`poll()` résout les limitations cosmétiques de `select()` mais conserve son problème fondamental de **scalabilité** :

**Complexité O(n) dans le noyau** — À chaque appel, le noyau doit parcourir **tout** le tableau de `pollfd` pour installer les callbacks de notification sur chaque fd, attendre, puis parcourir à nouveau le tableau pour remplir les `revents`. Si vous surveillez 10 000 fd mais que seuls 3 sont prêts, le noyau fait quand même 10 000 vérifications.

**Copie userspace ↔ kernel à chaque appel** — Le tableau `pollfd` complet est copié de l'espace utilisateur vers l'espace noyau à chaque appel, puis recopié dans l'autre sens. Pour 10 000 fd, c'est environ 80 KiB de copies à chaque itération de la boucle événementielle.

**Pas de persistance** — Le noyau ne conserve aucun état entre les appels. Chaque appel à `poll()` repart de zéro : installer les callbacks, attendre, désinstaller les callbacks. Ce travail répété est du gaspillage pur quand l'ensemble de fd change rarement (ajout/suppression de quelques clients par seconde, mais surveillance de milliers).

Pour quelques dizaines de fd, `poll()` est parfaitement acceptable et plus simple qu'`epoll`. Au-delà de quelques centaines, la dégradation devient mesurable.

---

## `epoll` — La solution Linux (2002)

### Le changement de paradigme

`epoll` résout le problème de scalabilité en inversant le modèle. Au lieu de passer la liste complète des fd à chaque appel, vous créez une **instance epoll persistante** dans le noyau, vous y ajoutez/supprimez des fd au fil du temps, et vous demandez uniquement les événements survenus :

```
select/poll :                          epoll :
─────────────                          ──────

À chaque itération :                   Une fois :
  1. Construire la liste (N fd)          1. epoll_create()
  2. Copier vers le noyau               2. epoll_ctl(ADD, fd1)
  3. Noyau scanne N fd                  3. epoll_ctl(ADD, fd2)
  4. Copier les résultats                  ...
  5. Scanner les résultats
                                       À chaque itération :
  Coût : O(N) à chaque appel            1. epoll_wait()
                                         2. Seuls les fd PRÊTS sont retournés

                                       Coût : O(ready), pas O(total)
```

La différence est spectaculaire quand le nombre total de fd est grand mais que peu sont actifs à un instant donné — ce qui est le cas typique d'un serveur réseau (des milliers de connexions ouvertes, quelques dizaines actives simultanément).

### Les trois appels système

```cpp
#include <sys/epoll.h>

// Créer une instance epoll
int epoll_create1(int flags);

// Ajouter/modifier/supprimer un fd dans l'instance
int epoll_ctl(int epfd, int op, int fd, struct epoll_event* event);

// Attendre des événements
int epoll_wait(int epfd, struct epoll_event* events,
               int maxevents, int timeout);
```

### `epoll_create1()` — Créer l'instance

```cpp
int epfd = epoll_create1(EPOLL_CLOEXEC);  
if (epfd == -1) {  
    throw std::system_error(errno, std::system_category(), "epoll_create1()");
}
```

L'instance epoll est elle-même un file descriptor. Elle doit être fermée avec `close()` quand vous n'en avez plus besoin — un candidat naturel pour un wrapper RAII.

> 💡 L'ancienne fonction `epoll_create(int size)` existe encore mais le paramètre `size` est ignoré depuis Linux 2.6.8. Utilisez `epoll_create1` qui accepte des flags, notamment `EPOLL_CLOEXEC`.

### `epoll_ctl()` — Gérer les fd surveillés

```cpp
struct epoll_event {
    uint32_t events;    // Masque d'événements (EPOLLIN, EPOLLOUT, ...)
    epoll_data_t data;  // Données utilisateur associées au fd
};

typedef union epoll_data {
    void*    ptr;       // Pointeur vers vos données
    int      fd;        // Le fd lui-même
    uint32_t u32;
    uint64_t u64;
} epoll_data_t;
```

Les opérations :

```cpp
epoll_event ev{};  
ev.events = EPOLLIN;    // Surveiller la lecture  
ev.data.fd = client_fd; // Associer le fd pour le retrouver dans epoll_wait  

// Ajouter un fd
epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);

// Modifier les événements surveillés
ev.events = EPOLLIN | EPOLLOUT;  // Lecture ET écriture  
epoll_ctl(epfd, EPOLL_CTL_MOD, client_fd, &ev);  

// Supprimer un fd
epoll_ctl(epfd, EPOLL_CTL_DEL, client_fd, nullptr);
```

Le champ `data` est crucial : c'est votre moyen de retrouver le contexte associé à un fd quand `epoll_wait` le signale. Stocker le fd dans `data.fd` est le cas simple. Pour des serveurs plus complexes, vous utiliserez `data.ptr` pour pointer vers une structure contenant l'état de la connexion (buffer partiel, état du protocole, etc.).

### `epoll_wait()` — Récupérer les événements

```cpp
constexpr int MAX_EVENTS = 64;  
epoll_event events[MAX_EVENTS];  

int n = epoll_wait(epfd, events, MAX_EVENTS, -1);  // -1 = infini  
if (n == -1) {  
    if (errno == EINTR) { /* signal, réessayer */ }
    else { /* erreur réelle */ }
}

for (int i = 0; i < n; ++i) {
    int fd = events[i].data.fd;
    uint32_t ev = events[i].events;

    if (ev & EPOLLIN) {
        // fd est lisible
    }
    if (ev & EPOLLOUT) {
        // fd est inscriptible
    }
    if (ev & (EPOLLERR | EPOLLHUP)) {
        // Erreur ou déconnexion
    }
}
```

Le point essentiel : `epoll_wait` ne retourne **que les fd prêts**. Si vous surveillez 10 000 fd et que 3 sont prêts, `n` vaut 3. Vous n'itérez que sur 3 éléments, pas 10 000.

### Événements epoll

| Constante | Rôle |
|-----------|------|
| `EPOLLIN` | Données disponibles en lecture |
| `EPOLLOUT` | Écriture possible sans blocage |
| `EPOLLERR` | Erreur (toujours reporté, inutile de le demander) |
| `EPOLLHUP` | Hangup (toujours reporté) |
| `EPOLLRDHUP` | Le pair a fermé son côté écriture (half-close) — nécessite d'être demandé explicitement |
| `EPOLLET` | Mode edge-triggered (voir ci-dessous) |
| `EPOLLONESHOT` | Le fd est désactivé après un événement — nécessite `EPOLL_CTL_MOD` pour le réactiver |

---

## Serveur echo complet avec `epoll`

Voici un serveur echo événementiel utilisant `epoll`. Ce serveur gère des milliers de connexions dans un seul thread :

```cpp
// echo_server_epoll.cpp
#include "net_utils.hpp"
#include <sys/epoll.h>
#include <fcntl.h>
#include <csignal>

static volatile sig_atomic_t running = 1;  
void signal_handler(int) { running = 0; }  

// Passer un fd en mode non-bloquant
void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        throw std::system_error(errno, std::system_category(), "fcntl(F_GETFL)");
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        throw std::system_error(errno, std::system_category(), "fcntl(F_SETFL)");
}

// RAII wrapper pour l'instance epoll
class Epoll {  
public:  
    Epoll() : fd_{epoll_create1(EPOLL_CLOEXEC)} {
        if (fd_ == -1)
            throw std::system_error(errno, std::system_category(), "epoll_create1()");
    }
    ~Epoll() { if (fd_ != -1) close(fd_); }

    Epoll(const Epoll&) = delete;
    Epoll& operator=(const Epoll&) = delete;

    void add(int fd, uint32_t events) {
        epoll_event ev{};
        ev.events = events;
        ev.data.fd = fd;
        if (epoll_ctl(fd_, EPOLL_CTL_ADD, fd, &ev) == -1)
            throw std::system_error(errno, std::system_category(), "epoll_ctl(ADD)");
    }

    void modify(int fd, uint32_t events) {
        epoll_event ev{};
        ev.events = events;
        ev.data.fd = fd;
        if (epoll_ctl(fd_, EPOLL_CTL_MOD, fd, &ev) == -1)
            throw std::system_error(errno, std::system_category(), "epoll_ctl(MOD)");
    }

    void remove(int fd) {
        // EPOLL_CTL_DEL ignore le paramètre event depuis Linux 2.6.9
        epoll_ctl(fd_, EPOLL_CTL_DEL, fd, nullptr);
    }

    int wait(epoll_event* events, int max_events, int timeout_ms) {
        return epoll_wait(fd_, events, max_events, timeout_ms);
    }

    [[nodiscard]] int fd() const noexcept { return fd_; }

private:
    int fd_;
};

int main() {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Créer le socket d'écoute
    auto addr = resolve(nullptr, "8080", AF_INET6, SOCK_STREAM);
    Socket server{addr->ai_family,
                  addr->ai_socktype | SOCK_NONBLOCK,  // Non-bloquant !
                  addr->ai_protocol};
    server.enable_reuse_addr();
    server.set_option(IPPROTO_IPV6, IPV6_V6ONLY, 0);

    if (bind(server.fd(), addr->ai_addr, addr->ai_addrlen) == -1)
        throw std::system_error(errno, std::system_category(), "bind()");
    if (listen(server.fd(), SOMAXCONN) == -1)
        throw std::system_error(errno, std::system_category(), "listen()");

    // Créer l'instance epoll et y ajouter le socket d'écoute
    Epoll epoll;
    epoll.add(server.fd(), EPOLLIN);

    std::println("Serveur echo (epoll) sur le port 8080");

    constexpr int MAX_EVENTS = 256;
    epoll_event events[MAX_EVENTS];
    int num_clients = 0;

    while (running) {
        int n = epoll.wait(events, MAX_EVENTS, 1000);  // Timeout 1s
        if (n == -1) {
            if (errno == EINTR) continue;
            throw std::system_error(errno, std::system_category(), "epoll_wait()");
        }

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;

            // --- Nouvelle connexion ---
            if (fd == server.fd()) {
                // Accepter toutes les connexions en attente
                while (true) {
                    sockaddr_storage client_addr{};
                    socklen_t addr_len = sizeof(client_addr);

                    int client_fd = accept4(server.fd(),
                        reinterpret_cast<sockaddr*>(&client_addr),
                        &addr_len,
                        SOCK_NONBLOCK | SOCK_CLOEXEC);

                    if (client_fd == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;  // Plus de connexions en attente
                        }
                        std::println(stderr, "accept4: {}", strerror(errno));
                        break;
                    }

                    epoll.add(client_fd, EPOLLIN | EPOLLRDHUP);
                    num_clients++;

                    auto info = format_address(client_addr);
                    std::println("[{}] Connecté (fd={}, total={})",
                                 info, client_fd, num_clients);
                }
                continue;
            }

            // --- Événement sur un client ---

            // Erreur ou déconnexion
            if (ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
                epoll.remove(fd);
                close(fd);
                num_clients--;
                std::println("fd={} déconnecté (total={})", fd, num_clients);
                continue;
            }

            // Données disponibles
            if (ev & EPOLLIN) {
                char buffer[4096];
                ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);

                if (bytes <= 0) {
                    if (bytes == 0 || (bytes == -1 && errno != EAGAIN)) {
                        epoll.remove(fd);
                        close(fd);
                        num_clients--;
                        std::println("fd={} déconnecté (total={})", fd, num_clients);
                    }
                    continue;
                }

                // Echo — envoi non-bloquant
                ssize_t sent = send(fd, buffer, static_cast<size_t>(bytes),
                                    MSG_NOSIGNAL);
                if (sent == -1 && errno != EAGAIN) {
                    epoll.remove(fd);
                    close(fd);
                    num_clients--;
                }
            }
        }
    }

    std::println("Arrêt — {} clients encore connectés", num_clients);
}
```

### Points architecturaux importants

**Tous les sockets sont non-bloquants** — C'est une règle absolue avec `epoll`. Un `recv()` bloquant dans une boucle événementielle bloquerait le traitement de **tous** les autres clients. Le socket d'écoute est non-bloquant (`SOCK_NONBLOCK` à la création), et les sockets clients aussi (`SOCK_NONBLOCK` dans `accept4()`).

**Accept en boucle** — Quand le socket d'écoute est signalé comme lisible, il peut y avoir **plusieurs** connexions en attente. En mode level-triggered (le défaut), `epoll_wait` le re-signalera à la prochaine itération s'il reste des connexions, mais il est plus efficace de toutes les accepter en une fois.

**`EPOLLRDHUP`** — Ce flag (disponible depuis Linux 2.6.17) signale spécifiquement la fermeture de l'écriture par le pair. C'est plus fiable que de détecter la déconnexion via `recv()` retournant 0, car `EPOLLRDHUP` est signalé même si vous ne faites pas de `recv()`.

---

## Level-triggered vs edge-triggered

`epoll` offre deux modes de notification, et comprendre leur différence est crucial pour écrire un serveur correct.

### Level-triggered (LT) — le mode par défaut

En mode level-triggered, `epoll_wait` signale un fd **tant que** la condition est vraie :

- Si 1000 octets sont disponibles en lecture et que vous n'en lisez que 500, `epoll_wait` re-signalera le fd au prochain appel — il reste 500 octets à lire.
- Si le buffer d'envoi a de la place, `EPOLLOUT` est signalé **à chaque appel** tant qu'il y a de la place.

C'est le mode le plus intuitif et le plus sûr. Le risque de perdre des données est minimal — si vous oubliez de tout lire, `epoll_wait` vous le rappellera.

### Edge-triggered (ET) — `EPOLLET`

En mode edge-triggered, `epoll_wait` signale un fd **une seule fois**, au moment de la **transition** :

- Des données arrivent → `EPOLLIN` signalé une fois. Même si vous ne lisez rien, `epoll_wait` ne re-signalera pas le fd tant que de **nouvelles** données n'arrivent pas. Les données non lues restent dans le buffer, silencieusement.
- De la place se libère dans le buffer d'envoi → `EPOLLOUT` signalé une fois.

L'avantage du mode ET est la réduction du nombre de notifications : `epoll_wait` retourne moins d'événements, ce qui réduit l'overhead dans les serveurs très chargés.

Le prix est une **obligation de tout consommer** à chaque notification :

```cpp
// Mode edge-triggered : OBLIGATION de lire jusqu'à EAGAIN
if (ev & EPOLLIN) {
    while (true) {
        ssize_t n = recv(fd, buffer, sizeof(buffer), 0);
        if (n == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;  // Plus rien à lire — arrêter
            }
            // Erreur réelle
            break;
        }
        if (n == 0) {
            // Connexion fermée
            break;
        }
        process(buffer, n);
    }
}
```

Si vous oubliez de boucler jusqu'à `EAGAIN`, les données restantes dans le buffer seront **perdues** (jamais re-signalées) jusqu'à ce que de nouvelles données arrivent — un bug silencieux et intermittent, cauchemar à diagnostiquer.

### Activation du mode edge-triggered

```cpp
epoll.add(client_fd, EPOLLIN | EPOLLET);  // Ajouter le flag EPOLLET
```

### Quel mode choisir ?

**Level-triggered** — C'est le choix par défaut, et c'est celui que vous devriez utiliser sauf raison spécifique. Il est plus tolérant aux erreurs de programmation et plus facile à raisonner. C'est le mode utilisé par la plupart des applications.

**Edge-triggered** — Utile quand vous optimisez pour le throughput maximal sur un serveur gérant des dizaines de milliers de connexions. Nginx utilise `EPOLLET`. Mais le gain de performance n'est significatif que dans des scénarios extrêmes, et le risque de bugs est réel.

```
                Level-triggered          Edge-triggered
                ───────────────          ──────────────
Notification    Tant que prêt            À la transition  
Lecture         Lire ce qu'on veut       OBLIGÉ de tout lire (→ EAGAIN)  
Risque          Notifications multiples  Données perdues si oubli  
Complexité      Faible                   Élevée  
Usage           Par défaut               Optimisation haute perf  
```

---

## `EPOLLONESHOT` — Sécurité multi-thread

Quand plusieurs threads appellent `epoll_wait` sur la même instance epoll (modèle multi-thread + epoll), un même fd peut être signalé à **deux threads simultanément**, provoquant des accès concurrents non protégés sur la même connexion.

`EPOLLONESHOT` résout ce problème : après avoir été signalé une fois, le fd est automatiquement désactivé dans l'instance epoll. Le thread qui le traite doit explicitement le réactiver avec `EPOLL_CTL_MOD` quand il a terminé :

```cpp
// Ajout avec EPOLLONESHOT
epoll.add(client_fd, EPOLLIN | EPOLLONESHOT);

// Dans le worker, après traitement :
epoll.modify(client_fd, EPOLLIN | EPOLLONESHOT);  // Réactiver
```

C'est le mode recommandé pour les architectures multi-thread + epoll :

```
                    ┌──────────────────┐
                    │  Instance epoll  │
                    └────────┬─────────┘
                             │
              ┌──────────────┼──────────────┐
              ▼              ▼              ▼
          Thread 1       Thread 2       Thread 3
          epoll_wait()   epoll_wait()   epoll_wait()
              │
              ▼
          fd=7 signalé → Thread 1 le traite
          (fd=7 désactivé automatiquement — Thread 2 et 3 ne le voient pas)
              │
              ▼
          Traitement terminé → epoll_ctl(MOD, fd=7) → réactivé
```

---

## Benchmarks : scalabilité comparée

Pour illustrer concrètement la différence de performance, voici le comportement attendu en fonction du nombre de connexions surveillées :

```
Temps par itération (µs)
    │
200 │  select ╱
    │        ╱
150 │       ╱
    │      ╱     poll
100 │     ╱     ╱
    │    ╱     ╱
 50 │   ╱     ╱
    │  ╱     ╱
  0 │ ╱─────╱───────────── epoll (constant)
    └──────────────────────────────────
    0    2000   4000   6000   8000  10000
              Nombre de connexions
```

- **`select`** — Limité à ~1024 fd. Croissance linéaire, plafonné par `FD_SETSIZE`.
- **`poll`** — Pas de limite de fd, mais croissance linéaire avec le nombre de fd surveillés. Utilisable jusqu'à quelques milliers.
- **`epoll`** — Temps quasi-constant quel que soit le nombre total de fd. Le coût dépend du nombre de fd **prêts**, pas du nombre total. Testé à plus de 100 000 connexions simultanées.

---

## Comparaison synthétique

```
                  select           poll             epoll
                  ──────           ────             ─────
Max fd            FD_SETSIZE       Illimité         Illimité
                  (~1024)

Complexité        O(N total)       O(N total)       O(N prêts)  
par appel  

Copie user↔       Tout le bitmap   Tout le          Seuls les fd  
kernel            à chaque appel   tableau          prêts  

État noyau        Aucun            Aucun            Instance
                  (reconstruit)    (reconstruit)    persistante

Edge-triggered    Non              Non              Oui (EPOLLET)

Portabilité       POSIX, Windows   POSIX            Linux uniquement

Quand             Jamais en        < 100 fd         Toujours  
l'utiliser        nouveau code     Code simple      sur Linux  
```

---

## Patterns avancés avec `epoll`

### Gérer les écritures partielles

L'exemple echo ci-dessus triche légèrement : il fait un seul `send()` et suppose que toutes les données sont acceptées. En réalité, avec un socket non-bloquant, `send()` peut retourner moins que demandé (buffer d'envoi plein) ou échouer avec `EAGAIN`.

Le pattern correct implique un **buffer d'envoi par connexion** et la surveillance de `EPOLLOUT` :

```cpp
struct Connection {
    int fd;
    std::vector<std::byte> send_buffer;  // Données en attente d'envoi
};

void try_send(Epoll& epoll, Connection& conn) {
    while (!conn.send_buffer.empty()) {
        ssize_t sent = send(conn.fd, conn.send_buffer.data(),
                            conn.send_buffer.size(), MSG_NOSIGNAL);
        if (sent == -1) {
            if (errno == EAGAIN) {
                // Buffer noyau plein — surveiller EPOLLOUT
                epoll.modify(conn.fd, EPOLLIN | EPOLLOUT | EPOLLRDHUP);
                return;
            }
            // Erreur réelle — fermer la connexion
            throw std::system_error(errno, std::system_category(), "send()");
        }

        // Retirer les octets envoyés du buffer
        conn.send_buffer.erase(conn.send_buffer.begin(),
                                conn.send_buffer.begin() + sent);
    }

    // Tout envoyé — arrêter de surveiller EPOLLOUT
    epoll.modify(conn.fd, EPOLLIN | EPOLLRDHUP);
}

// Dans la boucle événementielle :
if (ev & EPOLLOUT) {
    try_send(epoll, connections[fd]);
}
```

Le principe est de ne surveiller `EPOLLOUT` que **quand il y a des données en attente**. Surveiller `EPOLLOUT` en permanence en mode level-triggered déclencherait une notification à chaque itération (le buffer d'envoi a presque toujours de la place) — un gaspillage appelé « busy notification ».

### Timeouts de connexion

`epoll_wait` ne gère que le timeout global de l'attente. Pour des timeouts par connexion (déconnecter un client inactif depuis 60 secondes), vous avez besoin d'une structure auxiliaire :

```cpp
#include <chrono>
#include <map>

using Clock = std::chrono::steady_clock;

// Suivi de la dernière activité par fd
std::map<int, Clock::time_point> last_activity;

// Dans la boucle, après chaque recv réussi :
last_activity[fd] = Clock::now();

// Vérification périodique (par exemple toutes les 10 secondes)
void check_timeouts(Epoll& epoll, std::map<int, Clock::time_point>& activity,
                    std::chrono::seconds max_idle) {
    auto now = Clock::now();
    for (auto it = activity.begin(); it != activity.end(); ) {
        if (now - it->second > max_idle) {
            int fd = it->first;
            std::println("fd={} : timeout après inactivité", fd);
            epoll.remove(fd);
            close(fd);
            it = activity.erase(it);
        } else {
            ++it;
        }
    }
}
```

Pour des timeouts précis à grande échelle, on utilise des structures spécialisées comme les **timer wheels** (roues de temporisation) — des structures O(1) pour l'insertion et l'expiration des timers, utilisées par le noyau Linux lui-même.

### `timerfd` — Intégrer les timers dans `epoll`

Linux offre `timerfd_create()` qui crée un file descriptor qui devient « lisible » quand un timer expire. Ce fd s'intègre directement dans `epoll`, éliminant le besoin de vérifications manuelles :

```cpp
#include <sys/timerfd.h>

// Créer un timer qui expire toutes les 10 secondes
int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);

itimerspec spec{};  
spec.it_interval.tv_sec = 10;  // Répétition toutes les 10s  
spec.it_value.tv_sec = 10;     // Première expiration dans 10s  
timerfd_settime(tfd, 0, &spec, nullptr);  

// Ajouter à epoll comme n'importe quel fd
epoll.add(tfd, EPOLLIN);

// Dans la boucle événementielle :
if (fd == tfd && (ev & EPOLLIN)) {
    uint64_t expirations;
    read(tfd, &expirations, sizeof(expirations));  // Consommer l'événement
    check_timeouts(/* ... */);
}
```

Ce pattern illustre la puissance du modèle « everything is a file descriptor » de Linux : sockets, timers, signaux (`signalfd`), événements de fichiers (`inotify`) — tout peut être multiplexé dans une seule boucle `epoll`.

---

## Alternatives à `epoll` sur d'autres systèmes

`epoll` est spécifique à Linux. Les autres systèmes ont leurs propres mécanismes haute performance :

| Système | Mécanisme | Équivalent de |
|---------|-----------|---------------|
| Linux | `epoll` | — |
| BSD / macOS | `kqueue` | `epoll` (souvent considéré comme plus élégant) |
| Windows | IOCP (I/O Completion Ports) | `epoll` (modèle proactif vs réactif) |
| Solaris | `/dev/poll`, event ports | `epoll` |

Si vous avez besoin de portabilité, les librairies d'abstraction comme Asio (section 22.4), libuv (le moteur de Node.js) ou libevent encapsulent ces mécanismes spécifiques derrière une interface unifiée. C'est la raison principale d'utiliser ces librairies plutôt que d'appeler `epoll` directement.

---

## Quand utiliser `epoll` directement vs une librairie

**Utilisez `epoll` directement** quand :

- Vous construisez une librairie bas niveau ou un framework réseau.
- Vous avez besoin de performances maximales sans aucune abstraction.
- Le projet est exclusivement Linux et le restera.
- Vous voulez comprendre exactement ce qui se passe (formation, diagnostic).

**Utilisez Asio ou une librairie équivalente** quand :

- Vous construisez un serveur d'application (l'immense majorité des cas).
- La portabilité compte (Linux, macOS, Windows).
- Vous voulez des abstractions de plus haut niveau (timers, résolution DNS asynchrone, SSL).
- Vous préférez un modèle asynchrone structuré (coroutines, callbacks) plutôt qu'une boucle événementielle brute.

Dans la pratique, même les projets purement Linux utilisent souvent Asio — non pas parce qu'`epoll` est insuffisant, mais parce que les abstractions de gestion d'état, de timers et de buffers qu'offre Asio sont un gain de productivité considérable.

---

## Résumé

Le multiplexage I/O est la technique fondamentale qui permet aux serveurs modernes de gérer des milliers de connexions simultanées avec un nombre réduit de threads :

- **`select()`** — Historique, limité à 1024 fd, complexité O(N) à chaque appel. N'utilisez jamais dans du nouveau code.
- **`poll()`** — Supprime la limite de fd, mais conserve la complexité O(N) et les copies systématiques. Acceptable pour quelques dizaines de fd.
- **`epoll`** — Instance persistante dans le noyau, complexité O(prêts) au lieu de O(total), supporte edge-triggered et multi-thread. C'est **le** choix sur Linux pour tout serveur sérieux.
- **Level-triggered** est le mode par défaut, sûr et simple. **Edge-triggered** offre plus de performance au prix de la complexité — réservé à l'optimisation avancée.
- **`EPOLLONESHOT`** est indispensable dans les architectures multi-thread + epoll.
- **`timerfd`**, **`signalfd`**, **`eventfd`** s'intègrent naturellement dans la boucle `epoll`, unifiant le traitement de tous les types d'événements.

---

> **Prochaine étape** → Section 22.4 : Librairies réseau modernes — où Asio encapsule tout ce que vous venez d'apprendre dans une abstraction asynchrone portable et puissante.

⏭️ [Librairies réseau modernes](/22-networking/04-librairies-reseau.md)
