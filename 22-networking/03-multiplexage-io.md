🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 22.3 Multiplexage I/O et I/O asynchrone

## Le problème fondamental : un thread, plusieurs connexions

Dans la section précédente, nous avons construit un serveur TCP capable d'accepter des connexions et d'échanger des données. Mais ce serveur souffre d'une limitation majeure : il ne peut traiter qu'**un seul client à la fois**. Lorsqu'il est bloqué dans un appel `recv()` en attente de données d'un client, il est incapable de répondre à un autre client — ni même d'accepter de nouvelles connexions.

Considérons ce scénario concret :

```cpp
// Serveur naïf : bloquant, un seul client à la fois
int client_fd = accept(server_fd, nullptr, nullptr);  // Bloque jusqu'à une connexion

char buffer[4096];  
ssize_t n = recv(client_fd, buffer, sizeof(buffer), 0);  // Bloque jusqu'à réception  
// Pendant ce recv(), AUCUN autre client ne peut être servi
```

Ce modèle **un thread = un client** est la première solution qui vient à l'esprit pour contourner le problème :

```cpp
// Approche thread-per-connection
while (true) {
    int client_fd = accept(server_fd, nullptr, nullptr);
    std::jthread([client_fd] {
        handle_client(client_fd);  // Chaque client a son thread
    }).detach();
}
```

Cette approche fonctionne pour un petit nombre de clients, mais elle ne passe pas à l'échelle. Avec 10 000 connexions simultanées, on se retrouve avec 10 000 threads — chacun consommant de la mémoire pour sa stack (typiquement 8 Mo par défaut sous Linux), du temps CPU pour le context switching, et de la pression sur le scheduler du noyau. C'est le cœur du fameux **problème C10K** (10 000 connexions concurrentes), formulé par Dan Kegel en 1999, qui a profondément influencé l'architecture des serveurs modernes.

## Multiplexage I/O : surveiller plusieurs descripteurs depuis un seul thread

Le **multiplexage I/O** (I/O multiplexing) est la réponse du système d'exploitation à ce problème. Le principe est simple mais puissant : au lieu de bloquer sur un seul descripteur de fichier, on demande au noyau de **surveiller simultanément un ensemble de descripteurs** et de nous notifier dès que l'un d'eux est prêt pour une opération (lecture, écriture, ou condition exceptionnelle).

Le modèle conceptuel est celui d'une **boucle événementielle** (event loop) :

```
                    ┌──────────────────────────────────────────┐
                    │                                          │
                    ▼                                          │
        ┌───────────────────────┐                              │
        │  Enregistrer les fd   │                              │
        │  à surveiller auprès  │                              │
        │  du noyau             │                              │
        └───────────┬───────────┘                              │
                    │                                          │
                    ▼                                          │
        ┌───────────────────────┐                              │
        │  Attendre qu'au moins │                              │
        │  un fd soit prêt      │  ← bloque ici (ou timeout)   │
        └───────────┬───────────┘                              │
                    │                                          │
                    ▼                                          │
        ┌───────────────────────┐                              │
        │  Traiter uniquement   │                              │
        │  les fd signalés      │                              │
        │  comme prêts          │                              │
        └───────────┬───────────┘                              │
                    │                                          │
                    └──────────────────────────────────────────┘
```

Un seul thread peut ainsi gérer des milliers de connexions de manière efficace, en ne traitant que les descripteurs qui ont effectivement des données à lire ou de la capacité à écrire.

## Deux paradigmes : readiness notification vs completion notification

Il est essentiel de comprendre la distinction entre les deux grands modèles d'I/O non bloquante, car elle conditionne l'architecture de votre serveur.

### Modèle « readiness » (notification de disponibilité)

Le noyau vous informe qu'un descripteur **est prêt** pour une opération. C'est ensuite à vous d'effectuer l'opération (read, write, accept). Si l'opération n'est pas possible au moment où vous la tentez (par exemple, un autre thread a lu les données entre-temps), vous devez gérer ce cas.

C'est le modèle utilisé par `select`, `poll` et `epoll`.

```
Application                          Noyau
    │                                  │
    │  "Préviens-moi quand fd=7        │
    │   est prêt en lecture"           │
    │ ────────────────────────────►    │
    │                                  │
    │         (attente)                │
    │                                  │
    │  "fd=7 est prêt !"               │
    │ ◄────────────────────────────    │
    │                                  │
    │  read(fd=7, buffer, len)         │
    │ ────────────────────────────►    │  ← C'est l'application qui lit
    │                                  │
    │  données lues                    │
    │ ◄────────────────────────────    │
```

### Modèle « completion » (notification d'achèvement)

L'application soumet une opération d'I/O au noyau et est notifiée **une fois l'opération terminée**. Le noyau effectue lui-même la lecture ou l'écriture, et vous récupérez directement le résultat.

C'est le modèle utilisé par `io_uring` (Linux) et `IOCP` (Windows).

```
Application                          Noyau
    │                                  │
    │  "Lis 4096 octets depuis fd=7    │
    │   dans ce buffer"                │
    │ ────────────────────────────►    │
    │                                  │
    │         (attente)                │
    │                                  │  ← C'est le noyau qui lit
    │  "Lecture terminée :             │
    │   2048 octets dans ton buffer"   │
    │ ◄────────────────────────────    │
    │                                  │
    │  (buffer déjà rempli)            │
```

Le modèle completion est conceptuellement plus simple pour l'application (pas de gestion des cas `EAGAIN` ou de lectures partielles inattendues) et offre de meilleures performances en réduisant les transitions user-space/kernel-space.

## Descripteurs de fichiers non bloquants : le prérequis

Quel que soit le mécanisme de multiplexage choisi, les descripteurs de fichiers doivent généralement être configurés en **mode non bloquant**. Sans cela, un appel `read()` ou `write()` sur un descripteur signalé comme prêt pourrait tout de même bloquer dans certains cas limites (race conditions, edge-triggered mode, données insuffisantes).

La configuration se fait via `fcntl` :

```cpp
#include <fcntl.h>

/// @brief Configure un descripteur de fichier en mode non bloquant.
/// @param fd Le descripteur à configurer.
/// @return true en cas de succès, false sinon.
bool set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return false;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}
```

En mode non bloquant, les appels système d'I/O retournent immédiatement avec l'erreur `EAGAIN` (ou `EWOULDBLOCK`, qui est synonyme sur Linux) si l'opération ne peut pas être effectuée instantanément. C'est à la boucle événementielle de réessayer plus tard, lorsque le descripteur sera à nouveau signalé comme prêt.

## Level-triggered vs edge-triggered : deux modes de notification

Les mécanismes de multiplexage (en particulier `epoll`) offrent deux modes de notification qu'il faut bien comprendre pour éviter des bugs subtils :

**Level-triggered (LT)** — le comportement par défaut. Le noyau notifie le descripteur comme prêt **tant que** la condition est vraie. Si vous ne lisez pas toutes les données disponibles, le descripteur sera à nouveau signalé comme prêt au prochain appel. C'est le mode le plus permissif et le plus simple à utiliser correctement.

**Edge-triggered (ET)** — le noyau ne notifie qu'**au moment de la transition** (passage de "pas prêt" à "prêt"). Si vous ne consommez pas toutes les données disponibles, vous ne recevrez plus de notification jusqu'à l'arrivée de nouvelles données. Ce mode impose de lire en boucle jusqu'à `EAGAIN` à chaque notification, mais il réduit le nombre de notifications et peut améliorer les performances sous forte charge.

```
Données disponibles dans le buffer noyau :
─────────────────────────────────────────────────────────

Temps ──►     ▕██████████▕          ▕████▕
              │          │          │    │
              ▼          ▼          ▼    ▼

Level-triggered :   🔔 🔔 🔔 🔔      🔔 🔔
                    (notifie tant qu'il reste des données)

Edge-triggered :    🔔                🔔
                    (notifie une seule fois par arrivée)
```

## Panorama des mécanismes disponibles sous Linux

Linux a fait évoluer ses mécanismes de multiplexage au fil des décennies, chaque génération répondant aux limitations de la précédente :

| Mécanisme | Année | Complexité | Connexions | Modèle |
|-----------|-------|-----------|------------|--------|
| `select` | 1983 (BSD) | O(n) | Limité (1024 par défaut) | Readiness, LT |
| `poll` | 1986 (System V) | O(n) | Illimité | Readiness, LT |
| `epoll` | 2002 (Linux 2.5.44) | O(1) pour les événements | Illimité | Readiness, LT ou ET |
| `io_uring` | 2019 (Linux 5.1) | O(1), zéro-copy possible | Illimité | Completion |

Chacun de ces mécanismes fait l'objet d'une sous-section dédiée. Les trois premiers (`select`, `poll`, `epoll`) suivent le modèle readiness avec des niveaux croissants de performance et de sophistication. Le dernier, `io_uring`, représente un changement de paradigme vers le modèle completion et constitue l'avenir de l'I/O haute performance sous Linux.

## Quel mécanisme choisir ?

Avant de plonger dans les détails de chaque API, voici les grandes lignes pour orienter votre choix :

- **`select` et `poll`** sont des interfaces POSIX portables, présentes sur tous les systèmes Unix. Elles conviennent pour des prototypes, des outils avec peu de connexions simultanées, ou lorsque la portabilité multi-plateforme est prioritaire.

- **`epoll`** est le standard de facto pour les serveurs réseau performants sous Linux. C'est le mécanisme sous-jacent utilisé par la majorité des frameworks et serveurs de production (Nginx, Node.js, Redis). Si vous ciblez exclusivement Linux et avez besoin de gérer des milliers de connexions, `epoll` est le choix par défaut.

- **`io_uring`** est le mécanisme le plus récent et le plus performant, particulièrement adapté aux workloads intensifs en I/O (serveurs de fichiers, bases de données, proxies haute performance). Il nécessite un noyau Linux 5.1+ et une compréhension plus approfondie, mais offre des gains significatifs grâce à la réduction des appels système et au batching des opérations.

- **Les librairies de haut niveau** comme Asio (standalone ou Boost) abstraient ces mécanismes derrière une API unifiée et portable. Elles sont traitées en section 22.4.

Les sous-sections suivantes détaillent chaque mécanisme avec des exemples complets et compilables.

⏭️ [select et poll : Interfaces POSIX classiques](/22-networking/03.1-select-poll.md)
