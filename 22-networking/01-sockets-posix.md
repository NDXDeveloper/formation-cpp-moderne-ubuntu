🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 22.1 — Sockets TCP/UDP : API POSIX

## Chapitre 22 : Networking et Communication

---

## Introduction

Avant d'utiliser Asio, gRPC ou n'importe quelle librairie réseau de haut niveau, il faut comprendre ce qui se passe en dessous. Et en dessous, sur Linux, il y a l'**API sockets POSIX** — une interface vieille de plus de quarante ans, héritée de BSD Unix (1983), qui reste aujourd'hui encore le mécanisme fondamental par lequel tout programme communique sur un réseau.

Chaque connexion HTTP que votre navigateur établit, chaque requête DNS que votre système résout, chaque paquet que Nginx ou Redis envoie — tout passe par des appels système sockets. Quand vous appelez `connect()` dans Boost.Asio ou que gRPC ouvre un canal, c'est cette même API qui est invoquée en dernière instance.

Cette section vous donne une compréhension solide de cette couche fondatrice : ce qu'est un socket, comment TCP et UDP se distinguent à ce niveau, quelles structures de données le noyau utilise pour représenter les adresses réseau, et comment l'ensemble s'articule dans le modèle client/serveur.

---

## Qu'est-ce qu'un socket ?

Un socket est un **point de terminaison de communication** (endpoint). C'est l'abstraction que le noyau Linux expose aux programmes en espace utilisateur pour envoyer et recevoir des données sur un réseau — ou même entre processus sur la même machine.

Du point de vue du programme, un socket est un **file descriptor** — un simple entier, exactement comme celui retourné par `open()` pour un fichier. Cette unification est un choix de design fondamental d'Unix : « everything is a file ». Vous pouvez utiliser `read()` et `write()` sur un socket, tout comme sur un fichier ordinaire. Mais les sockets offrent en plus des opérations spécifiques (`send`, `recv`, `bind`, `listen`, `accept`, `connect`) qui n'ont pas d'équivalent dans le monde des fichiers.

Concrètement, quand vous créez un socket avec l'appel système `socket()`, le noyau alloue une structure interne qui contient l'état de la connexion (ou de l'association, en UDP), les buffers d'envoi et de réception, les options de configuration, et le protocole utilisé. Le file descriptor que vous récupérez est simplement un index dans la table des descripteurs du processus, qui pointe vers cette structure noyau.

```
Programme utilisateur                    Noyau Linux
┌──────────────────┐                ┌─────────────────────────┐
│                  │                │                         │
│  int sockfd = 3  │───────────────►│  struct socket {        │
│                  │  file          │    état, buffers,       │
│                  │  descriptor    │    protocole, options   │
│                  │                │  }                      │
└──────────────────┘                └──────────┬──────────────┘
                                               │
                                               ▼
                                    ┌─────────────────────┐
                                    │  Pile réseau        │
                                    │  (TCP/IP, UDP/IP)   │
                                    └──────────┬──────────┘
                                               │
                                               ▼
                                    ┌─────────────────────┐
                                    │  Interface réseau   │
                                    │  (eth0, lo, ...)    │
                                    └─────────────────────┘
```

---

## TCP vs UDP : deux modèles de communication

L'API sockets supporte plusieurs protocoles, mais dans l'immense majorité des cas, vous travaillerez avec deux d'entre eux : **TCP** et **UDP**. Le choix entre les deux détermine fondamentalement le comportement de vos sockets.

### TCP (Transmission Control Protocol)

TCP est un protocole **orienté connexion**, **fiable** et **ordonné**. Quand vous envoyez des données via TCP, le protocole garantit qu'elles arriveront à destination, dans l'ordre, sans duplication et sans corruption. Si un paquet est perdu en route, TCP le retransmet automatiquement. Si des paquets arrivent dans le désordre, TCP les réordonne avant de les livrer à l'application.

Cette fiabilité a un coût : TCP nécessite un **three-way handshake** pour établir une connexion (SYN → SYN-ACK → ACK), maintient un état par connexion dans le noyau, et utilise des mécanismes de contrôle de flux et de congestion qui ajoutent de la latence.

TCP fonctionne comme un **flux d'octets** (byte stream). Il n'y a pas de notion de « message » ou de « paquet » au niveau applicatif : si vous envoyez 1000 octets en un seul appel à `send()`, le destinataire peut les recevoir en plusieurs morceaux (par exemple 400 puis 600, ou 100 dix fois). C'est à l'application de reconstituer les messages — un point qui surprend régulièrement les développeurs débutants en réseau.

Du côté de l'API, TCP utilise le type de socket `SOCK_STREAM`.

### UDP (User Datagram Protocol)

UDP est un protocole **sans connexion**, **non fiable** et **non ordonné**. Chaque appel à `sendto()` envoie un datagramme indépendant. Le protocole ne garantit ni la livraison, ni l'ordre, ni l'absence de duplication. Un datagramme peut être perdu, arriver après un autre envoyé plus tard, ou arriver en double — et l'application n'en sera pas informée.

En contrepartie, UDP est **léger et rapide**. Pas de handshake, pas d'état de connexion maintenu, pas de retransmission automatique. Chaque datagramme est envoyé indépendamment et le plus vite possible.

Contrairement à TCP, UDP **préserve les frontières de messages**. Si vous envoyez un datagramme de 500 octets, le destinataire le recevra en un seul bloc de 500 octets — jamais fragmenté au niveau applicatif (la fragmentation IP, si elle intervient, est transparente pour l'application).

Du côté de l'API, UDP utilise le type de socket `SOCK_DGRAM`.

### Résumé comparatif

```
                    TCP (SOCK_STREAM)           UDP (SOCK_DGRAM)
                    ─────────────────           ─────────────────
Connexion           Oui (3-way handshake)       Non  
Fiabilité           Garantie (retransmission)   Aucune garantie  
Ordre               Garanti                     Non garanti  
Frontières msg      Non (flux d'octets)         Oui (datagrammes)  
Overhead            Élevé                       Faible  
Latence             Plus élevée                 Plus faible  
Cas d'usage         HTTP, gRPC, bases de        DNS, streaming vidéo,  
                    données, SSH, SMTP          jeux temps réel, VoIP
```

### Quand choisir l'un ou l'autre ?

La règle par défaut est simple : **choisissez TCP**, sauf si vous avez une raison spécifique de ne pas le faire. TCP est le bon choix pour la grande majorité des applications : API REST, communication inter-services, transfert de fichiers, protocoles applicatifs personnalisés.

UDP se justifie dans des cas plus ciblés : quand la latence prime sur la fiabilité (streaming audio/vidéo, jeux en temps réel), quand le modèle requête-réponse est simple et sans état (DNS), quand le multicast est nécessaire, ou quand vous implémentez votre propre mécanisme de fiabilité au-dessus d'UDP (comme QUIC, le protocole qui propulse HTTP/3).

---

## Familles d'adresses : IPv4, IPv6 et Unix

Au moment de créer un socket, vous devez spécifier une **famille d'adresses** (address family) qui détermine le type d'adresses utilisé :

**`AF_INET`** — La famille IPv4. Les adresses sont sur 32 bits (ex : `192.168.1.1`). C'est historiquement la plus utilisée, et la plupart des tutoriels et exemples que vous trouverez en ligne l'utilisent.

**`AF_INET6`** — La famille IPv6. Les adresses sont sur 128 bits (ex : `2001:db8::1`). IPv6 est incontournable dans les environnements cloud modernes, et de nombreux services (AWS, GCP) l'utilisent nativement. Un socket IPv6 peut, selon sa configuration, accepter également des connexions IPv4 — c'est le mode **dual-stack**.

**`AF_UNIX`** (ou `AF_LOCAL`) — Communication entre processus sur la même machine, via le système de fichiers. Pas de réseau impliqué, mais la même API sockets. C'est ce qu'utilisent Docker, PostgreSQL ou Nginx pour leur communication locale. Ce sujet est abordé plus en détail au chapitre 23 (IPC).

---

## Les structures d'adresses

L'API sockets utilise un système de structures d'adresses qui reflète son héritage C et son design générique — il doit supporter des familles d'adresses très différentes avec une interface commune. Ce système peut sembler verbeux comparé aux abstractions modernes, mais il est important de le comprendre car vous le retrouverez partout.

### La structure générique : `sockaddr`

Toutes les fonctions de l'API sockets (`bind`, `connect`, `accept`, etc.) prennent un pointeur `struct sockaddr*` comme paramètre d'adresse. C'est la structure **générique**, un peu comme une classe de base dans un design orienté objet :

```cpp
struct sockaddr {
    sa_family_t sa_family;   // Famille d'adresses (AF_INET, AF_INET6, ...)
    char        sa_data[14]; // Données d'adresse (format dépend de la famille)
};
```

En pratique, vous ne remplissez jamais `sockaddr` directement. Vous utilisez une structure **spécifique** à la famille d'adresses, puis vous la castez en `sockaddr*` au moment de l'appel.

### IPv4 : `sockaddr_in`

Pour les adresses IPv4 :

```cpp
#include <netinet/in.h>

struct sockaddr_in {
    sa_family_t    sin_family;  // Toujours AF_INET
    in_port_t      sin_port;    // Port (en network byte order)
    struct in_addr sin_addr;    // Adresse IPv4 (32 bits, network byte order)
    char           sin_zero[8]; // Padding (remplir avec des zéros)
};
```

### IPv6 : `sockaddr_in6`

Pour les adresses IPv6 :

```cpp
#include <netinet/in.h>

struct sockaddr_in6 {
    sa_family_t     sin6_family;   // Toujours AF_INET6
    in_port_t       sin6_port;     // Port (en network byte order)
    uint32_t        sin6_flowinfo; // Flow information
    struct in6_addr sin6_addr;     // Adresse IPv6 (128 bits)
    uint32_t        sin6_scope_id; // Scope ID
};
```

### Le pattern d'utilisation

Le pattern est toujours le même : vous déclarez et remplissez la structure spécifique, puis vous la castez au moment de l'appel :

```cpp
struct sockaddr_in addr{};  
addr.sin_family = AF_INET;  
addr.sin_port = htons(8080);  
addr.sin_addr.s_addr = INADDR_ANY;  

// Cast vers sockaddr* pour l'API
bind(sockfd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
```

Ce cast est l'une des rares utilisations légitimes de `reinterpret_cast` en C++ moderne. L'API sockets POSIX a été conçue en C, avant l'existence du polymorphisme — ce mécanisme de « pointeur générique + taille » est la manière C de faire du dispatch sur le type.

### L'alternative moderne : `sockaddr_storage`

Quand vous devez stocker une adresse dont vous ne connaissez pas la famille à l'avance (par exemple, l'adresse retournée par `accept()`), utilisez `sockaddr_storage`. Cette structure est dimensionnée pour contenir **n'importe quel type d'adresse** supporté par le système :

```cpp
struct sockaddr_storage addr{};  
socklen_t addr_len = sizeof(addr);  

int client_fd = accept(sockfd, reinterpret_cast<struct sockaddr*>(&addr), &addr_len);

// Ensuite, inspecter addr.ss_family pour savoir le type réel
if (addr.ss_family == AF_INET) {
    auto* ipv4 = reinterpret_cast<struct sockaddr_in*>(&addr);
    // Traiter l'adresse IPv4
} else if (addr.ss_family == AF_INET6) {
    auto* ipv6 = reinterpret_cast<struct sockaddr_in6*>(&addr);
    // Traiter l'adresse IPv6
}
```

---

## Network byte order : le piège classique

Les protocoles réseau utilisent un ordre d'octets standardisé appelé **network byte order**, qui correspond au **big-endian** (l'octet de poids fort en premier). Or, la majorité des processeurs modernes (x86, x86_64) fonctionnent en **little-endian** (l'octet de poids faible en premier).

Si vous stockez le port `8080` directement dans `sin_port` sans conversion, les octets seront dans le mauvais ordre et le système écoutera sur un port complètement différent. C'est un bug classique, silencieux et frustrant.

Les fonctions de conversion sont :

```cpp
#include <arpa/inet.h>

// Host to Network
uint16_t htons(uint16_t hostshort);  // Pour les ports (16 bits)  
uint32_t htonl(uint32_t hostlong);   // Pour les adresses IPv4 (32 bits)  

// Network to Host
uint16_t ntohs(uint16_t netshort);   // Port réseau → hôte  
uint32_t ntohl(uint32_t netlong);    // Adresse réseau → hôte  
```

Le moyen mnémotechnique : **h**ost **to** **n**etwork **s**hort / **l**ong. Utilisez `htons()` systématiquement pour les ports et `htonl()` pour les adresses — même si votre machine est déjà en big-endian. Les fonctions sont des no-ops dans ce cas, et votre code reste portable.

---

## Résolution d'adresses : `getaddrinfo`

Les anciens tutoriels utilisent `inet_pton()` ou `inet_addr()` pour convertir des adresses textuelles en binaire. Ces fonctions sont limitées à une seule famille d'adresses et ne gèrent pas la résolution DNS.

L'approche moderne est `getaddrinfo()`, qui fait tout : résolution DNS, support IPv4/IPv6, et retour d'une liste de résultats prête à l'emploi :

```cpp
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

struct addrinfo hints{};  
hints.ai_family = AF_UNSPEC;      // IPv4 ou IPv6, peu importe  
hints.ai_socktype = SOCK_STREAM;  // TCP  

struct addrinfo* result = nullptr;  
int status = getaddrinfo("example.com", "8080", &hints, &result);  

if (status != 0) {
    std::cerr << "getaddrinfo: " << gai_strerror(status) << '\n';
    // Gérer l'erreur
}

// result est une liste chaînée de struct addrinfo
// Chaque élément contient un ai_addr prêt pour connect() ou bind()

// Parcourir et tenter la connexion sur chaque résultat...

freeaddrinfo(result);  // Libérer la mémoire allouée
```

`getaddrinfo` est la **seule fonction de résolution que vous devriez utiliser** dans du code moderne. Elle offre plusieurs avantages décisifs : elle gère IPv4 et IPv6 de manière transparente via `AF_UNSPEC`, elle effectue la résolution DNS, et elle retourne des structures directement utilisables avec `socket()`, `bind()` et `connect()` — éliminant ainsi le besoin de remplir manuellement les `sockaddr_in` ou `sockaddr_in6` dans la plupart des cas.

> ⚠️ **Attention** : `getaddrinfo` alloue de la mémoire dynamiquement. L'appel à `freeaddrinfo()` est obligatoire pour éviter une fuite. Dans un contexte RAII, encapsulez le résultat dans un wrapper qui appelle `freeaddrinfo` dans son destructeur — ou utilisez un `std::unique_ptr` avec un deleter personnalisé.

---

## Le cycle de vie d'un socket : vue d'ensemble

Avant de détailler chaque opération dans les sous-sections suivantes, voici une vue d'ensemble des appels système impliqués dans une communication TCP complète. Ce schéma est le plan de route pour les sections 22.1.1 à 22.1.3 :

```
         Serveur                                Client
         ───────                                ──────

    ┌─ socket()                            socket() ──┐
    │                                                 │
    ├─ bind()                                         │
    │  (associer à une                                │
    │   adresse + port)                               │
    │                                                 │
    ├─ listen()                                       │
    │  (file d'attente                                │
    │   de connexions)                                │
    │                                                 │
    │              ┌── SYN ──────────────►            │
    │              │                                  │
    │              ◄── SYN-ACK ──────────┐            │
    │              │                     │            │
    │              ├── ACK ──────────────►       connect()
    │              │   (3-way handshake)              │
    │              │                                  │
    ├─ accept() ───┘                                  │
    │  (retourne un                                   │
    │   nouveau sockfd)                               │
    │                                                 │
    │         ◄──── send() / recv() ────►             │
    │              (échange de données)               │
    │                                                 │
    ├─ close()                              close() ──┘
    │
    └─ (retour à accept() pour le prochain client)
```

Pour UDP, le cycle est plus simple — pas de `listen()`, pas de `accept()`, pas de handshake :

```
         Serveur                                Client
         ───────                                ──────

    ┌─ socket()                            socket() ──┐
    │                                                 │
    ├─ bind()                                         │
    │                                                 │
    │         ◄── sendto() / recvfrom() ──►           │
    │              (datagrammes indépendants)         │
    │                                                 │
    ├─ close()                              close() ──┘
```

---

## Headers et includes

Pour travailler avec l'API sockets sur Linux, vous aurez besoin des headers suivants. Voici un récapitulatif organisé par fonction :

```cpp
// Création et manipulation de sockets
#include <sys/socket.h>    // socket, bind, listen, accept, connect, send, recv
#include <sys/types.h>     // Types de données (historique, souvent inclus implicitement)

// Structures d'adresses
#include <netinet/in.h>    // sockaddr_in, sockaddr_in6, INADDR_ANY, in_port_t
#include <arpa/inet.h>     // htons, htonl, ntohs, ntohl, inet_ntop, inet_pton

// Résolution d'adresses
#include <netdb.h>         // getaddrinfo, freeaddrinfo, gai_strerror

// Fermeture de sockets
#include <unistd.h>        // close

// Gestion d'erreurs
#include <cerrno>          // errno
#include <cstring>         // strerror
```

> 💡 Ces headers sont des headers C. En C++, vous pouvez utiliser les variantes préfixées `c` (`<cerrno>`, `<cstring>`) pour les headers de la librairie standard C, mais les headers POSIX (`<sys/socket.h>`, `<netinet/in.h>`, etc.) n'ont pas d'équivalent C++ — ils s'utilisent tels quels.

---

## Plan de la section

Les trois sous-sections qui suivent détaillent chaque phase du cycle de vie :

| Sous-section | Contenu |
|--------------|---------|
| **22.1.1** — Création de sockets | L'appel `socket()`, choix du domaine, du type et du protocole. Options de socket avec `setsockopt()`. Encapsulation RAII du file descriptor. |
| **22.1.2** — bind, listen, accept, connect | Les opérations qui établissent la communication : côté serveur (`bind` → `listen` → `accept`) et côté client (`connect`). Le backlog et ses implications. |
| **22.1.3** — send, recv, sendto, recvfrom | L'échange de données proprement dit. Gestion des envois partiels, signaux interrupteurs (`EINTR`), et différences entre les variantes TCP et UDP. |

---

> **Prochaine étape** → Section 22.1.1 : Création de sockets — où vous créerez votre premier socket et l'encapsulerez dans un wrapper RAII propre.

⏭️ [Création de sockets](/22-networking/01.1-creation-sockets.md)
