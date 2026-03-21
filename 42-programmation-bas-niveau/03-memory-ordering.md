🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 42.3 — Memory Ordering et Barrières Mémoire

> **Niveau** : Expert  
> **Prérequis** : Chapitre 21 (Threads et Programmation Concurrente), Section 21.4 (Atomiques : std::atomic), Section 42.1 (Inline Assembly)  
> **Fichiers source** : `42-programmation-bas-niveau/03-memory-ordering/`

---

## Introduction

Quand vous écrivez du code C++ séquentiel, vous tenez pour acquis que les instructions s'exécutent dans l'ordre où vous les avez écrites. C'est une illusion confortable — et fausse. Deux mécanismes indépendants conspirent pour réordonner vos accès mémoire : le **compilateur**, qui réarrange les instructions pour améliorer l'utilisation du pipeline et des registres, et le **processeur**, qui exécute les instructions dans le désordre (*out-of-order execution*) et utilise des buffers d'écriture qui retardent la visibilité des stores vers les autres cœurs.

Dans un programme mono-thread, ce réordonnancement est invisible : le compilateur et le processeur garantissent que le résultat observable est identique à une exécution séquentielle (c'est la règle *as-if*). Mais dès qu'un second thread entre en jeu, le réordonnancement devient un problème réel. Un thread peut observer les écritures d'un autre thread dans un ordre différent de celui dans lequel elles ont été effectuées, ce qui conduit à des bugs subtils et non reproductibles.

Le **modèle mémoire C++**, introduit en C++11, fournit un cadre formel pour raisonner sur ces phénomènes et des outils concrets — les ordres mémoire de `std::atomic` — pour les contrôler. Cette section explore en profondeur ce modèle, les différents niveaux d'ordonnancement disponibles, et leur interaction avec le matériel.

---

## Pourquoi les accès mémoire sont-ils réordonnés ?

Pour comprendre les ordres mémoire, il faut d'abord comprendre *pourquoi* le réordonnancement existe. Il ne s'agit pas d'un bug matériel ou d'une négligence du compilateur — c'est une optimisation fondamentale sans laquelle les processeurs modernes seraient plusieurs fois plus lents.

### Réordonnancement par le compilateur

Le compilateur analyse le flux de données de votre programme et réarrange les instructions pour minimiser les dépendances et maximiser le parallélisme au niveau des instructions (ILP). Considérez ce fragment :

```cpp
int a = 0;  
int b = 0;  

void write_values() {
    a = 1;     // (1)
    b = 2;     // (2)
}
```

Le compilateur est libre d'émettre l'écriture de `b` avant celle de `a`, car dans un contexte mono-thread, l'ordre n'a aucune importance (aucune des deux lignes ne dépend de l'autre). En `-O2`, il pourrait même regrouper les deux stores en un seul accès mémoire si les variables sont adjacentes, ou retarder l'un d'eux pour mieux remplir le pipeline.

Ce réordonnancement respecte la sémantique *as-if* pour le thread courant. Mais si un autre thread lit `a` et `b`, il pourrait voir `b == 2` alors que `a == 0` — une situation impossible si les écritures avaient lieu dans l'ordre du code source.

### Réordonnancement par le processeur

Même si le compilateur émet les instructions dans l'ordre, le processeur peut les réordonner à l'exécution. Les processeurs modernes possèdent plusieurs mécanismes qui produisent ce phénomène :

**Store buffers.** Quand un cœur exécute une instruction d'écriture (`store`), la valeur n'est pas immédiatement écrite dans le cache L1. Elle est placée dans un *store buffer* local au cœur, qui sera vidé vers le cache ultérieurement. Le cœur qui a effectué l'écriture voit immédiatement la nouvelle valeur (grâce au *store forwarding*), mais les autres cœurs ne la voient pas tant que le buffer n'est pas vidé. Cela signifie qu'un store peut devenir visible aux autres cœurs *après* un store ultérieur qui, lui, a été vidé plus tôt du buffer.

**Exécution out-of-order.** Le processeur dispatch les instructions vers ses unités d'exécution dès que leurs opérandes sont disponibles, indépendamment de leur ordre dans le flux d'instructions. Un load dont les données sont déjà en cache peut s'exécuter avant un load précédent qui attend une ligne de cache venant de la mémoire principale.

**Invalidation différée des lignes de cache.** Dans un système multi-cœurs avec cohérence de cache (protocole MESI ou dérivés), l'invalidation d'une ligne de cache sur un cœur distant prend du temps. Pendant cette fenêtre, le cœur distant peut lire une valeur périmée.

### Catégories de réordonnancement

Le réordonnancement se décrit en termes de paires d'opérations. Chaque architecture matérielle autorise ou interdit certaines catégories :

| Réordonnancement | Description | x86-64 | ARM / RISC-V |
|-----------------|-------------|--------|--------------|
| **Store-Store** | Deux écritures réordonnées | ❌ Interdit | ✅ Possible |
| **Load-Load** | Deux lectures réordonnées | ❌ Interdit | ✅ Possible |
| **Load-Store** | Une lecture déplacée après une écriture | ❌ Interdit | ✅ Possible |
| **Store-Load** | Une écriture déplacée après une lecture | ✅ Possible | ✅ Possible |

x86-64 implémente le modèle **Total Store Ordering (TSO)** : le seul réordonnancement autorisé par le matériel est Store-Load (une écriture peut être retardée par rapport à une lecture ultérieure, à cause du store buffer). C'est un modèle relativement « fort » qui cache beaucoup de bugs de synchronisation.

ARM et RISC-V implémentent des modèles mémoire **faibles** (*weakly ordered*) où les quatre catégories de réordonnancement sont possibles. C'est pourquoi un programme concurrent qui fonctionne parfaitement sur x86-64 peut planter immédiatement sur un serveur ARM (Graviton, Apple Silicon) — les bugs de memory ordering qui étaient masqués par le TSO deviennent soudainement visibles.

---

## Le modèle mémoire C++ (C++11)

Avant C++11, le standard C++ n'avait tout simplement aucune notion de threads ni de modèle mémoire. La programmation concurrente reposait entièrement sur des extensions spécifiques au système d'exploitation (pthreads, Win32 threads) et sur des hypothèses non portables sur le comportement du matériel. C++11 a fondamentalement changé la donne en intégrant un modèle mémoire formel directement dans le standard du langage.

### Principes fondamentaux

Le modèle mémoire C++ repose sur trois concepts clés :

**1. Les accès non atomiques sur des données partagées sans synchronisation constituent un data race, et un data race est un comportement indéfini.** Ce n'est pas juste « les résultats sont imprévisibles » — c'est un UB au sens plein du terme. Le compilateur est autorisé à supposer qu'il n'y a pas de data race, ce qui peut conduire à des optimisations qui rendent le programme catastrophiquement incorrect en présence d'un race.

```cpp
int shared_value = 0;  
bool ready = false;  

// Thread 1                        // Thread 2
shared_value = 42;                 if (ready) {  
ready = true;                          use(shared_value);  // DATA RACE → UB  
                                   }
```

Ce code contient un data race même si l'intention est claire. `ready` et `shared_value` sont des variables non atomiques accédées par deux threads sans synchronisation. Le compilateur peut réordonner les écritures dans le thread 1, ou le processeur peut rendre `ready = true` visible avant `shared_value = 42` sur un autre cœur.

**2. Les opérations atomiques (`std::atomic`) sont le mécanisme de base pour la synchronisation inter-threads.** Elles garantissent que l'accès lui-même est indivisible (pas de *torn read/write*) et permettent de spécifier des contraintes d'ordonnancement via les *memory orders*.

**3. L'ordonnancement est un spectre, pas un binaire.** Plutôt que d'offrir un unique mode « tout ordonné » (coûteux) ou « rien ordonné » (inutile), le modèle C++ propose six niveaux d'ordonnancement qui permettent au développeur de spécifier exactement le degré de synchronisation nécessaire — et pas plus.

### La relation *happens-before*

Le concept central du modèle mémoire est la relation **happens-before**. Si une opération A *happens-before* une opération B, alors les effets de A sont garantis d'être visibles par B. Cette relation est transitive.

Plusieurs mécanismes établissent une relation happens-before :

- **Séquencement dans un même thread** : toute instruction qui précède une autre dans le même thread *happens-before* cette dernière.  
- **Synchronization** : un `release` store sur un atomic qui est lu par un `acquire` load du même atomic établit une relation happens-before entre le store et le load (et transitoirement entre tout ce qui précède le store et tout ce qui suit le load).  
- **Création/jonction de thread** : `std::thread::thread()` *happens-before* la première instruction du nouveau thread ; la dernière instruction du thread *happens-before* le retour de `join()`.  
- **Mutex** : `unlock()` *happens-before* le prochain `lock()` réussi sur le même mutex.

---

## Les six ordres mémoire

C++ définit six constantes dans `<atomic>` qui spécifient le degré d'ordonnancement d'une opération atomique. Elles forment un spectre du plus relâché au plus strict.

### `memory_order_relaxed`

L'ordre le plus faible. Garantit uniquement l'atomicité de l'opération elle-même (pas de *torn read/write*), mais n'impose aucune contrainte d'ordonnancement par rapport aux autres opérations, atomiques ou non.

```cpp
#include <atomic>

std::atomic<int> counter{0};

void increment() {
    // Seule l'atomicité du read-modify-write est garantie
    // L'ordre relatif avec les autres opérations mémoire est libre
    counter.fetch_add(1, std::memory_order_relaxed);
}
```

**Cas d'usage** : compteurs statistiques, génération d'identifiants uniques — toute situation où la seule chose qui importe est que l'opération elle-même est atomique, sans besoin de synchroniser d'autres données autour d'elle.

**Ce que `relaxed` ne fait PAS** : il ne crée aucune relation happens-before entre threads. Deux threads qui font des stores relaxed sur des atomics différents ne donnent aucune garantie sur l'ordre dans lequel un troisième thread verra ces stores.

### `memory_order_acquire`

S'applique aux opérations de **lecture** (load). Garantit que toutes les lectures et écritures mémoire qui suivent l'acquire dans le code source ne peuvent pas être réordonnées avant lui. C'est une « barrière vers le bas » : rien ne remonte au-dessus de l'acquire.

```cpp
std::atomic<bool> flag{false};  
int payload = 0;  

// Thread 2 : lecture avec acquire
if (flag.load(std::memory_order_acquire)) {  // (A) acquire load
    // Toutes les lectures/écritures ici sont garanties
    // de voir les effets de tout ce qui précède le release
    // correspondant dans le thread 1
    use(payload);  // (B) garanti de voir payload == 42
}
```

### `memory_order_release`

S'applique aux opérations d'**écriture** (store). Garantit que toutes les lectures et écritures mémoire qui précèdent le release dans le code source ne peuvent pas être réordonnées après lui. C'est une « barrière vers le haut » : rien ne descend en dessous du release.

```cpp
// Thread 1 : écriture avec release
payload = 42;                                   // (C) écriture ordinaire  
flag.store(true, std::memory_order_release);    // (D) release store  
```

**La paire acquire/release** : quand un thread effectue un release store sur un atomic et qu'un autre thread effectue un acquire load sur le même atomic et lit la valeur stockée par le release, une relation happens-before est établie entre (D) et (A). Par transitivité, (C) happens-before (B), ce qui garantit que `payload == 42` est visible dans le thread 2.

C'est le mécanisme fondamental de synchronisation en C++. Voici l'exemple complet :

```cpp
#include <atomic>
#include <thread>
#include <cassert>

std::atomic<bool> ready{false};  
int data = 0;  

void producer() {
    data = 42;                                       // Écriture non-atomique
    ready.store(true, std::memory_order_release);    // Release : publie data
}

void consumer() {
    while (!ready.load(std::memory_order_acquire))   // Acquire : attend ready
        ;                                            // Spin-wait
    assert(data == 42);                              // Toujours vrai ✅
}

int main() {
    std::thread t1(producer);
    std::thread t2(consumer);
    t1.join();
    t2.join();
}
```

Sans les ordres acquire/release (par exemple avec `relaxed` des deux côtés), l'assertion pourrait échouer : le thread 2 pourrait voir `ready == true` mais `data == 0`, car rien ne garantirait que l'écriture de `data` est visible avant la lecture de `ready` sur un autre cœur.

### `memory_order_acq_rel`

Combine acquire et release en une seule opération. S'applique aux opérations **read-modify-write** (RMW) comme `fetch_add`, `exchange`, `compare_exchange_*`. L'opération agit comme un acquire pour la partie lecture et comme un release pour la partie écriture.

```cpp
std::atomic<int> sync_counter{0};

void thread_work() {
    // ... prépare des données partagées ...

    // Le fetch_add agit comme release (publie les données préparées)
    // ET comme acquire (lit les données publiées par d'autres threads
    // qui ont déjà incrémenté le compteur)
    int prev = sync_counter.fetch_add(1, std::memory_order_acq_rel);

    // ... peut lire les données publiées par les 'prev' threads précédents ...
}
```

### `memory_order_seq_cst`

L'ordre le plus fort. C'est le **défaut** quand aucun ordre n'est spécifié. Il fournit toutes les garanties de acquire/release, plus une propriété supplémentaire : toutes les opérations `seq_cst` dans le programme apparaissent dans un **ordre total unique** (un *total order*) cohérent entre tous les threads. Chaque thread observe les opérations `seq_cst` dans le même ordre.

```cpp
std::atomic<bool> x{false};  
std::atomic<bool> y{false};  
int z = 0;  

void thread_a() {
    x.store(true, std::memory_order_seq_cst);  // (1)
}

void thread_b() {
    y.store(true, std::memory_order_seq_cst);  // (2)
}

void thread_c() {
    while (!x.load(std::memory_order_seq_cst))
        ;
    if (y.load(std::memory_order_seq_cst))     // (3) voit y == true ?
        ++z;
}

void thread_d() {
    while (!y.load(std::memory_order_seq_cst))
        ;
    if (x.load(std::memory_order_seq_cst))     // (4) voit x == true ?
        ++z;
}

// Avec seq_cst : z >= 1 est GARANTI à la fin
// Avec acquire/release : z == 0 est POSSIBLE (pas d'ordre total)
```

L'ordre total global de `seq_cst` est la seule raison pour laquelle `z >= 1` est garanti : soit (1) précède (2) dans l'ordre total (auquel cas le thread d voit `x == true`), soit (2) précède (1) (auquel cas le thread c voit `y == true`). Avec acquire/release, chaque thread peut avoir une vue différente de l'ordre des stores, et `z == 0` devient possible.

**Coût de `seq_cst`** : sur x86-64, un store `seq_cst` génère une instruction `MFENCE` ou un `XCHG` (qui a une sémantique de barrière implicite), ce qui vide le store buffer et force la sérialisation. Sur ARM, le coût est encore plus élevé — une barrière `DMB` complète est émise. Dans du code critique en performance, remplacer `seq_cst` par `acquire`/`release` là où l'ordre total n'est pas nécessaire peut apporter un gain mesurable.

### `memory_order_consume`

Théoriquement, `consume` est une version allégée de `acquire` qui ne garantit l'ordonnancement que pour les opérations qui ont une **dépendance de données** avec la valeur lue. En pratique, aucun compilateur majeur ne l'implémente correctement — GCC, Clang et MSVC le traitent tous comme un `acquire`. Le comité C++ a formellement déconseillé son utilisation depuis C++17.

**Recommandation** : n'utilisez jamais `memory_order_consume`. Utilisez `memory_order_acquire` à la place.

---

## Tableau récapitulatif des ordres mémoire

| Ordre | Applicable à | Garantie | Coût relatif |
|-------|-------------|----------|--------------|
| `relaxed` | load, store, RMW | Atomicité seule | Le plus faible |
| `acquire` | load, RMW | Rien ne remonte au-dessus | Faible |
| `release` | store, RMW | Rien ne descend en dessous | Faible |
| `acq_rel` | RMW | Acquire + Release combinés | Moyen |
| `seq_cst` | load, store, RMW | Ordre total global | Le plus élevé |
| `consume` | load | *(Déprécié — traité comme acquire)* | — |

---

## Barrières mémoire explicites : `std::atomic_thread_fence`

Les ordres mémoire sont normalement attachés à une opération atomique spécifique. Mais il est parfois utile de poser une barrière d'ordonnancement indépendamment de toute opération atomique. C'est le rôle de `std::atomic_thread_fence`.

### Fence acquire

Une fence acquire empêche les lectures et écritures qui la suivent d'être réordonnées avant une lecture atomique `relaxed` qui la précède :

```cpp
std::atomic<bool> flag{false};  
int payload = 0;  

void consumer() {
    while (!flag.load(std::memory_order_relaxed))
        ;
    std::atomic_thread_fence(std::memory_order_acquire);  // Barrière acquire
    // À partir d'ici, payload == 42 est garanti
    // (si le producteur a utilisé un release avant de stocker flag)
    use(payload);
}
```

### Fence release

Symétriquement, une fence release empêche les lectures et écritures qui la précèdent d'être réordonnées après un store atomique `relaxed` qui la suit :

```cpp
void producer() {
    payload = 42;
    std::atomic_thread_fence(std::memory_order_release);  // Barrière release
    flag.store(true, std::memory_order_relaxed);
}
```

### Fence `seq_cst`

Une fence `seq_cst` est une barrière complète : elle empêche tout réordonnancement de part et d'autre et s'intègre dans l'ordre total des opérations `seq_cst`.

### Quand utiliser les fences

Les fences sont utiles dans deux situations :

**Amortir le coût sur plusieurs atomics.** Si un thread doit effectuer plusieurs stores relaxed qui doivent tous être visibles avant un certain point, une seule fence release suivie des stores relaxed est plus efficace que plusieurs stores release individuels :

```cpp
// Plutôt que :
data_a.store(1, std::memory_order_release);  
data_b.store(2, std::memory_order_release);  
data_c.store(3, std::memory_order_release);  

// On peut écrire :
data_a.store(1, std::memory_order_relaxed);  
data_b.store(2, std::memory_order_relaxed);  
data_c.store(3, std::memory_order_relaxed);  
std::atomic_thread_fence(std::memory_order_release);  
ready.store(true, std::memory_order_relaxed);  
```

Sur ARM, cela génère une seule instruction de barrière (`DMB`) au lieu de trois.

**Synchroniser du code qui n'utilise pas directement `std::atomic`.** Par exemple, du code legacy ou des interactions avec du C via des variables `volatile` (bien que cette approche soit fragile et déconseillée en code moderne).

---

## Barrière compilateur vs barrière processeur

Il est crucial de distinguer deux types de barrières, car les confondre est une source majeure de bugs :

### Barrière compilateur

Empêche le **compilateur** de réordonner les instructions au-delà de la barrière, mais n'émet aucune instruction machine pour empêcher le réordonnancement par le **processeur**.

```cpp
// Barrière compilateur pure — via inline assembly
asm volatile("" ::: "memory");

// Équivalent avec std::atomic_signal_fence
std::atomic_signal_fence(std::memory_order_acq_rel);
```

`std::atomic_signal_fence` est la barrière compilateur portable du standard C++. Elle est conçue pour la synchronisation entre un thread et un signal handler dans le même thread — pas entre threads différents.

### Barrière processeur (+ compilateur)

Empêche à la fois le compilateur et le processeur de réordonner. C'est ce que font `std::atomic_thread_fence` et les opérations atomiques avec des ordres non-relaxed.

```cpp
// Barrière complète (compilateur + processeur)
std::atomic_thread_fence(std::memory_order_seq_cst);
```

Sur x86-64, cette fence se traduit par une instruction `MFENCE`. Sur ARM, par un `DMB ISH`.

### Erreur classique : utiliser volatile comme barrière

En C++, `volatile` n'est **pas** un mécanisme de synchronisation entre threads. Il empêche le compilateur d'optimiser les accès (suppression de lectures « inutiles », fusion d'écritures successives), mais n'émet aucune barrière processeur et ne garantit aucun ordonnancement entre threads.

```cpp
volatile bool ready = false;  // ❌ N'est PAS thread-safe  
int data = 0;  

void producer() {
    data = 42;
    ready = true;  // Le compilateur ne réordonnera pas (grâce à volatile),
                   // mais le PROCESSEUR peut rendre ready visible avant data
                   // sur un autre cœur (surtout sur ARM)
}
```

`volatile` a un rôle légitime en C++ : l'accès à des registres matériels mappés en mémoire (MMIO) où chaque lecture et écriture doit effectivement atteindre le périphérique. Pour la synchronisation inter-threads, utilisez exclusivement `std::atomic`.

---

## Impact sur les architectures : x86-64 vs ARM

La différence entre les modèles mémoire matériels a des conséquences directes sur le code généré et sur la détectabilité des bugs.

### x86-64 : Total Store Ordering

Le modèle TSO de x86-64 est relativement strict. En pratique, il ne permet qu'un seul type de réordonnancement visible par les logiciels : un store peut être retardé par rapport à un load ultérieur (Store-Load reordering), à cause du store buffer. Cela signifie que :

- Un `acquire` load ne génère aucune instruction supplémentaire sur x86-64 — le matériel fournit cette garantie nativement.  
- Un `release` store ne génère aucune instruction supplémentaire non plus.  
- Seul `seq_cst` store nécessite une instruction de barrière (`MFENCE` ou `XCHG`).

Conséquence perverse : beaucoup de bugs de memory ordering sont **invisibles** sur x86-64 et ne se manifestent que lors du portage sur ARM.

### ARM et RISC-V : modèle faible

Sur ARM (aarch64), le modèle mémoire est faible. Les quatre types de réordonnancement sont possibles. En conséquence :

- Un `acquire` load se traduit par une instruction `LDAR` (Load-Acquire Register).  
- Un `release` store se traduit par une instruction `STLR` (Store-Release Register).  
- Un `seq_cst` utilise `LDAR`/`STLR` plus des barrières `DMB` selon le contexte.  
- Même un simple `relaxed` sur ARM est plus coûteux conceptuellement, car le développeur doit raisonner sur un modèle beaucoup plus permissif.

### Visualiser le code généré

Compiler Explorer (godbolt.org) est l'outil idéal pour comparer le code généré par différentes architectures. Voici ce qu'un simple store/load acquire-release produit :

```cpp
std::atomic<int> x{0};

void store_release(int val) {
    x.store(val, std::memory_order_release);
}

int load_acquire() {
    return x.load(std::memory_order_acquire);
}
```

Sur x86-64 (GCC 15, -O2) :

```nasm
store_release:
    mov  DWORD PTR x[rip], edi    ; Simple store — TSO suffit
    ret

load_acquire:
    mov  eax, DWORD PTR x[rip]    ; Simple load — TSO suffit
    ret
```

Sur ARM aarch64 (GCC 15, -O2) :

```nasm
store_release:
    adrp x1, x
    stlr w0, [x1, :lo12:x]       ; Store-Release (instruction spéciale)
    ret

load_acquire:
    adrp x0, x
    ldar w0, [x0, :lo12:x]       ; Load-Acquire (instruction spéciale)
    ret
```

La différence est éloquente : sur x86-64, acquire et release sont « gratuits » — ce sont de simples `mov`. Sur ARM, ils nécessitent des instructions dédiées avec une sémantique de barrière intégrée.

---

## Patterns pratiques

### Pattern 1 : Publication de données (le plus courant)

Le pattern acquire/release est le pattern de synchronisation le plus fréquent. Un thread prépare des données, puis « publie » un flag atomique. Un autre thread attend le flag, puis lit les données :

```cpp
#include <atomic>
#include <memory>
#include <thread>
#include <cassert>

struct Config {
    std::string server;
    int port;
    bool tls;
};

std::atomic<Config*> global_config{nullptr};

void loader_thread() {
    // Prépare la config (allocations, I/O, parsing...)
    auto* cfg = new Config{"api.example.com", 443, true};

    // Publie le pointeur — release garantit que la construction
    // de Config est visible par quiconque lira ce pointeur via acquire
    global_config.store(cfg, std::memory_order_release);
}

void worker_thread() {
    Config* cfg = nullptr;

    // Attend que la config soit publiée
    while (!(cfg = global_config.load(std::memory_order_acquire)))
        ;

    // Garanti : tous les champs de *cfg sont visibles et cohérents
    assert(cfg->port == 443);  // Toujours vrai ✅
}
```

### Pattern 2 : Compteur de références (relaxed + acq_rel)

Le comptage de références (à la `std::shared_ptr`) est un cas classique d'utilisation mixte des ordres mémoire :

```cpp
#include <atomic>
#include <cstdint>

class RefCounted {
    std::atomic<int32_t> ref_count_{1};

public:
    void add_ref() {
        // Relaxed suffit : l'incrémentation n'a pas besoin
        // de synchroniser d'autres données
        ref_count_.fetch_add(1, std::memory_order_relaxed);
    }

    void release() {
        // Le décrément doit être acq_rel :
        //  - release : les écritures sur l'objet par ce thread
        //    doivent être visibles avant la destruction potentielle
        //  - acquire : si le décrément atteint zéro, ce thread doit
        //    voir toutes les écritures des threads qui ont précédemment
        //    fait un release via leur propre décrément
        if (ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            delete this;
        }
    }
};
```

Ce pattern est exactement celui utilisé par les implémentations de `std::shared_ptr` dans libstdc++ et libc++.

### Pattern 3 : Spin-lock minimaliste (acquire/release)

Un spin-lock est le cas d'école de la synchronisation acquire/release appliquée à un verrou :

```cpp
#include <atomic>

class SpinLock {
    std::atomic<bool> locked_{false};

public:
    void lock() {
        while (locked_.exchange(true, std::memory_order_acquire)) {
            // Spin — en attente active
            // Optionnel : __builtin_ia32_pause() pour réduire la contention
            // (voir section 42.1, instruction 'pause')
        }
        // Acquire : tout ce qui suit voit les écritures
        // du thread qui a fait le dernier unlock (release)
    }

    void unlock() {
        locked_.store(false, std::memory_order_release);
        // Release : toutes les écritures dans la section critique
        // sont visibles par le prochain thread qui fera lock (acquire)
    }
};
```

> ⚠️ Ce spin-lock est volontairement minimaliste pour illustrer acquire/release. Un spin-lock en production devrait utiliser un `test-and-test-and-set` (TTAS) avec backoff exponentiel, ou mieux, `std::mutex` qui cède le CPU au noyau après un temps d'attente. Voir aussi `std::atomic::wait()` (C++20) pour une attente passive.

---

## Vérification et débogage

Les bugs de memory ordering sont parmi les plus difficiles à diagnostiquer en programmation. Ils sont non déterministes, sensibles à la charge du système, au nombre de cœurs, à l'architecture matérielle, et au niveau d'optimisation du compilateur. Voici les outils pour les traquer.

### ThreadSanitizer (TSan)

ThreadSanitizer (section 29.4.3) détecte les data races à l'exécution. Il instrumente chaque accès mémoire et vérifie que toute paire d'accès concurrents sur la même adresse est correctement synchronisée :

```bash
g++ -std=c++23 -O2 -g -fsanitize=thread -o my_program my_program.cpp
./my_program
```

TSan détecte la majorité des data races mais ne vérifie pas la correction de l'ordre mémoire choisi. Un programme qui utilise `relaxed` partout là où `acquire/release` serait nécessaire ne déclenchera pas TSan si les accès sont atomiques — TSan considère que tout accès atomique est synchronisé, quel que soit l'ordre.

### Tests multi-architectures

La méthode la plus fiable pour détecter les bugs de memory ordering est de tester sur ARM. Plusieurs approches sont possibles :

- **QEMU user-mode** : émule un processeur ARM en espace utilisateur sur x86-64. Plus lent que le natif, mais suffisant pour les tests.  
- **Instances cloud ARM** : AWS Graviton, Ampere Altra sur GCP/Azure. Le moyen le plus fiable de tester le comportement réel.  
- **Cross-compilation** : GCC et Clang supportent la cross-compilation vers aarch64 via les toolchains `aarch64-linux-gnu-*`.

```bash
# Cross-compilation vers ARM aarch64
aarch64-linux-gnu-g++ -std=c++23 -O2 -o my_program_arm my_program.cpp

# Exécution via QEMU user-mode
qemu-aarch64 -L /usr/aarch64-linux-gnu ./my_program_arm
```

### Relacy Race Detector

Pour les algorithmes lock-free critiques, **Relacy** est un outil de model checking qui explore systématiquement tous les entrelacements possibles de threads et tous les réordonnancements mémoire autorisés par le modèle C++. Il trouve des bugs que ni TSan ni les tests sur ARM ne révèlent, car il explore l'espace complet des exécutions possibles — pas seulement celles qui se produisent lors d'une exécution donnée.

---

## Guide de choix : quel ordre mémoire utiliser ?

La sélection de l'ordre mémoire correct est un exercice de raisonnement rigoureux. Voici un arbre de décision simplifié :

**Commencez par `seq_cst` (le défaut).** Écrivez d'abord un programme correct avec l'ordre le plus strict. Mesurez la performance. Dans la majorité des cas, la différence est négligeable et `seq_cst` est le bon choix.

**Descendez vers `acquire`/`release` si** le profiling montre que les barrières `seq_cst` sont un goulot (ce qui arrive principalement sur ARM dans des boucles très serrées). Vérifiez que vous n'avez pas besoin de l'ordre total — la question clé est : « Est-ce qu'un troisième thread doit observer les opérations de deux autres threads dans un ordre cohérent ? » Si non, acquire/release suffit.

**Descendez vers `relaxed` uniquement si** l'opération atomique est véritablement indépendante de tout autre état partagé. Les compteurs statistiques et les flags de « meilleur effort » sont les cas typiques.

**N'utilisez jamais `consume`.** Utilisez `acquire` à la place.

> ⚠️ **Règle d'or** : un programme incorrect est infiniment plus lent qu'un programme correct avec `seq_cst`. L'optimisation des ordres mémoire est la dernière étape, après avoir prouvé la correction et mesuré un problème de performance.

---

## Résumé

Le modèle mémoire C++ est le fondement sur lequel repose toute programmation concurrente correcte en C++. Les points essentiels à retenir :

- Le compilateur et le processeur réordonnent les accès mémoire pour des raisons de performance. Sans synchronisation explicite, les threads peuvent observer les modifications des autres threads dans un ordre arbitraire.  
- Un data race sur des variables non atomiques est un comportement indéfini — pas simplement « imprévisible ».  
- Les six ordres mémoire forment un spectre du plus relâché (`relaxed`) au plus strict (`seq_cst`). Le défaut `seq_cst` est correct dans la grande majorité des cas.  
- La paire `acquire`/`release` est le mécanisme de synchronisation fondamental : le release « publie » les écritures, l'acquire les « consomme ».  
- x86-64 masque beaucoup de bugs de memory ordering grâce à son modèle TSO. Tester sur ARM est indispensable pour les algorithmes lock-free.  
- `volatile` n'est pas un mécanisme de synchronisation entre threads. Utilisez `std::atomic`.  
- Commencez toujours par `seq_cst`, mesurez, puis relâchez les contraintes si nécessaire — et seulement si vous pouvez prouver la correction.

> 📎 *La section suivante, **42.4 — Lock-free Programming**, met en application les ordres mémoire pour concevoir des structures de données sans verrous : piles, files et compteurs lock-free, avec le pattern compare-and-swap (CAS) au centre de la conception.*

⏭️ [Lock-free programming](/42-programmation-bas-niveau/04-lock-free.md)
