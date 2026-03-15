# Chapitre 21 — Threads et Concurrence : Exemples

## Compilation

Tous les exemples utilisent C++23 avec g++-15 :

```bash
g++-15 -std=c++23 -Wall -Wextra -pthread -o exemple fichier.cpp
```

Pour les exemples avec algorithmes parallèles (ex38, ex39), ajouter `-ltbb` si TBB est installé :

```bash
g++-15 -std=c++23 -Wall -Wextra -pthread -ltbb -o exemple fichier.cpp
```

---

## Liste des exemples

### README.md — Introduction

| Fichier | Section | Description | Sortie attendue |
|---------|---------|-------------|-----------------|
| `ex00_data_race.cpp` | 21 | Data race sur un compteur sans synchronisation | `counter` variable (≠ 2000000), illustre le problème |

### 01-std-thread.md — Création et cycle de vie des threads

| Fichier | Section | Description | Sortie attendue |
|---------|---------|-------------|-----------------|
| `ex01_hello_thread.cpp` | 21.1 | Premier thread avec join() | `Hello depuis le thread !` puis `Thread terminé` |
| `ex02_callables.cpp` | 21.1 | Callables : fonction, lambda, foncteur, méthode membre | Messages des 4 types de callables |
| `ex03_passage_arguments.cpp` | 21.1 | Passage d'arguments : copie, std::ref, std::move | Démonstration des 3 modes de passage |
| `ex04_join_lifecycle.cpp` | 21.1 | join() bloque le thread appelant | Travail 1s dans un thread, join() bloque le main |
| `ex05_move_semantics.cpp` | 21.1 | Sémantique de déplacement et vecteur de threads | Factory + vector de 5 threads |
| `ex06_thread_id.cpp` | 21.1 | Thread IDs et hardware_concurrency | IDs des threads et nombre de cœurs |
| `ex07_exception_propagation.cpp` | 21.1 | Propagation d'exception via exception_ptr | `Exception du thread : Échec du traitement` |

### 02.1-mutex.md — std::mutex et variantes

| Fichier | Section | Description | Sortie attendue |
|---------|---------|-------------|-----------------|
| `ex08_mutex_counter.cpp` | 21.2.1 | Compteur protégé par mutex | `counter = 2000000` (toujours) |
| `ex09_try_lock.cpp` | 21.2.1 | Tentative non-bloquante try_lock() | `Mise à jour effectuée`, `shared_data = 42` |
| `ex10_recursive_mutex.cpp` | 21.2.1 | Mutex récursif | Profondeur 3, 2, 1, 0 |
| `ex11_shared_mutex_cache.cpp` | 21.2.1 | Cache thread-safe avec shared_mutex | `hello = world`, `foo = bar`, `missing = ''` |
| `ex12_timed_mutex.cpp` | 21.2.1 | Acquisition avec timeout | `Verrou acquis` (2 fois) |
| `ex13_synchronized_pattern.cpp` | 21.2.1 | Template Synchronized<T> avec with_lock() | Taille=1, contenu: 42, 100, 200 |

### 02.2-lock-guard.md — std::lock_guard

| Fichier | Section | Description | Sortie attendue |
|---------|---------|-------------|-----------------|
| `ex14_lock_guard_queue.cpp` | 21.2.2 | File d'attente thread-safe avec lock_guard | `Taille : 200` (toujours) |
| `ex15_adopt_lock.cpp` | 21.2.2 | Verrouillage multi-mutex avec adopt_lock | Messages de réussite |

### 02.3-unique-lock.md — std::unique_lock

| Fichier | Section | Description | Sortie attendue |
|---------|---------|-------------|-----------------|
| `ex16_unique_lock_transfer.cpp` | 21.2.3 | Transfert bancaire avec defer_lock | Soldes cohérents (total conservé) |
| `ex17_unique_lock_condvar.cpp` | 21.2.3 | Producteur/consommateur avec condition_variable | Tâches 0-9 traitées, puis `Toutes les tâches traitées` |

### 02.4-scoped-lock.md — std::scoped_lock

| Fichier | Section | Description | Sortie attendue |
|---------|---------|-------------|-----------------|
| `ex18_scoped_lock_transfer.cpp` | 21.2.4 | Transferts croisés 3 comptes sans deadlock | `Total = 3000.00` (toujours) |

### 03-condition-variable.md — Variables de condition et primitives C++20

| Fichier | Section | Description | Sortie attendue |
|---------|---------|-------------|-----------------|
| `ex19_blocking_queue.cpp` | 21.3 | BlockingQueue<T> avec 1 producteur, 3 consommateurs | 20 items produits et traités, 3 consommateurs terminés |
| `ex20_wait_for_timeout.cpp` | 21.3 | wait_for() avec timeout et signal | Test 1: `Timeout après 500ms`, Test 2: `Événement reçu à temps` |
| `ex21_latch.cpp` | 21.3 | std::latch (C++20) — synchronisation ponctuelle | 4 workers initialisés, puis `Tous les workers sont prêts` |
| `ex22_barrier.cpp` | 21.3 | std::barrier (C++20) — synchronisation cyclique | 3 phases, `--- Phase terminée ---` entre chaque |
| `ex23_semaphore.cpp` | 21.3 | std::counting_semaphore — limitation de concurrence | 8 workers, max 3 connectés simultanément |

### 04-atomiques.md — std::atomic et memory ordering

| Fichier | Section | Description | Sortie attendue |
|---------|---------|-------------|-----------------|
| `ex24_atomic_counter.cpp` | 21.4 | Compteur atomique (2 threads) | `counter = 2000000` (toujours) |
| `ex25_cas_loop.cpp` | 21.4 | Boucle CAS — multiplication atomique | `value = 1024 (attendu : 1024)` |
| `ex26_spinlock.cpp` | 21.4 | SpinLock TTAS avec atomic<bool> | `shared_counter = 2000000` |

### 05-async-future.md — Programmation asynchrone

| Fichier | Section | Description | Sortie attendue |
|---------|---------|-------------|-----------------|
| `ex27_async_basic.cpp` | 21.5 | std::async avec launch::async | `Résultat : 1764` |
| `ex28_future_exception.cpp` | 21.5 | Propagation d'exception via future | `Exception capturée : Erreur dans le calcul` |
| `ex29_promise.cpp` | 21.5 | std::promise — transmission de résultat | `Résultat : 42` |
| `ex30_shared_future.cpp` | 21.5 | std::shared_future — résultat partagé | 5 consumers affichant `42` |
| `ex31_packaged_task.cpp` | 21.5 | std::packaged_task dans un thread | `Résultat : 30` |
| `ex32_task_queue.cpp` | 21.5 | TaskQueue avec submit() et worker_loop | `f1 = 42`, `f2 = 100` |

### 06-thread-safety.md — Patterns de conception thread-safe

| Fichier | Section | Description | Sortie attendue |
|---------|---------|-------------|-----------------|
| `ex33_monitor_pattern.cpp` | 21.6 | Template Monitor<T> avec apply/wait_and_apply | 10 items produits et traités |
| `ex34_rcu_pattern.cpp` | 21.6 | Pattern RCU — Read-Copy-Update | Lecteurs concurrents, puis mise à jour visible |

### 07-jthread.md — std::jthread (C++20)

| Fichier | Section | Description | Sortie attendue |
|---------|---------|-------------|-----------------|
| `ex35_jthread_basic.cpp` | 21.7 | jthread avec stop_token et join automatique | Worker travaille ~350ms puis arrêt propre |
| `ex36_stop_callback.cpp` | 21.7 | std::stop_callback — callback à l'arrêt | Callback exécuté, worker terminé |
| `ex37_worker_pool.cpp` | 21.7 | WorkerPool avec jthread et condition_variable_any | 20 tâches exécutées, pool arrêté proprement |

### 08-algorithmes-paralleles.md — Algorithmes parallèles

| Fichier | Section | Description | Sortie attendue |
|---------|---------|-------------|-----------------|
| `ex38_transform_reduce.cpp` | 21.8 | transform_reduce — masse salariale | `Masse salariale active : 213000.00` |
| `ex39_parallel_sort_benchmark.cpp` | 21.8 | Benchmark sort séquentiel vs parallèle | Temps + speedup (~1x sans TBB) |

---

## Notes

- **Compilateur** : g++-15 (version 15.2.0) avec `-std=c++23`
- **Header `<print>`** : nécessite g++-15 pour `std::println`
- **TBB** : les exemples 38-39 utilisent `std::execution::par` qui nécessite `libtbb-dev` pour une vraie parallélisation. Sans TBB, ils compilent et fonctionnent mais s'exécutent séquentiellement
- **Sorties non-déterministes** : les exemples multi-threads (surtout ex00, ex19, ex22, ex23, ex37) peuvent produire des sorties dans un ordre variable d'une exécution à l'autre
