🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 23.4 — Message queues POSIX

## Chapitre 23 : Processus et IPC

---

## Introduction

Les pipes (section 23.2) offrent un flux d'octets brut — comme TCP, sans frontières de messages. La mémoire partagée (section 23.3) offre des performances maximales mais impose une synchronisation manuelle complexe. Les **message queues POSIX** se positionnent entre les deux : elles fournissent un canal de communication structuré en **messages délimités**, avec gestion des **priorités**, le tout orchestré par le noyau.

Quand vous déposez un message dans une queue, le récepteur le récupère en entier — jamais fragmenté, jamais fusionné avec un autre. C'est le modèle producteur-consommateur clé en main de Linux : vous définissez la taille maximale des messages et la profondeur de la queue, et le noyau gère le stockage, l'ordre, le blocage et la notification.

```
Pipes :            flux d'octets continu ────────────────►
                   (pas de frontières)

Message queues :   [msg1] [msg2] [msg3] ──► [msg1] [msg2] [msg3]
                   messages individuels, délimités, prioritisés
```

---

## L'API POSIX Message Queue

L'API tient en une poignée de fonctions :

| Fonction | Rôle |
|----------|------|
| `mq_open` | Créer ou ouvrir une queue |
| `mq_send` | Envoyer un message |
| `mq_receive` | Recevoir un message |
| `mq_close` | Fermer le descripteur local |
| `mq_unlink` | Supprimer la queue du système |
| `mq_getattr` / `mq_setattr` | Lire / modifier les attributs |
| `mq_notify` | Demander une notification asynchrone à l'arrivée d'un message |
| `mq_timedsend` / `mq_timedreceive` | Versions avec timeout |

Toutes ces fonctions nécessitent la liaison avec `-lrt` (POSIX realtime library) :

```cmake
target_link_libraries(my_app PRIVATE rt)
```

---

## Création et ouverture : `mq_open()`

```cpp
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>

mqd_t mq_open(const char* name, int oflag);  
mqd_t mq_open(const char* name, int oflag, mode_t mode, struct mq_attr* attr);  
```

Le **descripteur** retourné (`mqd_t`) est similaire à un file descriptor mais spécifique aux message queues. Le nom suit la même convention que `shm_open` : il doit commencer par `/` et ne pas contenir d'autre `/`.

### Attributs de la queue

```cpp
struct mq_attr {
    long mq_flags;     // Flags (0 ou O_NONBLOCK)
    long mq_maxmsg;    // Nombre maximum de messages dans la queue
    long mq_msgsize;   // Taille maximale d'un message (octets)
    long mq_curmsgs;   // Nombre actuel de messages (lecture seule)
};
```

`mq_maxmsg` et `mq_msgsize` définissent la capacité de la queue à la création. Le noyau alloue l'espace nécessaire en interne. Sur Linux, ces valeurs sont bornées par les limites système :

```bash
# Limites par défaut sur Ubuntu
cat /proc/sys/fs/mqueue/msg_max        # Typiquement 10  
cat /proc/sys/fs/mqueue/msgsize_max    # Typiquement 8192  

# Augmenter (nécessite root ou ajustement via sysctl)
sudo sysctl -w fs.mqueue.msg_max=256  
sudo sysctl -w fs.mqueue.msgsize_max=65536  
```

### Exemple de création

```cpp
#include <mqueue.h>
#include <fcntl.h>
#include <print>

mqd_t create_queue(const char* name, long max_msgs, long max_msg_size) {
    mq_attr attr{};
    attr.mq_maxmsg = max_msgs;
    attr.mq_msgsize = max_msg_size;

    mqd_t mq = mq_open(name, O_CREAT | O_RDWR, 0666, &attr);
    if (mq == static_cast<mqd_t>(-1)) {
        throw std::system_error(errno, std::system_category(), "mq_open()");
    }

    return mq;
}

mqd_t open_existing(const char* name, bool read_only = false) {
    int flags = read_only ? O_RDONLY : O_RDWR;
    mqd_t mq = mq_open(name, flags);
    if (mq == static_cast<mqd_t>(-1)) {
        throw std::system_error(errno, std::system_category(), "mq_open()");
    }

    return mq;
}
```

### La queue dans le filesystem

Sur Linux, les message queues POSIX sont visibles dans un pseudo-filesystem qui doit être monté :

```bash
# Vérifier si le filesystem mqueue est monté
mount | grep mqueue
# mqueue on /dev/mqueue type mqueue (rw,nosuid,nodev,noexec,relatime)

# Si pas monté :
sudo mkdir -p /dev/mqueue  
sudo mount -t mqueue none /dev/mqueue  

# Les queues apparaissent comme des fichiers
ls -la /dev/mqueue/
# -rw-rw-r-- 1 user user 80 Mar 14 10:00 my_queue

# Contenu : informations sur la queue
cat /dev/mqueue/my_queue
# QSIZE:0          NOTIFY:0     SIGNO:0     NOTIFY_PID:0
```

---

## Envoi et réception de messages

### `mq_send()` — Envoyer un message

```cpp
#include <mqueue.h>

int mq_send(mqd_t mqdes, const char* msg_ptr, size_t msg_len,
            unsigned int msg_prio);
```

| Paramètre | Rôle |
|-----------|------|
| `mqdes` | Descripteur de la queue |
| `msg_ptr` | Pointeur vers les données du message |
| `msg_len` | Taille du message (doit être ≤ `mq_msgsize`) |
| `msg_prio` | Priorité (0 = la plus basse, 31 max recommandé) |

```cpp
const char* message = "Hello, queue!";  
if (mq_send(mq, message, strlen(message), 0) == -1) {  
    throw std::system_error(errno, std::system_category(), "mq_send()");
}
```

**Comportement bloquant** — Si la queue est pleine (`mq_curmsgs == mq_maxmsg`), `mq_send` bloque jusqu'à ce qu'un espace se libère (un consommateur retire un message). Avec `O_NONBLOCK`, il retourne immédiatement `-1` avec `errno = EAGAIN`.

### `mq_receive()` — Recevoir un message

```cpp
ssize_t mq_receive(mqd_t mqdes, char* msg_ptr, size_t msg_len,
                   unsigned int* msg_prio);
```

| Paramètre | Rôle |
|-----------|------|
| `mqdes` | Descripteur de la queue |
| `msg_ptr` | Buffer de réception |
| `msg_len` | Taille du buffer (doit être ≥ `mq_msgsize` de la queue) |
| `msg_prio` | Pointeur recevant la priorité du message (ou `nullptr`) |

```cpp
mq_attr attr;  
mq_getattr(mq, &attr);  

// Le buffer doit être au moins aussi grand que mq_msgsize
std::vector<char> buffer(static_cast<size_t>(attr.mq_msgsize));  
unsigned int priority;  

ssize_t n = mq_receive(mq, buffer.data(), buffer.size(), &priority);  
if (n == -1) {  
    throw std::system_error(errno, std::system_category(), "mq_receive()");
}

std::string_view message(buffer.data(), static_cast<size_t>(n));  
std::println("Reçu (priorité {}): {}", priority, message);  
```

**Comportement bloquant** — Si la queue est vide, `mq_receive` bloque jusqu'à l'arrivée d'un message. Avec `O_NONBLOCK`, il retourne `-1` avec `errno = EAGAIN`.

**Règle critique sur la taille du buffer** — Le buffer passé à `mq_receive` doit avoir une taille **au moins égale à `mq_msgsize`** (la taille maximale configurée à la création de la queue). Si le buffer est plus petit, `mq_receive` échoue avec `EMSGSIZE` — même si le message réel est plus petit que le buffer. C'est un piège classique.

---

## Priorités

Les message queues POSIX supportent les **priorités de messages**. Quand plusieurs messages sont en attente, `mq_receive` retourne toujours le message de **plus haute priorité** en premier. À priorité égale, l'ordre est FIFO (premier arrivé, premier servi).

```cpp
// Envoyer des messages avec différentes priorités
mq_send(mq, "urgence critique", 16, 10);   // Priorité 10 (haute)  
mq_send(mq, "tâche normale", 13, 1);        // Priorité 1 (basse)  
mq_send(mq, "alerte importante", 17, 5);    // Priorité 5 (moyenne)  
mq_send(mq, "routine", 7, 0);               // Priorité 0 (minimale)  

// Réception : ordre par priorité décroissante
unsigned int prio;  
char buf[256];  

mq_receive(mq, buf, sizeof(buf), &prio);  // "urgence critique"  (prio 10)  
mq_receive(mq, buf, sizeof(buf), &prio);  // "alerte importante" (prio 5)  
mq_receive(mq, buf, sizeof(buf), &prio);  // "tâche normale"     (prio 1)  
mq_receive(mq, buf, sizeof(buf), &prio);  // "routine"           (prio 0)  
```

Ce mécanisme de priorité est directement utilisable pour implémenter des files de traitement différencié : les requêtes critiques passent devant les requêtes de routine, sans que le producteur ait besoin de connaître l'état de la file.

---

## Versions avec timeout

Les versions temporisées évitent les blocages indéfinis — indispensable en production :

```cpp
#include <mqueue.h>
#include <time.h>

int mq_timedsend(mqd_t mqdes, const char* msg_ptr, size_t msg_len,
                 unsigned int msg_prio, const struct timespec* abs_timeout);

ssize_t mq_timedreceive(mqd_t mqdes, char* msg_ptr, size_t msg_len,
                         unsigned int* msg_prio, const struct timespec* abs_timeout);
```

Le timeout est une **deadline absolue** (heure système), pas une durée relative. Pour calculer la deadline :

```cpp
// Helper : deadline dans N secondes
timespec deadline_from_now(int seconds) {
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += seconds;
    return ts;
}

// Envoi avec timeout de 5 secondes
timespec deadline = deadline_from_now(5);  
int ret = mq_timedsend(mq, msg, len, priority, &deadline);  
if (ret == -1) {  
    if (errno == ETIMEDOUT) {
        std::println(stderr, "Envoi timeout — queue pleine depuis 5s");
    } else {
        throw std::system_error(errno, std::system_category(), "mq_timedsend()");
    }
}

// Réception avec timeout de 3 secondes
deadline = deadline_from_now(3);  
ssize_t n = mq_timedreceive(mq, buffer.data(), buffer.size(),  
                             &priority, &deadline);
if (n == -1) {
    if (errno == ETIMEDOUT) {
        std::println("Aucun message reçu en 3 secondes");
    } else {
        throw std::system_error(errno, std::system_category(), "mq_timedreceive()");
    }
}
```

---

## Notification asynchrone : `mq_notify()`

Au lieu de bloquer dans `mq_receive`, vous pouvez demander au noyau de vous notifier quand un message arrive dans une queue **vide**. La notification peut prendre la forme d'un signal ou du lancement d'un thread :

### Notification par signal

```cpp
#include <mqueue.h>
#include <csignal>

// Handler de signal
volatile sig_atomic_t message_available = 0;

void notify_handler(int sig, siginfo_t* info, void*) {
    message_available = 1;
}

void setup_notification(mqd_t mq) {
    // Installer le handler de signal
    struct sigaction sa{};
    sa.sa_sigaction = notify_handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR1, &sa, nullptr);

    // Demander la notification
    sigevent sev{};
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGUSR1;

    if (mq_notify(mq, &sev) == -1) {
        throw std::system_error(errno, std::system_category(), "mq_notify()");
    }
}
```

### Notification par thread

```cpp
// Le descripteur doit survivre aux appels de notification
// → variable globale ou heap-allouée (pas une locale)
mqd_t g_mq;

void message_arrived(sigval) {
    // Lire le message
    char buffer[8192];
    unsigned int prio;
    ssize_t n = mq_receive(g_mq, buffer, sizeof(buffer), &prio);

    if (n > 0) {
        std::println("[Thread notif] Reçu (prio {}): {}",
                     prio, std::string_view(buffer, static_cast<size_t>(n)));
    }

    // Réenregistrer la notification (elle est one-shot)
    sigevent sev{};
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = message_arrived;
    mq_notify(g_mq, &sev);
}
```

### Piège : la notification est one-shot

`mq_notify` ne notifie qu'**une seule fois** — quand la queue passe de vide à non-vide. Après chaque notification, vous devez **réenregistrer** l'abonnement avec un nouvel appel à `mq_notify`. Si vous oubliez de réenregistrer, les messages suivants arrivent silencieusement sans notification.

De plus, la notification est envoyée uniquement à la transition **vide → non-vide**. Si la queue n'est pas vide au moment du `mq_notify` (des messages sont en attente), la notification ne sera envoyée qu'après que la queue ait été vidée puis ait reçu un nouveau message.

En pratique, le pattern `mq_notify` est fragile et rarement utilisé en production. Il est généralement plus robuste de dédier un thread bloquant sur `mq_receive` ou d'intégrer la queue dans une boucle `epoll` (voir la section sur `mq_getattr` et les file descriptors plus bas).

---

## Fermeture et suppression

```cpp
// Fermer le descripteur (local au processus)
mq_close(mq);

// Supprimer la queue du système
// (les données persistent jusqu'à ce que tous les processus
//  aient fait mq_close ET que mq_unlink ait été appelé)
mq_unlink("/my_queue");
```

Comme pour `shm_unlink`, le `mq_unlink` supprime le **nom** de la queue. La queue elle-même continue d'exister tant que des processus l'ont ouverte. C'est un comportement analogue à `unlink` sur un fichier ordinaire sous Unix.

```bash
# Suppression en ligne de commande
rm /dev/mqueue/my_queue
```

---

## Wrapper RAII : `MessageQueue`

```cpp
#include <mqueue.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <system_error>
#include <chrono>

struct MQConfig {
    long max_messages = 10;
    long max_message_size = 4096;
};

class MessageQueue {  
public:  
    struct ReceivedMessage {
        std::string data;
        unsigned int priority;
    };

    // Créer une nouvelle queue
    static MessageQueue create(const std::string& name, MQConfig config = {}) {
        mq_attr attr{};
        attr.mq_maxmsg = config.max_messages;
        attr.mq_msgsize = config.max_message_size;

        mqd_t mq = mq_open(name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0666, &attr);
        if (mq == static_cast<mqd_t>(-1)) {
            throw std::system_error(errno, std::system_category(),
                                    "mq_open(create: " + name + ")");
        }

        return MessageQueue(name, mq, config.max_message_size, true);
    }

    // Ouvrir une queue existante
    static MessageQueue open(const std::string& name, int flags = O_RDWR) {
        mqd_t mq = mq_open(name.c_str(), flags);
        if (mq == static_cast<mqd_t>(-1)) {
            throw std::system_error(errno, std::system_category(),
                                    "mq_open(open: " + name + ")");
        }

        mq_attr attr;
        mq_getattr(mq, &attr);

        return MessageQueue(name, mq, attr.mq_msgsize, false);
    }

    ~MessageQueue() {
        if (mq_ != static_cast<mqd_t>(-1)) {
            mq_close(mq_);
        }
        if (owner_) {
            mq_unlink(name_.c_str());
        }
    }

    // Non copiable, déplaçable
    MessageQueue(const MessageQueue&) = delete;
    MessageQueue& operator=(const MessageQueue&) = delete;

    MessageQueue(MessageQueue&& other) noexcept
        : name_{std::move(other.name_)}, mq_{other.mq_},
          msg_size_{other.msg_size_}, owner_{other.owner_} {
        other.mq_ = static_cast<mqd_t>(-1);
        other.owner_ = false;
    }

    MessageQueue& operator=(MessageQueue&& other) noexcept {
        if (this != &other) {
            if (mq_ != static_cast<mqd_t>(-1)) mq_close(mq_);
            if (owner_) mq_unlink(name_.c_str());

            name_ = std::move(other.name_);
            mq_ = other.mq_;
            msg_size_ = other.msg_size_;
            owner_ = other.owner_;
            other.mq_ = static_cast<mqd_t>(-1);
            other.owner_ = false;
        }
        return *this;
    }

    // Envoyer un message (bloquant si queue pleine)
    void send(std::string_view message, unsigned int priority = 0) {
        if (mq_send(mq_, message.data(), message.size(), priority) == -1) {
            throw std::system_error(errno, std::system_category(), "mq_send()");
        }
    }

    // Envoyer avec timeout
    bool try_send(std::string_view message, unsigned int priority,
                  std::chrono::seconds timeout) {
        timespec deadline = make_deadline(timeout);
        int ret = mq_timedsend(mq_, message.data(), message.size(),
                               priority, &deadline);
        if (ret == -1) {
            if (errno == ETIMEDOUT) return false;
            throw std::system_error(errno, std::system_category(), "mq_timedsend()");
        }
        return true;
    }

    // Recevoir un message (bloquant si queue vide)
    ReceivedMessage receive() {
        std::vector<char> buffer(static_cast<size_t>(msg_size_));
        unsigned int priority;

        ssize_t n = mq_receive(mq_, buffer.data(), buffer.size(), &priority);
        if (n == -1) {
            throw std::system_error(errno, std::system_category(), "mq_receive()");
        }

        return {std::string(buffer.data(), static_cast<size_t>(n)), priority};
    }

    // Recevoir avec timeout
    std::optional<ReceivedMessage> try_receive(std::chrono::seconds timeout) {
        std::vector<char> buffer(static_cast<size_t>(msg_size_));
        unsigned int priority;
        timespec deadline = make_deadline(timeout);

        ssize_t n = mq_timedreceive(mq_, buffer.data(), buffer.size(),
                                     &priority, &deadline);
        if (n == -1) {
            if (errno == ETIMEDOUT) return std::nullopt;
            throw std::system_error(errno, std::system_category(), "mq_timedreceive()");
        }

        return ReceivedMessage{
            std::string(buffer.data(), static_cast<size_t>(n)),
            priority
        };
    }

    // Nombre de messages en attente
    long pending() const {
        mq_attr attr;
        mq_getattr(mq_, &attr);
        return attr.mq_curmsgs;
    }

    // Descripteur brut (pour epoll ou select)
    mqd_t native_handle() const noexcept { return mq_; }

private:
    MessageQueue(std::string name, mqd_t mq, long msg_size, bool owner)
        : name_{std::move(name)}, mq_{mq}, msg_size_{msg_size}, owner_{owner} {}

    static timespec make_deadline(std::chrono::seconds timeout) {
        timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeout.count();
        return ts;
    }

    std::string name_;
    mqd_t mq_ = static_cast<mqd_t>(-1);
    long msg_size_ = 0;
    bool owner_ = false;
};
```

---

## Exemple complet : producteur-consommateur

### Le producteur

```cpp
// producer.cpp
#include "message_queue.hpp"  // Le wrapper ci-dessus
#include <print>
#include <thread>
#include <chrono>

int main() {
    auto mq = MessageQueue::create("/task_queue", {
        .max_messages = 32,
        .max_message_size = 1024
    });

    std::println("Producteur démarré — queue /task_queue créée");

    for (int i = 0; i < 20; ++i) {
        // Priorité basée sur l'importance simulée
        unsigned int prio = (i % 5 == 0) ? 5 : 1;
        std::string msg = "Tâche #" + std::to_string(i);

        mq.send(msg, prio);
        std::println("  Envoyé [prio={}]: {}", prio, msg);

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // Envoyer un message de fin (priorité 0 = la plus basse)
    mq.send("__STOP__", 0);
    std::println("Signal d'arrêt envoyé");

    // Attendre que le consommateur ait vidé la queue avant de détruire
    while (mq.pending() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::println("Producteur terminé");
    // Le destructeur fait mq_close + mq_unlink (owner = true)
}
```

### Le consommateur

```cpp
// consumer.cpp
#include "message_queue.hpp"
#include <print>

int main() {
    auto mq = MessageQueue::open("/task_queue", O_RDONLY);

    std::println("Consommateur démarré — en attente de messages...");

    while (true) {
        // Réception avec timeout (pour ne pas bloquer indéfiniment
        // si le producteur meurt)
        auto msg = mq.try_receive(std::chrono::seconds(10));

        if (!msg) {
            std::println("Timeout — aucun message depuis 10s");
            break;
        }

        if (msg->data == "__STOP__") {
            std::println("Signal d'arrêt reçu — fin");
            break;
        }

        std::println("  Traité [prio={}]: {}", msg->priority, msg->data);

        // Simuler un traitement
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::println("Consommateur terminé");
}
```

```bash
# Terminal 1
$ ./producer
Producteur démarré — queue /task_queue créée
  Envoyé [prio=5]: Tâche #0
  Envoyé [prio=1]: Tâche #1
  Envoyé [prio=1]: Tâche #2
  ...

# Terminal 2
$ ./consumer
Consommateur démarré — en attente de messages...
  Traité [prio=5]: Tâche #0        ← Les messages haute priorité
  Traité [prio=5]: Tâche #5           passent en premier
  Traité [prio=5]: Tâche #10
  Traité [prio=1]: Tâche #1        ← Puis les messages normaux
  Traité [prio=1]: Tâche #2
  ...
Signal d'arrêt reçu — fin
```

---

## Intégration avec `epoll`

Sur Linux, les descripteurs de message queues POSIX (`mqd_t`) sont en réalité des file descriptors. Cela signifie qu'ils peuvent être surveillés avec `epoll`, `poll` ou `select` — ce qui permet de les intégrer dans une boucle événementielle aux côtés de sockets et de pipes :

```cpp
#include <sys/epoll.h>
#include <mqueue.h>

void event_loop(mqd_t mq, int socket_fd) {
    int epfd = epoll_create1(EPOLL_CLOEXEC);

    // Surveiller la message queue
    epoll_event ev_mq{};
    ev_mq.events = EPOLLIN;
    ev_mq.data.fd = static_cast<int>(mq);  // mqd_t est un fd sur Linux
    epoll_ctl(epfd, EPOLL_CTL_ADD, static_cast<int>(mq), &ev_mq);

    // Surveiller un socket
    epoll_event ev_sock{};
    ev_sock.events = EPOLLIN;
    ev_sock.data.fd = socket_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, socket_fd, &ev_sock);

    // Configurer la queue en non-bloquant
    mq_attr attr;
    mq_getattr(mq, &attr);
    attr.mq_flags = O_NONBLOCK;
    mq_setattr(mq, &attr, nullptr);

    epoll_event events[16];
    while (true) {
        int n = epoll_wait(epfd, events, 16, -1);

        for (int i = 0; i < n; ++i) {
            if (events[i].data.fd == static_cast<int>(mq)) {
                // Message queue : lire tous les messages disponibles
                char buffer[4096];
                unsigned int prio;
                ssize_t len;
                while ((len = mq_receive(mq, buffer, sizeof(buffer), &prio)) > 0) {
                    std::println("[MQ prio={}] {}",
                                 prio, std::string_view(buffer, len));
                }
            }
            else if (events[i].data.fd == socket_fd) {
                // Socket : traiter la connexion réseau
                // ...
            }
        }
    }

    close(epfd);
}
```

Cette capacité d'intégration avec `epoll` est un avantage significatif des message queues POSIX par rapport aux anciennes message queues System V. Elle permet de construire des serveurs hybrides qui réagissent à la fois aux événements réseau et aux messages IPC dans le même thread, sans polling ni threads dédiés.

---

## Plusieurs consommateurs

Les message queues POSIX supportent nativement **plusieurs lecteurs**. Si plusieurs processus appellent `mq_receive` sur la même queue, chaque message est délivré à **un seul** consommateur (pas de duplication). Le noyau réveille un seul processus en attente — le choix dépend des priorités de scheduling.

C'est un pattern de **work distribution** clé en main : un producteur dépose des tâches, un pool de workers les consomme en parallèle, et le noyau distribue le travail.

```
Producteur ──► [msg1] [msg2] [msg3] [msg4] [msg5]
                  │      │      │      │      │
                  ▼      ▼      ▼      ▼      ▼
              Worker A  Worker B  Worker A  Worker C  Worker B
              (chaque message est traité par un seul worker)
```

Aucune synchronisation côté application n'est nécessaire — le noyau garantit qu'un message n'est jamais délivré à deux consommateurs.

---

## Comparaison avec les pipes et la mémoire partagée

```
Critère               Pipe            Message Queue     Shared Memory
────────────────────  ──────────────  ────────────────  ──────────────
Modèle de données     Flux d'octets   Messages          Mémoire brute
                      (pas de         délimités          (accès libre)
                       frontières)
Priorités             Non             Oui               N/A  
Direction             Unidirectionnel Bidirectionnel(*) Bidirectionnel  
Processus liés        Oui (anonyme)   Non requis        Oui (anonyme)  
                      ou FIFO                            ou shm_open
Multi-consommateurs   Non (**)        Oui (1 msg →      Oui (mais
                                       1 consumer)       synchro manuelle)
Persistance           Non             Jusqu'à unlink    Jusqu'à unlink  
Performance           Bonne           Bonne             Excellente  
Synchronisation       Noyau (auto)    Noyau (auto)      Manuelle  
                                                         (mutex, sem)
Taille messages       Illimité (flux) Bornée (msgsize)  Bornée (segment)  
Intégration epoll     Oui             Oui (Linux)       Non  
Complexité            Très faible     Faible             Élevée  

(*) N'importe quel processus peut envoyer ET recevoir sur la même queue
(**) Un seul lecteur par pipe, mais plusieurs pipes possibles
```

### Guide de choix

**Utilisez un pipe** quand vous avez une relation parent-enfant simple avec un flux de données continu (capture stdout, pipeline de commandes).

**Utilisez une message queue** quand vous avez besoin de messages délimités, de priorités, de plusieurs consommateurs, ou de communication entre processus indépendants avec une sémantique producteur-consommateur. C'est le bon choix pour les files de tâches, les systèmes d'événements, et les commandes structurées.

**Utilisez la mémoire partagée** quand le volume de données est important, que la latence est critique, ou que vous avez besoin d'un accès aléatoire aux données (pas séquentiel). Le coût est la synchronisation manuelle.

**Utilisez un socket Unix** (chapitre 22) quand vous avez besoin de communication bidirectionnelle structurée, de passage de file descriptors, ou d'une sémantique de connexion (sessions client-serveur).

---

## Limites et considérations pratiques

### Limites système

Les message queues POSIX sont sujettes à des limites système qui peuvent surprendre :

```bash
# Nombre max de queues par utilisateur
cat /proc/sys/fs/mqueue/queues_max    # Typiquement 256

# Nombre max de messages par queue (défaut)
cat /proc/sys/fs/mqueue/msg_max       # Typiquement 10

# Taille max d'un message (défaut)
cat /proc/sys/fs/mqueue/msgsize_max   # Typiquement 8192

# Limites RLIMIT (par processus)
ulimit -q                              # Octets max pour toutes les queues du user
```

La limite `msg_max = 10` est particulièrement restrictive. Pour les applications avec un débit élevé, augmentez-la via `sysctl` ou au démarrage de votre service (nécessite les privilèges appropriés ou une configuration système).

### Pas de diffusion (broadcast)

Un message est consommé par **un seul** lecteur. Si vous avez besoin de diffuser un message à plusieurs consommateurs (pattern pub/sub), les message queues POSIX ne conviennent pas. Utilisez plutôt la mémoire partagée avec un ring buffer, des sockets multicast, ou un broker de messages externe (Redis, ZeroMQ, Kafka).

### Nettoyage des queues orphelines

Comme pour `shm_open`, les queues persistent dans `/dev/mqueue` jusqu'à `mq_unlink`. Un crash sans nettoyage laisse une queue orpheline. Automatisez le nettoyage au démarrage de vos services, ou utilisez le wrapper RAII avec ownership.

```bash
# Nettoyer les queues orphelines
rm /dev/mqueue/my_queue
# ou
ls /dev/mqueue/  # Inspecter et supprimer manuellement
```

---

## Résumé

Les message queues POSIX offrent un canal IPC structuré et géré par le noyau :

- **Messages délimités** — Chaque `mq_send` / `mq_receive` porte sur un message entier. Pas de framing applicatif nécessaire (contrairement aux pipes et TCP).
- **Priorités** — Les messages haute priorité sont délivrés en premier. Utile pour les systèmes de traitement différencié.
- **Multi-consommateurs** — Plusieurs processus peuvent lire la même queue. Le noyau garantit qu'un message n'est consommé que par un seul lecteur.
- **Timeout** — `mq_timedsend` et `mq_timedreceive` pour un contrôle fin des délais.
- **Intégration `epoll`** — Sur Linux, les message queues sont des file descriptors, intégrables dans une boucle événementielle.
- **Wrapper RAII** — Encapsulez la création/fermeture/unlink pour éviter les fuites de ressources.
- **Limites système** — Attention à `msg_max` (10 par défaut) et `msgsize_max` (8192 par défaut). Augmentez-les via `sysctl` pour les applications à haut débit.

---

> **Ce chapitre est maintenant terminé.** Vous avez couvert les quatre mécanismes IPC fondamentaux de Linux : fork/exec pour la gestion de processus, pipes pour la communication par flux, mémoire partagée pour le zéro-copie haute performance, et message queues pour les messages structurés avec priorités. Le module 8 (Parsing et Formats de Données) aborde la sérialisation — le complément naturel de la communication, qu'elle soit inter-processus ou réseau.

⏭️ [Module 8 : Parsing et Formats de Données](/module-08-parsing-formats.md)
