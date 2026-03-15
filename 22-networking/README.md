🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 22 — Networking et Communication ⭐

## Module 7 : Programmation Système sur Linux — Niveau Avancé

---

## Vue d'ensemble

La communication réseau est au cœur de pratiquement tous les systèmes modernes. Qu'il s'agisse d'un microservice répondant à des requêtes HTTP, d'un agent de monitoring qui remonte des métriques, d'un outil CLI qui interroge une API distante ou d'un moteur de jeu gérant des connexions temps réel — **tout finit par passer par le réseau**.

C++ occupe une position particulière dans cet écosystème. Là où des langages comme Go ou Java offrent des abstractions réseau intégrées au langage, C++ s'appuie sur une combinaison de l'API POSIX héritée du C, de la bibliothèque standard (encore limitée sur ce terrain) et de librairies tierces matures. Cette situation, parfois perçue comme un inconvénient, est en réalité un **atout pour quiconque veut comprendre ce qui se passe réellement** sous le capot — et pour quiconque a besoin de performances réseau maximales.

Ce chapitre vous emmène des fondations bas niveau jusqu'aux abstractions modernes utilisées en production.

---

## Ce que vous allez apprendre

Ce chapitre couvre l'intégralité du spectre réseau en C++ sur Linux, organisé en une progression logique :

**Les fondations POSIX** — Vous commencerez par l'API sockets, la brique élémentaire de toute communication réseau sous Linux. Création de sockets, connexion, écoute, envoi et réception de données en TCP et UDP : c'est le socle sur lequel tout le reste repose. Même si vous n'écrirez probablement jamais un serveur de production avec des appels `send()` et `recv()` bruts, comprendre cette couche est indispensable pour diagnostiquer des problèmes réseau, interpréter des traces, ou évaluer les abstractions que vous utiliserez ensuite.

**Le modèle client/serveur** — Vous implémenterez un serveur TCP et son client associé, en comprenant le cycle de vie complet d'une connexion : du `bind` initial jusqu'au `close` final, en passant par le three-way handshake TCP et les subtilités du backlog de connexions.

**Le multiplexage I/O** — Un serveur qui ne gère qu'un seul client à la fois n'a pas grand intérêt. Vous découvrirez les mécanismes de multiplexage (`select`, `poll`, `epoll`) qui permettent de gérer des milliers de connexions simultanées dans un seul thread — le fondement des serveurs haute performance sous Linux.

**Les librairies réseau modernes** — Vous passerez ensuite aux abstractions que l'on utilise réellement en production. Standalone Asio et Boost.Asio offrent un modèle asynchrone puissant, inspiré des patterns qui ont façonné la programmation réseau C++ depuis plus d'une décennie. Vous apprendrez quand choisir l'un ou l'autre, et comment les intégrer dans vos projets.

**Les clients HTTP** — La communication HTTP est omniprésente dans les architectures modernes. Vous découvrirez `cpr` (un wrapper élégant autour de libcurl) et `cpp-httplib`, deux approches complémentaires pour effectuer des requêtes HTTP depuis du C++ sans réinventer la roue.

**gRPC et Protocol Buffers** — Enfin, vous aborderez gRPC, le framework RPC haute performance de Google. Définition de services avec Protocol Buffers, génération de code, streaming bidirectionnel : c'est l'outil de choix pour la communication inter-services dans les architectures microservices modernes.

---

## Pourquoi ce chapitre est important

### Pour le développeur système

La maîtrise de l'API sockets POSIX et du multiplexage I/O est un prérequis pour tout travail de programmation système sous Linux. Les serveurs web (Nginx, HAProxy), les bases de données (Redis, PostgreSQL), les brokers de messages (Kafka) — tous reposent sur `epoll` et des sockets non-bloquantes. Comprendre ces mécanismes, c'est pouvoir lire, modifier et optimiser le code qui fait tourner l'infrastructure.

### Pour le développeur DevOps / SRE

Les outils que vous construisez ou maintenez — agents de déploiement, collecteurs de métriques, health checkers, outils CLI — communiquent tous sur le réseau. Savoir implémenter un client HTTP robuste, intégrer une communication gRPC, ou diagnostiquer un problème de socket est une compétence quotidienne.

### Pour la performance

C++ est souvent choisi précisément parce que les performances réseau comptent. Comprendre la différence entre I/O bloquante et non-bloquante, entre `select` et `epoll`, entre une copie en espace utilisateur et un zero-copy `sendfile`, c'est ce qui sépare un programme qui tient la charge d'un programme qui s'effondre sous la pression.

---

## Prérequis

Avant d'aborder ce chapitre, assurez-vous d'être à l'aise avec :

- **La gestion de la mémoire** (chapitre 5) — Les buffers réseau impliquent de l'allocation, du dimensionnement et de la gestion de durée de vie. Les smart pointers (chapitre 9) seront vos alliés pour gérer les ressources réseau.
- **Les smart pointers et RAII** (chapitres 6 et 9) — Les sockets sont des ressources système : elles doivent être fermées proprement. Le pattern RAII est la manière idiomatique de garantir cela en C++.
- **Les threads et la concurrence** (chapitre 21) — Le networking et la concurrence sont intimement liés. Les serveurs multi-threads, les pools de connexions et la programmation asynchrone supposent une bonne compréhension des primitives de synchronisation.
- **Les appels système POSIX** (chapitres 19-20) — L'API sockets s'inscrit dans la continuité des appels système que vous avez déjà manipulés pour le système de fichiers et les signaux.

---

## Paquets et outils nécessaires

Installez les dépendances nécessaires pour suivre l'ensemble du chapitre :

```bash
# Outils réseau de diagnostic
sudo apt install net-tools iproute2 netcat-openbsd tcpdump wireshark

# Librairies de développement réseau
sudo apt install libssl-dev

# Boost.Asio (si vous choisissez la version Boost)
sudo apt install libboost-all-dev

# Protobuf et gRPC
sudo apt install protobuf-compiler libprotobuf-dev libgrpc++-dev protobuf-compiler-grpc
```

> Les librairies comme `cpr`, `cpp-httplib` et standalone Asio seront installées via votre gestionnaire de dépendances (Conan ou vcpkg) ou via CMake `FetchContent`, comme détaillé dans leurs sections respectives.

---

## Plan du chapitre

| Section | Sujet | Niveau |
|---------|-------|--------|
| **22.1** | Sockets TCP/UDP : API POSIX | Avancé |
| **22.2** | Client/Serveur basique en C++ | Avancé |
| **22.3** | Multiplexage I/O : select, poll, epoll | Avancé |
| **22.4** | Librairies réseau modernes (Asio) | Avancé |
| **22.5** | Clients HTTP : cpr, cpp-httplib | Avancé |
| **22.6** | gRPC et Protocol Buffers | Avancé |

---

## Fil conducteur du chapitre

La progression de ce chapitre suit une logique ascendante délibérée :

```
Couche basse                                          Couche haute
─────────────────────────────────────────────────────────────────►

  Sockets     Client/     Multiplexage    Asio       HTTP      gRPC
  POSIX       Serveur     I/O             async      clients   + Protobuf
  (22.1)      (22.2)      (22.3)          (22.4)     (22.5)    (22.6)

  ◄── Comprendre ──►  ◄── Maîtriser ──►  ◄── Produire ──►
```

Les sections 22.1 à 22.3 construisent votre **compréhension fondamentale** : vous saurez exactement ce qui se passe quand un paquet traverse la pile réseau. Les sections 22.4 à 22.6 vous donnent les **outils de production** : les librairies et frameworks que vous utiliserez dans vos projets réels.

Cette progression n'est pas un accident pédagogique. Un développeur qui utilise gRPC sans comprendre les sockets est un développeur qui ne pourra pas diagnostiquer un timeout étrange, un problème de backpressure, ou une fuite de file descriptors. À l'inverse, un développeur qui n'utilise que l'API POSIX brute en production réinvente la roue — mal.

---

## Conventions utilisées dans ce chapitre

Tout au long de ce chapitre, les exemples de code suivent ces conventions :

- **Gestion d'erreurs systématique** — Chaque appel système est vérifié. En production réseau, ignorer une erreur de `send()` ou de `connect()` est une source de bugs subtils et difficiles à reproduire.
- **RAII pour les ressources réseau** — Les file descriptors de sockets sont encapsulés dans des wrappers RAII. Vous ne verrez pas de `close()` appelé manuellement dans du code de haut niveau.
- **IPv4 et IPv6** — Les exemples privilégient `getaddrinfo` plutôt que des adresses codées en dur, pour supporter les deux protocoles de manière transparente.
- **C++ moderne** — Les exemples utilisent C++17 au minimum, avec des incursions en C++20/C++23 lorsque les fonctionnalités le justifient (par exemple `std::expected` pour la gestion d'erreurs réseau).

---

## Connexions avec les autres chapitres

Ce chapitre s'articule étroitement avec plusieurs autres parties de la formation :

- **Chapitre 21 (Threads et concurrence)** — Les serveurs réseau sont intrinsèquement concurrents. Les patterns de synchronisation vus au chapitre 21 seront directement appliqués ici.
- **Chapitre 24 (Sérialisation JSON/YAML)** — Les données échangées sur le réseau doivent être sérialisées. JSON est le format dominant pour les API REST, tandis que Protocol Buffers (section 22.6) est le choix performant pour le RPC.
- **Chapitre 25 (Formats binaires)** — Protocol Buffers, introduit ici dans le contexte gRPC, est couvert en profondeur au chapitre 25 en tant que format de sérialisation autonome.
- **Chapitre 37 (Dockerisation)** — Les applications réseau sont les premières candidates à la conteneurisation. La configuration des ports, des réseaux Docker et des health checks fait le lien direct avec ce chapitre.
- **Chapitre 40 (Observabilité)** — En production, le monitoring réseau (latence, throughput, erreurs de connexion) est critique. Les métriques Prometheus et le tracing OpenTelemetry vus au chapitre 40 s'appliquent directement aux services réseau.

---

## Note sur l'état du networking dans le standard C++

Il est important de situer où en est le standard C++ concernant le networking. Contrairement à Python (`socket`, `http.server`, `asyncio`), Go (`net/http`) ou Java (`java.net`), **C++ n'a toujours pas de librairie réseau dans son standard** en 2026.

La proposition `std::net`, basée sur Asio, a été discutée pendant des années au sein du comité ISO mais n'a finalement pas été intégrée à C++23 ni à C++26. L'arrivée de `std::execution` (Senders/Receivers) en C++26 redéfinit cependant le paysage : une future librairie réseau standardisée s'appuierait probablement sur ce nouveau modèle d'asynchronisme plutôt que sur le modèle completion-handler d'Asio.

En attendant, l'écosystème C++ s'appuie sur des librairies tierces éprouvées — Asio en tête — qui sont stables, performantes et largement adoptées. C'est cette réalité pragmatique que reflète ce chapitre.

---

> **Prêt ?** Commencez par la section 22.1 pour plonger dans l'API sockets POSIX — le point de départ de toute communication réseau sous Linux.

⏭️ [Sockets TCP/UDP : API POSIX](/22-networking/01-sockets-posix.md)
