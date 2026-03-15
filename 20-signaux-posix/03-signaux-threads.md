🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 20.3 — Signaux et threads : Problématiques

## Module 7 — Programmation Système sur Linux *(Niveau Avancé → Expert)*

---

## Introduction

Les signaux et les threads sont deux mécanismes fondamentaux de la programmation système, mais leur cohabitation est l'un des terrains les plus minés de la programmation Unix. Les signaux ont été conçus dans les années 1970 pour des processus mono-threadés. Le threading POSIX (pthreads) est arrivé deux décennies plus tard, et les deux mécanismes n'ont jamais été conçus pour fonctionner ensemble harmonieusement.

Le résultat est un ensemble de règles complexes sur quel thread reçoit quel signal, avec quelle disposition, et comment le masquage interagit entre les threads. Ignorer ces règles produit des programmes qui semblent fonctionner en développement (où les conditions de timing sont favorables) et échouent de manière intermittente en production sous charge.

Cette section couvre les règles de délivrance, les techniques de masquage, et les patterns éprouvés pour gérer les signaux de manière fiable dans un programme C++ multi-threadé.

---

## Règles fondamentales : signaux et threads

### Ce qui est partagé, ce qui ne l'est pas

Dans un processus multi-threadé, les signaux interagissent avec deux niveaux d'état :

| Propriété | Portée | Conséquence |
|---|---|---|
| **Disposition** (handler, SIG_IGN, SIG_DFL) | Processus entier | Installer un handler dans un thread l'installe pour tous les threads |
| **Masque de signaux** (signal mask) | Par thread | Chaque thread a son propre ensemble de signaux bloqués |
| **Signaux pending** pour le processus | Processus entier | Un `kill(pid, sig)` met le signal en pending pour le processus |
| **Signaux pending** pour un thread | Par thread | Un `pthread_kill(tid, sig)` met le signal en pending pour ce thread |

Le fait que la disposition soit partagée mais le masque soit par thread est la source de la quasi-totalité des complexités. Si le thread A installe un handler pour `SIGTERM`, le thread B est aussi affecté. Mais si le thread A masque `SIGTERM`, le thread B peut toujours le recevoir.

### Quel thread reçoit le signal ?

La règle POSIX est la suivante :

**Signaux synchrones** (`SIGSEGV`, `SIGFPE`, `SIGBUS`, `SIGILL`) — Délivrés au **thread qui a causé l'erreur**. Un déréférencement de pointeur nul dans le thread 3 envoie `SIGSEGV` au thread 3, pas à un autre.

**Signaux asynchrones** (`SIGTERM`, `SIGINT`, `SIGHUP`, `SIGUSR1`…) — Délivrés à **un thread arbitraire** parmi ceux qui ne masquent pas le signal. Le noyau choisit un thread éligible (c'est-à-dire un thread dont le signal mask ne bloque pas ce signal) de manière non déterministe. Il n'y a aucune garantie sur quel thread sera choisi. Si tous les threads masquent le signal, il reste en pending jusqu'à ce qu'un thread le débloque.

**Signaux dirigés vers un thread** (`pthread_kill`) — Délivrés au thread ciblé, indépendamment de son masque. Si le thread masque le signal, il reste pending pour ce thread spécifiquement.

Cette indétermination de la délivrance des signaux asynchrones est le problème central. Un handler qui s'exécute dans un worker thread peut interrompre un calcul critique, tenter d'accéder à un mutex déjà détenu par ce thread (deadlock), ou corrompre une structure de données en cours de modification.

---

## Le problème du deadlock dans un handler

Considérons un scénario courant et dangereux :

```cpp
#include <mutex>
#include <print>

std::mutex g_mutex;

// ❌ DEADLOCK — le handler peut s'exécuter dans un thread qui détient g_mutex
void bad_handler(int) {
    // Si le thread interrompu détenait g_mutex, ceci deadlock immédiatement
    std::lock_guard lock(g_mutex);
    // ... journaliser quelque chose ...
}

void worker_function() {
    while (true) {
        std::lock_guard lock(g_mutex);
        // SIGTERM arrive ICI → le handler s'exécute dans CE thread
        // → le handler tente de prendre g_mutex → DEADLOCK
        do_work();
    }
}
```

Ce scénario n'est pas théorique : c'est l'un des bugs les plus courants dans les programmes multi-threadés qui gèrent les signaux. Le deadlock ne se produit que si le signal arrive pendant la fenêtre où le mutex est détenu — ce qui est rare en test mais statistiquement inévitable en production sous charge.

La solution n'est pas de rendre le handler plus sophistiqué. La solution est d'empêcher le handler de s'exécuter dans les worker threads.

---

## La stratégie : un thread dédié aux signaux

Le pattern recommandé pour gérer les signaux dans un programme multi-threadé est :

1. **Masquer tous les signaux gérés** dans le thread principal **avant** de créer les worker threads (le masque est hérité par les threads enfants).
2. Créer un **thread dédié** qui attend les signaux de manière synchrone.
3. Les worker threads ne sont **jamais** interrompus par des signaux.

Ce pattern élimine complètement le problème de l'asynchronisme : le thread dédié appelle une fonction bloquante (`sigwait` ou `read` sur un `signalfd`) et traite le signal comme un événement synchrone, dans un contexte normal où toutes les fonctions sont autorisées.

### Implémentation avec `sigwait()`

```cpp
#include <signal.h>
#include <pthread.h>
#include <atomic>
#include <thread>
#include <print>
#include <vector>
#include <cstring>
#include <unistd.h>

std::atomic<bool> g_running{true};

void signal_thread_func(sigset_t wait_set) {
    while (true) {
        int sig;
        // sigwait() attend de manière SYNCHRONE qu'un signal pending arrive
        int ret = sigwait(&wait_set, &sig);
        if (ret != 0) {
            continue;  // Erreur, réessayer
        }

        // === Contexte normal — toutes les fonctions sont autorisées ===
        switch (sig) {
            case SIGTERM:
                std::println("[signal_thread] SIGTERM reçu — arrêt demandé");
                g_running.store(false, std::memory_order_relaxed);
                return;

            case SIGINT:
                std::println("[signal_thread] SIGINT reçu — arrêt demandé");
                g_running.store(false, std::memory_order_relaxed);
                return;

            case SIGHUP:
                std::println("[signal_thread] SIGHUP reçu — rechargement config");
                reload_configuration();
                break;

            case SIGUSR1:
                std::println("[signal_thread] SIGUSR1 — dump des statistiques");
                dump_stats();
                break;

            default:
                std::println("[signal_thread] Signal {} ignoré", sig);
                break;
        }
    }
}

void worker_func(int id) {
    while (g_running.load(std::memory_order_relaxed)) {
        // Ce thread ne recevra JAMAIS de signal (ils sont tous masqués)
        // Pas de risque de deadlock, pas de EINTR, pas d'interruption
        std::println("[worker {}] Traitement...", id);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::println("[worker {}] Arrêt", id);
}

void reload_configuration() { std::println("  Configuration rechargée"); }  
void dump_stats() { std::println("  Statistiques affichées"); }  

int main() {
    // 1. Masquer les signaux AVANT de créer les threads
    //    Le masque est hérité par tous les threads créés ensuite
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask, nullptr);

    // Ignorer SIGPIPE globalement (pas besoin de thread dédié pour ça)
    signal(SIGPIPE, SIG_IGN);

    std::println("Serveur démarré (PID = {})", getpid());

    // 2. Créer le thread dédié aux signaux
    std::thread signal_thread(signal_thread_func, mask);

    // 3. Créer les worker threads
    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back(worker_func, i);
    }

    // 4. Attendre la terminaison
    signal_thread.join();

    // Le flag g_running est maintenant false — les workers vont terminer
    for (auto& w : workers) {
        w.join();
    }

    std::println("Tous les threads terminés — arrêt propre");
    return 0;
}
```

L'ordre est crucial : le masquage doit se faire **avant** `std::thread()` et avant tout `pthread_create()`. Si un thread est créé avant le masquage, il hérite de l'ancien masque et peut recevoir des signaux.

### `sigwait()` vs `sigwaitinfo()`

```cpp
#include <signal.h>

// sigwait — retourne uniquement le numéro du signal
int sig;  
int ret = sigwait(&wait_set, &sig);  

// sigwaitinfo — retourne aussi les informations étendues (comme SA_SIGINFO)
siginfo_t info;  
int sig2 = sigwaitinfo(&wait_set, &info);  
// info.si_pid, info.si_uid, info.si_code disponibles

// sigtimedwait — comme sigwaitinfo, avec timeout
struct timespec timeout = {.tv_sec = 5, .tv_nsec = 0};  
int sig3 = sigtimedwait(&wait_set, &info, &timeout);  
if (sig3 == -1 && errno == EAGAIN) {  
    // Timeout expiré, aucun signal reçu
}
```

`sigtimedwait()` est particulièrement utile pour le thread dédié : il permet de combiner l'attente de signaux avec des vérifications périodiques (heartbeat, nettoyage, etc.).

---

## Implémentation avec `signalfd` et `epoll`

Pour les serveurs qui utilisent déjà `epoll` comme boucle d'événements, `signalfd` s'intègre naturellement sans thread supplémentaire :

```cpp
#include <sys/signalfd.h>
#include <sys/epoll.h>
#include <signal.h>
#include <unistd.h>
#include <atomic>
#include <print>
#include <cstring>
#include <cerrno>
#include <thread>
#include <vector>

std::atomic<bool> g_running{true};

class EventLoop {  
public:  
    bool init() {
        // Masquer les signaux
        sigemptyset(&sig_mask_);
        sigaddset(&sig_mask_, SIGTERM);
        sigaddset(&sig_mask_, SIGINT);
        sigaddset(&sig_mask_, SIGHUP);
        sigprocmask(SIG_BLOCK, &sig_mask_, nullptr);

        // Créer le signalfd
        signal_fd_ = signalfd(-1, &sig_mask_, SFD_NONBLOCK | SFD_CLOEXEC);
        if (signal_fd_ == -1) return false;

        // Créer l'instance epoll
        epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
        if (epoll_fd_ == -1) return false;

        // Enregistrer le signalfd dans epoll
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = signal_fd_;
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, signal_fd_, &ev) == -1) {
            return false;
        }

        // Ici, on enregistrerait aussi les sockets serveur dans epoll
        return true;
    }

    void run() {
        constexpr int MAX_EVENTS = 64;
        struct epoll_event events[MAX_EVENTS];

        while (g_running.load(std::memory_order_relaxed)) {
            int n = epoll_wait(epoll_fd_, events, MAX_EVENTS, 1000);

            for (int i = 0; i < n; ++i) {
                if (events[i].data.fd == signal_fd_) {
                    handle_signal();
                } else {
                    // Traiter les événements réseau...
                }
            }
        }
    }

    ~EventLoop() {
        if (signal_fd_ >= 0) close(signal_fd_);
        if (epoll_fd_ >= 0) close(epoll_fd_);
    }

private:
    void handle_signal() {
        struct signalfd_siginfo info;
        while (read(signal_fd_, &info, sizeof(info)) == sizeof(info)) {
            switch (info.ssi_signo) {
                case SIGTERM:
                case SIGINT:
                    std::println("Signal {} — arrêt demandé", info.ssi_signo);
                    g_running.store(false, std::memory_order_relaxed);
                    break;
                case SIGHUP:
                    std::println("SIGHUP — rechargement");
                    break;
            }
        }
    }

    sigset_t sig_mask_{};
    int signal_fd_ = -1;
    int epoll_fd_ = -1;
};

int main() {
    signal(SIGPIPE, SIG_IGN);

    EventLoop loop;
    if (!loop.init()) {
        std::println("Erreur d'initialisation : {}", strerror(errno));
        return 1;
    }

    std::println("Serveur epoll démarré (PID = {})", getpid());

    // Les worker threads sont créés APRÈS le masquage (fait dans init())
    // Ils héritent du masque → aucun signal ne les atteint
    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([i] {
            while (g_running.load(std::memory_order_relaxed)) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            std::println("[worker {}] terminé", i);
        });
    }

    // La boucle d'événements tourne dans le thread principal
    loop.run();

    for (auto& w : workers) w.join();
    std::println("Arrêt propre terminé");
    return 0;
}
```

Cette architecture est celle des serveurs de production performants : un seul thread gère les signaux et les événements réseau via `epoll`, les worker threads ne sont jamais interrompus.

---

## `pthread_sigmask` : masquage par thread

Dans le thread principal, `sigprocmask()` suffit. Mais dans un thread créé après le masquage initial, ou si un thread a besoin de modifier son propre masque, c'est `pthread_sigmask()` qu'il faut utiliser :

```cpp
#include <signal.h>
#include <pthread.h>

// pthread_sigmask a la même interface que sigprocmask
// mais est explicitement défini comme thread-safe

void worker_that_needs_signals() {
    // Ce worker veut recevoir SIGUSR2 (les autres threads le masquent)
    sigset_t unblock;
    sigemptyset(&unblock);
    sigaddset(&unblock, SIGUSR2);
    pthread_sigmask(SIG_UNBLOCK, &unblock, nullptr);

    // Ce thread peut maintenant recevoir SIGUSR2
    // Les autres threads ne sont pas affectés
}
```

En pratique, le besoin de démasquer un signal dans un worker thread est rare. Le pattern recommandé (masquage global + thread dédié) couvre la quasi-totalité des cas. `pthread_sigmask` est surtout utile pour des scénarios avancés comme le débogage ou les pools de threads spécialisés.

### `sigprocmask` vs `pthread_sigmask`

| Fonction | Contexte | Thread-safe | Norme |
|---|---|---|---|
| `sigprocmask()` | Mono-thread ou thread principal | Non garanti dans un contexte multi-thread | POSIX |
| `pthread_sigmask()` | Multi-thread | Oui | POSIX Threads |

Sur Linux avec glibc, `sigprocmask()` appelle en réalité `pthread_sigmask()` en interne, donc les deux fonctionnent dans un programme multi-threadé. Cependant, pour la clarté et la portabilité, utilisez `pthread_sigmask()` dès que votre programme crée des threads.

---

## `pthread_kill` : envoyer un signal à un thread spécifique

```cpp
#include <signal.h>
#include <pthread.h>

// Envoyer un signal à un thread spécifique
pthread_kill(thread.native_handle(), SIGUSR1);

// Vérifier si un thread existe (comme kill(pid, 0))
int ret = pthread_kill(thread.native_handle(), 0);  
if (ret == ESRCH) {  
    // Le thread n'existe plus
}
```

`pthread_kill` envoie le signal au thread ciblé, pas au processus. C'est utile dans des cas spécifiques, comme interrompre un thread bloqué dans un `read()` en lui envoyant un signal qui provoque `EINTR`. Cependant, cette technique est fragile et les alternatives (flag atomique + timeout sur les appels bloquants, ou `pthread_cancel`) sont généralement préférables.

---

## Interaction avec `std::jthread` et les stop tokens (C++20)

C++20 a introduit `std::jthread` avec un mécanisme de **stop token** qui offre une alternative coopérative aux signaux pour la terminaison des threads. La combinaison des deux permet un arrêt propre élégant :

```cpp
#include <signal.h>
#include <atomic>
#include <thread>
#include <print>
#include <vector>
#include <chrono>
#include <cstring>
#include <unistd.h>

// Source d'arrêt partagée — le thread de signaux la déclenche,
// les jthreads la consultent via leur stop_token
std::stop_source g_stop_source;

void signal_thread_func(sigset_t wait_set) {
    int sig;
    while (sigwait(&wait_set, &sig) == 0) {
        if (sig == SIGTERM || sig == SIGINT) {
            std::println("[signal] {} reçu — demande d'arrêt", strsignal(sig));
            g_stop_source.request_stop();
            return;
        }
    }
}

void worker_func(std::stop_token stop_tok, int id) {
    while (!stop_tok.stop_requested()) {
        std::println("[worker {}] traitement...", id);
        
        // Attente interruptible — se réveille si stop est demandé
        // Note : std::condition_variable_any supporte les stop_tokens
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::println("[worker {}] arrêt coopératif", id);
}

int main() {
    // Masquer les signaux avant la création des threads
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, nullptr);
    signal(SIGPIPE, SIG_IGN);

    std::println("Serveur démarré (PID = {})", getpid());

    // Thread dédié aux signaux
    std::thread sig_thread(signal_thread_func, mask);

    // Workers avec jthread — reliés au stop_source global
    // On capture le token de g_stop_source pour que les workers réagissent
    // immédiatement à g_stop_source.request_stop()
    std::vector<std::jthread> workers;
    for (int i = 0; i < 4; ++i) {
        auto token = g_stop_source.get_token();
        workers.emplace_back(
            [i, token](std::stop_token /* jthread_tok */) { worker_func(token, i); }
        );
    }

    // Attendre que le thread de signaux détecte un arrêt
    sig_thread.join();

    // g_stop_source.request_stop() a déjà été appelé par le signal thread
    // → les workers voient stop_requested() == true et terminent leur boucle
    // Les destructeurs des jthreads appellent join automatiquement
    workers.clear();

    std::println("Arrêt propre terminé");
    return 0;
}
```

### `std::condition_variable_any` avec stop token

Pour des worker threads qui attendent sur une condition variable, C++20 permet une attente directement interruptible par le stop token :

```cpp
#include <mutex>
#include <condition_variable>
#include <queue>
#include <print>

std::mutex g_queue_mutex;  
std::condition_variable_any g_queue_cv;  
std::queue<int> g_work_queue;  

void consumer(std::stop_token stop_tok, int id) {
    while (true) {
        std::unique_lock lock(g_queue_mutex);

        // Attente interruptible : se réveille si
        // (a) un élément est disponible, OU
        // (b) l'arrêt est demandé via le stop_token
        bool got_work = g_queue_cv.wait(lock, stop_tok, [] {
            return !g_work_queue.empty();
        });

        if (!got_work) {
            // Le stop_token a été déclenché
            std::println("[consumer {}] arrêt demandé", id);
            return;
        }

        int item = g_work_queue.front();
        g_work_queue.pop();
        lock.unlock();

        // Traiter l'item...
        std::println("[consumer {}] traitement item {}", id, item);
    }
}
```

Cette intégration est l'approche la plus propre en C++ moderne : le signal déclenche un `stop_source`, qui réveille automatiquement tous les threads en attente sur des condition variables, sans aucun risque de deadlock ni de race condition.

---

## Le piège de `fork()` dans un programme multi-threadé

`fork()` dans un programme multi-threadé crée un processus enfant avec **un seul thread** — le thread qui a appelé `fork()`. Tous les autres threads disparaissent dans l'enfant. Si l'un de ces threads détenait un mutex (y compris des mutexes internes de la libc, comme celui de `malloc`), le mutex reste verrouillé à jamais dans le processus enfant.

C'est un problème direct pour la gestion des signaux car un handler peut appeler `fork()` :

```cpp
// ❌ DANGEREUX — fork() dans un handler multi-threadé
void handler(int) {
    pid_t pid = fork();
    if (pid == 0) {
        // Processus enfant — un seul thread, mutexes potentiellement verrouillés
        execve("/usr/bin/cleanup", ...);  // OK si exec réussit immédiatement
        _exit(1);
    }
}
```

La seule opération sûre après `fork()` dans un programme multi-threadé est `exec()` (qui remplace entièrement l'image du processus) ou `_exit()`. Toute autre opération (allocation mémoire, I/O buffered, manipulation de fichiers) peut deadlocker sur un mutex interne.

La règle pratique : dans un programme multi-threadé, utilisez `fork()` uniquement suivi immédiatement par `exec()` ou `_exit()`, et préférez `posix_spawn()` qui combine les deux de manière atomique.

---

## Signaux synchrones dans les threads

Les signaux d'erreur (`SIGSEGV`, `SIGFPE`, `SIGBUS`, `SIGILL`) sont toujours délivrés au thread qui a causé l'erreur, indépendamment du masquage. Masquer `SIGSEGV` ne protège pas un thread : si le thread provoque un accès mémoire invalide, le signal est délivré quand même.

Tenter de masquer un signal synchrone dans le thread qui le déclenche provoque un **comportement indéfini** selon POSIX. En pratique sur Linux, le comportement typique est la terminaison du processus.

```cpp
// ❌ Inutile et dangereux — masquer SIGSEGV ne protège pas
sigset_t mask;  
sigemptyset(&mask);  
sigaddset(&mask, SIGSEGV);  
pthread_sigmask(SIG_BLOCK, &mask, nullptr);  

int* p = nullptr;
*p = 42;  // SIGSEGV déclenché et délivré malgré le masque
```

La seule manière de "gérer" un `SIGSEGV` est d'installer un handler (qui sera invoqué dans le thread fautif) pour le diagnostic, puis de terminer le processus. Les sanitizers (chapitre 29) sont l'outil approprié pour prévenir ces erreurs.

---

## Résumé : checklist de gestion des signaux en multi-threadé

| Étape | Action | Quand |
|---|---|---|
| **1** | Masquer les signaux gérés avec `sigprocmask` | **Avant** toute création de thread |
| **2** | Ignorer `SIGPIPE` avec `signal(SIGPIPE, SIG_IGN)` | Au démarrage |
| **3** | Créer un thread dédié aux signaux | Après le masquage |
| **4** | Utiliser `sigwait` ou `signalfd` dans le thread dédié | Boucle du thread signal |
| **5** | Communiquer via `std::atomic` ou `std::stop_source` | Signal thread → workers |
| **6** | Ne jamais installer de handler asynchrone (sauf `SIG_IGN`) | Tout le programme |
| **7** | Ne jamais appeler `fork()` sans `exec()` immédiat | Tout le programme |

---

## Comparaison des approches

| Approche | Complexité | Async-signal-safety | Multi-thread safe | Portabilité |
|---|---|---|---|---|
| Flag `sig_atomic_t` + handler | Faible | Risque minime | Fragile (quel thread ?) | Tout POSIX |
| Self-pipe trick | Moyenne | Handler minimal | OK avec masquage | Tout POSIX |
| `sigwait` + thread dédié | Moyenne | Aucun handler | Robuste | Tout POSIX |
| `signalfd` + epoll | Moyenne | Aucun handler | Robuste | Linux uniquement |
| `signalfd` + thread dédié | Faible | Aucun handler | Robuste | Linux uniquement |
| `std::jthread` + stop token | Faible | Via thread dédié | Natif C++20 | Portable C++20 |

Pour les programmes multi-threadés, les approches sans handler (lignes 3 à 6) sont systématiquement préférables. Le flag `sig_atomic_t` reste acceptable pour les programmes simples mono-threadés ou avec peu de threads.

---

## Architecture recommandée

Voici l'architecture qui couvre la majorité des programmes C++ multi-threadés en production :

```
┌────────────────────────────────────────────────────────────┐
│                     Processus C++                          │
│                                                            │
│   main()                                                   │
│     │                                                      │
│     ├── 1. sigprocmask(SIG_BLOCK, {SIGTERM,SIGINT,SIGHUP}) │
│     ├── 2. signal(SIGPIPE, SIG_IGN)                        │
│     │                                                      │
│     ├── 3. Thread Signal ─── sigwait() ou signalfd ────┐   │
│     │         │                                        │   │
│     │         ├── SIGTERM → g_stop_source.request_stop()   │
│     │         ├── SIGHUP  → recharger config           │   │
│     │         └── SIGUSR1 → dump statistiques          │   │
│     │                                                      │
│     ├── 4. Worker threads ──── stop_token.stop_requested() │
│     │         │ (signaux masqués, jamais interrompus)      │
│     │         ├── Worker 0                                 │
│     │         ├── Worker 1                                 │
│     │         ├── Worker 2                                 │
│     │         └── Worker 3                                 │
│     │                                                      │
│     └── 5. join tous les threads → nettoyage → exit(0)     │
│                                                            │
└────────────────────────────────────────────────────────────┘
```

Cette architecture garantit :

- **Aucun handler asynchrone** — pas de problème d'async-signal-safety.
- **Aucune interruption des workers** — pas de `EINTR`, pas de deadlock, pas de corruption.
- **Arrêt coopératif** — via les stop tokens C++20, les workers terminent proprement.
- **Extensible** — ajouter la gestion d'un nouveau signal se résume à un `case` supplémentaire dans le thread dédié.

---

> 💡 **Note** — La complexité de l'interaction signaux/threads conduit à une recommandation simple mais contre-intuitive : dans un programme multi-threadé, **n'utilisez pas de signal handlers**. Masquez tout, dédiez un thread, traitez les signaux de manière synchrone. C'est la seule approche qui élimine simultanément les problèmes d'async-signal-safety, les deadlocks de mutex dans les handlers, l'indétermination du thread de délivrance, et les race conditions entre le handler et le code principal. Les sections 20.1 et 20.2 vous ont donné les connaissances nécessaires pour comprendre *pourquoi* les signaux sont dangereux. Cette section vous donne la solution : les traiter comme des événements synchrones dans un thread dédié, pas comme des interruptions asynchrones.

⏭️ [Threads et Programmation Concurrente](/21-threads-concurrence/README.md)
