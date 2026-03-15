🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 21.7 std::jthread (C++20) : Threads auto-stoppables ⭐

## Le thread que std::thread aurait dû être

`std::jthread` (*joining thread*), introduit par C++20, corrige les deux défauts majeurs de `std::thread` :

1. **Join automatique dans le destructeur** : plus de crash par `std::terminate()` si vous oubliez d'appeler `join()`.
2. **Arrêt coopératif intégré** : un mécanisme standardisé pour demander à un thread de s'arrêter proprement, via `std::stop_token`.

```cpp
#include <thread>
#include <print>

void worker(std::stop_token stoken) {
    while (!stoken.stop_requested()) {
        do_work();
    }
    std::println("Arrêt propre du worker");
}

int main() {
    std::jthread t(worker);  // Le stop_token est passé automatiquement

    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Pas besoin d'appeler join() — le destructeur s'en charge.
    // Pas besoin de flag atomique — stop_token est intégré.
}
// ~jthread() appelle request_stop() puis join()
```

Comparez avec l'équivalent `std::thread` :

```cpp
// Équivalent pré-C++20 — beaucoup plus verbeux
std::atomic<bool> running{true};

void worker_old() {
    while (running.load()) {
        do_work();
    }
}

int main() {
    std::thread t(worker_old);
    std::this_thread::sleep_for(std::chrono::seconds(5));
    running.store(false);  // Signal d'arrêt manuel
    t.join();              // Join obligatoire
}
```

`std::jthread` rend ce pattern à la fois plus court, plus sûr et plus composable.

---

## Join automatique

### Le problème résolu

Rappel de la section 21.1 : détruire un `std::thread` joinable appelle `std::terminate()`. C'est un piège en cas d'exception, de retour anticipé, ou simplement d'oubli :

```cpp
void dangerous() {
    std::thread t([] { /* travail */ });
    might_throw();  // Si exception → t est détruit joinable → crash
    t.join();
}
```

La solution classique était le `ThreadGuard` RAII (section 21.1) ou un `try`/`catch` englobant. `std::jthread` rend tout cela inutile :

```cpp
void safe() {
    std::jthread t([] { /* travail */ });
    might_throw();  // Exception → destructeur de t → join automatique
    // Même sans exception, pas besoin de join explicite
}
```

### Comportement du destructeur

Le destructeur de `std::jthread` effectue deux opérations dans cet ordre :

1. **`request_stop()`** : demande au thread de s'arrêter (si le thread utilise un `stop_token`).
2. **`join()`** : attend que le thread se termine.

```cpp
{
    std::jthread t([] {
        std::this_thread::sleep_for(std::chrono::seconds(10));
    });
    // À la sortie du bloc :
    // 1. t.request_stop() — signal d'arrêt (ignoré ici, le lambda ne vérifie pas)
    // 2. t.join() — attend les 10 secondes
}
```

Si le thread ne vérifie pas son `stop_token`, `request_stop()` n'a aucun effet et le destructeur attend simplement la fin naturelle du thread. Le join automatique garantit qu'aucun thread n'est orphelin.

### Quand join() explicite reste utile

Le join automatique se produit à la **destruction** de l'objet `jthread`. Si vous avez besoin du résultat du thread ou de la garantie qu'il a terminé **avant** d'autres opérations dans le même scope, un `join()` explicite reste pertinent :

```cpp
void explicit_join_needed() {
    std::jthread t(populate_database);
    t.join();  // On veut que la DB soit remplie AVANT la suite

    query_database();  // Garanti après la fin du thread
}
```

---

## Le mécanisme d'arrêt coopératif

### Les trois composants

Le système d'arrêt coopératif de C++20 repose sur trois types complémentaires, définis dans `<stop_token>` :

| Type | Rôle |
|------|------|
| `std::stop_source` | Émet la demande d'arrêt (`request_stop()`) |
| `std::stop_token` | Observe la demande d'arrêt (`stop_requested()`) |
| `std::stop_callback` | Exécute un callback quand l'arrêt est demandé |

`std::jthread` possède en interne un `stop_source`. Il en dérive automatiquement un `stop_token` qu'il passe au callable si celui-ci accepte un `std::stop_token` comme premier paramètre.

### Passage automatique du stop_token

`std::jthread` détecte automatiquement si le callable accepte un `std::stop_token` en premier argument :

```cpp
// Le jthread passe le stop_token automatiquement
void worker_with_stop(std::stop_token stoken) {
    while (!stoken.stop_requested()) {
        // travail...
    }
}

// Le jthread ne passe PAS de stop_token (le callable n'en attend pas)
void worker_without_stop() {
    // travail...
}

int main() {
    std::jthread t1(worker_with_stop);     // stop_token injecté automatiquement
    std::jthread t2(worker_without_stop);  // Pas de stop_token, pas de problème

    // Les deux threads ont le join automatique.
    // Seul t1 réagira à request_stop().
}
```

Le `stop_token` est toujours le **premier** paramètre. Les arguments supplémentaires suivent :

```cpp
void worker(std::stop_token stoken, int id, const std::string& name) {
    while (!stoken.stop_requested()) {
        std::println("[{}] {} travaille...", id, name);
        std::this_thread::sleep_for(100ms);
    }
}

std::jthread t(worker, 1, "Alice");
// Equivalent à : worker(t.get_stop_token(), 1, "Alice")
```

### Demander l'arrêt

L'arrêt peut être demandé de trois façons :

```cpp
std::jthread t(worker_with_stop);

// 1. Explicitement via l'objet jthread
t.request_stop();

// 2. Implicitement par le destructeur
// ~jthread() appelle request_stop() puis join()

// 3. Via le stop_source directement
std::stop_source source = t.get_stop_source();  
source.request_stop();  
```

`request_stop()` est **thread-safe** et **idempotent** : l'appeler plusieurs fois n'a aucun effet après le premier appel. Il retourne `true` si c'est cet appel qui a déclenché l'arrêt, `false` si l'arrêt avait déjà été demandé.

---

## std::stop_token en détail

### Interface

```cpp
std::stop_token stoken = t.get_stop_token();

stoken.stop_requested();   // true si l'arrêt a été demandé  
stoken.stop_possible();    // true si un stop_source existe encore  
                           // (false si le jthread a été détaché ou déplacé
                           //  sans que le stop_source soit accessible)
```

### Utilisation dans les boucles

Le pattern le plus courant est la vérification dans la condition de boucle :

```cpp
void polling_worker(std::stop_token stoken) {
    while (!stoken.stop_requested()) {
        auto data = try_fetch_data();
        if (data) {
            process(*data);
        }
        std::this_thread::sleep_for(50ms);
    }
    cleanup();
}
```

### Intégration avec std::condition_variable_any

C++20 ajoute une surcharge de `wait()` sur `std::condition_variable_any` qui accepte un `stop_token`. Le thread se réveille automatiquement quand l'arrêt est demandé, sans que le producteur ait besoin de notifier la condition variable :

```cpp
#include <condition_variable>
#include <mutex>
#include <queue>
#include <print>

std::mutex mtx;  
std::condition_variable_any cv;  
std::queue<int> tasks;  

void consumer(std::stop_token stoken) {
    while (true) {
        std::unique_lock lock(mtx);

        // wait() se réveille si :
        //   - le prédicat est vrai (tâche disponible)
        //   - OU l'arrêt est demandé via le stop_token
        bool stopped = !cv.wait(lock, stoken, [] {
            return !tasks.empty();
        });
        // wait() retourne false si arrêté via stop_token

        if (stopped) {
            std::println("Arrêt demandé, fin du consumer");
            return;
        }

        int task = tasks.front();
        tasks.pop();
        lock.unlock();
        process(task);
    }
}

int main() {
    std::jthread t(consumer);

    {
        std::lock_guard lock(mtx);
        tasks.push(1);
        tasks.push(2);
    }
    cv.notify_one();

    std::this_thread::sleep_for(1s);
    // Le destructeur de t :
    //   1. request_stop() → réveille le wait() via le stop_token
    //   2. join() → attend la fin du thread
}
```

C'est un avantage considérable par rapport au pattern classique où il fallait maintenir un flag `stopped`, le vérifier dans le prédicat, et faire un `notify_all()` lors de l'arrêt. Le `stop_token` unifie tout cela.

> ⚠️ Cette intégration fonctionne uniquement avec `std::condition_variable_any`, pas avec `std::condition_variable`. C'est l'un des rares cas où `condition_variable_any` est préférable à `condition_variable`.

---

## std::stop_callback : réagir à l'arrêt

`std::stop_callback` enregistre un callback qui sera exécuté quand l'arrêt est demandé. Si l'arrêt a déjà été demandé au moment de la construction du callback, il est exécuté immédiatement.

```cpp
#include <stop_token>
#include <print>

void interruptible_worker(std::stop_token stoken) {
    // Enregistrer une action à exécuter à l'arrêt
    std::stop_callback on_stop(stoken, [] {
        std::println("Callback d'arrêt exécuté !");
        // Fermer des sockets, annuler des I/O, notifier des sous-systèmes...
    });

    while (!stoken.stop_requested()) {
        do_work();
    }
}
```

### Cas d'usage : annulation d'I/O bloquantes

`stop_callback` permet d'interrompre des opérations bloquantes qui ne vérifient pas naturellement le `stop_token` :

```cpp
#include <stop_token>

void network_worker(std::stop_token stoken, int socket_fd) {
    // Si l'arrêt est demandé, fermer le socket pour débloquer recv()
    std::stop_callback on_stop(stoken, [socket_fd] {
        ::shutdown(socket_fd, SHUT_RDWR);
    });

    char buffer[1024];
    while (true) {
        ssize_t n = ::recv(socket_fd, buffer, sizeof(buffer), 0);
        if (n <= 0) break;  // Erreur ou socket fermé par le callback
        process(buffer, n);
    }
    // Nettoyage...
}
```

Sans `stop_callback`, il aurait fallu un mécanisme externe (pipe de signalisation, `epoll` avec un eventfd) pour interrompre un `recv()` bloquant. Le callback centralise la logique d'annulation à côté de l'opération qu'il annule.

### Propriétés de stop_callback

- Le callback est exécuté dans le thread qui appelle `request_stop()` (ou dans le constructeur si l'arrêt est déjà demandé).
- Le callback est automatiquement désenregistré quand le `stop_callback` est détruit.
- Plusieurs callbacks peuvent être enregistrés sur le même `stop_token` — ils sont tous exécutés.
- Le callback doit être rapide et non-bloquant (il s'exécute dans le contexte de `request_stop()`).

---

## std::jthread vs std::thread : comparaison complète

| Aspect | `std::thread` | `std::jthread` |
|--------|:-------------:|:--------------:|
| Standard | C++11 | C++20 |
| Join automatique | Non (terminate si oublié) | Oui (dans le destructeur) |
| Arrêt coopératif | Manuel (flag atomique) | Intégré (`stop_token`) |
| `request_stop()` | N/A | Oui |
| `get_stop_token()` | N/A | Oui |
| `get_stop_source()` | N/A | Oui |
| `join()` / `detach()` | Obligatoire avant destruction | `join()` automatique, `detach()` disponible |
| `joinable()` | Oui | Oui |
| Move-only | Oui | Oui |
| Copiable | Non | Non |
| `get_id()` | Oui | Oui |
| `native_handle()` | Oui | Oui |
| Passage d'arguments | Identique | Identique (+ stop_token optionnel en premier) |

### Migration de std::thread vers std::jthread

Dans la plupart des cas, la migration est triviale :

```cpp
// Avant (std::thread)
std::atomic<bool> stop_flag{false};

void worker() {
    while (!stop_flag.load()) {
        do_work();
    }
}

{
    std::thread t(worker);
    // ...
    stop_flag.store(true);
    t.join();
}

// Après (std::jthread) — plus court, plus sûr
void worker(std::stop_token stoken) {
    while (!stoken.stop_requested()) {
        do_work();
    }
}

{
    std::jthread t(worker);
    // ...
    // Le destructeur fait request_stop() + join()
}
```

Cas où la migration n'est pas directe :

- **`detach()`** : `std::jthread` supporte `detach()`, mais le join automatique ne s'applique plus après. Les threads détachés perdent aussi la connexion au `stop_source`. Si vous utilisez `detach()`, les avantages de `jthread` sont perdus.
- **Thread pools existants** : si votre pool gère ses propres threads et leur cycle de vie, remplacer `std::thread` par `std::jthread` n'apporte pas nécessairement d'avantage — le pool gère déjà le join.

---

## Exemple complet : pool de workers stoppable

Un exemple réaliste combinant `std::jthread`, `stop_token`, et `condition_variable_any` :

```cpp
#include <thread>
#include <stop_token>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <vector>
#include <functional>
#include <print>

class WorkerPool {
    std::mutex mtx_;
    std::condition_variable_any cv_;
    std::queue<std::function<void()>> tasks_;
    std::vector<std::jthread> workers_;

public:
    explicit WorkerPool(int num_threads) {
        for (int i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this](std::stop_token stoken) {
                worker_loop(stoken);
            });
        }
        std::println("Pool démarré avec {} workers", num_threads);
    }

    // Le destructeur arrête tout proprement :
    //   ~vector<jthread> détruit chaque jthread
    //   ~jthread appelle request_stop() + join()
    //   Les workers voient le stop_token et sortent de leur boucle
    ~WorkerPool() {
        cv_.notify_all();  // Réveiller les workers endormis
        // Les ~jthread font le reste
        std::println("Pool arrêté");
    }

    void submit(std::function<void()> task) {
        {
            std::lock_guard lock(mtx_);
            tasks_.push(std::move(task));
        }
        cv_.notify_one();
    }

private:
    void worker_loop(std::stop_token stoken) {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock lock(mtx_);

                // Attente interruptible par stop_token
                bool stopped = !cv_.wait(lock, stoken, [this] {
                    return !tasks_.empty();
                });

                if (stopped) return;  // Arrêt demandé

                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task();
        }
    }
};

int main() {
    {
        WorkerPool pool(4);

        for (int i = 0; i < 20; ++i) {
            pool.submit([i] {
                std::println("Tâche {} exécutée par thread {}",
                             i, std::this_thread::get_id());
            });
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    // ~WorkerPool → request_stop sur chaque worker → join → propre
}
```

Notez la simplicité du destructeur : un simple `notify_all()` suffit. La destruction des `jthread` dans le vecteur fait automatiquement `request_stop()` puis `join()` pour chaque worker. Le `wait()` avec `stop_token` se réveille automatiquement et retourne `false`, provoquant la sortie propre de la boucle. Aucun flag manuel, aucun état `stopped_` à gérer.

---

## Recommandation

En C++20 et au-delà, **`std::jthread` devrait être votre choix par défaut** pour la création de threads. Il est strictement supérieur à `std::thread` :

- Même interface de base (construction, arguments, `join()`, `detach()`, `get_id()`, `native_handle()`).
- Join automatique qui élimine une classe entière de bugs.
- Arrêt coopératif intégré qui remplace les flags atomiques manuels.
- Intégration avec `condition_variable_any` pour l'attente interruptible.
- Aucun surcoût mesurable par rapport à `std::thread`.

Réservez `std::thread` au code qui doit rester compatible C++11/14/17, ou aux rares cas où le join automatique est indésirable (threads transférés à un système de gestion externe).

---

## Résumé

| Aspect | Détail |
|--------|--------|
| Header | `<thread>`, `<stop_token>` |
| Standard | C++20 |
| Join automatique | Oui — `~jthread()` appelle `request_stop()` puis `join()` |
| Arrêt coopératif | `stop_token` passé automatiquement si le callable l'accepte |
| `request_stop()` | Thread-safe, idempotent |
| `stop_callback` | Callback exécuté à la demande d'arrêt (annulation d'I/O, nettoyage) |
| `condition_variable_any` | `wait()` avec `stop_token` — réveil automatique à l'arrêt |
| Move-only | Oui (comme `std::thread`) |
| Migration depuis `std::thread` | Quasi-directe dans la majorité des cas |
| Recommandation | **Choix par défaut** en C++20+ pour la création de threads |

> **À suivre** : la section 21.8 boucle le chapitre en montrant comment exploiter les **algorithmes parallèles de la STL** pour paralléliser des calculs sans gérer manuellement des threads, avec les politiques d'exécution `std::execution::par` et `std::execution::par_unseq`.

⏭️ [Algorithmes parallèles appliqués à la concurrence](/21-threads-concurrence/08-algorithmes-paralleles.md)
