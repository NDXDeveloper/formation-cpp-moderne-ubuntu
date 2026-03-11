🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 15.7 — Algorithmes parallèles : std::execution policies ⭐

## Chapitre 15 — Algorithmes de la STL

---

## Introduction

Depuis le début de ce chapitre, tous les algorithmes présentés s'exécutent séquentiellement — un seul cœur CPU traite les éléments un par un. C'était la seule option jusqu'à C++17. Or les processeurs modernes ont 8, 16, 32 cœurs ou plus, et la majorité de cette puissance reste inutilisée par du code séquentiel.

C++17 a résolu ce problème de manière élégante : plutôt que de créer de nouveaux algorithmes parallèles, il a ajouté un **premier paramètre optionnel** — une politique d'exécution — aux algorithmes existants. Un seul mot ajouté à un appel `std::sort` ou `std::transform` suffit pour que l'algorithme exploite tous les cœurs disponibles :

```cpp
#include <algorithm>
#include <execution>
#include <vector>

std::vector<int> v = /* ...millions d'éléments... */;

// Séquentiel (comportement classique)
std::sort(v.begin(), v.end());

// Parallèle — un seul mot ajouté
std::sort(std::execution::par, v.begin(), v.end());
```

C'est la promesse des algorithmes parallèles de la STL : **la parallélisation sans réécriture**. Pas de threads à créer, pas de mutex à gérer, pas de synchronisation manuelle. L'algorithme gère tout en interne.

```cpp
#include <algorithm>
#include <numeric>
#include <execution>
#include <vector>
```

---

## Le contexte : la fin du free lunch

Pendant des décennies, les programmes devenaient plus rapides simplement parce que les processeurs montaient en fréquence. Ce « repas gratuit » (free lunch) s'est terminé vers 2005 : les limites physiques ont plafonné les fréquences autour de 4-5 GHz. Les fabricants ont compensé en multipliant les cœurs. Mais un programme séquentiel ne peut utiliser qu'un seul cœur — les autres sont inactifs.

Pour exploiter le matériel moderne, il faut **paralléliser**. Le chapitre 21 couvre la programmation concurrente explicite avec `std::thread`, mutex et atomiques. Mais cette approche est complexe, sujette aux bugs (data races, deadlocks), et exige une expertise significative. Les algorithmes parallèles de la STL offrent une alternative de plus haut niveau : le développeur exprime **quoi** paralléliser, et la bibliothèque gère le **comment**.

---

## Principe de fonctionnement

Quand on passe une politique d'exécution parallèle à un algorithme, l'implémentation découpe automatiquement le travail en morceaux distribués sur plusieurs threads. Le schéma général :

```
                    std::sort(std::execution::par, v.begin(), v.end())
                                        │
                            ┌───────────┼───────────┐
                            │           │           │
                         Thread 1    Thread 2    Thread 3
                         ┌──────┐    ┌──────┐    ┌──────┐
                         │ Tri  │    │ Tri  │    │ Tri  │
                         │ bloc │    │ bloc │    │ bloc │
                         │  1   │    │  2   │    │  3   │
                         └──┬───┘    └──┬───┘    └──┬───┘
                            │           │           │
                            └───────────┼───────────┘
                                        │
                                  Fusion finale
                                        │
                                        ▼
                              Résultat trié complet
```

L'implémentation gère un **pool de threads** interne. Le nombre de threads est typiquement adapté au nombre de cœurs disponibles. Le développeur n'a pas besoin de gérer la création de threads, le partitionnement des données, ni la synchronisation.

---

## Les quatre politiques d'exécution

C++17 définit trois politiques, et C++20 en ajoute une quatrième. Chacune offre un compromis différent entre séquentialité, parallélisme et vectorisation :

| Politique | Constante | Threads multiples | Vectorisation SIMD | Standard |
|---|---|---|---|---|
| Séquentielle | `std::execution::seq` | Non | Non | C++17 |
| Parallèle | `std::execution::par` | Oui | Non | C++17 |
| Parallèle + vectorisée | `std::execution::par_unseq` | Oui | Oui | C++17 |
| Vectorisée (non parallèle) | `std::execution::unseq` | Non | Oui | C++20 |

La sous-section 15.7.1 détaille chacune de ces politiques, leurs garanties et leurs contraintes.

---

## Quels algorithmes sont parallélisables ?

La grande majorité des algorithmes de `<algorithm>` et `<numeric>` acceptent une politique d'exécution en premier paramètre. Plus de 70 algorithmes sont concernés :

**Recherche et inspection** — `for_each`, `find`, `find_if`, `count`, `count_if`, `all_of`, `any_of`, `none_of`, `adjacent_find`, `mismatch`, `equal`…

**Tri et ordonnancement** — `sort`, `stable_sort`, `partial_sort`, `nth_element`, `is_sorted`…

**Transformation et réduction** — `transform`, `reduce`, `transform_reduce`, `inclusive_scan`, `exclusive_scan`, `for_each`…

**Manipulation** — `copy`, `copy_if`, `move`, `fill`, `generate`, `replace`, `replace_if`, `remove`, `remove_if`, `unique`, `reverse`, `rotate`, `partition`…

**Opérations ensemblistes** — `merge`, `set_union`, `set_intersection`, `set_difference`…

**Min/Max** — `min_element`, `max_element`, `minmax_element`…

Quelques algorithmes notables **ne supportent pas** les politiques d'exécution : `std::accumulate` (par conception séquentiel — utiliser `std::reduce` à la place), `std::partial_sum` (utiliser `std::inclusive_scan`), et `std::iota`.

---

## Un aperçu avant les détails

Voici un avant-goût de ce que les algorithmes parallèles permettent, avant d'entrer dans les détails des sous-sections :

```cpp
std::vector<double> data(10'000'000);

// Remplir avec des valeurs aléatoires (séquentiel)
std::mt19937 gen(42);  
std::uniform_real_distribution<double> dist(0.0, 100.0);  
std::generate(data.begin(), data.end(), [&]() { return dist(gen); });  

// Tri parallèle — potentiellement 4-8x plus rapide sur 8 cœurs
std::sort(std::execution::par, data.begin(), data.end());

// Somme parallèle
double total = std::reduce(std::execution::par, data.begin(), data.end(), 0.0);

// Transformation parallèle
std::transform(std::execution::par, data.begin(), data.end(), data.begin(),
    [](double x) { return std::sqrt(x); }
);

// Comptage parallèle
auto n = std::count_if(std::execution::par, data.begin(), data.end(),
    [](double x) { return x > 5.0; }
);
```

Chaque appel est identique à sa version séquentielle, avec un simple `std::execution::par` ajouté en premier argument. La promesse de « parallélisation sans réécriture » est tenue — au moins en surface. Les sous-sections suivantes explorent les nuances, les contraintes et les pièges qui se cachent derrière cette simplicité apparente.

---

## Disponibilité et support des implémentations

Le support des algorithmes parallèles varie selon les implémentations de la bibliothèque standard :

**GCC (libstdc++)** — Supporte les algorithmes parallèles via Intel TBB (Threading Building Blocks) comme backend. Il faut lier la bibliothèque TBB à la compilation :

```bash
# Installation de TBB sur Ubuntu
sudo apt install libtbb-dev

# Compilation avec support parallèle
g++ -std=c++17 -O2 main.cpp -ltbb
```

Sans `-ltbb`, le code compile mais les algorithmes s'exécutent séquentiellement comme fallback silencieux, ce qui est une source de confusion fréquente : le programme fonctionne mais n'est pas réellement parallélisé.

**Clang (libc++)** — Le support est plus récent et a longtemps été incomplet. Les versions récentes de libc++ (Clang 19+) implémentent la majorité des algorithmes parallèles, également via un backend de type thread pool. Vérifiez la documentation de votre version.

**MSVC (Microsoft STL)** — Offre un support complet et mature des algorithmes parallèles depuis Visual Studio 2017, avec son propre pool de threads interne. Aucune dépendance externe nécessaire.

Pour un projet multiplateforme, il est prudent de vérifier que les algorithmes parallèles sont effectivement disponibles et fonctionnels sur toutes les cibles.

---

## Algorithmes parallèles et Ranges : situation actuelle

Comme mentionné en section 15.6, les algorithmes `std::ranges` ne supportent **pas** les politiques d'exécution dans le standard C++20/C++23 :

```cpp
// ✅ Algorithme classique avec politique d'exécution
std::sort(std::execution::par, v.begin(), v.end());

// ✅ Algorithme Ranges sans politique d'exécution
std::ranges::sort(v);

// ❌ Combinaison inexistante dans le standard actuel
// std::ranges::sort(std::execution::par, v);
```

Cette limitation impose un choix pratique : les algorithmes Ranges pour le code séquentiel expressif (projections, composition, sécurité), et les algorithmes classiques pour la parallélisation. L'intégration des deux est un objectif reconnu pour un futur standard, possiblement en lien avec `std::execution` (Senders/Receivers) introduit en C++26.

---

## Quand paralléliser — et quand ne pas le faire

La parallélisation n'est pas toujours bénéfique. Distribuer le travail sur plusieurs threads a un **coût fixe** : création ou réveil des threads du pool, partitionnement des données, synchronisation des résultats. Si le travail total est faible, ce coût dépasse le gain.

Règles empiriques :

- **Moins de ~10 000 éléments** — Le surcoût de la parallélisation domine. Rester séquentiel.
- **10 000 à 100 000 éléments** — Le gain dépend de la complexité de l'opération par élément. Mesurer.
- **Plus de 100 000 éléments** — La parallélisation est presque toujours bénéfique pour les opérations simples (tri, somme, transformation).

Ces seuils sont indicatifs et dépendent du matériel, de l'implémentation et de la complexité de l'opération. La seule façon fiable de savoir si la parallélisation est bénéfique est de **mesurer** avec un benchmark (chapitre 35). Les sous-sections suivantes approfondissent cette question.

---

## Plan des sous-sections

- **15.7.1** — **Politiques d'exécution : `seq`, `par`, `par_unseq`, `unseq`** — Détail de chaque politique, ses garanties, ses contraintes sur le code utilisateur (thread-safety, absence de mutex…), et comment choisir.
- **15.7.2** — **Parallélisation de `std::sort`, `std::transform`, `std::reduce`** — Exemples pratiques et benchmarks sur les algorithmes les plus couramment parallélisés, avec mesures de speedup réel.
- **15.7.3** — **Précautions et limitations pratiques** — Data races, exceptions, surcoût de synchronisation, fallback séquentiel, et les pièges qui transforment une optimisation en bug.

⏭️ [Politiques d'exécution : seq, par, par_unseq, unseq](/15-algorithmes-stl/07.1-politiques-execution.md)
