🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 21.3 Variables de condition : std::condition_variable

## Le problème de l'attente

Les mutex protègent l'accès aux données partagées, mais ils ne répondent pas à une question fondamentale : **comment un thread peut-il attendre efficacement qu'une condition soit remplie par un autre thread ?**

Prenons un scénario classique — un thread producteur génère des tâches, et un thread consommateur les traite. Le consommateur a besoin de savoir quand une nouvelle tâche est disponible. Sans variable de condition, les deux approches naïves sont désastreuses :

```cpp
// ❌ Approche 1 : Polling (attente active)
void consumer_polling() {
    while (true) {
        std::lock_guard lock(mtx);
        if (!queue.empty()) {
            auto task = queue.front();
            queue.pop();
            process(task);
        }
        // Si la queue est vide, on boucle immédiatement.
        // → Consomme 100% d'un cœur CPU pour ne rien faire.
    }
}

// ❌ Approche 2 : Polling avec sleep
void consumer_sleep() {
    while (true) {
        {
            std::lock_guard lock(mtx);
            if (!queue.empty()) {
                auto task = queue.front();
                queue.pop();
                process(task);
            }
        }
        std::this_thread::sleep_for(10ms);
        // → Gaspille moins de CPU, mais introduit une latence de 0 à 10ms
        //   entre la production d'une tâche et son traitement.
    }
}
```

La première approche brûle un cœur CPU entier. La seconde ajoute une latence moyenne de 5ms, inacceptable pour de nombreuses applications. Ni l'une ni l'autre ne permet au consommateur de se réveiller *exactement* quand une tâche est disponible.

C'est précisément ce que `std::condition_variable` résout : un thread se suspend gratuitement (aucun CPU consommé) et est réveillé instantanément par un autre thread quand la condition change.

---

## Fonctionnement conceptuel

Une variable de condition est un mécanisme de **signalisation** entre threads. Elle fonctionne toujours en tandem avec un mutex et une condition (un prédicat) :

1. Le **consommateur** verrouille le mutex, vérifie la condition, et si elle est fausse, **s'endort** en relâchant atomiquement le mutex.
2. Le **producteur** verrouille le mutex, modifie l'état partagé (rendant la condition vraie), relâche le mutex, puis **signale** la variable de condition.
3. Le consommateur est **réveillé**, re-acquiert le mutex, vérifie la condition (elle peut être fausse — voir *spurious wakeups*), et procède si elle est vraie.

Le point crucial est l'**atomicité** de l'étape 1 : le relâchement du mutex et la mise en attente se font en une seule opération indivisible. Sans cette garantie, un signal pourrait être envoyé entre le relâchement du mutex et la mise en attente, et le consommateur le raterait — c'est ce qu'on appelle le *lost wakeup* problem.

---

## Interface de base

```cpp
#include <condition_variable>
#include <mutex>

std::mutex mtx;  
std::condition_variable cv;  
bool data_ready = false;  

// Thread producteur
void producer() {
    {
        std::lock_guard lock(mtx);
        prepare_data();
        data_ready = true;
    }                          // Le mutex est relâché AVANT notify
    cv.notify_one();           // Réveille un thread en attente
}

// Thread consommateur
void consumer() {
    std::unique_lock lock(mtx);   // unique_lock obligatoire
    cv.wait(lock, [] {
        return data_ready;         // Prédicat : condition à vérifier
    });
    // Ici : le mutex est verrouillé ET data_ready == true
    use_data();
}
```

### Pourquoi std::unique_lock est obligatoire

`std::condition_variable::wait()` exige un `std::unique_lock<std::mutex>`. Ni `lock_guard` ni `scoped_lock` ne conviennent, car `wait()` doit pouvoir :

1. **Déverrouiller** le mutex pour permettre au producteur de modifier l'état partagé.
2. **Re-verrouiller** le mutex au réveil pour que le consommateur puisse vérifier la condition en sécurité.

Seul `std::unique_lock` expose les opérations `lock()` et `unlock()` nécessaires à ce mécanisme.

---

## wait() en détail

### Forme avec prédicat (recommandée)

```cpp
cv.wait(lock, predicate);
```

Cette forme est équivalente à :

```cpp
while (!predicate()) {
    cv.wait(lock);  // Relâche le mutex et dort — re-verrouille au réveil
}
```

Le prédicat est vérifié **avant** la première mise en attente (si la condition est déjà vraie, le thread ne dort jamais) et **après chaque réveil** (pour gérer les spurious wakeups). C'est la forme que vous devez utiliser dans 99% des cas.

### Forme sans prédicat

```cpp
cv.wait(lock);
```

Le thread se suspend et ne se réveille que lorsqu'il est notifié (ou en cas de spurious wakeup). Vous devez alors vérifier manuellement la condition dans une boucle `while` englobante. Cette forme est plus error-prone et n'a aucun avantage sur la version avec prédicat — évitez-la dans du code nouveau.

### Ce qui se passe pendant wait()

L'appel à `wait()` exécute les étapes suivantes, de façon atomique vis-à-vis des notifications :

```
1. lock.unlock()             — Relâche le mutex
2. Suspend le thread         — Aucun CPU consommé
   ... le thread dort ...
3. [Notification reçue ou spurious wakeup]
4. lock.lock()               — Re-acquiert le mutex
5. Vérifie le prédicat       — Si faux, retour à l'étape 1
6. wait() retourne           — Le mutex est détenu, le prédicat est vrai
```

---

## Spurious wakeups : pourquoi le prédicat est vital

Un **spurious wakeup** est un réveil du thread sans qu'aucune notification n'ait été envoyée. Ce phénomène est autorisé par le standard C++ (et par POSIX) car il simplifie l'implémentation sur certaines architectures et permet des optimisations du noyau.

En pratique, les spurious wakeups sont rares sur Linux, mais votre code doit les gérer correctement. C'est pourquoi la vérification du prédicat dans une boucle est **obligatoire**, pas optionnelle :

```cpp
// ❌ DANGEREUX : pas de prédicat, pas de boucle
cv.wait(lock);
// Le thread peut se réveiller alors que la condition est fausse
process(queue.front());  // Crash potentiel : la queue est peut-être vide !

// ✅ CORRECT : prédicat vérifié à chaque réveil
cv.wait(lock, [] { return !queue.empty(); });
// Ici, la queue est garantie non-vide
process(queue.front());
```

La forme avec prédicat intègre automatiquement cette boucle de vérification. Utilisez-la systématiquement.

---

## notify_one() et notify_all()

### notify_one()

Réveille **un seul** thread parmi ceux en attente sur la variable de condition. Si aucun thread n'attend, la notification est perdue (ce n'est pas une erreur — les notifications ne sont pas mises en queue).

```cpp
cv.notify_one();
```

Utilisez `notify_one()` quand un seul thread peut progresser grâce au changement d'état. C'est le cas typique d'un producteur-consommateur où chaque élément produit est consommé par un seul thread.

### notify_all()

Réveille **tous** les threads en attente. Chacun se réveille, re-acquiert le mutex (un par un — le mutex garantit l'exclusion), vérifie le prédicat, et soit continue soit se rendort.

```cpp
cv.notify_all();
```

Utilisez `notify_all()` quand :

- Plusieurs threads peuvent progresser simultanément (par exemple, une condition partagée change qui affecte tous les threads).
- Vous signaler la fin d'un traitement (tous les consommateurs doivent vérifier un flag `finished`).
- Vous n'êtes pas sûr qu'un seul thread suffit — `notify_all()` est toujours correct, au prix d'éventuels réveils inutiles.

### Faut-il notifier sous verrou ou après ?

C'est une question fréquente, et les deux approches sont correctement synchronisées. La différence est une question de performance :

```cpp
// Option A : notifier sous verrou
{
    std::lock_guard lock(mtx);
    queue.push(item);
    cv.notify_one();  // Le thread réveillé tentera d'acquérir le mutex
                      // mais il est encore détenu → il bloque brièvement
}

// Option B : notifier après relâchement (généralement préféré)
{
    std::lock_guard lock(mtx);
    queue.push(item);
}
cv.notify_one();  // Le mutex est libre → le thread réveillé peut l'acquérir
                  // immédiatement
```

L'option B évite un « wakeup-then-block » inutile : le thread réveillé peut acquérir le mutex directement sans passer par une étape de blocage supplémentaire. La différence est minime en pratique, mais l'option B est légèrement plus efficiente et généralement recommandée.

> ⚠️ L'option B est sûre **à condition que la modification de l'état partagé ait été faite sous verrou**. La notification elle-même n'a pas besoin du mutex. Ce qui doit être protégé, c'est le changement de l'état que le prédicat interroge.

---

## Pattern producteur/consommateur

C'est le pattern le plus courant avec les variables de condition. Voici une implémentation complète et robuste :

```cpp
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <print>
#include <vector>

template <typename T>  
class BlockingQueue {  
    std::mutex mtx_;
    std::condition_variable cv_not_empty_;
    std::queue<T> queue_;
    bool closed_ = false;

public:
    void push(const T& item) {
        {
            std::lock_guard lock(mtx_);
            if (closed_) {
                throw std::runtime_error("push sur une queue fermée");
            }
            queue_.push(item);
        }
        cv_not_empty_.notify_one();
    }

    // Retourne false si la queue est fermée et vide (plus rien à consommer)
    bool pop(T& item) {
        std::unique_lock lock(mtx_);
        cv_not_empty_.wait(lock, [this] {
            return !queue_.empty() || closed_;
        });

        if (queue_.empty()) {
            return false;  // Queue fermée et vide → arrêt du consommateur
        }

        item = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    void close() {
        {
            std::lock_guard lock(mtx_);
            closed_ = true;
        }
        cv_not_empty_.notify_all();  // Réveiller TOUS les consommateurs
    }
};

int main() {
    BlockingQueue<int> queue;

    // Producteur
    std::thread producer([&queue] {
        for (int i = 0; i < 20; ++i) {
            queue.push(i);
            std::println("[P] Produit : {}", i);
        }
        queue.close();
    });

    // Consommateurs
    std::vector<std::thread> consumers;
    for (int id = 0; id < 3; ++id) {
        consumers.emplace_back([&queue, id] {
            int item;
            while (queue.pop(item)) {
                std::println("[C{}] Traité : {}", id, item);
            }
            std::println("[C{}] Terminé", id);
        });
    }

    producer.join();
    for (auto& c : consumers) {
        c.join();
    }
}
```

Quelques points clés de cette implémentation :

- **`close()`** utilise `notify_all()` car tous les consommateurs doivent se réveiller pour constater la fermeture. Un `notify_one()` ne réveillerait qu'un seul consommateur, laissant les autres bloqués indéfiniment.

- **Le prédicat combine deux conditions** : `!queue_.empty() || closed_`. Le consommateur se réveille soit quand il y a un élément, soit quand la queue est fermée. Après le réveil, il vérifie lequel des deux cas s'applique.

- **`pop()` retourne un `bool`** plutôt que de lever une exception pour signaler la fin. C'est un pattern courant pour les queues bloquantes — le consommateur boucle tant que `pop()` retourne `true`.

- **Le producteur pousse et le consommateur tire** — à aucun moment ils n'accèdent directement à l'état partagé sans passer par les méthodes synchronisées.

---

## Pattern : attente ponctuelle d'un résultat

Un autre usage courant est l'attente d'un résultat calculé par un autre thread — un rendez-vous entre producteur et consommateur unique :

```cpp
#include <condition_variable>
#include <mutex>
#include <thread>
#include <optional>
#include <print>

std::mutex mtx;  
std::condition_variable cv;  
std::optional<int> result;  

void compute() {
    int answer = heavy_computation();
    {
        std::lock_guard lock(mtx);
        result = answer;
    }
    cv.notify_one();
}

void waiter() {
    std::unique_lock lock(mtx);
    cv.wait(lock, [] { return result.has_value(); });
    std::println("Résultat : {}", *result);
}
```

> 💡 Pour ce type de scénario (attendre un résultat unique), `std::future` et `std::promise` (section 21.5) offrent une abstraction plus propre et plus sûre. Les variables de condition brillent davantage dans les patterns continus (queues, flux de données, coordination de pools).

---

## wait_for() et wait_until() : attente avec timeout

Parfois, un thread ne peut pas attendre indéfiniment. Les variantes avec timeout permettent de borner l'attente :

### wait_for()

```cpp
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <print>

std::mutex mtx;  
std::condition_variable cv;  
bool event_occurred = false;  

void timed_waiter() {
    using namespace std::chrono_literals;

    std::unique_lock lock(mtx);

    // Forme avec prédicat et timeout (recommandée)
    bool success = cv.wait_for(lock, 500ms, [] {
        return event_occurred;
    });

    if (success) {
        std::println("Événement reçu à temps");
    } else {
        std::println("Timeout après 500ms");
    }
}
```

`wait_for()` avec prédicat retourne `true` si le prédicat est devenu vrai avant l'expiration du timeout, `false` sinon. C'est la forme la plus pratique.

### wait_until()

```cpp
void deadline_waiter() {
    using namespace std::chrono_literals;
    auto deadline = std::chrono::steady_clock::now() + 2s;

    std::unique_lock lock(mtx);
    bool success = cv.wait_until(lock, deadline, [] {
        return event_occurred;
    });

    if (!success) {
        std::println("Deadline dépassé");
    }
}
```

`wait_until()` attend jusqu'à un instant absolu. C'est utile quand plusieurs étapes doivent respecter un budget de temps global : vous calculez le deadline une fois et le passez à chaque étape.

### Formes sans prédicat

Les formes sans prédicat retournent un `std::cv_status` :

```cpp
std::unique_lock lock(mtx);  
auto status = cv.wait_for(lock, 500ms);  

if (status == std::cv_status::timeout) {
    // Le timeout a expiré
} else {
    // Notifié (ou spurious wakeup — vérifier la condition manuellement !)
}
```

Comme pour `wait()`, préférez les formes avec prédicat qui gèrent automatiquement les spurious wakeups.

---

## Lost wakeup : le piège classique

Un **lost wakeup** se produit quand une notification est envoyée alors qu'aucun thread n'est en attente. La notification est perdue — les variables de condition n'ont pas de « mémoire ».

```cpp
// ❌ Séquence menant à un lost wakeup

// Thread A (producteur)                    // Thread B (consommateur)
{                                           //
    std::lock_guard lock(mtx);              //
    data_ready = true;                      //
}                                           //
cv.notify_one();                            //
                                            // std::unique_lock lock(mtx);
                                            // cv.wait(lock, [] { return data_ready; });
                                            // → Ne se bloque PAS car data_ready est déjà true
```

Dans cet exemple précis, le lost wakeup est inoffensif grâce au prédicat : quand le consommateur arrive à `wait()`, il vérifie `data_ready` **avant** de se suspendre, constate qu'il est `true`, et continue sans dormir.

Le problème survient quand le prédicat n'est pas utilisé :

```cpp
// ❌ Lost wakeup réel sans prédicat

// Thread A (producteur)                    // Thread B (consommateur)
{                                           //
    std::lock_guard lock(mtx);              //
    queue.push(item);                       //
}                                           //
cv.notify_one();                            // (pas encore en attente)
                                            //
                                            // std::unique_lock lock(mtx);
                                            // cv.wait(lock);  // Dort indéfiniment !
                                            // La notification est perdue.
```

**C'est une raison de plus pour toujours utiliser la forme avec prédicat.** Le prédicat transforme le modèle de « je dors jusqu'à ce qu'on me réveille » en « je dors tant que la condition est fausse » — ce second modèle est résistant aux lost wakeups.

---

## std::condition_variable_any

Le standard fournit une seconde variante : `std::condition_variable_any`. Là où `std::condition_variable` ne fonctionne qu'avec `std::unique_lock<std::mutex>`, la variante `_any` accepte **tout type de verrou** qui expose `lock()` et `unlock()` :

```cpp
#include <condition_variable>
#include <shared_mutex>

std::shared_mutex smtx;  
std::condition_variable_any cv_any;  
bool ready = false;  

void waiter() {
    std::shared_lock lock(smtx);
    cv_any.wait(lock, [] { return ready; });
    // Utilisable avec shared_lock, unique_lock, ou tout custom lock
}
```

`std::condition_variable_any` est plus flexible mais légèrement plus coûteuse que `std::condition_variable` — elle alloue un mutex interne supplémentaire pour synchroniser les opérations. Préférez `std::condition_variable` (avec `std::unique_lock<std::mutex>`) dans les cas standards, et réservez `_any` aux situations où un autre type de verrou est nécessaire.

---

## Erreurs classiques

### Oublier de verrouiller le mutex avant wait()

```cpp
// ❌ Comportement indéfini
void broken_consumer() {
    std::unique_lock lock(mtx);
    lock.unlock();  // Oups
    cv.wait(lock);  // UB : le mutex doit être verrouillé avant wait()
}
```

`wait()` requiert que le mutex soit détenu par le `unique_lock` au moment de l'appel. C'est `wait()` lui-même qui le relâche de façon atomique.

### Modifier l'état partagé sans verrouiller le mutex

```cpp
// ❌ Data race sur data_ready
void broken_producer() {
    data_ready = true;     // Pas de mutex ! Data race.
    cv.notify_one();
}

// ✅ Correct
void correct_producer() {
    {
        std::lock_guard lock(mtx);
        data_ready = true;  // Protégé par le mutex
    }
    cv.notify_one();
}
```

La notification elle-même n'a pas besoin du mutex, mais la modification de l'état qui rend le prédicat vrai **doit** être sous verrou. Sans cela, le consommateur pourrait vérifier le prédicat entre la modification et la notification, ou pire, ne jamais voir la modification.

### Utiliser notify_one() quand notify_all() est nécessaire

```cpp
// ❌ Si plusieurs consommateurs attendent et qu'on ferme la queue :
void broken_close() {
    {
        std::lock_guard lock(mtx);
        closed = true;
    }
    cv.notify_one();  // Seul UN consommateur se réveille.
                      // Les autres restent bloqués indéfiniment.
}

// ✅ Correct
void correct_close() {
    {
        std::lock_guard lock(mtx);
        closed = true;
    }
    cv.notify_all();  // TOUS les consommateurs vérifient le flag
}
```

En cas de doute, `notify_all()` est toujours correct. Le coût des réveils supplémentaires est négligeable comparé au risque d'un consommateur bloqué à vie.

### Capturer le mutex dans le prédicat

```cpp
// ❌ Deadlock : le mutex est déjà détenu par unique_lock
cv.wait(lock, [&mtx] {
    std::lock_guard inner(mtx);  // Tente de re-verrouiller → deadlock
    return !queue.empty();
});

// ✅ Le prédicat s'exécute sous verrou — accès direct aux données
cv.wait(lock, [&queue] {
    return !queue.empty();  // Le mutex est déjà détenu
});
```

Le prédicat passé à `wait()` est toujours exécuté alors que le mutex est détenu. Vous pouvez accéder aux données partagées directement, sans verrouillage supplémentaire.

---

## Primitives C++20 : latch, barrier, semaphore

C++20 a introduit des primitives de synchronisation complémentaires qui couvrent des patterns spécifiques plus simplement que les variables de condition :

### std::latch

Un compteur décrémenté par des threads. Quand il atteint zéro, tous les threads en attente sont libérés. Usage unique — un latch ne peut pas être réinitialisé.

```cpp
#include <latch>
#include <thread>
#include <print>
#include <vector>

void latch_example() {
    const int num_workers = 4;
    std::latch startup_latch(num_workers);

    std::vector<std::thread> workers;
    for (int i = 0; i < num_workers; ++i) {
        workers.emplace_back([&startup_latch, i] {
            initialize_worker(i);
            startup_latch.count_down();  // Signale que ce worker est prêt
        });
    }

    startup_latch.wait();  // Attend que TOUS les workers soient initialisés
    std::println("Tous les workers sont prêts — démarrage");

    for (auto& w : workers) w.join();
}
```

### std::barrier

Similaire à un latch, mais **réutilisable**. Les threads se synchronisent à la barrière, et quand tous sont arrivés, ils sont libérés et la barrière se réinitialise pour le prochain cycle. Idéal pour les algorithmes itératifs où chaque phase doit attendre que la précédente soit terminée par tous les threads.

```cpp
#include <barrier>
#include <thread>
#include <print>
#include <vector>

void barrier_example() {
    const int num_threads = 4;

    // Callback exécuté quand tous les threads ont atteint la barrière
    auto on_completion = [] noexcept {
        std::println("--- Phase terminée ---");
    };

    std::barrier sync_point(num_threads, on_completion);

    std::vector<std::thread> threads;
    for (int id = 0; id < num_threads; ++id) {
        threads.emplace_back([&sync_point, id] {
            for (int phase = 0; phase < 3; ++phase) {
                compute_phase(id, phase);
                sync_point.arrive_and_wait();  // Synchronisation inter-phase
            }
        });
    }

    for (auto& t : threads) t.join();
}
```

### std::counting_semaphore et std::binary_semaphore

Un sémaphore limite le nombre de threads pouvant accéder simultanément à une ressource. `std::binary_semaphore` est un alias pour `std::counting_semaphore<1>`.

```cpp
#include <semaphore>
#include <thread>
#include <print>
#include <vector>

// Limiter à 3 connexions simultanées
std::counting_semaphore<3> connection_pool(3);

void worker(int id) {
    connection_pool.acquire();  // Bloque si les 3 slots sont pris
    std::println("Worker {} : connecté", id);
    do_database_work();
    connection_pool.release();  // Libère un slot
    std::println("Worker {} : déconnecté", id);
}
```

Ces primitives C++20 ne remplacent pas `std::condition_variable` — elles couvrent des patterns spécifiques (synchronisation ponctuelle, synchronisation cyclique, limitation de concurrence) de façon plus expressive et moins error-prone. Si votre besoin correspond exactement à un latch, une barrière ou un sémaphore, utilisez-les plutôt que de réinventer le mécanisme avec des condition variables.

---

## Résumé

| Aspect | Détail |
|--------|--------|
| Header | `<condition_variable>` |
| Wrapper requis | `std::unique_lock<std::mutex>` (pour `condition_variable`) |
| Attente | `wait()`, `wait_for()`, `wait_until()` |
| Notification | `notify_one()` (un thread), `notify_all()` (tous) |
| Prédicat | Toujours utiliser la forme avec prédicat |
| Spurious wakeups | Gérés automatiquement par la forme avec prédicat |
| Lost wakeups | Prévenus par la vérification du prédicat avant la suspension |
| Variante générique | `std::condition_variable_any` (tout type de verrou) |
| Primitives C++20 | `std::latch`, `std::barrier`, `std::counting_semaphore` |
| Règle d'or | Modifier l'état sous verrou, notifier après relâchement |

> **À suivre** : la section 21.4 aborde les opérations atomiques avec `std::atomic` — la synchronisation la plus légère possible, sans mutex, pour les types simples et les patterns lock-free.

⏭️ [Atomiques : std::atomic et memory ordering](/21-threads-concurrence/04-atomiques.md)
