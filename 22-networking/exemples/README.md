# Chapitre 22 — Networking et Communication : Exemples

Tous les exemples sont compilés avec `g++-15 -std=c++23 -Wall -Wextra -pthread`.

## Fichiers partagés

| Fichier | Description |
|---------|-------------|
| `net_utils.hpp` | Header RAII partagé : Socket, AddrInfoPtr, resolve(), send_all(), recv_exact(), format_address(), connect_to() |
| `httplib.h` | cpp-httplib (header-only) |
| `proto/` | Fichiers Protocol Buffers (.proto) |
| `generated/` | Code C++ généré par protoc/grpc_cpp_plugin |

---

## Section 22.1 — Sockets TCP/UDP (API POSIX)

### ex01_1_socket_wrapper.cpp
- **Section** : 22.1.1 — Création de sockets
- **Description** : Wrapper RAII Socket, AddrInfoPtr, resolve(), setsockopt, déplacement (move semantics), getsockopt
- **Fichier source** : `01.1-creation-sockets.md`
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -pthread -o ex01_1 ex01_1_socket_wrapper.cpp`
- **Sortie attendue** : Création de sockets TCP/UDP IPv4/IPv6, test déplacement, resolve(), taille buffer réception

### ex01_2_server_client.cpp
- **Section** : 22.1.2 — bind, listen, accept, connect
- **Description** : Serveur/client TCP avec accept4, log_client_info, résolution DNS multi-adresses
- **Fichier source** : `01.2-operations-sockets.md`
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -pthread -o ex01_2 ex01_2_server_client.cpp`
- **Sortie attendue** : Serveur écoute port 18080, client envoie "Hello, server!", reçoit l'echo

### ex01_3_send_recv.cpp
- **Section** : 22.1.3 — send, recv, sendto, recvfrom
- **Description** : send_all, recv_exact, framing length-prefixed, SendResult robuste, safe_recv, retry_on_eintr
- **Fichier source** : `01.3-envoi-reception.md`
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -pthread -o ex01_3 ex01_3_send_recv.cpp`
- **Sortie attendue** : Échange exact de 14 octets, echo, message framé "Message avec framing!", réponse framée "Bonjour du serveur!"

---

## Section 22.2 — Client/Serveur basique

### ex02_echo.cpp
- **Section** : 22.2 — Client/Serveur basique en C++
- **Description** : Serveur echo séquentiel avec SIGPIPE, dual-stack IPv6, client echo
- **Fichier source** : `02-client-serveur.md`
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -pthread -o ex02_echo ex02_echo.cpp`
- **Sortie attendue** : Echo de "hello world" et "bonjour le monde"

### ex02_protocol.cpp
- **Section** : 22.2 — Client/Serveur basique en C++
- **Description** : Serveur protocole avec framing, commandes JSON (echo, time, stats), statistiques atomiques
- **Fichier source** : `02-client-serveur.md`
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -pthread -o ex02_protocol ex02_protocol.cpp`
- **Sortie attendue** : Réponses JSON pour echo, time, stats et commande inconnue

### ex02_pool.cpp
- **Section** : 22.2 — Client/Serveur basique en C++
- **Description** : Serveur echo avec thread pool (ConnectionQueue), 4 workers
- **Fichier source** : `02-client-serveur.md`
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -pthread -o ex02_pool ex02_pool.cpp`
- **Sortie attendue** : 2 clients traités par des workers différents, echo correct

---

## Section 22.3 — Multiplexage I/O

### ex03_select.cpp
- **Section** : 22.3 — Multiplexage I/O
- **Description** : Serveur echo multiplexé avec select()
- **Fichier source** : `03-multiplexage-io.md`
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -pthread -o ex03_select ex03_select.cpp`
- **Sortie attendue** : Nouveau client, echo "Hello select!", déconnexion

### ex03_poll.cpp
- **Section** : 22.3 — Multiplexage I/O
- **Description** : Serveur echo multiplexé avec poll(), détection POLLHUP/POLLERR
- **Fichier source** : `03-multiplexage-io.md`
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -pthread -o ex03_poll ex03_poll.cpp`
- **Sortie attendue** : Nouveau client, echo "Hello poll!", déconnexion

### ex03_epoll.cpp
- **Section** : 22.3 — Multiplexage I/O
- **Description** : Serveur echo avec epoll (RAII wrapper), timerfd, sockets non-bloquants
- **Fichier source** : `03-multiplexage-io.md`
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -pthread -o ex03_epoll ex03_epoll.cpp`
- **Sortie attendue** : Client connecté, echo "Hello epoll!", 3 timers expirés, arrêt propre

### ex03_iouring_init.cpp
- **Section** : 22.3.3.2 — liburing : Interface C/C++ simplifiée
- **Description** : Initialisation d'une instance io_uring, obtention d'un SQE, cleanup
- **Fichier source** : `03.3.2-liburing.md`
- **Compilation** : `g++-15 -std=c++23 -O2 -o ex03_iouring_init ex03_iouring_init.cpp -luring`
- **Sortie attendue** :
```
io_uring initialise OK
  SQ entries: 256
  CQ entries: 512
  SQE obtenu OK
io_uring cleanup OK
```

### ex03_iouring_file.cpp
- **Section** : 22.3.3.3 — Cas d'usage : networking, fichiers, timeouts
- **Description** : Lecture asynchrone d'un fichier par blocs de 64 Ko avec io_uring. Toutes les lectures sont soumises avant la première completion.
- **Fichier source** : `03.3.3-cas-usage.md`
- **Compilation** : `g++-15 -std=c++23 -O2 -o ex03_iouring_file ex03_iouring_file.cpp -luring`
- **Execution** : `./ex03_iouring_file /etc/hostname`
- **Sortie attendue** :
```
Soumission de 1 lectures asynchrones (N octets)  
Lecture terminee : N octets lus, 0 erreurs  
```
- **Prerequis** : `sudo apt install liburing-dev`

---

## Section 22.4.1 — Standalone Asio

> Compilation : ajouter `-DASIO_STANDALONE`

### ex04_1_callback.cpp
- **Section** : 22.4.1 — Standalone Asio
- **Description** : Serveur echo async avec callbacks et shared_from_this
- **Fichier source** : `04.1-standalone-asio.md`
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -pthread -DASIO_STANDALONE -o ex04_1_callback ex04_1_callback.cpp`
- **Sortie attendue** : Nouveau client, réponse "Hello from Asio!\n", déconnexion

### ex04_1_coroutine.cpp
- **Section** : 22.4.1 — Standalone Asio
- **Description** : Serveur echo avec coroutines C++20 (co_await, co_spawn)
- **Fichier source** : `04.1-standalone-asio.md`
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -pthread -DASIO_STANDALONE -o ex04_1_coroutine ex04_1_coroutine.cpp`
- **Sortie attendue** : Client connecté, réponse "Hello from coroutine test!\n", déconnexion

### ex04_1_production.cpp
- **Section** : 22.4.1 — Standalone Asio
- **Description** : Serveur production-ready avec as_tuple, TCP_NODELAY, compteur connexions
- **Fichier source** : `04.1-standalone-asio.md`
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -pthread -DASIO_STANDALONE -o ex04_1_production ex04_1_production.cpp`
- **Sortie attendue** : [+] connexion (actives: 1), réponse, [-] déconnexion (actives: 0)

### ex04_1_async_client.cpp
- **Section** : 22.4.1 — Standalone Asio
- **Description** : Client TCP asynchrone avec coroutines — résolution DNS async, envoi/réception
- **Fichier source** : `04.1-standalone-asio.md`
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -pthread -DASIO_STANDALONE -o ex04_1_async_client ex04_1_async_client.cpp`
- **Sortie attendue** : Connecté, réponse "Hello from async client!\n"

### ex04_1_udp.cpp
- **Section** : 22.4.1 — Standalone Asio
- **Description** : Serveur UDP echo avec coroutines, dual-stack IPv6
- **Fichier source** : `04.1-standalone-asio.md`
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -pthread -DASIO_STANDALONE -o ex04_1_udp ex04_1_udp.cpp`
- **Sortie attendue** : UDP réponse "Hello UDP!"

### ex04_1_timers.cpp
- **Section** : 22.4.1 — Standalone Asio
- **Description** : Timers Asio — style callback (async_wait) et coroutine (periodic_timer)
- **Fichier source** : `04.1-standalone-asio.md`
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -pthread -DASIO_STANDALONE -o ex04_1_timers ex04_1_timers.cpp`
- **Sortie attendue** : 3 ticks (200ms), timer callback (500ms), "Timer terminé"

---

## Section 22.4.2 — Boost.Asio + Beast

### ex04_2_http_server.cpp
- **Section** : 22.4.2 — Boost.Asio — Écosystème complet
- **Description** : Serveur HTTP avec Beast — routage, keep-alive, coroutines
- **Fichier source** : `04.2-boost-asio.md`
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -pthread -o ex04_2_http_server ex04_2_http_server.cpp`
- **Sortie attendue** : GET / → 200 OK, GET /api/status → 200 OK (JSON), GET /nonexistent → 404

### ex04_2_http_client.cpp
- **Section** : 22.4.2 — Boost.Asio — Écosystème complet
- **Description** : Client HTTP async avec Beast — requête GET, affichage headers/body
- **Fichier source** : `04.2-boost-asio.md`
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -pthread -o ex04_2_http_client ex04_2_http_client.cpp`
- **Sortie attendue** : HTTP 200 OK, headers Server/Content-Type/Content-Length, body "Response from test server"

### ex04_2_websocket.cpp
- **Section** : 22.4.2 — Boost.Asio — Écosystème complet
- **Description** : Serveur et client WebSocket avec Beast — handshake, echo, fermeture propre
- **Fichier source** : `04.2-boost-asio.md`
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -pthread -o ex04_2_websocket ex04_2_websocket.cpp`
- **Sortie attendue** : WebSocket connecté, reçu "Hello WebSocket!", déconnecté

### ex04_2_json.cpp
- **Section** : 22.4.2 — Boost.Asio — Écosystème complet
- **Description** : API REST JSON avec Beast + Boost.JSON — GET liste, POST création
- **Fichier source** : `04.2-boost-asio.md`
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -pthread ex04_2_json.cpp -lboost_json -o ex04_2_json`
- **Sortie attendue** : GET → liste Alice/Bob, POST → création Charlie

---

## Section 22.5 — Clients HTTP

### ex05_httplib_server.cpp
- **Section** : 22.5 — Clients HTTP
- **Description** : Serveur HTTP cpp-httplib — routes GET/POST, regex, 404, client intégré
- **Fichier source** : `05-clients-http.md`
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -pthread -o ex05_httplib_server ex05_httplib_server.cpp`
- **Sortie attendue** : GET / → 200, GET /users/42 → 200 "User ID: 42", POST → 201, GET /nonexistent → 404

### ex05_cpr.cpp
- **Section** : 22.5 — Clients HTTP
- **Description** : Client cpr (wrapper curl) — GET/POST/PUT/DELETE, paramètres, timeout, erreurs
- **Fichier source** : `05-clients-http.md`
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -pthread -I/usr/local/include -L/usr/local/lib ex05_cpr.cpp -lcpr -lcurl -lssl -lcrypto -o ex05_cpr`
- **Sortie attendue** : GET 200, POST 200, PUT 200, DELETE 200, erreur connexion refusée, timeout 1s

---

## Section 22.6 — gRPC et Protocol Buffers

> Prérequis : générer le code avec :
> ```bash
> protoc --proto_path=proto --cpp_out=generated --grpc_out=generated \
>   --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) \
>   proto/taskmanager.proto proto/streaming_demo.proto
> ```

### ex06_3_grpc.cpp
- **Section** : 22.6.3 — Génération de code et implémentation
- **Description** : Serveur/client gRPC TaskManager — CreateTask, GetTask, ListTasks avec pagination
- **Fichier source** : `06.3-generation-code.md`
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -Wno-unused-parameter -pthread -I generated ex06_3_grpc.cpp generated/taskmanager.pb.cc generated/taskmanager.grpc.pb.cc $(pkg-config --cflags --libs grpc++ protobuf) -o ex06_3_grpc`
- **Sortie attendue** : Création de 3 tâches, récupération, NOT_FOUND pour task-999, liste de 3 tâches

### ex06_4_streaming.cpp
- **Section** : 22.6.4 — Streaming bidirectionnel
- **Description** : Streaming gRPC — serveur (Subscribe), client (Ingest), bidirectionnel (InteractiveQuery)
- **Fichier source** : `06.4-streaming.md`
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -Wno-unused-parameter -pthread -I generated ex06_4_streaming.cpp generated/streaming_demo.pb.cc generated/streaming_demo.grpc.pb.cc $(pkg-config --cflags --libs grpc++ protobuf) -o ex06_4_streaming`
- **Sortie attendue** : 5 événements cpu_usage, 100 samples ingérés, requête vide → erreur, 2 queries avec résultats
