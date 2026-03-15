🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 20.2 — Installation de handlers (signal, sigaction)

## Module 7 — Programmation Système sur Linux *(Niveau Avancé)*

---

## Introduction

La section 20.1 a présenté les signaux, leurs sources et leurs actions par défaut. Cette section aborde la question centrale : comment **personnaliser** le comportement d'un programme face aux signaux ? Deux API coexistent — `signal()` (héritage du C) et `sigaction()` (API POSIX complète). Nous verrons pourquoi `sigaction()` est la seule option sérieuse, quelles contraintes draconiennes s'imposent dans un handler, et quels patterns permettent de gérer les signaux de manière fiable en production.

---

## `signal()` : l'API historique (à ne pas utiliser)

```cpp
#include <csignal>

void handler(int sig) {
    // ...
}

int main() {
    std::signal(SIGTERM, handler);   // Installer un handler
    std::signal(SIGPIPE, SIG_IGN);   // Ignorer un signal
    std::signal(SIGINT, SIG_DFL);    // Restaurer l'action par défaut
}
```

`signal()` est simple mais souffre de défauts rédhibitoires :

**Comportement non portable** — Le standard C ne spécifie pas si la disposition est réinitialisée à `SIG_DFL` après la délivrance du signal (sémantique "System V") ou si elle persiste (sémantique "BSD"). Sur Linux avec glibc, `signal()` utilise la sémantique BSD (le handler persiste), mais ce n'est pas garanti sur d'autres Unix. Un handler installé avec `signal()` peut donc disparaître après la première invocation sur certaines plateformes, créant une fenêtre de vulnérabilité.

**Pas de contrôle du masquage** — Pendant l'exécution du handler, il n'est pas possible de spécifier quels signaux doivent être bloqués. Sur certaines implémentations, le même signal peut être délivré une seconde fois alors que le handler de la première invocation est encore en cours.

**Pas d'informations étendues** — `signal()` ne donne accès qu'au numéro du signal. Impossible de connaître l'émetteur (PID), la raison (`si_code`) ou les données associées.

L'unique usage légitime de `signal()` est l'appel `std::signal(SIGPIPE, SIG_IGN)` pour ignorer `SIGPIPE`, car c'est une opération simple sans handler. Pour tout le reste, utilisez `sigaction()`.

---

## `sigaction()` : l'API correcte

```cpp
#include <signal.h>

int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);
```

`sigaction()` est l'API POSIX qui corrige tous les défauts de `signal()`. Elle offre un contrôle total sur l'installation du handler :

```cpp
struct sigaction {
    void     (*sa_handler)(int);                    // Handler simple
    void     (*sa_sigaction)(int, siginfo_t*, void*); // Handler étendu
    sigset_t   sa_mask;                             // Signaux masqués pendant le handler
    int        sa_flags;                            // Options de comportement
};
```

### Installation d'un handler simple

```cpp
#include <signal.h>
#include <unistd.h>
#include <cstring>
#include <print>

void term_handler(int sig) {
    const char msg[] = "Signal SIGTERM reçu — arrêt propre\n";
    write(STDERR_FILENO, msg, sizeof(msg) - 1);
    _exit(0);
}

int main() {
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));   // Initialiser à zéro — indispensable

    sa.sa_handler = term_handler;
    sigemptyset(&sa.sa_mask);          // Aucun signal supplémentaire masqué
    sa.sa_flags = 0;

    if (sigaction(SIGTERM, &sa, nullptr) == -1) {
        std::println("sigaction: {}", strerror(errno));
        return 1;
    }

    std::println("En attente de SIGTERM (PID = {})...", getpid());
    while (true) {
        pause();  // Suspend le processus jusqu'au prochain signal
    }
}
```

L'initialisation avec `memset` (ou `= {}`) est critique : une structure `sigaction` non initialisée contient des valeurs aléatoires dans `sa_mask` et `sa_flags`, ce qui produit un comportement imprévisible.

### Le masque `sa_mask`

`sa_mask` spécifie les signaux **supplémentaires** à masquer pendant l'exécution du handler. Le signal en cours de traitement est automatiquement masqué (sauf si `SA_NODEFER` est utilisé). Cela garantit qu'un handler ne s'interrompt pas lui-même :

```cpp
#include <signal.h>
#include <cstring>

int main() {
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = term_handler;

    // Pendant le traitement de SIGTERM, masquer aussi SIGINT et SIGHUP
    // pour éviter qu'un Ctrl+C n'interrompe la procédure d'arrêt propre
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGINT);
    sigaddset(&sa.sa_mask, SIGHUP);

    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, nullptr);
}
```

### Les flags de `sigaction`

| Flag | Effet |
|---|---|
| `SA_RESTART` | Redémarre automatiquement les appels système lents interrompus par le signal |
| `SA_SIGINFO` | Utilise `sa_sigaction` au lieu de `sa_handler` (handler étendu avec `siginfo_t`) |
| `SA_NODEFER` | Ne masque pas automatiquement le signal pendant l'exécution du handler |
| `SA_RESETHAND` | Restaure `SIG_DFL` après la première invocation (sémantique one-shot) |
| `SA_NOCLDSTOP` | Pour `SIGCHLD` : ne pas notifier les arrêts/reprises, uniquement les terminaisons |
| `SA_NOCLDWAIT` | Pour `SIGCHLD` : ne pas créer de zombie quand l'enfant termine |

**`SA_RESTART`** est le flag le plus important en pratique. Sans lui, les appels système "lents" (`read`, `write`, `accept`, `select`, `poll`, `sleep`, `wait`…) interrompus par le signal retournent `-1` avec `errno == EINTR`. Avec `SA_RESTART`, le noyau relance automatiquement l'appel système après le retour du handler :

```cpp
struct sigaction sa;  
std::memset(&sa, 0, sizeof(sa));  
sa.sa_handler = my_handler;  
sigemptyset(&sa.sa_mask);  

// ✅ Avec SA_RESTART : les read()/write()/accept() reprennent automatiquement
sa.sa_flags = SA_RESTART;  
sigaction(SIGTERM, &sa, nullptr);  

// ❌ Sans SA_RESTART : chaque read()/write() doit gérer EINTR manuellement
sa.sa_flags = 0;  
sigaction(SIGTERM, &sa, nullptr);  
```

En règle générale, utilisez `SA_RESTART` sauf si vous avez besoin que le signal interrompe un appel bloquant (par exemple, pour sortir d'un `read()` infini lors d'un arrêt propre).

### Handler étendu avec `SA_SIGINFO`

Le flag `SA_SIGINFO` active le handler étendu qui reçoit des informations supplémentaires via `siginfo_t` :

```cpp
#include <signal.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>

void detailed_handler(int sig, siginfo_t* info, void* /* ucontext */) {
    // Toutes les fonctions utilisées ici doivent être async-signal-safe
    char buf[128];
    int len = snprintf(buf, sizeof(buf),
        "Signal %d reçu de PID %d (UID %d), code=%d\n",
        sig, info->si_pid, info->si_uid, info->si_code);
    if (len > 0) {
        write(STDERR_FILENO, buf, static_cast<size_t>(len));
    }
}

int main() {
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));

    sa.sa_sigaction = detailed_handler;  // Utiliser sa_sigaction, PAS sa_handler
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;            // Activer le mode étendu

    sigaction(SIGUSR1, &sa, nullptr);

    pause();
}
```

Les champs utiles de `siginfo_t` :

| Champ | Description |
|---|---|
| `si_signo` | Numéro du signal |
| `si_pid` | PID de l'émetteur |
| `si_uid` | UID de l'émetteur |
| `si_code` | Raison du signal (`SI_USER`, `SI_KERNEL`, `CLD_EXITED`…) |
| `si_status` | Statut de l'enfant (pour `SIGCHLD`) |
| `si_addr` | Adresse fautive (pour `SIGSEGV`, `SIGBUS`) |
| `si_value` | Valeur transmise par `sigqueue()` (signaux temps réel) |

Le champ `si_addr` est particulièrement précieux pour `SIGSEGV` : il indique l'adresse mémoire qui a provoqué le fault, ce qui est un diagnostic essentiel pour les crashes.

---

## La contrainte d'async-signal-safety

C'est le concept le plus important de tout ce chapitre, et celui qui est le plus souvent violé. Un signal handler s'exécute dans un contexte très contraint : il interrompt le flux d'exécution normal à un point **arbitraire**. Cela signifie que la plupart des fonctions habituelles sont **interdites** dans un handler, car elles pourraient être interrompues au milieu de leur exécution, laissant leurs structures internes dans un état incohérent.

### Fonctions interdites dans un handler

Les fonctions suivantes (entre autres) ne doivent **jamais** être appelées dans un signal handler :

- `malloc`, `free`, `new`, `delete` — L'allocateur peut être dans un état incohérent (mutex interne verrouillé, liste de blocs libres en cours de modification).
- `printf`, `fprintf`, `std::cout`, `std::println` — Utilisent des buffers internes protégés par des mutexes et appellent `malloc`.
- `std::string`, `std::vector` et la plupart de la STL — Allouent de la mémoire dynamiquement.
- `exit()` — Exécute les handlers `atexit`, qui peuvent appeler n'importe quoi. Utiliser `_exit()` à la place.
- Les fonctions de `<mutex>`, `<thread>` — Pas async-signal-safe.
- Toute fonction qui prend un verrou (mutex, spinlock) que le code interrompu pourrait détenir.

### Fonctions autorisées (async-signal-safe)

POSIX définit une liste restreinte de fonctions garanties async-signal-safe. Les plus utiles sont :

| Fonction | Usage dans un handler |
|---|---|
| `write()` | Écrire un message de diagnostic sur stderr |
| `read()` | Lire depuis un descripteur |
| `_exit()` / `_Exit()` | Terminer le processus immédiatement |
| `signal()` / `sigaction()` | Modifier la disposition d'un signal |
| `kill()` / `raise()` | Envoyer un signal |
| `open()` / `close()` | Ouvrir/fermer un descripteur |
| `fork()` | Créer un processus enfant |
| `execve()` | Remplacer le processus |
| `wait()` / `waitpid()` | Attendre un enfant |
| `getpid()` / `getppid()` | Identité du processus |
| `alarm()` | Configurer un timer |
| `sigprocmask()` | Modifier le masque de signaux |
| `sigpending()` | Consulter les signaux en attente |
| `sem_post()` | Incrémenter un sémaphore |
| `abort()` | Terminer avec core dump |

La liste complète est disponible dans `man 7 signal-safety`. La règle pratique : si une fonction n'est pas dans cette liste, elle est interdite.

### Conséquences pour le handler

Un handler correct est **minimaliste**. Il ne fait que l'une des choses suivantes :

1. Positionner un flag atomique que la boucle principale consulte.
2. Écrire un octet dans un pipe (self-pipe trick).
3. Appeler `_exit()` pour terminer immédiatement.

Tout le reste — journalisation, sauvegarde de données, fermeture de connexions, nettoyage de fichiers temporaires — est fait dans le code principal, **après** détection du flag ou de l'événement pipe.

---

## Pattern 1 : Flag atomique (le plus courant)

Le pattern le plus simple et le plus répandu pour l'arrêt propre :

```cpp
#include <signal.h>
#include <atomic>
#include <print>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <chrono>

// Variable globale — le seul moyen de communiquer avec un handler
// volatile sig_atomic_t est le type garanti par le standard C
// std::atomic<bool> est aussi sûr en pratique sur Linux
volatile sig_atomic_t g_shutdown_requested = 0;

void shutdown_handler(int /* sig */) {
    g_shutdown_requested = 1;
    // C'est TOUT ce que fait le handler. Rien d'autre.
}

int main() {
    // Installer le handler pour SIGTERM et SIGINT
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = shutdown_handler;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGINT);   // Pendant SIGTERM, masquer SIGINT
    sigaddset(&sa.sa_mask, SIGTERM);  // Pendant SIGINT, masquer SIGTERM
    sa.sa_flags = 0;  // Pas de SA_RESTART — on veut interrompre les appels bloquants

    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);

    std::println("Service démarré (PID = {}). Ctrl+C ou kill -TERM pour arrêter.",
        getpid());

    // Boucle principale — vérifie le flag régulièrement
    while (!g_shutdown_requested) {
        // Simuler du travail
        std::println("Traitement en cours...");
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // === Zone de nettoyage — code normal, toutes les fonctions sont autorisées ===
    std::println("Arrêt propre en cours...");

    // Sauvegarder l'état
    // Fermer les connexions réseau
    // Supprimer les fichiers temporaires
    // Flush des logs

    std::println("Arrêt terminé.");
    return 0;
}
```

Les avantages de ce pattern : le handler est minimaliste (une seule affectation), le nettoyage se fait dans le code normal (pas de contrainte d'async-signal-safety), et le flag est testable depuis n'importe quel point de la boucle principale.

### `volatile sig_atomic_t` vs `std::atomic<bool>`

Le standard C garantit que `volatile sig_atomic_t` est le seul type dont l'accès est atomique vis-à-vis des signaux. En pratique sur Linux x86_64, `std::atomic<bool>` fonctionne aussi car les lectures et écritures de `bool` sont atomiques au niveau matériel, et `std::atomic` ajoute les barrières mémoire nécessaires.

La recommandation pour du code C++ moderne est :

```cpp
// ✅ Portable et garanti par le standard C/C++
volatile sig_atomic_t g_flag = 0;

// ✅ Fonctionne en pratique sur Linux, plus idiomatique C++
// Utiliser uniquement load/store (pas fetch_add, compare_exchange, etc.)
std::atomic<bool> g_flag{false};
```

Dans un handler, n'utilisez que des opérations de lecture (`load`) et d'écriture (`store`) simples. Les opérations read-modify-write (`fetch_add`, `compare_exchange`) ne sont pas async-signal-safe.

---

## Pattern 2 : Self-pipe trick

Le flag atomique a une limitation : si le code principal est bloqué dans un appel système (`read`, `accept`, `poll`, `epoll_wait`), il ne peut pas vérifier le flag tant que l'appel ne retourne pas. Sans `SA_RESTART`, le signal interrompt l'appel avec `EINTR`, mais cela oblige à gérer `EINTR` partout.

Le **self-pipe trick** résout élégamment ce problème en convertissant le signal en un événement de descripteur de fichier, intégrable dans la boucle d'événements (`poll`/`epoll`) :

```cpp
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <print>
#include <cstring>
#include <cerrno>

// Pipe global — le handler écrit dedans, la boucle principale le surveille
int g_signal_pipe[2] = {-1, -1};

void pipe_handler(int sig) {
    // Écrire un octet identifiant le signal dans le pipe
    // write() sur un pipe est async-signal-safe
    char s = static_cast<char>(sig);
    write(g_signal_pipe[1], &s, 1);  // Ignorer le retour — best effort
}

int main() {
    // Créer le pipe en mode non-bloquant
    if (pipe(g_signal_pipe) == -1) {
        std::println("pipe: {}", strerror(errno));
        return 1;
    }
    fcntl(g_signal_pipe[0], F_SETFL, O_NONBLOCK);
    fcntl(g_signal_pipe[1], F_SETFL, O_NONBLOCK);
    fcntl(g_signal_pipe[0], F_SETFD, FD_CLOEXEC);
    fcntl(g_signal_pipe[1], F_SETFD, FD_CLOEXEC);

    // Installer le handler
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = pipe_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);

    std::println("Serveur démarré (PID = {})", getpid());

    // Boucle d'événements avec poll
    // En production, on surveillerait aussi les sockets clients
    struct pollfd fds[1];
    fds[0].fd = g_signal_pipe[0];
    fds[0].events = POLLIN;

    bool running = true;
    while (running) {
        int ret = poll(fds, 1, 1000);  // Timeout 1 seconde

        if (ret > 0 && (fds[0].revents & POLLIN)) {
            // Signal reçu — lire l'octet du pipe
            char sig;
            while (read(g_signal_pipe[0], &sig, 1) > 0) {
                std::println("Signal {} reçu via self-pipe", static_cast<int>(sig));
                running = false;
            }
        }

        if (running) {
            // Travail normal...
            std::println("En attente de connexions...");
        }
    }

    // Nettoyage (code normal)
    std::println("Arrêt propre terminé");
    close(g_signal_pipe[0]);
    close(g_signal_pipe[1]);
    return 0;
}
```

Le pipe doit être en mode **non-bloquant** (`O_NONBLOCK`) pour deux raisons : éviter que le handler ne bloque si le pipe est plein (16 pages soit 64 Ko par défaut sous Linux, largement suffisant), et permettre au code principal de drainer le pipe sans bloquer.

Le self-pipe trick est le pattern utilisé par la plupart des serveurs réseau Unix (OpenSSH, Nginx, HAProxy) pour intégrer les signaux dans leur boucle d'événements.

---

## Pattern 3 : `signalfd` (Linux-spécifique)

Linux 2.6.22 a introduit `signalfd()`, qui crée un descripteur de fichier depuis lequel on peut **lire** les signaux comme des données structurées. C'est une alternative plus propre au self-pipe trick, car elle ne nécessite pas de handler du tout :

```cpp
#include <sys/signalfd.h>
#include <signal.h>
#include <unistd.h>
#include <poll.h>
#include <print>
#include <cstring>
#include <cerrno>

int main() {
    // 1. Masquer les signaux qu'on veut lire via signalfd
    //    (un signal masqué n'invoque pas de handler, il reste pending
    //    et sera lu par signalfd)
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, nullptr);

    // 2. Créer le signalfd
    int sfd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
    if (sfd == -1) {
        std::println("signalfd: {}", strerror(errno));
        return 1;
    }

    std::println("Serveur démarré (PID = {})", getpid());

    // 3. Intégrer dans la boucle d'événements
    struct pollfd fds[1];
    fds[0].fd = sfd;
    fds[0].events = POLLIN;

    bool running = true;
    while (running) {
        int ret = poll(fds, 1, 1000);

        if (ret > 0 && (fds[0].revents & POLLIN)) {
            struct signalfd_siginfo info;
            ssize_t n = read(sfd, &info, sizeof(info));
            if (n == sizeof(info)) {
                std::println("Signal {} reçu de PID {} (UID {})",
                    info.ssi_signo, info.ssi_pid, info.ssi_uid);
                running = false;
            }
        }

        if (running) {
            std::println("Traitement...");
        }
    }

    std::println("Arrêt propre terminé");
    close(sfd);
    return 0;
}
```

Les avantages de `signalfd` par rapport au self-pipe trick :

| Critère | Self-pipe | `signalfd` |
|---|---|---|
| Handler nécessaire | Oui (minimaliste) | Non |
| Informations sur le signal | Numéro seulement | PID, UID, code, adresse… |
| Risque d'async-signal-safety | Minimal mais présent | Aucun |
| Portabilité | Tout POSIX | Linux uniquement |
| Descripteurs consommés | 2 (pipe) | 1 |

`signalfd` est l'approche recommandée pour les programmes Linux-only, en particulier les serveurs utilisant `epoll`. Le self-pipe trick reste pertinent pour le code portable ou les cas où `signalfd` n'est pas disponible.

---

## Graceful shutdown : le pattern complet

Voici un squelette complet de serveur avec arrêt propre, combinant les techniques vues dans cette section :

```cpp
#include <signal.h>
#include <atomic>
#include <print>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <thread>
#include <vector>

std::atomic<bool> g_shutdown{false};

void signal_handler(int /* sig */) {
    g_shutdown.store(true, std::memory_order_relaxed);
}

class Server {  
public:  
    void start() {
        install_signal_handlers();
        std::println("Serveur démarré (PID = {})", getpid());
        run();
        shutdown();
    }

private:
    void install_signal_handlers() {
        struct sigaction sa;
        std::memset(&sa, 0, sizeof(sa));
        sa.sa_handler = signal_handler;
        sigemptyset(&sa.sa_mask);
        // Masquer SIGINT pendant le traitement de SIGTERM et vice versa
        sigaddset(&sa.sa_mask, SIGTERM);
        sigaddset(&sa.sa_mask, SIGINT);
        sa.sa_flags = 0;  // Pas SA_RESTART — on veut interrompre les sleeps

        sigaction(SIGTERM, &sa, nullptr);
        sigaction(SIGINT, &sa, nullptr);

        // Ignorer SIGPIPE globalement
        signal(SIGPIPE, SIG_IGN);
    }

    void run() {
        while (!g_shutdown.load(std::memory_order_relaxed)) {
            // Simuler le traitement de requêtes
            process_pending_requests();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    void process_pending_requests() {
        // ... traitement des requêtes en cours ...
    }

    void shutdown() {
        std::println("Arrêt en cours...");

        // 1. Arrêter d'accepter de nouvelles connexions
        std::println("  Refus des nouvelles connexions");

        // 2. Terminer les requêtes en cours (avec timeout)
        std::println("  Finalisation des requêtes en cours...");
        auto deadline = std::chrono::steady_clock::now()
                      + std::chrono::seconds(10);

        while (has_pending_work() 
               && std::chrono::steady_clock::now() < deadline) 
        {
            process_pending_requests();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        if (has_pending_work()) {
            std::println("  Timeout — requêtes restantes abandonnées");
        }

        // 3. Fermer les ressources
        std::println("  Fermeture des connexions");
        std::println("  Flush des logs");
        std::println("  Suppression des fichiers temporaires");

        std::println("Arrêt terminé.");
    }

    bool has_pending_work() const { return false; /* stub */ }
};

int main() {
    Server server;
    server.start();
    return 0;
}
```

Les étapes d'un arrêt propre sont toujours les mêmes :

1. **Cesser d'accepter** du nouveau travail (fermer les sockets d'écoute, arrêter les consumers).
2. **Terminer le travail en cours** avec un timeout (les requêtes en vol, les écritures en cours).
3. **Libérer les ressources** (fermer les fichiers, les connexions réseau, supprimer les fichiers temporaires).
4. **Quitter** avec un code de retour approprié (`0` si tout s'est bien passé, non-zéro sinon).

Le timeout de l'étape 2 est important : si le programme ne termine pas dans le délai de grâce (30 secondes par défaut dans Kubernetes, configurable via `terminationGracePeriodSeconds`), il recevra un `SIGKILL` auquel il ne pourra pas échapper.

---

## Sauvegarder et restaurer les dispositions

Lors de l'installation d'un handler, `sigaction()` permet de récupérer la disposition précédente via le troisième paramètre `oldact`. C'est essentiel pour les bibliothèques et les programmes qui doivent restaurer le comportement original :

```cpp
#include <signal.h>
#include <cstring>

class ScopedSignalHandler {  
public:  
    ScopedSignalHandler(int signum, void (*handler)(int))
        : signum_(signum)
    {
        struct sigaction sa;
        std::memset(&sa, 0, sizeof(sa));
        sa.sa_handler = handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;

        sigaction(signum_, &sa, &old_action_);  // Sauvegarde l'ancienne disposition
    }

    ~ScopedSignalHandler() {
        sigaction(signum_, &old_action_, nullptr);  // Restaure
    }

    ScopedSignalHandler(const ScopedSignalHandler&) = delete;
    ScopedSignalHandler& operator=(const ScopedSignalHandler&) = delete;

private:
    int signum_;
    struct sigaction old_action_;
};

// Utilisation RAII
void critical_section() {
    ScopedSignalHandler guard(SIGINT, SIG_IGN);
    // SIGINT est ignoré pendant cette portée
    // ...
}   // L'ancienne disposition de SIGINT est restaurée ici
```

---

## Masquage temporaire de signaux

Parfois, il faut empêcher la délivrance de signaux pendant une section critique du code principal (pas du handler). `sigprocmask()` modifie le masque de signaux du thread appelant :

```cpp
#include <signal.h>
#include <print>
#include <cstring>

void critical_update() {
    sigset_t block_set, old_set;
    sigemptyset(&block_set);
    sigaddset(&block_set, SIGTERM);
    sigaddset(&block_set, SIGINT);

    // Bloquer SIGTERM et SIGINT
    sigprocmask(SIG_BLOCK, &block_set, &old_set);

    // === Section critique ===
    // Les signaux sont en pending, pas perdus
    std::println("Mise à jour atomique en cours...");
    // ... opérations qui ne doivent pas être interrompues ...
    std::println("Mise à jour terminée");

    // Restaurer le masque — les signaux pending sont délivrés maintenant
    sigprocmask(SIG_SETMASK, &old_set, nullptr);
}
```

Une version RAII pour le masquage temporaire :

```cpp
#include <signal.h>
#include <initializer_list>

class ScopedSignalBlock {  
public:  
    explicit ScopedSignalBlock(std::initializer_list<int> signals) {
        sigset_t block_set;
        sigemptyset(&block_set);
        for (int sig : signals) {
            sigaddset(&block_set, sig);
        }
        sigprocmask(SIG_BLOCK, &block_set, &old_set_);
    }

    ~ScopedSignalBlock() {
        sigprocmask(SIG_SETMASK, &old_set_, nullptr);
    }

    ScopedSignalBlock(const ScopedSignalBlock&) = delete;
    ScopedSignalBlock& operator=(const ScopedSignalBlock&) = delete;

private:
    sigset_t old_set_;
};

// Utilisation
void safe_update() {
    ScopedSignalBlock guard({SIGTERM, SIGINT});
    // Aucun signal ne sera délivré dans cette portée
    // ...
}   // Signaux débloqués et délivrés ici
```

---

## Erreurs courantes et comment les éviter

### Appeler des fonctions non-safe dans un handler

```cpp
// ❌ DANGEREUX — chaque ligne est potentiellement fatale
void bad_handler(int sig) {
    std::println("Signal {} reçu", sig);           // malloc, mutex
    std::string msg = "arrêt en cours";              // allocation dynamique
    log_to_file(msg);                                // fopen/fprintf
    cleanup_temporary_files();                       // opendir/closedir, free
    exit(0);                                         // exécute atexit handlers
}

// ✅ CORRECT — minimaliste, async-signal-safe uniquement
void good_handler(int sig) {
    g_shutdown_requested = 1;
    // Ou : write(g_signal_pipe[1], &sig_byte, 1);
    // Ou : _exit(128 + sig);
}
```

### Oublier d'initialiser `struct sigaction`

```cpp
// ❌ sa_mask et sa_flags contiennent des valeurs aléatoires
struct sigaction sa;  
sa.sa_handler = my_handler;  
sigaction(SIGTERM, &sa, nullptr);  

// ✅ Initialisation propre
struct sigaction sa;  
std::memset(&sa, 0, sizeof(sa));  
sa.sa_handler = my_handler;  
sigemptyset(&sa.sa_mask);  
sa.sa_flags = SA_RESTART;  
sigaction(SIGTERM, &sa, nullptr);  
```

### Ignorer `EINTR` quand `SA_RESTART` n'est pas utilisé

```cpp
// ❌ Ne gère pas l'interruption par signal
ssize_t n = read(fd, buf, sizeof(buf));  
if (n == -1) {  
    perror("read");  // Peut afficher "Interrupted system call" à tort
    return;
}

// ✅ Relance après interruption
ssize_t n;  
do {  
    n = read(fd, buf, sizeof(buf));
} while (n == -1 && errno == EINTR);
```

### Installer un handler pour `SIGSEGV` qui tente de continuer

```cpp
// ❌ Comportement indéfini — le SIGSEGV sera re-déclenché immédiatement
void segv_handler(int) {
    // "Récupérer" et continuer est impossible
    // Le programme va boucler sur le même fault
}

// ✅ Diagnostic puis terminaison
void segv_handler(int sig) {
    const char msg[] = "SIGSEGV: crash mémoire\n";
    write(STDERR_FILENO, msg, sizeof(msg) - 1);
    // Optionnel : écrire un backtrace via backtrace_symbols_fd()
    _exit(128 + sig);
}
```

---

## Résumé des patterns recommandés

| Situation | Pattern |
|---|---|
| Arrêt propre d'un service simple | Flag `volatile sig_atomic_t` ou `std::atomic<bool>` |
| Serveur avec boucle d'événements (poll/epoll) | Self-pipe trick ou `signalfd` |
| Programme Linux-only avec epoll | `signalfd` (pas de handler nécessaire) |
| Ignorer un signal | `std::signal(SIG, SIG_IGN)` |
| Section critique insensible aux signaux | `sigprocmask` / `ScopedSignalBlock` RAII |
| Diagnostic sur `SIGSEGV` | Handler minimaliste → `write()` + `_exit()` |
| Bibliothèque (ne pas altérer les handlers du programme) | Sauvegarder/restaurer avec `sigaction` RAII |

---

> 💡 **Note** — La règle d'or des signal handlers tient en une phrase : **faire le minimum absolu dans le handler, tout le reste dans le code principal**. Un flag atomique positionné dans le handler et testé dans la boucle principale est le pattern qui a fait ses preuves dans des décennies de programmation Unix. Chaque fois que vous êtes tenté d'ajouter de la logique dans un handler — ne serait-ce qu'un `std::println` pour le debug — rappelez-vous que ce code s'exécute dans un contexte où le processus peut être dans n'importe quel état interne. La simplicité du handler n'est pas un manque de sophistication, c'est la forme la plus aboutie de robustesse.

⏭️ [Signaux et threads : Problématiques](/20-signaux-posix/03-signaux-threads.md)
