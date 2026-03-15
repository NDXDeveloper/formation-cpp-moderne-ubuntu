🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 21.5 std::async et std::future : Programmation asynchrone

## Une abstraction de plus haut niveau

Les sections précédentes ont couvert les briques de base : `std::thread` pour l'exécution, les mutex pour la protection, les condition variables pour la signalisation, les atomiques pour les opérations indivisibles. Ces primitives sont puissantes mais imposent au programmeur de gérer manuellement le cycle de vie des threads, la transmission des résultats, et la propagation des exceptions.

`std::async` et `std::future` proposent un modèle plus simple : **lancer un calcul, récupérer son résultat plus tard**. Pas de `join()` explicite, pas de `std::exception_ptr` à manipuler, pas de variable partagée protégée par un mutex pour transmettre le résultat. Le futur encapsule tout cela.

```cpp
#include <future>
#include <print>

int heavy_computation(int x) {
    // Calcul coûteux...
    return x * x;
}

int main() {
    // Lancer le calcul de façon asynchrone
    std::future<int> result = std::async(std::launch::async, heavy_computation, 42);

    // Faire autre chose pendant que le calcul tourne...
    do_other_work();

    // Récupérer le résultat (bloque si pas encore prêt)
    int value = result.get();
    std::println("Résultat : {}", value);  // 1764
}
```

Ce code est fonctionnellement équivalent à créer un thread, capturer le résultat dans une variable partagée protégée par un mutex, joindre le thread, et gérer les exceptions — mais en trois lignes au lieu de vingt.

---

## std::future : le conteneur de résultat différé

Un `std::future<T>` représente un résultat de type `T` qui sera disponible **dans le futur**. C'est un point de rendez-vous entre le thread qui produit le résultat et celui qui le consomme.

### get() : récupérer le résultat

```cpp
std::future<int> fut = std::async(std::launch::async, [] { return 42; });

int value = fut.get();
// - Si le résultat est déjà disponible : retour immédiat.
// - Si le calcul est en cours : bloque jusqu'à sa fin.
// - Si le calcul a levé une exception : l'exception est relancée ici.
```

> ⚠️ **`get()` ne peut être appelé qu'une seule fois.** Après l'appel, le futur est dans un état invalide (*moved-from*). Appeler `get()` une seconde fois est un comportement indéfini. Si vous avez besoin de distribuer le résultat à plusieurs consommateurs, utilisez `std::shared_future` (voir plus bas).

### wait() : attendre sans consommer

```cpp
std::future<int> fut = std::async(std::launch::async, long_computation);

fut.wait();  // Bloque jusqu'à ce que le résultat soit prêt
// Le résultat est prêt mais pas encore consommé
int value = fut.get();  // Retour immédiat
```

### wait_for() et wait_until() : attente bornée

```cpp
#include <future>
#include <chrono>
#include <print>

void poll_result(std::future<int>& fut) {
    using namespace std::chrono_literals;

    while (true) {
        auto status = fut.wait_for(100ms);

        switch (status) {
            case std::future_status::ready:
                std::println("Résultat : {}", fut.get());
                return;

            case std::future_status::timeout:
                std::println("Pas encore prêt, patience...");
                break;

            case std::future_status::deferred:
                std::println("Calcul différé — sera lancé au get()");
                std::println("Résultat : {}", fut.get());
                return;
        }
    }
}
```

Les trois états retournés par `wait_for()` et `wait_until()` sont :

| Status | Signification |
|--------|--------------|
| `std::future_status::ready` | Le résultat est disponible |
| `std::future_status::timeout` | Le délai a expiré sans que le résultat soit prêt |
| `std::future_status::deferred` | Le calcul est différé et n'a pas encore démarré (voir `std::launch::deferred`) |

### valid() : vérifier l'état du futur

```cpp
std::future<int> fut;             // Construit par défaut — invalide  
assert(!fut.valid());  

fut = std::async(std::launch::async, [] { return 1; });  
assert(fut.valid());              // Associé à un résultat  

int v = fut.get();  
assert(!fut.valid());             // Consommé — invalide  
```

---

## std::async : lancer une tâche asynchrone

`std::async` est la façon la plus simple de lancer un calcul asynchrone. Il accepte un callable et ses arguments, et retourne un `std::future` contenant le résultat :

```cpp
// Signature simplifiée
template <typename F, typename... Args>  
std::future<std::invoke_result_t<F, Args...>>  
async(std::launch policy, F&& f, Args&&... args);  
```

### Politiques de lancement

Le premier argument optionnel contrôle **comment** la tâche est exécutée :

#### std::launch::async

Force l'exécution dans un **nouveau thread** (ou un thread du pool, selon l'implémentation). Le calcul démarre immédiatement, en parallèle du thread appelant :

```cpp
auto fut = std::async(std::launch::async, [] {
    // Exécuté dans un thread séparé, immédiatement
    return expensive_computation();
});
```

C'est la politique que vous utiliserez dans la grande majorité des cas quand vous voulez du parallélisme réel.

#### std::launch::deferred

Le calcul est **différé** : il ne s'exécute que lorsque `get()` ou `wait()` est appelé sur le futur, et il s'exécute **dans le thread appelant** (celui qui appelle `get()`). C'est de l'évaluation paresseuse (*lazy evaluation*), pas du parallélisme :

```cpp
auto fut = std::async(std::launch::deferred, [] {
    // Ne s'exécute PAS maintenant
    return expensive_computation();
});

// ... du temps passe, le calcul n'a toujours pas démarré ...

int result = fut.get();  // Le calcul s'exécute ICI, dans CE thread
```

`deferred` est utile quand vous voulez préparer un calcul mais ne l'exécuter que si son résultat est effectivement demandé.

#### Politique par défaut : async | deferred

Si vous ne spécifiez pas de politique, le comportement est `std::launch::async | std::launch::deferred` — l'implémentation **choisit** entre exécution asynchrone et différée :

```cpp
// L'implémentation décide — peut être asynchrone OU différé
auto fut = std::async([] { return compute(); });
```

> ⚠️ **Cette ambiguïté est problématique.** Si l'implémentation choisit `deferred`, votre code qui attend le résultat avec `wait_for()` en boucle ne terminera jamais — le statut sera toujours `deferred`, jamais `ready`, car le calcul ne démarre qu'au `get()`. Pour cette raison, **spécifiez toujours explicitement `std::launch::async`** quand vous voulez du parallélisme.

```cpp
// ❌ Piège : boucle infinie si la politique choisie est deferred
auto fut = std::async(maybe_long_task);  
while (fut.wait_for(100ms) != std::future_status::ready) {  
    // Si deferred : wait_for retourne toujours deferred, jamais ready
    show_progress();
}

// ✅ Explicite : garantit l'exécution asynchrone
auto fut = std::async(std::launch::async, maybe_long_task);  
while (fut.wait_for(100ms) != std::future_status::ready) {  
    show_progress();  // Fonctionne correctement
}
```

---

## Propagation automatique des exceptions

L'un des avantages majeurs de `std::future` sur la gestion manuelle est la propagation transparente des exceptions. Si la tâche asynchrone lève une exception, elle est capturée, stockée dans le futur, et relancée dans le thread appelant lors du `get()` :

```cpp
#include <future>
#include <stdexcept>
#include <print>

int risky_computation() {
    throw std::runtime_error("Erreur dans le calcul");
    return 42;  // Jamais atteint
}

int main() {
    auto fut = std::async(std::launch::async, risky_computation);

    try {
        int value = fut.get();  // Relance l'exception ici
    } catch (const std::runtime_error& e) {
        std::println("Exception capturée : {}", e.what());
    }
}
```

Comparez avec le mécanisme manuel vu en section 21.1 (`std::exception_ptr`, `std::current_exception()`, `std::rethrow_exception()`) — `std::future` automatise tout cela.

---

## Le piège du futur temporaire

Un `std::future` retourné par `std::async` a un comportement spécial dans son destructeur : **si le futur est le dernier à référencer l'état partagé et que la tâche a été lancée avec `std::launch::async`, le destructeur bloque jusqu'à la fin de la tâche**. C'est équivalent à un `join()` implicite.

Ce comportement est souvent source de confusion quand le futur n'est pas stocké :

```cpp
// ❌ PIÈGE : le futur temporaire est détruit immédiatement
//    → le destructeur bloque jusqu'à la fin de la tâche
//    → l'exécution est séquentielle, pas parallèle !
std::async(std::launch::async, task_a);  // Bloque ici  
std::async(std::launch::async, task_b);  // Puis bloque ici  
std::async(std::launch::async, task_c);  // Puis bloque ici  
// Temps total ≈ task_a + task_b + task_c (séquentiel)

// ✅ Stocker les futurs pour maintenir le parallélisme
auto fa = std::async(std::launch::async, task_a);  
auto fb = std::async(std::launch::async, task_b);  
auto fc = std::async(std::launch::async, task_c);  
// Les trois tâches tournent en parallèle
fa.get();  
fb.get();  
fc.get();  
// Temps total ≈ max(task_a, task_b, task_c) (parallèle)
```

Tant que les futurs sont vivants, les tâches s'exécutent en parallèle. Dès qu'un futur est détruit (fin de portée, réassignation), il attend la fin de sa tâche. Ce `join()` implicite est un choix controversé du standard — il évite les threads orphelins mais crée des blocages surprenants.

---

## std::promise : le côté producteur

`std::future` est le côté **consommateur** — il lit le résultat. `std::promise` est le côté **producteur** — il écrit le résultat. Ensemble, ils forment un canal de communication unidirectionnel entre deux threads.

```cpp
#include <future>
#include <thread>
#include <print>

int main() {
    std::promise<int> prom;
    std::future<int> fut = prom.get_future();  // Lier promise et future

    std::thread producer([&prom] {
        int result = heavy_computation();
        prom.set_value(result);  // Rend le résultat disponible
    });

    std::println("Résultat : {}", fut.get());  // Bloque jusqu'au set_value
    producer.join();
}
```

### set_value() et set_exception()

```cpp
std::promise<int> prom;

// Chemin nominal : fournir le résultat
prom.set_value(42);

// OU chemin d'erreur : fournir une exception
prom.set_exception(std::make_exception_ptr(
    std::runtime_error("Échec du calcul")
));

// Appeler set_value() OU set_exception() une seule fois.
// Un second appel lève std::future_error.
```

### Quand utiliser promise directement ?

`std::async` crée implicitement un couple promise/future. Vous n'avez besoin de `std::promise` explicitement que dans ces cas :

- Vous gérez vos propres threads (pas via `std::async`) et voulez transmettre un résultat.
- Le résultat est produit par un callback ou un événement, pas par une fonction linéaire.
- Vous implémentez un pool de threads personnalisé.

```cpp
#include <future>
#include <thread>
#include <functional>
#include <print>

// Pool de threads simplifié : le résultat est transmis via promise
void submit_to_pool(std::function<int()> task, std::promise<int> prom) {
    std::thread([task = std::move(task), prom = std::move(prom)]() mutable {
        try {
            prom.set_value(task());
        } catch (...) {
            prom.set_exception(std::current_exception());
        }
    }).detach();
}

int main() {
    std::promise<int> prom;
    auto fut = prom.get_future();

    submit_to_pool([] { return 42; }, std::move(prom));

    std::println("Résultat : {}", fut.get());
}
```

---

## std::shared_future : partager un résultat

`std::future` est move-only — un seul consommateur peut appeler `get()`. Si plusieurs threads doivent lire le même résultat, utilisez `std::shared_future` :

```cpp
#include <future>
#include <thread>
#include <vector>
#include <print>

int main() {
    // Créer un shared_future à partir d'un future
    std::shared_future<int> shared =
        std::async(std::launch::async, [] { return 42; }).share();

    std::vector<std::thread> consumers;
    for (int i = 0; i < 5; ++i) {
        // shared_future est copiable — chaque thread a sa copie
        consumers.emplace_back([shared, i] {
            int value = shared.get();  // Chaque thread peut appeler get()
            std::println("Consumer {} : {}", i, value);
        });
    }

    for (auto& t : consumers) t.join();
}
```

Différences clés avec `std::future` :

| Aspect | `std::future` | `std::shared_future` |
|--------|:-------------:|:--------------------:|
| Copiable | Non (move-only) | Oui |
| `get()` appelable | Une seule fois | Plusieurs fois, par plusieurs threads |
| `get()` retourne | `T` (par déplacement) | `const T&` (par référence constante) |
| Création | Depuis `std::async` ou `std::promise` | Via `future.share()` |

---

## std::packaged_task : encapsuler un callable

`std::packaged_task` encapsule un callable dans un objet qui, quand il est invoqué, exécute le callable et stocke le résultat dans un futur. C'est un pont entre les API basées sur des callbacks et le monde des futures.

```cpp
#include <future>
#include <functional>
#include <print>

int main() {
    // Encapsuler un lambda dans un packaged_task
    std::packaged_task<int(int, int)> task([](int a, int b) {
        return a + b;
    });

    // Obtenir le futur AVANT d'exécuter la tâche
    std::future<int> fut = task.get_future();

    // Exécuter la tâche (dans un autre thread, un pool, une queue...)
    std::thread t(std::move(task), 10, 20);

    std::println("Résultat : {}", fut.get());  // 30
    t.join();
}
```

### Cas d'usage typique : queue de tâches

`std::packaged_task` est la brique idéale pour implémenter une queue de tâches où le soumetteur récupère un futur :

```cpp
#include <future>
#include <queue>
#include <mutex>
#include <thread>
#include <functional>
#include <print>

class TaskQueue {
    std::mutex mtx_;
    std::condition_variable cv_;
    std::queue<std::packaged_task<void()>> tasks_;
    bool stopped_ = false;

public:
    template <typename F>
    std::future<std::invoke_result_t<F>> submit(F&& func) {
        using ReturnType = std::invoke_result_t<F>;

        std::packaged_task<ReturnType()> task(std::forward<F>(func));
        auto fut = task.get_future();

        {
            std::lock_guard lock(mtx_);
            // Encapsuler dans un packaged_task<void()> via un lambda
            tasks_.emplace([t = std::move(task)]() mutable { t(); });
        }
        cv_.notify_one();

        return fut;
    }

    void worker_loop() {
        while (true) {
            std::packaged_task<void()> task;
            {
                std::unique_lock lock(mtx_);
                cv_.wait(lock, [this] {
                    return !tasks_.empty() || stopped_;
                });
                if (stopped_ && tasks_.empty()) return;
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task();  // Exécution — le résultat est envoyé au futur
        }
    }

    void stop() {
        {
            std::lock_guard lock(mtx_);
            stopped_ = true;
        }
        cv_.notify_all();
    }
};

int main() {
    TaskQueue queue;
    std::thread worker([&queue] { queue.worker_loop(); });

    auto f1 = queue.submit([] { return 42; });
    auto f2 = queue.submit([] { return 100; });

    std::println("f1 = {}", f1.get());   // 42
    std::println("f2 = {}", f2.get());   // 100

    queue.stop();
    worker.join();
}
```

---

## Comparaison des mécanismes asynchrones

| Mécanisme | Quand l'utiliser |
|-----------|-----------------|
| `std::async` | Lancer un calcul ponctuel et récupérer son résultat. Le plus simple. |
| `std::promise` / `std::future` | Transmettre un résultat entre threads quand `async` ne convient pas (callbacks, événements, pools personnalisés). |
| `std::packaged_task` | Encapsuler un callable pour l'exécuter plus tard tout en récupérant un futur sur son résultat. Idéal pour les queues de tâches. |
| `std::thread` + mutex/CV | Contrôle total sur le cycle de vie et la synchronisation. Plus verbeux mais plus flexible. |

---

## Limites de std::async et std::future

Malgré leur simplicité, `std::async` et `std::future` ont des limitations reconnues par la communauté C++ :

### Pas de continuation (then)

Vous ne pouvez pas chaîner des opérations : « quand ce futur est prêt, lance cette autre tâche avec le résultat ». Il faut appeler `get()` (bloquant) puis lancer la suite manuellement :

```cpp
// Ce qu'on aimerait écrire (pas possible avec std::future)
// auto result = async(task_a).then(task_b).then(task_c);

// Ce qu'il faut écrire
auto fa = std::async(std::launch::async, task_a);  
int ra = fa.get();  // Bloque  
auto fb = std::async(std::launch::async, task_b, ra);  
int rb = fb.get();  // Bloque  
```

### Pas de when_all / when_any

Il n'existe pas de mécanisme standard pour attendre que **tous** ou **l'un** de plusieurs futurs soient prêts. Vous devez itérer et appeler `get()` sur chacun.

### Pas de pool de threads garanti

Le standard ne spécifie pas comment `std::async(std::launch::async)` crée ses threads. Chaque appel peut créer un nouveau thread système — coûteux si vous soumettez des milliers de petites tâches. L'implémentation de libstdc++ (GCC) crée effectivement un thread par appel. Celle de MSVC utilise un pool. Le comportement n'est pas portable.

### L'avenir : std::execution (C++26)

Ces limitations sont adressées par `std::execution` (Senders/Receivers), intégré au standard C++26. Ce nouveau framework fournit un modèle d'exécution asynchrone composable, avec des continuations, des schedulers, et une intégration native avec les pools de threads.

> 📎 *Pour une couverture de `std::execution` (Senders/Receivers), voir la **section 12.14.4**.*

En attendant l'adoption généralisée de `std::execution` par les compilateurs, `std::async` et `std::future` restent les outils standard les plus accessibles pour la programmation asynchrone en C++.

---

## Résumé

| Aspect | Détail |
|--------|--------|
| Header | `<future>` |
| `std::async` | Lance un callable de façon asynchrone, retourne un `std::future` |
| Politique recommandée | `std::launch::async` (toujours explicite) |
| `std::future<T>` | Conteneur de résultat différé — `get()` une seule fois |
| `std::shared_future<T>` | Version copiable — `get()` multiple, par plusieurs threads |
| `std::promise<T>` | Côté producteur — `set_value()` ou `set_exception()` |
| `std::packaged_task<F>` | Encapsule un callable avec un futur associé |
| Propagation d'exception | Automatique — relancée au `get()` |
| Piège principal | Futur temporaire non stocké → exécution séquentielle |
| Limitation majeure | Pas de continuations, pas de pool garanti |
| Évolution C++26 | `std::execution` (Senders/Receivers) |

> **À suivre** : la section 21.6 prend du recul pour aborder les principes de **thread-safety** et les patterns de conception qui permettent d'écrire du code concurrent correct par construction.

⏭️ [Thread-safety et data races](/21-threads-concurrence/06-thread-safety.md)
