🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Partie III — Programmation Système Linux

Cette partie descend au niveau du système d'exploitation. Après avoir acquis le langage et ses idiomes modernes, il s'agit maintenant de l'utiliser là où C++ n'a pas d'équivalent dans les langages de haut niveau : accès direct au filesystem, gestion des signaux POSIX, programmation concurrente avec contrôle fin du memory ordering, sockets réseau, IPC, et sérialisation performante. C'est ici que vous commencez à écrire des programmes qui interagissent avec le noyau Linux, pas seulement avec la librairie standard.

---

## Ce que vous allez maîtriser

- Vous serez capable de manipuler le système de fichiers via `std::filesystem` (C++17) et les appels système POSIX (`open`, `read`, `write`), et de choisir l'API appropriée selon le contexte.
- Vous serez capable d'installer et de gérer des handlers de signaux POSIX (`sigaction`) en tenant compte des contraintes spécifiques aux environnements multi-threadés.
- Vous serez capable de concevoir des programmes concurrents avec `std::thread`, `std::jthread` (C++20), mutex, variables de condition, et opérations atomiques avec contrôle du memory ordering.
- Vous serez capable d'identifier et de prévenir les data races, deadlocks et problèmes de thread-safety à l'aide des outils appropriés (ThreadSanitizer, `std::scoped_lock`).
- Vous serez capable de programmer des communications réseau TCP/UDP via l'API POSIX (sockets, `epoll`, `io_uring` avec liburing), via les librairies modernes (Standalone Asio, Boost.Asio) et via les clients HTTP (cpr, cpp-httplib).
- Vous serez capable d'implémenter des services gRPC avec Protocol Buffers, y compris le streaming bidirectionnel.
- Vous comprendrez les compromis entre les modèles readiness (`epoll`) et completion (`io_uring`), et saurez choisir le mécanisme adapté à votre cas d'usage.
- Vous serez capable de gérer des processus fils (`fork`/`exec`) et de mettre en place des mécanismes IPC : pipes, shared memory (`mmap`), message queues POSIX.
- Vous serez capable de parser et d'écrire des fichiers de configuration dans les formats courants : JSON (nlohmann/json), YAML (yaml-cpp), TOML (toml++), XML (pugixml).
- Vous maîtriserez les expressions régulières en C++ avec le bon outil selon le contexte : `std::regex` (standard), CTRE (compile-time, C++20), RE2 (temps linéaire garanti, Google) et PCRE2 (Perl-compatible, JIT).
- Vous serez capable de choisir un format de sérialisation binaire (Protobuf, FlatBuffers, Cap'n Proto, MessagePack) en fonction des contraintes de performance, de taille, de zero-copy et de compatibilité.

---

## Prérequis

- **Partie I — Fondations** : gestion mémoire (chapitre 5), RAII et Rule of Five (chapitre 6) — indispensables pour comprendre la gestion des ressources système (file descriptors, sockets, shared memory).
- **Partie II — C++ Moderne** : smart pointers (chapitre 9), move semantics (chapitre 10), lambdas (chapitre 11), gestion d'erreurs (chapitres 17-18) — les APIs système modernes en C++ s'appuient sur ces patterns.

---

## Modules de cette partie

| # | Titre | Niveau | Chapitres | Lien |
|---|-------|--------|-----------|------|
| Module 7 | Programmation Système sur Linux | Avancé | 19, 20, 21, 22, 23 | [module-07-programmation-systeme.md](/module-07-programmation-systeme.md) |
| Module 8 | Parsing et Formats de Données | Avancé | 24, 25 | [module-08-parsing-formats.md](/module-08-parsing-formats.md) |

---

## Fil conducteur

Le Module 7 suit la progression naturelle des interactions avec le système : on commence par le filesystem (chapitre 19), la couche la plus accessible, puis on descend vers les signaux (chapitre 20) qui imposent des contraintes strictes sur ce qu'un handler peut faire. Les threads (chapitre 21) constituent le cœur du module — c'est le chapitre le plus dense, couvrant `std::thread`, `std::jthread`, synchronisation, atomiques et algorithmes parallèles. Le networking (chapitre 22) étend la concurrence au réseau, des sockets POSIX bruts jusqu'à `io_uring`, Asio, les clients HTTP et gRPC. Les IPC (chapitre 23) complètent le tableau en couvrant les mécanismes de communication entre processus sur la même machine. Le Module 8 traite un problème transversal à tous ces contextes : comment structurer les données échangées. Les formats texte (JSON, YAML, TOML, XML) et les expressions régulières (CTRE, RE2, PCRE2) servent la configuration, le parsing et les APIs REST, tandis que les formats binaires (Protobuf, FlatBuffers, Cap'n Proto, MessagePack) répondent aux contraintes de performance des communications réseau et IPC traitées dans le module précédent. À la sortie de cette partie, vous savez écrire des programmes C++ qui exploitent les capacités du système Linux et communiquent efficacement, localement ou sur le réseau.

---


⏭️ [Module 7 : Programmation Système sur Linux](/module-07-programmation-systeme.md)
