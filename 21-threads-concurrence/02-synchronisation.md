🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 21.2 Synchronisation

## Le problème fondamental

Dès que deux threads accèdent à la même donnée et qu'au moins un des accès est une écriture, vous avez un problème de synchronisation. Sans mécanisme de protection, le résultat est une **data race** — un comportement indéfini selon le standard C++. Pas une valeur incorrecte, pas un résultat imprévisible : un programme dont le comportement entier est invalide, et sur lequel le compilateur ne fait plus aucune garantie.

Considérons l'exemple le plus simple possible :

```cpp
#include <thread>
#include <print>

int counter = 0;

void increment() {
    for (int i = 0; i < 1'000'000; ++i) {
        ++counter;
    }
}

int main() {
    std::thread t1(increment);
    std::thread t2(increment);
    t1.join();
    t2.join();
    std::println("counter = {}", counter);  // Devrait être 2'000'000... mais non.
}
```

Sur une machine typique, ce programme affiche des valeurs comme 1'243'817, 1'589'002 ou parfois même 2'000'000 — le résultat change à chaque exécution. L'opération `++counter` semble atomique dans le code source, mais elle se décompose en trois étapes au niveau machine : **lire** la valeur courante, **incrémenter** en registre, **écrire** le résultat. Quand deux threads exécutent ces trois étapes de façon entrelacée, des mises à jour sont perdues.

---

## Pourquoi les data races sont si dangereuses

Le piège des data races est qu'elles ne se manifestent pas systématiquement. Un programme avec une data race peut fonctionner parfaitement pendant des mois sur votre machine de développement, puis crasher en production sous une charge plus élevée, sur un processeur avec plus de cœurs, ou après une mise à jour du compilateur qui a changé les optimisations.

Le compilateur est en droit de supposer qu'il n'y a pas de data race (puisque c'est un comportement indéfini). Il peut donc réordonner les accès mémoire, les mettre en cache dans un registre, ou les éliminer entièrement. Voici un exemple classique :

```cpp
// Thread 1
bool ready = false;   // Variable partagée, pas de synchronisation

void producer() {
    data = compute();  // Prépare les données
    ready = true;      // Signale que c'est prêt
}

// Thread 2
void consumer() {
    while (!ready) {}  // Attend le signal
    use(data);         // Utilise les données
}
```

Ce code a l'air logique, mais il contient deux problèmes graves :

1. **Réordonnancement** : le compilateur (ou le CPU) peut réordonner `data = compute()` et `ready = true`. Le consommateur pourrait voir `ready == true` alors que `data` n'est pas encore écrit.

2. **Visibilité mémoire** : même sans réordonnancement, le thread consommateur pourrait ne jamais voir la mise à jour de `ready` car le compilateur a le droit de cacher la valeur dans un registre et de ne jamais relire la mémoire.

La synchronisation résout ces deux problèmes en établissant des **barrières mémoire** qui garantissent à la fois l'ordre des opérations et la visibilité des écritures entre threads.

---

## Les primitives de synchronisation en C++

Le standard C++ offre plusieurs niveaux de primitives, du plus lourd au plus léger :

### Exclusion mutuelle (mutex)

Le mécanisme le plus courant. Un **mutex** (mutual exclusion) garantit qu'un seul thread à la fois peut exécuter une section de code protégée (la *section critique*). Quand un thread verrouille un mutex, tout autre thread qui tente de le verrouiller est mis en attente jusqu'à ce que le premier le libère.

C++ fournit plusieurs types de mutex, adaptés à différents besoins :

| Type | Standard | Usage |
|------|----------|-------|
| `std::mutex` | C++11 | Exclusion mutuelle simple |
| `std::recursive_mutex` | C++11 | Peut être verrouillé plusieurs fois par le même thread |
| `std::timed_mutex` | C++11 | Supporte le verrouillage avec timeout |
| `std::shared_mutex` | C++17 | Lecteurs multiples / écrivain unique (reader-writer lock) |

### Wrappers RAII pour les mutex

Verrouiller et déverrouiller manuellement un mutex est dangereux : un oubli de `unlock()`, une exception entre le `lock()` et le `unlock()`, et c'est le deadlock assuré. C++ fournit des wrappers RAII qui automatisent le déverrouillage :

| Wrapper | Standard | Caractéristique principale |
|---------|----------|---------------------------|
| `std::lock_guard` | C++11 | Verrouillage simple, non-déplaçable, non-déverrouillable manuellement |
| `std::unique_lock` | C++11 | Verrouillage flexible : déverrouillage anticipé, déplacement, compatible condition variables |
| `std::scoped_lock` | C++17 | Verrouillage simultané de plusieurs mutex sans risque de deadlock |

Le principe est toujours le même : le constructeur verrouille, le destructeur déverrouille. Tant que l'objet est en vie, le mutex est tenu. Quand il sort de portée — que ce soit par un retour normal, un `break`, ou une exception — le mutex est automatiquement libéré.

```cpp
// Principe RAII appliqué aux mutex — pseudo-code
{
    WrapperRAII lock(mon_mutex);   // Constructeur → mutex.lock()
    // ... section critique ...
}                                   // Destructeur → mutex.unlock()
// Le mutex est libéré, même si une exception est levée dans la section critique
```

### Variables de condition

Les mutex protègent l'accès aux données, mais ne permettent pas à un thread d'**attendre** qu'une condition soit remplie. C'est le rôle de `std::condition_variable`, qui permet à un thread de se suspendre efficacement jusqu'à ce qu'un autre thread le réveille. Elle est couverte en détail dans la **section 21.3**.

### Opérations atomiques

Pour les cas simples (compteurs, flags, pointeurs), `std::atomic` offre des opérations garanties atomiques sans verrou (lock-free sur la plupart des architectures). C'est la solution la plus performante quand elle s'applique, mais elle se limite à des opérations individuelles sur des types simples. Voir la **section 21.4**.

---

## Section critique : un concept central

Le terme **section critique** désigne toute portion de code qui accède à une ressource partagée et qui doit être exécutée par un seul thread à la fois. La qualité d'un code concurrent se mesure en grande partie à la façon dont les sections critiques sont gérées :

- **Trop larges** : si vous verrouillez un mutex autour de tout un traitement incluant des I/O réseau, vous sérialisez vos threads et perdez tout le bénéfice de la concurrence.

- **Trop étroites** : si vous multipliez les verrous fins autour de chaque variable, vous augmentez la complexité, le risque de deadlock, et les allers-retours d'acquisition/relâchement de mutex.

- **Bien calibrées** : protéger exactement ce qui doit l'être, et rien de plus. Garder les sections critiques courtes et prévisibles.

```cpp
// ❌ Section critique trop large
{
    std::lock_guard lock(mtx);
    auto data = fetch_from_network();  // Bloque pendant des millisecondes
    process(data);                     // Calcul local
    results.push_back(data);           // La seule opération qui a besoin du mutex
}

// ✅ Section critique minimale
auto data = fetch_from_network();  // Pas besoin du mutex ici  
process(data);                     // Pas besoin non plus  
{
    std::lock_guard lock(mtx);
    results.push_back(data);       // Seule la modification partagée est protégée
}
```

---

## Le spectre du deadlock

Un **deadlock** se produit lorsque deux threads (ou plus) s'attendent mutuellement, chacun détenant un verrou dont l'autre a besoin. Le programme se fige indéfiniment — aucune exception n'est levée, aucun crash ne se produit, le programme est simplement immobile.

Le scénario classique implique deux mutex :

```
Thread A : verrouille mutex_1, puis tente de verrouiller mutex_2  
Thread B : verrouille mutex_2, puis tente de verrouiller mutex_1  
→ Deadlock : A attend B, B attend A, ni l'un ni l'autre ne progresse.
```

Les quatre conditions nécessaires au deadlock (conditions de Coffman) sont :

1. **Exclusion mutuelle** : la ressource ne peut être détenue que par un seul thread.
2. **Détention et attente** : un thread détient une ressource tout en attendant d'en obtenir une autre.
3. **Pas de préemption** : un verrou ne peut pas être retiré de force à un thread.
4. **Attente circulaire** : une chaîne de threads où chacun attend une ressource détenue par le suivant dans la chaîne.

Pour prévenir les deadlocks :

- **Ordonnez les verrous** : si tous les threads verrouillent toujours les mutex dans le même ordre global, l'attente circulaire est impossible.
- **Utilisez `std::scoped_lock`** (C++17) : il verrouille plusieurs mutex simultanément avec un algorithme d'évitement de deadlock intégré (section 21.2.4).
- **Minimisez la durée de détention** : plus un mutex est tenu longtemps, plus le risque de conflit augmente.
- **Évitez les verrous imbriqués** quand c'est possible : un seul mutex à la fois élimine toute possibilité de deadlock.

---

## Granularité du verrouillage

Le choix de la granularité de verrouillage est l'un des dilemmes fondamentaux de la programmation concurrente :

**Un seul gros verrou** (*coarse-grained locking*) protège l'ensemble d'une structure ou d'un sous-système. C'est simple à raisonner et rarement source de deadlock, mais cela limite la concurrence : si un thread modifie une entrée dans un `map`, tous les autres threads qui veulent lire ou modifier d'autres entrées sont bloqués.

**Des verrous fins** (*fine-grained locking*) protègent individuellement des éléments plus petits (par exemple un verrou par bucket dans une table de hachage). Cela maximise la concurrence, mais augmente la complexité, le risque de deadlock, et la consommation mémoire liée aux mutex.

En pratique, commencez par un verrouillage grossier. Ne passez à une granularité plus fine que si le profiling (avec `perf` ou l'observation de la contention mutex) révèle un goulot d'étranglement. L'optimisation prématurée de la concurrence est l'une des sources les plus prolifiques de bugs subtils.

---

## Plan des sous-sections

Les quatre sous-sections suivantes couvrent les primitives de verrouillage du plus simple au plus puissant :

- **21.2.1** — `std::mutex` : le verrou de base, verrouillage/déverrouillage manuel, variantes (recursive, timed, shared).
- **21.2.2** — `std::lock_guard` : le wrapper RAII minimal — verrouille à la construction, déverrouille à la destruction.
- **21.2.3** — `std::unique_lock` : le wrapper flexible — déverrouillage anticipé, verrouillage différé, déplaçable, et seul wrapper compatible avec `std::condition_variable`.
- **21.2.4** — `std::scoped_lock` (C++17) : le wrapper multi-mutex — verrouille plusieurs mutex simultanément avec prévention intégrée du deadlock.

Pour chaque primitive, vous verrez son fonctionnement interne, ses cas d'usage, et les erreurs classiques à éviter.

⏭️ [std::mutex](/21-threads-concurrence/02.1-mutex.md)
