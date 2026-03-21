🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 42.4 — Lock-free Programming

> **Niveau** : Expert  
> **Prérequis** : Section 21.2 (Synchronisation : mutex, lock_guard), Section 21.4 (Atomiques : std::atomic), Section 42.3 (Memory Ordering et Barrières Mémoire)  
> **Fichiers source** : `42-programmation-bas-niveau/04-lock-free/`

---

## Introduction

La programmation lock-free est l'art de concevoir des structures de données et des algorithmes concurrents qui ne reposent sur aucun verrou (mutex, spinlock, semaphore). Au lieu de sérialiser l'accès à une ressource partagée en bloquant les threads concurrents, les algorithmes lock-free utilisent des opérations atomiques — et en particulier l'opération **compare-and-swap** (CAS) — pour coordonner les modifications de manière non-bloquante.

C'est l'un des sujets les plus exigeants de la programmation système. Un algorithme lock-free correct est souvent contre-intuitif, fragile face aux modifications apparemment anodines, et difficile à tester exhaustivement. Mais quand il est bien conçu et appliqué au bon endroit, il élimine des catégories entières de problèmes — deadlocks, priority inversion, convoying — et peut offrir des gains de performance significatifs dans les scénarios à forte contention.

Cette section pose les fondations : le vocabulaire des garanties de progression, les raisons concrètes de choisir (ou non) une approche lock-free, l'opération CAS qui est au cœur de tout, et les pièges fondamentaux qu'il faut maîtriser avant d'écrire la moindre ligne de code lock-free. Les deux sous-sections qui suivent (42.4.1 et 42.4.2) mettront ces concepts en pratique avec des structures de données complètes et le pattern CAS en détail.

---

## Pourquoi se passer de verrous ?

Les verrous (`std::mutex`, `std::shared_mutex`, spinlocks) fonctionnent remarquablement bien dans la majorité des programmes concurrents. Ils sont compris, testés, et soutenus par des décennies d'expérience. `std::scoped_lock` (C++17) rend même les deadlocks entre plusieurs mutex quasi impossibles quand il est utilisé correctement. Alors pourquoi vouloir s'en passer ?

### Les problèmes inhérents aux verrous

**Deadlock.** Deux threads acquièrent deux verrous dans un ordre différent et se bloquent mutuellement. `std::scoped_lock` atténue ce problème, mais ne l'élimine pas dans tous les cas (verrous acquis à des moments différents du flux de contrôle, verrous cachés dans des callbacks, etc.).

**Priority inversion.** Un thread haute priorité attend un verrou détenu par un thread basse priorité, qui est lui-même préempté par un thread de priorité intermédiaire. Le thread haute priorité est effectivement bloqué par un thread de priorité inférieure. Ce problème a historiquement causé des incidents graves dans des systèmes temps réel — le plus célèbre étant le bug de la sonde Mars Pathfinder en 1997.

**Convoying.** Quand un thread détenant un verrou est deschedulé par le système d'exploitation (préemption, page fault, migration de cœur), tous les threads en attente sur ce verrou sont bloqués jusqu'à ce que le premier thread soit reschedulé. Sous forte contention, ce phénomène crée des « convois » de threads qui avancent par à-coups au lieu de progresser en parallèle.

**Contention et scalabilité.** Un mutex est un point de sérialisation : un seul thread à la fois peut exécuter la section critique. Plus le nombre de cœurs augmente, plus la contention sur le mutex grandit, et le programme peut devenir *plus lent* en ajoutant des threads. C'est l'anti-pattern classique de la scalabilité limitée par Amdahl.

**Composition.** Les verrous ne se composent pas facilement. Combiner deux opérations thread-safe en une seule opération atomique nécessite souvent d'exposer les verrous internes ou d'ajouter un verrou externe englobant — ce qui défait l'encapsulation et augmente la granularité du verrouillage.

### Ce que le lock-free apporte

Un algorithme lock-free élimine structurellement les deadlocks (il n'y a pas de verrou à acquérir dans le mauvais ordre) et la priority inversion (aucun thread ne détient une ressource exclusive). Il résiste au convoying : si un thread est préempté au milieu d'une opération lock-free, les autres threads peuvent continuer à progresser — le thread préempté réessaiera simplement son opération quand il reprendra.

En termes de performance, le bénéfice est surtout visible dans les scénarios à forte contention où de nombreux threads accèdent simultanément à la même structure. Un compteur atomique incrémenté par 64 threads sur une machine 64 cœurs sera toujours plus rapide qu'un compteur protégé par un mutex, car les opérations atomiques exploitent les protocoles de cohérence de cache (MESI) sans passer par le système d'exploitation.

### Ce que le lock-free ne résout PAS

**La performance n'est pas automatiquement meilleure.** Sous faible contention (peu de threads, accès peu fréquents), un `std::mutex` peut être plus rapide qu'un algorithme lock-free sophistiqué, car le cas non-contended d'un mutex moderne est très peu coûteux (un simple `compare_exchange` sur un mot en espace utilisateur, sans syscall).

**La complexité explose.** Un algorithme lock-free correct est considérablement plus difficile à concevoir, implémenter, tester et maintenir qu'un algorithme à verrous. Les bugs sont non-déterministes, dépendent de l'architecture matérielle, et peuvent ne se manifester que sous une charge très spécifique.

**La starvation reste possible.** Un algorithme lock-free garantit que *au moins un thread* progresse à tout moment, mais un thread individuel peut théoriquement être retardé indéfiniment si d'autres threads le « dépassent » systématiquement dans la boucle CAS. Seuls les algorithmes *wait-free* (voir ci-dessous) garantissent que chaque thread termine en un nombre borné d'étapes.

---

## Garanties de progression

La terminologie des algorithmes concurrents distingue plusieurs niveaux de garanties de progression. Cette hiérarchie est fondamentale pour comprendre ce que « lock-free » signifie précisément.

### Wait-free

**Définition** : chaque thread complète son opération en un nombre borné d'étapes, indépendamment du comportement des autres threads.

C'est la garantie la plus forte. Un algorithme wait-free n'a pas de boucle de réessai (retry loop) — chaque appel réussit en temps borné. En pratique, les algorithmes wait-free sont rares et souvent plus lents que leurs équivalents lock-free, car la borne supérieure sur le nombre d'étapes implique un mécanisme d'aide mutuelle (*helping*) entre threads qui ajoute de la complexité et du surcoût.

Exemples : un compteur atomique incrémenté via `fetch_add` est wait-free — l'instruction matérielle `LOCK XADD` sur x86-64 termine en un nombre borné de cycles. La lecture d'un `std::atomic<int>` est trivialement wait-free.

### Lock-free

**Définition** : si plusieurs threads exécutent des opérations concurrentes, au moins un thread progresse à tout moment. Un thread individuel peut échouer et réessayer, mais le système dans son ensemble fait toujours des progrès.

C'est la garantie la plus courante en pratique. La plupart des structures de données « lock-free » de la littérature sont au sens strict *lock-free* et non *wait-free*. Elles utilisent des boucles CAS (compare-and-swap) qui peuvent échouer si un autre thread a modifié la donnée entre-temps, mais le fait même que le CAS ait échoué signifie qu'un autre thread a réussi — d'où la garantie de progression globale.

### Obstruction-free

**Définition** : un thread seul, exécuté sans contention, complète toujours son opération. Mais en présence de contention, aucun thread n'est garanti de progresser — un *livelock* est possible.

C'est la garantie la plus faible. Un algorithme obstruction-free nécessite généralement un mécanisme de résolution de conflit externe (backoff exponentiel, arbitrage) pour garantir le progrès en pratique.

### Visualisation de la hiérarchie

```
Wait-free           (le plus fort)
    ↑
Lock-free           (standard en pratique)
    ↑
Obstruction-free    (le plus faible)
    ↑
Blocking            (mutex, locks — pas de garantie de progrès si un thread est bloqué)
```

### Implications pratiques

Pour la majorité des systèmes en production, **lock-free** est le niveau visé. Wait-free est réservé aux systèmes temps réel durs (avionique, nucléaire, médical) où une borne supérieure garantie sur le temps de réponse est une exigence réglementaire. Obstruction-free est rarement un objectif en soi — c'est plutôt une propriété émergente d'un algorithme en cours de développement, avant qu'il ne soit rendu lock-free.

---

## L'opération fondamentale : Compare-and-Swap (CAS)

L'opération compare-and-swap est la brique élémentaire de toute programmation lock-free. Elle effectue atomiquement trois actions en une seule instruction matérielle : lire la valeur courante d'un emplacement mémoire, la comparer à une valeur attendue, et si elles sont égales, écrire une nouvelle valeur. Si la comparaison échoue, la valeur courante est renvoyée pour que le thread puisse réessayer.

### CAS en C++ : `compare_exchange_weak` et `compare_exchange_strong`

`std::atomic` expose le CAS via deux méthodes :

```cpp
bool compare_exchange_weak(T& expected, T desired,
                           std::memory_order success,
                           std::memory_order failure);

bool compare_exchange_strong(T& expected, T desired,
                             std::memory_order success,
                             std::memory_order failure);
```

Le comportement est le suivant :

1. Lit atomiquement la valeur courante de l'atomic.
2. Compare cette valeur à `expected`.
3. Si égales : écrit `desired` et retourne `true`.
4. Si différentes : écrit la valeur courante dans `expected` et retourne `false`.

L'étape 4 est cruciale — elle met à jour `expected` avec la valeur réellement observée, ce qui permet au thread de réessayer immédiatement sans refaire un load séparé.

### `weak` vs `strong`

La différence est subtile mais importante :

**`compare_exchange_strong`** : ne retourne `false` que si la valeur courante est réellement différente de `expected`. C'est le comportement intuitif.

**`compare_exchange_weak`** : peut retourner `false` même si la valeur courante est égale à `expected`. C'est un *spurious failure* — un faux échec. Cela se produit sur les architectures qui implémentent le CAS via une paire d'instructions LL/SC (*Load-Linked / Store-Conditional*), comme ARM et RISC-V. L'instruction SC peut échouer pour des raisons matérielles sans rapport avec la valeur (invalidation de la ligne de cache par un autre cœur, interruption entre LL et SC, etc.).

**Règle de choix** :

- Utilisez `weak` quand le CAS est dans une **boucle de réessai** (le cas le plus courant en lock-free). Le spurious failure est absorbé par la boucle, et `weak` est plus efficace que `strong` sur ARM/RISC-V car il n'a pas besoin de gérer le cas de faux échec en interne.  
- Utilisez `strong` quand le CAS est exécuté **une seule fois** et que le résultat boolean conditionne directement un branchement (pas de boucle de réessai).

### Le pattern CAS-loop

La quasi-totalité des algorithmes lock-free suivent le même schéma : lire l'état courant, calculer le nouvel état, tenter un CAS, et réessayer en cas d'échec :

```cpp
#include <atomic>

std::atomic<int> value{0};

void lock_free_increment() {
    int expected = value.load(std::memory_order_relaxed);
    while (!value.compare_exchange_weak(
                expected,                        // Mis à jour en cas d'échec
                expected + 1,                    // Nouvelle valeur souhaitée
                std::memory_order_release,       // Ordre en cas de succès
                std::memory_order_relaxed)) {    // Ordre en cas d'échec
        // expected a été mis à jour avec la valeur courante
        // On boucle et on réessaie avec la valeur fraîche
    }
}
```

Ce pattern est si courant qu'il mérite d'être mémorisé comme un idiome. Notez que dans cet exemple précis, `fetch_add` serait préférable (il est wait-free et plus concis), mais le CAS-loop est nécessaire pour les cas où la transformation de l'ancien état vers le nouveau est plus complexe qu'une simple addition.

### CAS sur des pointeurs

Le CAS est particulièrement puissant sur les pointeurs, car il permet de modifier atomiquement un lien dans une structure de données. C'est la base des piles, files, listes chaînées et arbres lock-free :

```cpp
#include <atomic>

struct Node {
    int data;
    Node* next;
};

std::atomic<Node*> head{nullptr};

void push(int value) {
    Node* new_node = new Node{value, nullptr};
    new_node->next = head.load(std::memory_order_relaxed);
    while (!head.compare_exchange_weak(
                new_node->next,              // expected = head actuel
                new_node,                    // desired = nouveau nœud
                std::memory_order_release,
                std::memory_order_relaxed)) {
        // Échec : un autre thread a modifié head entre-temps
        // new_node->next a été mis à jour avec le nouveau head
        // On réessaie — new_node->next pointe déjà vers le bon endroit
    }
}
```

Ce code est une pile lock-free (push uniquement — le pop et les détails complets sont traités en section 42.4.1). L'élégance du CAS-loop est que le cas d'échec « gratuit » met à jour `new_node->next` sans code supplémentaire.

---

## Le problème ABA

Le problème ABA est le piège le plus insidieux de la programmation lock-free. Il se produit quand un thread lit une valeur A, est préempté, et quand il reprend, la valeur est toujours A — mais entre-temps, un autre thread a modifié la valeur en B puis l'a ramenée à A. Le CAS réussit (la valeur est bien A comme attendu), mais l'état sous-jacent a changé.

### Illustration concrète

Considérez une pile lock-free avec les opérations push et pop :

```
État initial : head → [A] → [B] → [C] → nullptr
```

1. **Thread 1** commence un `pop()`. Il lit `head == &A` et `A.next == &B`. Il s'apprête à faire `CAS(head, &A, &B)` pour retirer A.
2. **Thread 1 est préempté.**
3. **Thread 2** pop A, pop B, puis push A (avec un `next` différent) :
   ```
   Après pop A, pop B : head → [C] → nullptr
   Après push A :       head → [A] → [C] → nullptr
   ```
4. **Thread 1 reprend.** Il fait `CAS(head, &A, &B)`. La comparaison réussit (`head == &A`, comme attendu). Mais `head` est maintenant assigné à `&B` — un nœud qui a déjà été retiré de la pile et possiblement libéré.

```
Résultat corrompu : head → [B] → ??? (use-after-free)
```

Le CAS ne voit que la valeur du pointeur (l'adresse), pas l'état de la structure de données. L'adresse `&A` est la même, mais le contexte a complètement changé.

### Solutions au problème ABA

Plusieurs techniques existent pour contourner le problème ABA. Aucune n'est gratuite — elles ajoutent toutes de la complexité ou du surcoût :

**Tagged pointers (compteur de version).** On associe un compteur monotone au pointeur. Chaque modification incrémente le compteur. Le CAS porte sur la paire (pointeur, compteur) — même si le pointeur revient à la même adresse, le compteur sera différent.

Sur x86-64, les pointeurs n'utilisent que 48 bits d'adresse sur les 64 disponibles (avec l'extension à 57 bits pour la 5-level paging). Les 16 bits supérieurs (ou le bit de signe et les bits inutilisés) peuvent être utilisés pour stocker un compteur de version. Alternativement, `std::atomic<__int128>` (GCC/Clang avec `-mcx16`) permet un CAS sur 128 bits, donnant 64 bits pour le pointeur et 64 bits pour le compteur :

```cpp
#include <atomic>
#include <cstdint>

struct TaggedPtr {
    Node* ptr;
    uint64_t tag;  // Compteur de version

    bool operator==(const TaggedPtr&) const = default;
};

std::atomic<TaggedPtr> head{TaggedPtr{nullptr, 0}};

void push(Node* new_node) {
    TaggedPtr old_head = head.load(std::memory_order_relaxed);
    TaggedPtr new_head;
    do {
        new_node->next = old_head.ptr;
        new_head = {new_node, old_head.tag + 1};
    } while (!head.compare_exchange_weak(
                old_head, new_head,
                std::memory_order_release,
                std::memory_order_relaxed));
}
```

> ⚠️ Pour que le CAS sur une structure de 128 bits soit lock-free sur x86-64, il faut l'instruction `CMPXCHG16B`. Vérifiez avec `std::atomic<TaggedPtr>::is_lock_free()` et compilez avec `-mcx16`.

**Hazard pointers.** Chaque thread publie les pointeurs qu'il est en train de lire dans un tableau partagé. Avant de libérer un nœud, un thread vérifie qu'aucun autre thread ne le référence dans ses hazard pointers. C'est la technique recommandée par les auteurs de référence (Michael, 2004). C++26 pourrait inclure `std::hazard_pointer` dans le standard.

**Epoch-based reclamation (EBR).** Les threads s'enregistrent dans des « époques ». Un nœud retiré n'est libéré que lorsque tous les threads ont quitté l'époque dans laquelle le nœud était encore accessible. Cette technique est plus simple que les hazard pointers et offre un bon throughput, au prix d'une latence de libération mémoire plus élevée.

**Read-Copy-Update (RCU).** Largement utilisé dans le noyau Linux, RCU permet aux lecteurs d'accéder sans aucune synchronisation à des données partagées, tandis que les écrivains créent une copie, la modifient, puis remplacent atomiquement le pointeur. La libération de l'ancienne version est différée jusqu'à ce que tous les lecteurs aient terminé. C'est extrêmement efficace pour les scénarios à lecteurs multiples et écrivains rares.

### Résumé des solutions ABA

| Technique | Complexité | Surcoût mémoire | Latence de libération | Cas d'usage typique |
|-----------|-----------|------------------|----------------------|---------------------|
| Tagged pointer | Modérée | 8 octets par pointeur | Immédiate | Piles, files simples |
| Hazard pointers | Élevée | O(threads × pointeurs) | Différée (bornée) | Structures lock-free générales |
| Epoch-based (EBR) | Modérée | O(nœuds retirés) | Différée (non bornée) | Lecteurs fréquents |
| RCU | Élevée | O(nœuds retirés) | Différée (époque) | Lecteurs >> écrivains |

---

## Propriétés de `std::atomic` pour le lock-free

Avant de concevoir une structure lock-free, il faut vérifier que les types atomiques utilisés sont réellement lock-free sur l'architecture cible. Un `std::atomic<T>` qui n'est pas lock-free utilise un mutex interne — ce qui défait tout l'objectif.

### `is_lock_free()` et `is_always_lock_free`

```cpp
#include <atomic>
#include <print>

struct SmallData { int x; int y; };  
struct BigData   { int array[100]; };  

void check_lock_free() {
    std::atomic<int> ai;
    std::atomic<int*> ap;
    std::atomic<SmallData> as;
    std::atomic<BigData> ab;

    std::println("int:       {}", ai.is_lock_free());     // true (presque toujours)
    std::println("int*:      {}", ap.is_lock_free());     // true (presque toujours)
    std::println("SmallData: {}", as.is_lock_free());     // true sur x86-64 (8 octets)
    std::println("BigData:   {}", ab.is_lock_free());     // false (trop gros)
}
```

`is_lock_free()` est une vérification à l'exécution. `is_always_lock_free` est un `static constexpr bool` qui permet la vérification à la compilation :

```cpp
static_assert(std::atomic<int>::is_always_lock_free,
              "int doit être lock-free sur cette plateforme");

static_assert(std::atomic<Node*>::is_always_lock_free,
              "Les pointeurs doivent être lock-free");
```

### Taille maximale lock-free

Sur x86-64, les opérations atomiques lock-free sont disponibles pour les types de taille ≤ 8 octets nativement, et ≤ 16 octets avec `CMPXCHG16B` (flag `-mcx16`). Sur ARM aarch64, la limite native est 16 octets grâce aux instructions `LDXP`/`STXP` (Load/Store Exclusive Pair).

Tout type supérieur à cette limite sera implémenté avec un spinlock interne par la bibliothèque standard — il sera atomique mais pas lock-free.

### `std::atomic_ref` (C++20)

C++20 a introduit `std::atomic_ref<T>`, qui permet d'effectuer des opérations atomiques sur un objet existant sans le déclarer comme `std::atomic<T>`. C'est utile pour appliquer des opérations atomiques sur des données qui ne sont pas toujours accédées de manière concurrente :

```cpp
#include <atomic>

struct SharedState {
    int counter;  // Pas un std::atomic — accès atomique seulement quand nécessaire
};

void concurrent_increment(SharedState& state) {
    std::atomic_ref<int> ref(state.counter);
    ref.fetch_add(1, std::memory_order_relaxed);
}
```

> ⚠️ L'objet référencé par `atomic_ref` doit respecter les contraintes d'alignement de `std::atomic<T>`. Sur certaines architectures, `std::atomic<int>` requiert un alignement supérieur à celui de `int`. Utilisez `alignas(std::atomic_ref<int>::required_alignment)` si nécessaire.

---

## Quand utiliser le lock-free — et quand ne pas l'utiliser

Le lock-free est un outil de dernier recours, pas un objectif en soi. Voici un arbre de décision pragmatique :

**Étape 1 : `std::mutex` suffit-il ?** Dans la majorité des programmes, oui. Un `std::mutex` moderne en espace utilisateur (futex sur Linux) est très rapide dans le cas non-contended (un seul CAS). Si le profiling ne montre pas de contention significative sur le mutex, arrêtez-vous là.

**Étape 2 : la contention est-elle le problème ?** Si le profiling montre que les threads passent une fraction significative de leur temps à attendre un verrou, envisagez d'abord des solutions architecturales : réduire la granularité du verrou (un mutex par partition plutôt qu'un mutex global), utiliser `std::shared_mutex` pour les lectures concurrentes, ou revoir le design pour réduire le partage de données entre threads.

**Étape 3 : le scénario justifie-t-il la complexité ?** Le lock-free se justifie quand la latence au percentile 99.9 importe plus que le throughput moyen (finance haute fréquence), quand un deadlock est inacceptable (systèmes critiques), quand le convoying est un problème mesuré (serveurs à très haute charge), ou quand la scalabilité sur 64+ cœurs est un objectif. Si aucun de ces critères ne s'applique, un mutex bien placé sera plus correct, plus maintenable et souvent plus rapide.

**Étape 4 : utilisez une implémentation existante.** Avant d'écrire votre propre structure lock-free, cherchez une implémentation éprouvée. Les bibliothèques comme Folly (Facebook), Boost.Lockfree, libcds (Concurrent Data Structures), ou moodycamel::ConcurrentQueue offrent des structures lock-free testées en production à grande échelle. Écrire un algorithme lock-free correct est extraordinairement difficile — même les publications académiques contiennent parfois des erreurs découvertes des années après.

---

## Outils de vérification

Le test d'un algorithme lock-free ne peut pas reposer uniquement sur des exécutions répétées. Un bug qui ne se manifeste qu'avec un entrelacement spécifique sur une architecture spécifique sous une charge spécifique peut passer des millions d'exécutions sans apparaître.

### ThreadSanitizer (TSan)

Indispensable comme première ligne de défense. TSan détecte les data races, mais ne vérifie pas la logique de l'algorithme lock-free lui-même. Un algorithme qui utilise correctement `std::atomic` mais qui a un bug logique (ABA, perte de nœuds, duplication) ne sera pas détecté par TSan.

```bash
g++ -std=c++23 -O2 -g -fsanitize=thread -o test_lockfree test_lockfree.cpp
```

### Stress testing

Exécutez l'algorithme sous une charge maximale avec un nombre de threads égal (ou supérieur) au nombre de cœurs physiques, pendant des heures. Variez les paramètres : nombre de threads, ratio lecteurs/écrivains, taille des opérations, affinité CPU. Les bugs lock-free se manifestent souvent uniquement sous contention extrême.

### Model checking avec Relacy

Relacy (mentionné en section 42.3) est l'outil de référence pour la vérification formelle d'algorithmes lock-free en C++. Il explore tous les entrelacements possibles et tous les réordonnancements mémoire autorisés par le modèle C++. C'est le moyen le plus fiable de prouver qu'un algorithme lock-free est correct — ou de trouver le contre-exemple qui montre qu'il ne l'est pas.

### Tests sur ARM

Comme pour le memory ordering (section 42.3), tester sur une architecture à modèle mémoire faible est essentiel. Un algorithme qui utilise `relaxed` là où `acquire`/`release` est nécessaire fonctionnera sur x86-64 (grâce au TSO) mais plantera sur ARM.

---

## Plan des sous-sections

Les deux sous-sections qui suivent mettent en pratique les concepts introduits ici :

**[42.4.1 — Structures Lock-free](/42-programmation-bas-niveau/04.1-structures-lock-free.md)**
Implémentation détaillée d'une pile (stack) lock-free et d'une file (queue) lock-free. Gestion de la mémoire dans le contexte lock-free. Patterns d'aide mutuelle (*helping*) et de retrait différé.

**[42.4.2 — Compare-and-Swap (CAS)](/42-programmation-bas-niveau/04.2-compare-and-swap.md)**
Approfondissement du CAS : double-CAS (DCAS), CAS sur des structures complexes, implémentation de compteurs et accumulateurs lock-free, et techniques avancées (descriptors, multi-word CAS émulé).

---

## Résumé

La programmation lock-free est un outil puissant mais exigeant, qui élimine les deadlocks et le convoying au prix d'une complexité de conception considérable. Les points essentiels à retenir avant d'aborder les implémentations concrètes :

- Lock-free signifie « au moins un thread progresse à tout moment » — pas « tous les threads progressent ». La garantie individuelle est wait-free, qui est plus forte et plus coûteuse.  
- L'opération compare-and-swap (CAS) est la brique fondamentale. Le CAS-loop (lire, calculer, tenter CAS, réessayer) est le pattern universel. Préférez `compare_exchange_weak` dans les boucles de réessai.  
- Le problème ABA est le piège majeur du CAS sur des pointeurs. Les tagged pointers (compteur de version) sont la solution la plus simple ; les hazard pointers et l'epoch-based reclamation sont nécessaires pour la gestion mémoire des nœuds retirés.  
- Vérifiez que vos types atomiques sont réellement lock-free avec `is_always_lock_free`. Un `std::atomic<T>` trop gros utilise un mutex interne.  
- Utilisez des implémentations existantes (Folly, Boost.Lockfree, libcds, moodycamel) avant d'écrire les vôtres. Validez avec TSan, Relacy, des stress tests prolongés, et des tests sur ARM.  
- Le lock-free ne se justifie que quand le profiling montre un problème de contention, de latence extrême, ou de scalabilité que les verrous ne peuvent pas résoudre.

> 📎 *La sous-section suivante, **42.4.1 — Structures Lock-free**, implémente une pile et une file lock-free complètes, en appliquant les concepts de CAS, de memory ordering et de gestion du problème ABA vus dans cette introduction.*

⏭️ [Structures lock-free](/42-programmation-bas-niveau/04.1-structures-lock-free.md)
