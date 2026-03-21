🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 22.4 — Librairies réseau modernes

## Chapitre 22 : Networking et Communication

---

## Introduction

Les sections précédentes vous ont donné une compréhension solide de ce qui se passe sous le capot : sockets POSIX, modèle client/serveur, multiplexage I/O avec `epoll`. Vous êtes maintenant capable de construire un serveur réseau à partir de rien.

La question est : **devez-vous le faire ?**

La réponse est presque toujours non. Un serveur `epoll` brut comme celui de la section 22.3 fonctionne, mais il lui manque tout ce qu'un serveur de production exige : gestion propre des buffers partiels en écriture, timers intégrés, résolution DNS asynchrone, support TLS/SSL, gestion des reconnexions, backpressure, et un modèle de programmation qui ne se résume pas à une boucle `switch` géante sur des file descriptors.

C'est le rôle des librairies réseau. Elles encapsulent `epoll` (sur Linux), `kqueue` (sur macOS/BSD) et IOCP (sur Windows) derrière une abstraction unifiée, et offrent un modèle de programmation asynchrone structuré qui gère correctement toutes les subtilités que nous avons vues — envois partiels, `EINTR`, `EAGAIN`, fermeture ordonnée, timeouts.

---

## Le paysage des librairies réseau C++ en 2026

L'écosystème réseau C++ s'articule autour de quelques acteurs majeurs :

### Asio — Le standard de facto

**Asio** (Asynchronous I/O) est la librairie réseau dominante en C++ depuis plus de quinze ans. Écrite par Christopher Kohlhoff, elle a été la base de la proposition `std::net` pour le standard C++ (qui n'a finalement pas été adoptée, mais dont l'influence reste considérable).

Asio existe sous deux formes :

- **Standalone Asio** — La librairie seule, header-only, sans dépendance externe. C'est la version recommandée pour les nouveaux projets.  
- **Boost.Asio** — La version intégrée à Boost, avec accès à l'écosystème Boost complet (Boost.Beast pour HTTP/WebSocket, Boost.SSL, etc.).

Les deux partagent le même code et la même API. La différence est l'espace de noms (`asio::` vs `boost::asio::`) et les dépendances.

### Autres librairies notables

**libuv** — La librairie événementielle qui propulse Node.js. Écrite en C, elle offre une boucle événementielle multi-plateforme avec support des sockets, timers, signaux, processus enfants et I/O fichiers. Moins idiomatique en C++ qu'Asio (API C, callbacks par pointeurs de fonction), mais extrêmement éprouvée en production.

**libevent / libev** — Librairies C plus anciennes, toujours utilisées dans des projets comme Memcached (libevent) ou le Redis d'origine (libev, puis un event loop custom). Fonctionnelles mais pas conçues pour le C++ moderne.

**µWebSockets (uWS)** — Librairie spécialisée HTTP et WebSocket, extrêmement performante. Utilisée par des applications nécessitant un throughput maximal sur du WebSocket.

**libcurl / cpr / cpp-httplib** — Librairies spécialisées pour le rôle **client** HTTP. Couvertes en section 22.5.

### Pourquoi Asio domine

Plusieurs facteurs expliquent la position dominante d'Asio :

**Maturité** — Plus de 20 ans de développement, utilisée en production par d'innombrables projets, des startups aux systèmes de trading haute fréquence.

**Modèle asynchrone sophistiqué** — Asio a évolué avec le langage. Son modèle initial à base de callbacks a été complété par le support des coroutines C++20, offrant un code asynchrone qui se lit comme du code synchrone.

**Portabilité** — Une seule API, trois backends : `epoll` sur Linux, `kqueue` sur macOS/BSD, IOCP sur Windows. Votre code réseau fonctionne partout sans modification.

**Écosystème** — Boost.Beast (HTTP/WebSocket), Boost.MySQL, gRPC (qui peut utiliser Asio comme backend), et de nombreuses librairies tierces s'intègrent directement avec Asio.

**Influence sur le standard** — Même si `std::net` n'a pas été adopté, les concepts d'Asio (executors, completion tokens, I/O objects) influencent profondément `std::execution` (C++26). Apprendre Asio, c'est se préparer aux abstractions asynchrones du futur standard.

---

## Le modèle de programmation d'Asio

Avant de plonger dans les sous-sections spécifiques, il est important de comprendre le modèle fondamental d'Asio, commun aux deux variantes (standalone et Boost).

### L'`io_context` — Le cœur du réacteur

Au centre d'Asio se trouve l'`io_context` (anciennement `io_service`). C'est le **réacteur** — l'équivalent de la boucle `epoll_wait` de la section 22.3, mais avec une gestion complète des handlers, timers et dispatch :

```
Section 22.3 (epoll brut)              Asio
──────────────────────────              ────

while (running) {                       asio::io_context ctx;
    epoll_wait(...)                     // ... configurer les I/O ...
    for (event : events) {              ctx.run();  // Boucle événementielle
        switch (event.fd) {             // Asio gère tout :
            case server_fd: ...         //   - epoll/kqueue/IOCP
            case client_fd: ...         //   - dispatch des handlers
        }                               //   - timers
    }                                   //   - résolution DNS
}                                       //   - buffers
```

`io_context::run()` bloque le thread appelant et exécute la boucle événementielle. Quand un événement I/O survient (données reçues, connexion établie, timer expiré), Asio invoque le **handler** que vous avez associé à cette opération.

### Trois styles de programmation

Asio supporte trois modèles de programmation, qui ont évolué avec le langage :

**Callbacks (C++11)** — Le modèle originel. Vous lancez une opération asynchrone et passez une fonction à appeler quand elle se termine :

```cpp
// Pseudo-code illustratif
socket.async_read_some(buffer, [](error_code ec, size_t bytes) {
    if (!ec) {
        // Données reçues — traiter
    }
});
```

Simple pour les opérations isolées, mais les chaînes de callbacks imbriquées deviennent rapidement illisibles (le « callback hell » bien connu en JavaScript).

**Futures / Promises (C++11)** — Les opérations retournent un `std::future` que vous pouvez attendre. Plus linéaire que les callbacks, mais chaque `future::get()` bloque un thread — on perd l'avantage de l'asynchronisme.

**Coroutines (C++20)** — Le modèle moderne et recommandé. Les coroutines permettent d'écrire du code asynchrone qui **ressemble** à du code synchrone — pas de callbacks, pas de machines à états, pas de perte de l'asynchronisme :

```cpp
// Pseudo-code illustratif
asio::awaitable<void> handle_client(tcp::socket socket) {
    char buffer[4096];
    while (true) {
        auto [ec, n] = co_await socket.async_read_some(
            asio::buffer(buffer), asio::as_tuple(asio::use_awaitable));
        if (ec) break;

        co_await async_write(socket, asio::buffer(buffer, n),
                             asio::use_awaitable);
    }
}
```

Ce code est entièrement asynchrone — le thread n'est jamais bloqué — mais il se lit de haut en bas comme du code séquentiel. C'est le meilleur des deux mondes, et c'est **le style recommandé** pour tout nouveau code Asio en 2026.

---

## Concepts clés à retenir avant les sous-sections

### Opérations synchrones vs asynchrones

Asio offre les deux :

```
Synchrone                          Asynchrone
──────────                         ──────────
socket.read_some(buffer);          socket.async_read_some(buffer, handler);
// Bloque jusqu'à la lecture       // Retourne immédiatement
// Retour = données lues           // handler appelé plus tard par io_context::run()
```

Les opérations synchrones sont pratiques pour les scripts et prototypes, mais les opérations asynchrones sont le cœur d'Asio et la raison d'utiliser cette librairie.

### Ownership et durée de vie

Un piège majeur avec la programmation asynchrone : entre le lancement d'une opération et l'appel du handler, du temps passe. L'objet sur lequel porte l'opération (socket, buffer) doit rester **vivant** pendant toute cette durée. C'est la source de bug numéro un avec Asio.

Le pattern standard est d'utiliser `std::shared_ptr` pour les objets de session, de sorte que le handler maintienne une référence et empêche la destruction prématurée. Avec les coroutines, ce problème est largement atténué car la coroutine possède naturellement son état.

### Threads et `io_context`

Un `io_context` peut être exécuté par un seul thread (`ctx.run()`) ou par plusieurs (plusieurs threads appelant `ctx.run()` sur le même objet). Asio garantit que les handlers sont invoqués correctement dans les deux cas, mais les données partagées entre handlers nécessitent toujours une synchronisation explicite ou l'utilisation de strands (mécanisme de sérialisation d'Asio).

---

## Comment s'articulent les sous-sections

Les trois sous-sections qui suivent couvrent Asio sous ses deux formes et les critères de choix :

| Sous-section | Contenu |
|--------------|---------|
| **22.4.1** — Standalone Asio | Installation, configuration CMake, exemples client/serveur TCP avec callbacks et coroutines, timers, résolution DNS asynchrone. C'est la version recommandée pour les nouveaux projets. |
| **22.4.2** — Boost.Asio | Ce que Boost ajoute (Beast pour HTTP/WebSocket, SSL, sérialisation), installation de Boost, différences d'API (`boost::asio::` vs `asio::`), exemples HTTP avec Beast. |
| **22.4.3** — Quand choisir l'un ou l'autre | Critères de décision (dépendances, fonctionnalités nécessaires, projet existant), matrice de choix, stratégie de migration. |

### Guide de lecture rapide

Si vous démarrez un nouveau projet et voulez aller vite, lisez la section 22.4.1 (Standalone Asio) — c'est le chemin le plus direct vers un serveur réseau moderne en C++.

Si votre projet utilise déjà Boost ou si vous avez besoin de HTTP/WebSocket natif, allez directement à la section 22.4.2 (Boost.Asio + Beast).

Si vous hésitez entre les deux, la section 22.4.3 tranchera.

---

## Prérequis techniques

### Compilateur

Asio avec coroutines nécessite un compilateur supportant C++20 :

- **GCC 12+** (recommandé : GCC 14/15 pour un support coroutines mature)  
- **Clang 16+** (avec `-fcoroutines` ou nativement en Clang 18+)

Les exemples de cette section utilisent C++20 au minimum, avec `-std=c++20` ou `-std=c++23`.

### Version d'Asio

La version standalone recommandée en 2026 est **Asio 1.30+**, qui offre un support complet des coroutines C++20 et des completion tokens modernes. Pour Boost.Asio, **Boost 1.86+** est recommandé.

### Prérequis conceptuels

Vous bénéficierez pleinement de ces sections si vous êtes à l'aise avec :

- Les **lambdas** et captures (chapitre 11) — nécessaires pour les callbacks.  
- Les **smart pointers** `shared_ptr` / `enable_shared_from_this` (chapitre 9) — pattern fondamental pour la gestion de durée de vie des sessions.  
- Les **move semantics** (chapitre 10) — les sockets Asio sont déplaçables mais non copiables.  
- Les **coroutines C++20** (section 12.6) — pour le style de programmation recommandé. Si vous n'êtes pas familier avec les coroutines, les exemples avec callbacks restent accessibles.

---

## Le pont entre `epoll` et Asio

Pour relier cette section aux précédentes, voici comment les concepts que vous connaissez déjà se traduisent en Asio :

```
epoll brut (section 22.3)              Asio
──────────────────────────              ────

epoll_create1()                         io_context ctx;

epoll_ctl(ADD, fd, EPOLLIN)             socket.async_read_some(buf, handler)
                                        (Asio gère l'ajout à epoll en interne)

epoll_wait(events)                      ctx.run()
                                        (Asio appelle epoll_wait en boucle)

events[i].data.fd → switch              handler invoqué directement
                                        (plus de dispatch manuel)

timerfd + epoll_ctl                     asio::steady_timer timer(ctx, 5s);
                                        timer.async_wait(handler);

getaddrinfo (bloquant)                  asio::ip::tcp::resolver r(ctx);
                                        r.async_resolve("host", "port", handler);

setsockopt(TCP_NODELAY)                 socket.set_option(tcp::no_delay(true));

close(fd)                               socket.close();
                                        (ou destruction RAII)
```

Tout ce que vous avez appris sur les sockets, le multiplexage et la gestion d'erreurs reste valide — Asio ne fait que l'encapsuler dans une abstraction plus sûre et plus productive.

---

> **Prochaine étape** → Section 22.4.1 : Standalone Asio — installation, configuration, et construction d'un serveur TCP asynchrone complet avec coroutines C++20.

⏭️ [Standalone Asio : Networking sans Boost](/22-networking/04.1-standalone-asio.md)
