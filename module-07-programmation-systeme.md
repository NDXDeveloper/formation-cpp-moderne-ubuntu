🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Module 7 — Programmation Système sur Linux

> 🎯 Niveau : Avancé

Ce module descend au niveau du système d'exploitation. Filesystem, signaux POSIX, threads, sockets réseau, IPC — c'est ici que C++ interagit directement avec le noyau Linux via les appels système et les abstractions de la librairie standard. Le chapitre 21 (threads et concurrence) est le plus dense du module et l'un des plus exigeants de la formation : mutex, condition variables, atomiques, memory ordering, `std::jthread` (C++20). Les bugs de concurrence sont les plus difficiles à diagnostiquer car ils ne se reproduisent pas systématiquement.

---

## Objectifs pédagogiques

1. **Manipuler** le système de fichiers via `std::filesystem` (C++17) et les appels système POSIX (`open`, `read`, `write`, `close`), et choisir l'API appropriée selon le contexte.
2. **Implémenter** des handlers de signaux POSIX avec `sigaction` en respectant les contraintes async-signal-safe.
3. **Concevoir** des programmes concurrents avec `std::thread`, `std::jthread` (C++20), mutex, condition variables, et opérations atomiques avec contrôle du memory ordering.
4. **Programmer** des communications réseau TCP/UDP via l'API POSIX (sockets, `epoll`) et via les librairies modernes (Standalone Asio, Boost.Asio, cpr, gRPC).
5. **Implémenter** des mécanismes IPC : `fork`/`exec`, pipes, shared memory (`mmap`), message queues POSIX.
6. **Détecter** les data races et problèmes de concurrence avec ThreadSanitizer (`-fsanitize=thread`).

---

## Prérequis

- **Module 3, chapitre 6** : RAII et destructeurs — les ressources système (file descriptors, sockets, handles mmap) doivent être gérées via des wrappers RAII.
- **Module 4, chapitre 9** : smart pointers — `std::unique_ptr` avec custom deleter est le pattern standard pour encapsuler les ressources système.
- **Module 4, chapitre 11** : lambdas — utilisées comme callbacks dans Asio, comme fonctions thread, et dans les algorithmes parallèles.
- **Module 6** : gestion d'erreurs — les appels système retournent des codes d'erreur (`errno`), les API C++ lèvent des exceptions (`std::system_error`). La stratégie de gestion d'erreurs du Module 6 s'applique directement ici.

---

## Chapitres

### Chapitre 19 — Manipulation du Système de Fichiers

Les deux API pour manipuler le filesystem sur Linux : `std::filesystem` (C++17) portable et haut niveau, et les appels POSIX (`open`, `read`, `write`) pour le contrôle fin. Ce chapitre couvre aussi les permissions et la gestion des erreurs spécifiques au filesystem.

- `std::filesystem` : parcours de répertoires (`directory_iterator`, `recursive_directory_iterator`), manipulation de chemins (`std::filesystem::path`), opérations sur fichiers et répertoires (`copy`, `rename`, `remove`, `create_directories`).
- Appels système POSIX : `open` avec flags (`O_RDONLY`, `O_CREAT`, `O_TRUNC`), `read`/`write` avec gestion des lectures/écritures partielles, `close` et gestion des file descriptors.
- Comparaison API C++ vs API système : portabilité vs contrôle, gestion d'erreurs (exceptions vs `errno`), performance.
- Permissions, droits (`chmod`, `chown`) et gestion des erreurs (`std::filesystem::filesystem_error`, `errno`).

### Chapitre 20 — Signaux POSIX

Les signaux sont le mécanisme d'interruption asynchrone d'Unix. Ce chapitre est court mais critique — un signal handler incorrect peut corrompre silencieusement l'état du programme.

- Signaux Unix : `SIGINT` (Ctrl+C), `SIGTERM` (terminaison propre), `SIGSEGV` (segfault), `SIGPIPE` (écriture sur socket fermé) — comportements par défaut et quand les intercepter.
- Installation de handlers : `signal()` (simplifié, non recommandé) vs `sigaction()` (complet, portable) — masques de signaux, flags (`SA_RESTART`, `SA_SIGINFO`).
- Signaux et threads : dans un programme multi-thread, le signal est délivré à un thread arbitraire. Stratégies : bloquer les signaux dans tous les threads sauf un thread dédié, ou utiliser `signalfd` pour transformer les signaux en événements lisibles.

### Chapitre 21 — Threads et Programmation Concurrente

Le chapitre central du module. Couvre la concurrence de bout en bout : création de threads, synchronisation, communication, opérations atomiques, et les abstractions modernes C++20.

- `std::thread` : création, `join`, `detach`, passage de paramètres (attention aux copies implicites — passer par `std::ref` ou `std::move`).
- Synchronisation : `std::mutex`, `std::lock_guard` (RAII lock basique), `std::unique_lock` (lock transférable, utilisable avec `condition_variable`), `std::scoped_lock` (C++17 — lock multiple mutex sans deadlock).
- `std::condition_variable` : pattern producer/consumer, spurious wakeups — toujours utiliser un prédicat dans `wait()`.
- `std::atomic` : opérations atomiques sur des types primitifs, memory ordering (`memory_order_relaxed`, `acquire`, `release`, `seq_cst`) — impacte la visibilité des écritures entre threads.
- `std::async` et `std::future` : lancement asynchrone simplifié, limitations (pas de contrôle sur le thread pool, `std::future` bloque dans le destructeur).
- Thread-safety et data races : définition formelle, conditions nécessaires (accès concurrent, au moins une écriture, pas de synchronisation), détection avec ThreadSanitizer.
- `std::jthread` (C++20) : thread auto-joinable avec support de `std::stop_token` pour l'annulation coopérative.
- Algorithmes parallèles en contexte concurrent : mise en pratique des `std::execution` policies sur des données partagées.

> 📎 Pour la couverture des politiques d'exécution parallèle, voir section 15.7.

### Chapitre 22 — Networking et Communication

Programmation réseau de bas niveau (sockets POSIX) jusqu'aux librairies modernes (Asio, gRPC). Ce chapitre couvre TCP, UDP, le multiplexage I/O, et les protocoles applicatifs.

- Sockets TCP/UDP : création (`socket()`), opérations serveur (`bind`, `listen`, `accept`), opérations client (`connect`), envoi/réception (`send`, `recv`, `sendto`, `recvfrom`).
- Client/Serveur basique : implémentation TCP complète, gestion des connexions multiples.
- Multiplexage I/O : `select` (portable, limité à 1024 FDs), `poll` (sans cette limite), `epoll` (Linux, scalable à des milliers de connexions) — edge-triggered vs level-triggered.
- Librairies modernes : Standalone Asio (networking sans Boost, asynchrone avec callbacks/coroutines), Boost.Asio (écosystème complet), critères de choix entre les deux.
- Clients HTTP : cpr (wrapper C++ autour de libcurl, API simple), cpp-httplib (header-only, client et serveur).
- gRPC et Protocol Buffers : définition de services `.proto`, génération de code C++, implémentation client/serveur, streaming bidirectionnel.

### Chapitre 23 — Processus et IPC (Inter-Process Communication)

Création et gestion de processus fils, et les quatre mécanismes principaux de communication inter-processus sur Linux.

- `fork` et `exec` : duplication de processus, remplacement de l'image mémoire, gestion des processus fils avec `waitpid`.
- Pipes : communication unidirectionnelle entre processus parent et fils, `pipe()` et `pipe2()`.
- Shared memory et `mmap` : partage de mémoire entre processus, `mmap` avec `MAP_SHARED`, synchronisation nécessaire (semaphores POSIX ou mutex partagé).
- Message queues POSIX : `mq_open`, `mq_send`, `mq_receive` — communication structurée avec priorités.

---

## Points de vigilance

- **TOCTOU (Time-of-Check Time-of-Use) sur les fichiers.** Vérifier l'existence d'un fichier avec `std::filesystem::exists()` puis l'ouvrir dans une opération séparée crée une fenêtre pendant laquelle le fichier peut disparaître ou être modifié. Un autre processus ou thread peut intervenir entre les deux opérations. Solution : tenter l'opération directement (`open` avec les bons flags, ou `std::ifstream`) et gérer l'erreur si elle survient — ne pas vérifier avant d'agir.

- **Signal handlers limités aux fonctions async-signal-safe.** Un signal handler interrompt le programme à n'importe quel point d'exécution. Appeler `malloc`, `printf`, `std::cout` ou toute fonction non async-signal-safe dans un handler peut corrompre l'état interne du programme (structures de données du heap, buffers de la libc). La liste des fonctions async-signal-safe est définie par POSIX (`write`, `_exit`, `signal`). En pratique, le handler doit se limiter à écrire dans un `volatile sig_atomic_t` ou un `std::atomic<bool>`, et le traitement réel se fait dans la boucle principale.

- **Data race non détectée à la compilation.** Le compilateur C++ ne signale pas les data races — deux threads accédant à la même variable sans synchronisation compilent sans erreur ni warning. Le bug ne se manifeste que sporadiquement à l'exécution, souvent sous charge. ThreadSanitizer (`-fsanitize=thread`) détecte les data races à l'exécution mais uniquement sur les chemins de code effectivement empruntés. Compilez et testez systématiquement avec TSan pendant le développement.

- **Oublier `SO_REUSEADDR` sur les sockets serveur.** Sans `setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, ...)`, un serveur qui redémarre après un crash ou un arrêt propre se voit refuser le `bind` pendant 60 secondes (timeout `TIME_WAIT` du TCP). C'est un classique qui bloque les cycles de développement et les redémarrages en production. Activez `SO_REUSEADDR` systématiquement sur les sockets serveur.

- **Zombie processes sans `waitpid`.** Un processus fils terminé dont le parent n'appelle pas `waitpid` reste en état zombie — il occupe une entrée dans la table des processus du noyau. Accumuler des zombies peut épuiser la table. Deux solutions : appeler `waitpid` dans le parent (bloquant ou avec `WNOHANG`), ou ignorer `SIGCHLD` avec `signal(SIGCHLD, SIG_IGN)` pour que le noyau nettoie automatiquement.

---

## Compétences acquises

À l'issue de ce module, vous savez :
- Manipuler le filesystem via `std::filesystem` et les appels POSIX, en choisissant l'API selon les contraintes.
- Installer des signal handlers corrects avec `sigaction` en respectant les contraintes async-signal-safe.
- Écrire des programmes concurrents avec mutex, condition variables, atomiques, et `std::jthread`, sans data races.
- Programmer des serveurs réseau TCP avec `epoll` ou Asio, et des services gRPC avec streaming.
- Créer des processus fils avec `fork`/`exec` et communiquer via pipes, shared memory ou message queues.
- Diagnostiquer les problèmes de concurrence avec ThreadSanitizer.

---


⏭️ [Manipulation du Système de Fichiers](/19-systeme-fichiers/README.md)
