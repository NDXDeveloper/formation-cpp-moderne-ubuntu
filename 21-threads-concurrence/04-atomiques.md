🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 21.4 Atomiques : std::atomic et memory ordering

## Synchronisation sans verrou

Les mutex sont puissants mais lourds : chaque acquisition implique une opération atomique, et en cas de contention, un appel système et un changement de contexte. Pour les opérations simples — incrémenter un compteur, basculer un flag, mettre à jour un pointeur — ce coût est disproportionné.

`std::atomic` offre une alternative : des opérations garanties **indivisibles** (atomiques) sur des types simples, sans mutex et sans possibilité de data race. Sous le capot, le compilateur émet des instructions matérielles spécifiques (comme `LOCK XADD`, `CMPXCHG` sur x86) qui effectuent l'opération en un seul cycle mémoire indivisible.

```cpp
#include <atomic>
#include <thread>
#include <print>

std::atomic<int> counter{0};

void increment() {
    for (int i = 0; i < 1'000'000; ++i) {
        ++counter;  // Opération atomique — pas de data race
    }
}

int main() {
    std::thread t1(increment);
    std::thread t2(increment);
    t1.join();
    t2.join();
    std::println("counter = {}", counter.load());  // Toujours 2'000'000
}
```

Comparé à la version mutex de la section 21.2.1, ce code est à la fois plus simple et significativement plus performant. Sur un x86-64 typique, un incrément atomique coûte environ 5-15 ns contre 15-25 ns pour une acquisition/relâchement de mutex — et l'écart se creuse sous contention.

---

## Types atomiques

### Types fondamentaux

`std::atomic` est un template qui encapsule un type sous-jacent et garantit que toutes les opérations sont atomiques :

```cpp
#include <atomic>

std::atomic<int> a_int{0};  
std::atomic<long> a_long{0L};  
std::atomic<bool> a_bool{false};  
std::atomic<double> a_double{3.14};   // C++20 pour les flottants  
std::atomic<int*> a_ptr{nullptr};  

// Alias pratiques
std::atomic_int    ai{0};     // = std::atomic<int>  
std::atomic_bool   ab{false}; // = std::atomic<bool>  
std::atomic_size_t as{0};     // = std::atomic<std::size_t>  
```

### Quels types peut-on rendre atomiques ?

Un type `T` peut être utilisé avec `std::atomic<T>` s'il est :

- **Trivialement copiable** (`std::is_trivially_copyable_v<T> == true`) : pas de constructeur de copie personnalisé, pas de destructeur virtuel, pas de pointeurs internes. Le type peut être copié bit-à-bit avec `memcpy`.

En pratique, cela inclut tous les types primitifs (`int`, `double`, `bool`, pointeurs), les enums, et les petites structs POD :

```cpp
struct Point {
    float x, y;
};

// OK : Point est trivialement copiable
std::atomic<Point> a_point{{1.0f, 2.0f}};

struct Complex {
    std::string name;  // std::string n'est PAS trivialement copiable
};

// ❌ Erreur de compilation
// std::atomic<Complex> a_complex;
```

### Lock-free ou pas ?

Un `std::atomic<T>` n'est pas nécessairement lock-free. Si le type est trop grand pour une opération atomique matérielle (typiquement plus de 8 ou 16 octets selon l'architecture), l'implémentation utilise un mutex interne (*spin lock*). Vous pouvez vérifier à la compilation ou à l'exécution :

```cpp
std::atomic<int> ai{0};  
std::atomic<Point> ap{{0, 0}};  

// Vérification à la compilation (C++17)
static_assert(std::atomic<int>::is_always_lock_free,
              "int atomique devrait être lock-free");

// Vérification à l'exécution
std::println("atomic<int> lock-free : {}", ai.is_lock_free());  
std::println("atomic<Point> lock-free : {}", ap.is_lock_free());  
```

Sur x86-64 Linux, les types de 1, 2, 4 et 8 octets sont toujours lock-free. Les types de 16 octets le sont souvent (via `CMPXCHG16B`), mais ce n'est pas garanti partout.

---

## Opérations fondamentales

### load() et store()

Les opérations de base pour lire et écrire une valeur atomique :

```cpp
std::atomic<int> value{42};

int v = value.load();    // Lecture atomique  
value.store(100);        // Écriture atomique  

// Formes implicites (utilisent load/store en interne)
int v2 = value;          // Conversion implicite → load()  
value = 200;             // Affectation → store()  
```

Les formes implicites sont pratiques mais les formes explicites `load()`/`store()` documentent mieux l'intention et permettent de spécifier le memory ordering (voir plus bas).

### exchange()

Écrit une nouvelle valeur et retourne l'ancienne, le tout de manière atomique et indivisible :

```cpp
std::atomic<int> state{0};

int old = state.exchange(1);  // old = 0, state = 1
// Aucun autre thread ne peut voir un état intermédiaire
```

`exchange()` est utile pour implémenter des spin locks, des flags de prise de possession, ou tout mécanisme « swap atomique ».

### Opérations arithmétiques : fetch_add, fetch_sub, etc.

Ces opérations effectuent une lecture-modification-écriture atomique et retournent la valeur **avant** la modification :

```cpp
std::atomic<int> counter{10};

int old = counter.fetch_add(5);   // old = 10, counter = 15  
old = counter.fetch_sub(3);       // old = 15, counter = 12  

// Opérateurs surchargés (ne retournent PAS l'ancienne valeur)
counter += 7;    // counter = 19 (retourne 19, pas 12)  
counter -= 2;    // counter = 17  
++counter;       // counter = 18 (retourne 18 = nouvelle valeur)
counter++;       // counter = 19 (retourne 18 = ancienne valeur)
```

> ⚠️ Attention à la différence : `fetch_add(n)` retourne la valeur **avant** l'ajout, tandis que `operator+=` retourne la valeur **après**. Et `++counter` (pré-incrément) retourne la nouvelle valeur, tandis que `counter++` (post-incrément) retourne l'ancienne. Ces subtilités importent quand vous utilisez la valeur de retour pour prendre des décisions.

### Opérations bit-à-bit : fetch_and, fetch_or, fetch_xor

Disponibles pour les types entiers :

```cpp
std::atomic<unsigned int> flags{0b0000};

flags.fetch_or(0b0010);    // Active le bit 1 → 0b0010  
flags.fetch_and(0b1110);   // Désactive le bit 0 → 0b0010  
flags.fetch_xor(0b0011);   // Toggle bits 0 et 1 → 0b0001  

// Opérateurs
flags |= 0b0100;  
flags &= 0b1111;  
flags ^= 0b0001;  
```

### compare_exchange : la brique fondamentale

`compare_exchange_strong` et `compare_exchange_weak` sont les opérations les plus puissantes des atomiques. Elles implémentent le pattern **Compare-And-Swap (CAS)** :

> « Si la valeur courante est égale à `expected`, remplace-la par `desired` et retourne `true`. Sinon, charge la valeur courante dans `expected` et retourne `false`. Le tout de manière atomique. »

```cpp
std::atomic<int> value{100};

int expected = 100;  
bool success = value.compare_exchange_strong(expected, 200);  
// success = true, value = 200, expected = 100 (inchangé)

expected = 100;  // Mauvaise supposition  
success = value.compare_exchange_strong(expected, 300);  
// success = false, value = 200 (inchangé), expected = 200 (mis à jour)
```

CAS est la brique de base de quasiment tous les algorithmes lock-free. Le pattern d'utilisation typique est une boucle CAS :

```cpp
std::atomic<int> value{0};

void atomic_multiply_by_2() {
    int expected = value.load();
    while (!value.compare_exchange_weak(expected, expected * 2)) {
        // expected a été mis à jour avec la valeur courante.
        // On recalcule la valeur désirée et on réessaie.
    }
}
```

### compare_exchange_weak vs strong

- **`compare_exchange_strong`** : ne retourne `false` que si la valeur ne correspond pas à `expected`. Utilisation garantie en une seule tentative quand la valeur correspond.

- **`compare_exchange_weak`** : peut échouer de manière **spurieuse** — retourner `false` même si la valeur correspond à `expected`. Cet échec spurieux est autorisé car il permet une implémentation plus efficiente sur les architectures LL/SC (*Load-Linked/Store-Conditional*) comme ARM et RISC-V.

En pratique :
- Utilisez **`weak`** dans une **boucle CAS** (la boucle gère naturellement les échecs spurieux).
- Utilisez **`strong`** quand un seul essai suffit ou quand le coût de recalculer la valeur désirée est élevé.

Sur x86-64, les deux variantes ont les mêmes performances (x86 fournit nativement CMPXCHG qui ne souffre pas d'échecs spurieux). La distinction importe surtout sur ARM.

---

## std::atomic<bool> et le pattern flag

Le cas d'usage le plus simple des atomiques est un flag de contrôle partagé entre threads :

```cpp
#include <atomic>
#include <thread>
#include <print>

std::atomic<bool> running{true};

void worker() {
    while (running.load()) {
        do_work();
    }
    std::println("Worker arrêté proprement");
}

int main() {
    std::thread t(worker);

    std::this_thread::sleep_for(std::chrono::seconds(5));
    running.store(false);  // Signal d'arrêt

    t.join();
}
```

Ce pattern est correct et ne nécessite ni mutex ni variable de condition. Le flag atomique garantit que la modification par le thread principal est visible par le worker sans data race.

> 💡 C++20 offre un mécanisme plus structuré pour l'arrêt coopératif via `std::stop_token` et `std::jthread` (section 21.7), mais le flag atomique reste pertinent pour les cas simples ou le code pré-C++20.

### std::atomic_flag : le type atomique le plus primitif

`std::atomic_flag` est le seul type garanti lock-free par le standard. Il ne supporte que `test_and_set()` et `clear()`, ce qui le rend adapté à l'implémentation de spin locks :

```cpp
#include <atomic>

std::atomic_flag lock_flag{};  // Initialisé à false/clear (C++20+)

void spin_lock() {
    while (lock_flag.test_and_set(std::memory_order_acquire)) {
        // Attente active — le flag est déjà set par un autre thread
    }
}

void spin_unlock() {
    lock_flag.clear(std::memory_order_release);
}
```

> ⚠️ Les spin locks sont rarement appropriés en code applicatif. Ils consomment 100% d'un cœur CPU pendant l'attente et dégradent les performances sous contention. Préférez `std::mutex` sauf dans des contextes très spécifiques (sections critiques de quelques nanosecondes, code noyau, systèmes temps réel dur).

Depuis C++20, `std::atomic_flag` supporte aussi `test()` (lecture sans modification) et `wait()`/`notify_one()`/`notify_all()` qui en font un mécanisme de signalisation léger.

---

## Memory ordering : le modèle mémoire C++

C'est la partie la plus complexe et la plus subtile des atomiques. Le memory ordering contrôle **quelles garanties de visibilité** une opération atomique fournit vis-à-vis des opérations mémoire environnantes.

### Pourquoi c'est nécessaire

Les processeurs modernes et les compilateurs réordonnent les accès mémoire pour optimiser les performances. Sur un seul thread, ces réordonnancements sont invisibles — le résultat est toujours « comme si » les instructions s'étaient exécutées dans l'ordre. Mais entre threads, sans barrières mémoire explicites, un thread peut observer les écritures d'un autre thread dans un ordre différent de celui du code source.

```cpp
// Thread 1
data = 42;        // (A)  
flag.store(true); // (B)  

// Thread 2
if (flag.load()) {  // (C)
    use(data);      // (D) — data vaut-il forcément 42 ?
}
```

Sans garanties de memory ordering, le processeur ou le compilateur pourrait réordonner (A) après (B), et le thread 2 verrait `flag == true` alors que `data` n'a pas encore été écrit. Le memory ordering permet de l'empêcher.

### Les six orderings du standard

Le standard C++ définit six niveaux de memory ordering, du plus relâché au plus strict :

```cpp
enum memory_order {
    memory_order_relaxed,
    memory_order_consume,   // Rarement utilisé — voir note ci-dessous
    memory_order_acquire,
    memory_order_release,
    memory_order_acq_rel,
    memory_order_seq_cst    // Par défaut
};
```

---

### memory_order_seq_cst (par défaut)

Le **séquentiellement consistant** (*sequentially consistent*) est l'ordering par défaut de toutes les opérations atomiques. Il garantit un **ordre total global** : toutes les opérations `seq_cst` sont observées dans le même ordre par tous les threads, comme si elles avaient été exécutées séquentiellement sur une seule machine.

```cpp
std::atomic<int> x{0}, y{0};

// Thread 1
x.store(1);  // memory_order_seq_cst implicite

// Thread 2
y.store(1);

// Thread 3
if (x.load() == 1 && y.load() == 0) {
    // Thread 3 a vu x=1 avant y=1
}

// Thread 4
if (y.load() == 1 && x.load() == 0) {
    // Thread 4 a vu y=1 avant x=1
}

// Avec seq_cst : les threads 3 et 4 ne peuvent PAS tous les deux
// entrer dans leur if. L'un des deux ordres (x avant y, ou y avant x)
// est observé de manière cohérente par tous les threads.
```

`seq_cst` est le choix le plus sûr et le plus intuitif. **Utilisez-le par défaut.** Le surcoût par rapport à des orderings plus faibles est nul sur x86-64 pour les loads et les stores, et minime pour les opérations read-modify-write.

> 💡 **Conseil pragmatique** : si vous devez vous demander quel memory ordering utiliser, la réponse est `seq_cst`. Les orderings plus faibles sont des optimisations avancées qui ne se justifient que lorsque le profiling démontre un goulot d'étranglement et que vous maîtrisez parfaitement les implications.

---

### memory_order_acquire et memory_order_release

Ce couple forme le pattern le plus courant après `seq_cst`. Il établit une relation **happens-before** entre un store (release) et un load (acquire) :

- **Release** : toutes les écritures mémoire effectuées *avant* ce store sont garanties visibles...
- **Acquire** : ...par tout thread qui effectue un load *acquire* et observe la valeur écrite par le release.

```cpp
std::atomic<bool> flag{false};  
int data = 0;  // Variable non-atomique  

// Thread 1 (producteur)
data = 42;                                    // (A) — écriture ordinaire  
flag.store(true, std::memory_order_release);  // (B) — publication  

// Thread 2 (consommateur)
while (!flag.load(std::memory_order_acquire)) {}  // (C) — acquisition
// Ici, data == 42 est GARANTI                     // (D)
// Le release en (B) synchronise avec l'acquire en (C),
// donc (A) happens-before (D).
```

C'est exactement le pattern « publier des données puis signaler qu'elles sont prêtes ». Le release est la barrière qui empêche les écritures antérieures de « fuir » après le flag, et l'acquire empêche les lectures postérieures de « remonter » avant le flag.

Acquire/release n'établit **pas** d'ordre total global (contrairement à `seq_cst`). Deux threads tiers pourraient observer les stores release dans des ordres différents. C'est suffisant pour la majorité des patterns producteur/consommateur, et légèrement plus performant que `seq_cst` sur les architectures ARM et RISC-V.

### memory_order_acq_rel

Combine acquire et release en une seule opération read-modify-write. Utilisé pour les opérations comme `fetch_add` ou `compare_exchange` qui lisent et écrivent simultanément :

```cpp
std::atomic<int> counter{0};

// L'incrément est à la fois une acquisition et une publication
counter.fetch_add(1, std::memory_order_acq_rel);
```

---

### memory_order_relaxed

Aucune garantie d'ordering — seulement l'atomicité de l'opération elle-même. Les accès relaxed peuvent être réordonnés librement par le compilateur et le processeur par rapport aux autres opérations mémoire (atomiques ou non).

```cpp
std::atomic<int> counter{0};

void count() {
    // Relaxed : on se fiche de l'ordre par rapport aux autres opérations.
    // On veut juste que l'incrément soit atomique.
    counter.fetch_add(1, std::memory_order_relaxed);
}

int get_count() {
    return counter.load(std::memory_order_relaxed);
}
```

Les cas d'usage légitimes de `relaxed` sont limités :

- Compteurs de statistiques où la précision instantanée n'est pas critique.
- Génération d'identifiants uniques (la seule garantie nécessaire est l'absence de doublon).
- Implémentations de structures lock-free par des experts qui contrôlent précisément les dépendances.

> ⚠️ `memory_order_relaxed` ne fournit **aucune** garantie de synchronisation. Si vous l'utilisez pour communiquer entre threads (« je mets un flag à true, l'autre thread le lit »), les données associées ne sont pas garanties visibles. C'est une source de bugs extrêmement subtils.

---

### memory_order_consume

`memory_order_consume` est une version affaiblie de `acquire` qui ne garantit l'ordering que pour les lectures qui **dépendent de la valeur chargée** (data dependency). En théorie, c'est plus performant sur ARM car il exploite les dépendances matérielles sans barrière.

En pratique, **aucun compilateur majeur n'implémente correctement `consume` en 2026**. Tous le promeuvent silencieusement en `acquire`. Le comité C++ reconnaît le problème et travaille à une reformulation. En attendant, ne l'utilisez pas — utilisez `acquire` à la place.

---

## Guide de choix du memory ordering

```
Suis-je un expert en modèle mémoire C++ ?
├── Non → memory_order_seq_cst (le défaut, ne rien spécifier)
└── Oui
    ├── Pattern producteur/consommateur simple ?
    │   └── release (store) + acquire (load)
    ├── Compteur de stats / ID unique ?
    │   └── relaxed
    ├── Opération read-modify-write synchronisante ?
    │   └── acq_rel
    └── Besoin d'un ordre total observable par tous les threads ?
        └── seq_cst
```

En résumé : **partez toujours de `seq_cst`**. N'affaiblissez l'ordering que si un benchmark prouve que c'est nécessaire, et seulement si vous comprenez précisément les implications.

---

## std::atomic et les types utilisateur

### Petites structures

Les petites structs trivialement copiables peuvent être rendues atomiques :

```cpp
#include <atomic>

struct Config {
    int timeout_ms;
    int max_retries;
};

static_assert(std::is_trivially_copyable_v<Config>);

std::atomic<Config> current_config{{500, 3}};

void update_config(int timeout, int retries) {
    current_config.store(Config{timeout, retries});
}

Config get_config() {
    return current_config.load();
}
```

Si `sizeof(Config)` est ≤ 16 octets sur x86-64, l'opération est typiquement lock-free. Au-delà, l'implémentation utilise un verrou interne — vérifiez avec `is_always_lock_free`.

### std::atomic_ref (C++20)

`std::atomic_ref` permet d'effectuer des opérations atomiques sur une variable **non-atomique existante**, sans changer son type :

```cpp
#include <atomic>

int regular_counter = 0;

void atomic_increment() {
    std::atomic_ref ref(regular_counter);
    ref.fetch_add(1);
}
```

Cela est utile quand vous avez une structure de données existante que vous ne pouvez pas (ou ne voulez pas) modifier pour utiliser `std::atomic`, mais qui nécessite des accès atomiques dans certains contextes.

> ⚠️ Tant qu'un `atomic_ref` existe sur une variable, **tous** les accès à cette variable doivent passer par des `atomic_ref`. Mélanger des accès atomiques et non-atomiques sur la même variable est un comportement indéfini.

---

## Patterns courants

### Compteur thread-safe

Le cas le plus classique — aucun mutex nécessaire :

```cpp
class AtomicCounter {
    std::atomic<int64_t> count_{0};

public:
    void increment() { count_.fetch_add(1, std::memory_order_relaxed); }
    void decrement() { count_.fetch_sub(1, std::memory_order_relaxed); }
    int64_t get() const { return count_.load(std::memory_order_relaxed); }
};
```

`relaxed` est approprié ici si le compteur est utilisé pour des statistiques et qu'on n'a pas besoin de synchroniser d'autres données autour de ses modifications.

### Singleton thread-safe (double-checked locking)

```cpp
#include <atomic>
#include <mutex>
#include <memory>

class Singleton {
    static std::atomic<Singleton*> instance_;
    static std::mutex mtx_;

public:
    static Singleton* get() {
        Singleton* p = instance_.load(std::memory_order_acquire);
        if (!p) {
            std::lock_guard lock(mtx_);
            p = instance_.load(std::memory_order_relaxed);
            if (!p) {
                p = new Singleton();
                instance_.store(p, std::memory_order_release);
            }
        }
        return p;
    }
};
```

> 💡 En pratique, le *Meyers' Singleton* avec une variable `static` locale est plus simple et tout aussi thread-safe depuis C++11 :  
>  
> ```cpp  
> Singleton& Singleton::get() {  
>     static Singleton instance;  // Thread-safe depuis C++11  
>     return instance;  
> }  
> ```

### Spin lock minimaliste

```cpp
class SpinLock {
    std::atomic<bool> locked_{false};

public:
    void lock() {
        while (locked_.exchange(true, std::memory_order_acquire)) {
            // Attente active — hint au CPU pour économiser de l'énergie
            while (locked_.load(std::memory_order_relaxed)) {
                // Boucle interne de lecture seule (évite le bus locking)
            }
        }
    }

    void unlock() {
        locked_.store(false, std::memory_order_release);
    }
};
```

La boucle interne avec `load(relaxed)` est une optimisation appelée *TTAS* (Test-and-Test-and-Set) : elle évite de générer des écritures sur le bus mémoire pendant l'attente, réduisant la contention cache entre cœurs.

---

## Atomiques vs Mutex : guide de décision

| Critère | `std::atomic` | `std::mutex` |
|---------|:-------------:|:------------:|
| Opération unique sur un type simple | ✓ | Surdimensionné |
| Section critique multi-instructions | Insuffisant | ✓ |
| Accès à une structure complexe | ✗ | ✓ |
| Performance sous faible contention | ~5-15 ns | ~15-25 ns |
| Performance sous forte contention | Se dégrade (spin) | Meilleur (suspension) |
| Difficulté d'utilisation correcte | Élevée (memory ordering) | Modérée |
| Composabilité | Faible (une opération à la fois) | Forte (section critique arbitraire) |

**Règle simple** : si votre section critique se réduit à une seule opération sur un type primitif (incrémenter, comparer-et-échanger, lire un flag), utilisez `std::atomic`. Dès qu'il faut protéger plusieurs opérations liées ou des structures complexes, passez au mutex.

Le piège classique est de tenter de protéger une opération multi-étapes avec des atomiques individuels. Chaque opération atomique est indivisible, mais la **séquence** ne l'est pas :

```cpp
std::atomic<int> balance{1000};

void withdraw(int amount) {
    // ❌ Race condition : un autre thread peut modifier balance
    //    entre le load et le store
    if (balance.load() >= amount) {      // Étape 1 : vérification
        balance.fetch_sub(amount);        // Étape 2 : retrait
    }
    // Un autre thread peut retirer entre les étapes 1 et 2,
    // rendant le solde négatif.

    // ✅ Version correcte avec boucle CAS
    int current = balance.load();
    while (current >= amount) {
        if (balance.compare_exchange_weak(current, current - amount)) {
            return;  // Succès atomique
        }
        // current mis à jour automatiquement — on réessaie
    }
    // Solde insuffisant
}
```

---

## Résumé

| Aspect | Détail |
|--------|--------|
| Header | `<atomic>` |
| Types supportés | Types trivialement copiables (entiers, flottants C++20, pointeurs, petites structs) |
| Opérations de base | `load()`, `store()`, `exchange()` |
| Read-modify-write | `fetch_add()`, `fetch_sub()`, `fetch_and()`, `fetch_or()`, `fetch_xor()` |
| Compare-and-swap | `compare_exchange_weak()`, `compare_exchange_strong()` |
| Ordering par défaut | `memory_order_seq_cst` (le plus sûr) |
| Lock-free garanti | Seulement `std::atomic_flag` ; les autres dépendent de la taille et de l'architecture |
| Vérification lock-free | `is_always_lock_free` (compile-time), `is_lock_free()` (runtime) |
| C++20 | `std::atomic_ref`, `atomic<float/double>`, `wait()`/`notify_one()`/`notify_all()` sur tous les atomiques |
| Règle d'or | Utiliser `seq_cst` par défaut ; n'affaiblir que sur preuve de besoin |

> **À suivre** : la section 21.5 présente `std::async` et `std::future`, qui offrent une abstraction de plus haut niveau pour la programmation asynchrone — lancer un calcul en arrière-plan et récupérer son résultat (ou son exception) proprement.

⏭️ [std::async et std::future : Programmation asynchrone](/21-threads-concurrence/05-async-future.md)
