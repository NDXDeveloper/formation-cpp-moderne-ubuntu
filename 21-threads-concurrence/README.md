🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 21. Threads et Programmation Concurrente ⭐

## Objectif du chapitre

La programmation concurrente est au cœur des systèmes modernes. Avec des processeurs qui multiplient les cœurs plutôt que d'augmenter leur fréquence, un programme qui n'exploite qu'un seul thread sous-utilise massivement le matériel disponible. Ce chapitre vous donne les outils et les réflexes pour écrire du code concurrent correct, performant et maintenable en C++ moderne sur Linux.

Vous partirez de la création de threads avec `std::thread`, puis vous apprendrez à protéger les données partagées grâce aux primitives de synchronisation (`std::mutex`, `std::lock_guard`, `std::scoped_lock`), à coordonner des threads avec les variables de condition, et à exploiter les opérations atomiques pour des scénarios à haute performance. Vous découvrirez également `std::async` et `std::future` pour la programmation asynchrone, avant d'aborder les évolutions apportées par C++20 avec `std::jthread` et les algorithmes parallèles de la STL.

---

## Pourquoi la concurrence est incontournable

### L'évolution du matériel dicte le logiciel

Depuis le milieu des années 2000, la loi de Moore ne se traduit plus en gains de fréquence mais en multiplication des cœurs. Un serveur Ubuntu typique en 2026 dispose de 8 à 128 cœurs physiques (voire plus sur les instances cloud). Un programme mono-thread n'en utilise qu'un seul — il laisse plus de 90 % de la puissance de calcul inactive.

La programmation concurrente n'est donc plus un luxe réservé aux spécialistes. C'est une compétence fondamentale pour quiconque développe des services réseau, des outils CLI performants, des pipelines de données, ou tout logiciel destiné à un environnement de production.

### Ce que C++ apporte à la table

Contrairement à de nombreux langages qui abstraient la concurrence derrière un runtime (goroutines en Go, green threads en Rust async, GIL en Python), C++ offre un contrôle direct sur les threads système (pthreads sous Linux). Cette proximité avec le système d'exploitation est à la fois sa force et sa responsabilité :

- **Contrôle fin** : vous décidez précisément ce qui tourne sur quel thread, comment les données sont partagées, et quel modèle mémoire s'applique.
- **Performance prédictible** : pas de garbage collector, pas de runtime concurrent caché — le comportement est celui que vous programmez.
- **Responsabilité totale** : le compilateur ne vous empêche pas d'écrire une data race. C'est à vous de garantir la correction.

Depuis C++11, la bibliothèque standard fournit un ensemble complet de primitives portables dans le header `<thread>`, enrichi à chaque révision du standard :

| Standard | Apport principal |
|----------|-----------------|
| C++11 | `std::thread`, `std::mutex`, `std::atomic`, `std::future` |
| C++14 | `std::shared_timed_mutex` (reader-writer lock) |
| C++17 | `std::scoped_lock`, `std::shared_mutex`, algorithmes parallèles |
| C++20 | `std::jthread`, `std::stop_token`, `std::counting_semaphore`, `std::latch`, `std::barrier` |

---

## Concurrence vs Parallélisme

Ces deux termes sont souvent confondus, mais ils désignent des réalités distinctes :

- **Concurrence** (*concurrency*) : structurer un programme pour gérer plusieurs tâches qui progressent de manière entrelacée. Deux tâches concurrentes ne s'exécutent pas nécessairement au même instant — elles peuvent alterner sur un seul cœur. La concurrence est une question de **design**.

- **Parallélisme** (*parallelism*) : exécuter effectivement plusieurs calculs au même instant physique, sur des cœurs différents. Le parallélisme est une question d'**exécution**.

Un serveur web concurrent gère des milliers de connexions simultanées, même sur un seul cœur (via du multiplexage I/O). Un tri parallèle partitionne un tableau sur 8 cœurs pour diviser le temps de calcul par un facteur proche de 8. Ce chapitre couvre les deux dimensions, car C++ les expose toutes les deux.

---

## Les dangers de la programmation concurrente

Avant de plonger dans les API, il est essentiel de comprendre pourquoi ce domaine a la réputation d'être difficile. Les bugs concurrents sont parmi les plus vicieux en informatique : ils sont non-déterministes, difficiles à reproduire, et souvent invisibles lors des tests locaux pour ne se manifester qu'en production sous charge.

### Data race

Une data race survient lorsque deux threads accèdent à la même zone mémoire, qu'au moins un des accès est une écriture, et qu'il n'y a aucune synchronisation entre eux. En C++, une data race est un **comportement indéfini** — pas une simple erreur logique, mais une violation du contrat avec le langage qui rend le programme entier invalide.

```cpp
// ⚠️ DATA RACE — comportement indéfini
int counter = 0;

void increment() {
    for (int i = 0; i < 1'000'000; ++i) {
        ++counter;  // lecture-modification-écriture non atomique
    }
}
// Si deux threads exécutent increment(), le résultat final
// n'est PAS garanti d'être 2'000'000.
```

### Deadlock

Un deadlock se produit lorsque deux threads (ou plus) s'attendent mutuellement, chacun détenant une ressource dont l'autre a besoin. Le programme se fige indéfiniment.

### Race condition logique

Même en l'absence de data race (grâce à des mutex correctement placés), la logique du programme peut être incorrecte si l'ordre des opérations n'est pas celui attendu. La synchronisation garantit la cohérence mémoire, pas la logique métier.

### L'outil de détection incontournable

`ThreadSanitizer` (`-fsanitize=thread`) est votre meilleur allié. Compilez systématiquement vos tests avec cette option activée. Il détecte les data races à l'exécution avec un très faible taux de faux positifs.

```bash
g++ -std=c++23 -fsanitize=thread -g -O1 main.cpp -o main_tsan
./main_tsan
```

> 📎 *Pour une couverture détaillée de ThreadSanitizer, voir la **section 29.4.3**.*

---

## Modèle de threading Linux

Sous Linux, `std::thread` est implémenté au-dessus de pthreads (POSIX Threads), qui s'appuie lui-même sur l'appel système `clone()`. Chaque `std::thread` correspond à un thread noyau réel — il n'y a pas de couche intermédiaire de type M:N scheduling.

Cela signifie que :

- Chaque thread a sa propre pile (stack), typiquement de 8 Mo par défaut sur Ubuntu.
- Le noyau Linux ordonnance les threads via son scheduler (EEVDF depuis Linux 6.6, CFS sur les noyaux plus anciens).
- La création d'un thread a un coût non négligeable (allocation de pile, appel système). On ne crée pas un thread par requête dans un serveur à haute charge — on utilise un pool de threads.

Cette réalité système influence directement les choix de conception que vous ferez tout au long de ce chapitre.

---

## Plan du chapitre

Ce chapitre est structuré en une progression logique, des briques de base vers les abstractions modernes :

- **21.1** — `std::thread` : création, cycle de vie, `join` et `detach`.
- **21.2** — Synchronisation : `std::mutex`, `std::lock_guard`, `std::unique_lock`, `std::scoped_lock`.
- **21.3** — Variables de condition : coordination producteur/consommateur avec `std::condition_variable`.
- **21.4** — Atomiques : `std::atomic`, opérations lock-free et memory ordering.
- **21.5** — `std::async` et `std::future` : lancer des tâches asynchrones et récupérer leurs résultats.
- **21.6** — Thread-safety et data races : principes, patterns et pièges à éviter.
- **21.7** — `std::jthread` (C++20) : threads à arrêt coopératif et `std::stop_token`.
- **21.8** — Algorithmes parallèles : exploiter `std::execution::par` pour paralléliser les algorithmes STL.

---

## Prérequis

Avant d'aborder ce chapitre, assurez-vous d'être à l'aise avec :

- **La gestion de la mémoire** (chapitre 5) : comprendre stack vs heap, durée de vie des objets.
- **Les smart pointers** (chapitre 9) : `std::unique_ptr` et `std::shared_ptr`, essentiels pour le partage de données entre threads.
- **La sémantique de mouvement** (chapitre 10) : `std::move` est omniprésent dans le passage de données aux threads.
- **Les lambdas** (chapitre 11) : la manière idiomatique de définir le travail d'un thread.
- **Les références et le passage de paramètres** (section 4.3) : comprendre la différence entre copie et référence est critique quand un thread capture des variables.

---

## Compilation et flags

Tous les exemples de ce chapitre utilisent C++23 (pour `std::println`) et nécessitent le linkage avec la bibliothèque de threads :

```bash
# Compilation standard
g++ -std=c++23 -Wall -Wextra -pthread main.cpp -o main

# Avec ThreadSanitizer (recommandé pendant le développement)
g++ -std=c++23 -Wall -Wextra -pthread -fsanitize=thread -g -O1 main.cpp -o main_tsan

# Avec Clang
clang++ -std=c++23 -Wall -Wextra -pthread main.cpp -o main
```

Le flag `-pthread` est indispensable sous Linux. Il active à la fois les définitions du préprocesseur et le linkage avec libpthread. Sans lui, le code compile mais les threads ne fonctionneront pas correctement.

---

## Headers utilisés dans ce chapitre

```cpp
#include <thread>              // std::thread, std::jthread
#include <mutex>               // std::mutex, std::lock_guard, std::scoped_lock, std::unique_lock
#include <condition_variable>  // std::condition_variable
#include <atomic>              // std::atomic
#include <future>              // std::async, std::future, std::promise
#include <stop_token>          // std::stop_token, std::stop_source (C++20)
#include <semaphore>           // std::counting_semaphore, std::binary_semaphore (C++20)
#include <latch>               // std::latch (C++20)
#include <barrier>             // std::barrier (C++20)
```

⏭️ [std::thread : Création et gestion de threads](/21-threads-concurrence/01-std-thread.md)
