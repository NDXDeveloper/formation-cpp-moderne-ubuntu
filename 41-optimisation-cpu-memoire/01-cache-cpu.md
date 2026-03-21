🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 41.1 — Comprendre le cache CPU et la localité des données

> **Chapitre 41 : Optimisation CPU et Mémoire** · Section 1  
> **Niveau** : Expert · **Prérequis** : Chapitres 5 (mémoire), 13–14 (conteneurs STL), 31 (profiling)

---

## Introduction

Si vous ne deviez retenir qu'un seul concept de tout le chapitre 41, ce serait celui-ci : **sur un processeur moderne, le coût dominant d'un programme n'est presque jamais le calcul — c'est l'accès aux données.**

Un CPU cadencé à 4 GHz exécute une addition entière en moins d'un cycle, soit environ 0,25 ns. Lire un entier depuis la RAM principale prend entre 50 et 100 ns — soit **200 à 400 fois plus longtemps**. Dit autrement, pendant que le processeur attend une seule donnée en mémoire, il aurait pu effectuer des centaines d'opérations arithmétiques. Ce déséquilibre fondamental entre la vitesse du calcul et la vitesse de la mémoire porte un nom : le ***memory wall***.

La réponse architecturale à ce problème est la **hiérarchie de caches** : des mémoires intermédiaires, petites mais extrêmement rapides, intercalées entre le CPU et la RAM. Comprendre leur fonctionnement est la clé pour écrire du C++ performant.

---

## Le memory wall : un problème qui s'aggrave

L'écart de performance entre le processeur et la mémoire n'est pas un phénomène récent — il s'amplifie depuis les années 1980. Les fréquences CPU ont progressé bien plus vite que les latences mémoire. Même avec les technologies DDR5 actuelles, un accès DRAM reste fondamentalement lent comparé à un cycle CPU.

Pour donner une intuition concrète, voici une analogie souvent utilisée dans l'industrie. Si un cycle CPU correspondait à **une seconde**, alors :

| Opération | Latence réelle | Analogie « 1 cycle = 1 seconde » |
|-----------|---------------|-----------------------------------|
| Registre CPU | ~0,25 ns | **1 seconde** |
| Cache L1 | ~1 ns | **4 secondes** |
| Cache L2 | ~4–7 ns | **15–30 secondes** |
| Cache L3 | ~10–30 ns | **1–2 minutes** |
| RAM (DRAM) | ~50–100 ns | **3–7 minutes** |
| SSD NVMe | ~10–25 µs | **11 heures – 1 jour** |
| Disque HDD | ~5–10 ms | **plusieurs mois** |

Cette table illustre un point essentiel : la différence entre un accès L1 et un accès RAM n'est pas marginale. C'est la différence entre *4 secondes* et *plusieurs minutes* à l'échelle humaine. Un programme qui touche constamment la RAM au lieu du cache L1 est un programme qui passe son temps à attendre.

---

## Le cache : principe fondamental

Un cache CPU est une mémoire SRAM (*Static RAM*) intégrée directement sur la puce du processeur. Sa raison d'être repose sur deux propriétés statistiques observées dans la quasi-totalité des programmes :

### Localité temporelle

> **Si une donnée vient d'être accédée, elle a de fortes chances d'être accédée à nouveau dans un futur proche.**

C'est le cas typique d'une variable de boucle, d'un compteur, ou d'un pointeur de structure fréquemment consultée. Le cache conserve les données récemment accédées pour servir les accès suivants sans retourner en RAM.

### Localité spatiale

> **Si une donnée à l'adresse `A` vient d'être accédée, les données aux adresses voisines (`A+1`, `A+2`, …) ont de fortes chances d'être accédées bientôt.**

C'est le cas classique du parcours séquentiel d'un tableau. Lorsque le cache charge une donnée, il ne charge pas un seul octet : il charge un bloc entier de mémoire contiguë appelé ***cache line*** (typiquement 64 octets sur x86). Ainsi, les éléments voisins sont déjà disponibles quand le programme en a besoin.

Ces deux propriétés expliquent pourquoi un `std::vector` parcouru séquentiellement est si rapide : les éléments sont contigus en mémoire, le préchargement (*prefetch*) fonctionne à plein régime, et le taux de *cache hits* approche les 100 %. À l'inverse, parcourir une `std::list` chaînée, dont les nœuds sont dispersés dans le heap, génère un *cache miss* quasi systématique à chaque accès — un scénario catastrophique pour les performances.

---

## Cache hit vs cache miss : l'impact concret

Lorsque le processeur a besoin d'une donnée :

1. **Cache hit** — La donnée est trouvée dans le cache. L'accès est servi en quelques cycles. Le programme continue à pleine vitesse.
2. **Cache miss** — La donnée n'est pas dans le cache. Le processeur doit aller la chercher au niveau suivant de la hiérarchie (L2, L3, ou pire, en RAM). Le pipeline peut se retrouver bloqué (*stall*) en attendant la donnée.

Le ratio entre *hits* et *misses* détermine en grande partie la performance réelle d'un programme. Un taux de *miss* en L1 de seulement 5 % peut déjà dégrader significativement les performances, car chaque miss coûte des dizaines de cycles.

On peut mesurer ce ratio directement avec `perf stat` :

```bash
perf stat -e cache-references,cache-misses,L1-dcache-load-misses ./mon_programme
```

Exemple de sortie sur un parcours séquentiel de `std::vector` :

```
 1 200 000 000  cache-references
     2 400 000  cache-misses       # 0,20% de tous les accès cache
```

Et sur un parcours de `std::list` avec les mêmes données :

```
 1 200 000 000  cache-references
   380 000 000  cache-misses       # 31,7% de tous les accès cache
```

La différence est spectaculaire : 0,2 % de misses contre 31,7 %, pour un même nombre d'éléments et une même opération logique. En temps d'exécution, cela se traduit typiquement par un facteur 5× à 10× entre les deux.

---

## Pourquoi le C++ est particulièrement concerné

D'autres langages (Java, C#, Python) délèguent l'organisation mémoire au runtime ou au *garbage collector*. Le développeur a peu de contrôle sur l'endroit où les objets atterrissent en mémoire, et encore moins sur leur contiguïté.

Le C++ est différent. Le langage donne un **contrôle total** sur :

- **Le placement des données** — stack, heap, allocation custom, `placement new`.  
- **La disposition mémoire des structures** — ordre des champs, padding, alignement (`alignas`).  
- **Le choix des conteneurs** — `std::vector` (contigu) vs `std::list` (dispersé) vs `std::deque` (hybride).  
- **Les patterns d'accès** — AoS (*Array of Structures*) vs SoA (*Structure of Arrays*).  
- **Les allocateurs custom** — pools, arenas, allocateurs alignés.

Ce contrôle est à la fois la force et la responsabilité du développeur C++. Un mauvais choix de layout mémoire peut réduire à néant l'avantage théorique du langage. Un choix éclairé peut produire du code qui s'exécute *plus vite que ce que le compilateur seul aurait pu atteindre*.

---

## Démonstration : le coût d'un mauvais pattern d'accès

Pour ancrer ces concepts, considérons un exemple simple : sommer les éléments d'une matrice 4096×4096 stockée dans un tableau 2D.

```cpp
#include <array>
#include <cstddef>

constexpr std::size_t N = 4096;
// Matrice allouée comme un tableau 2D classique (row-major en C++)
int matrix[N][N];

// Parcours par lignes (row-major) — suit l'ordre mémoire
long long sum_row_major() {
    long long sum = 0;
    for (std::size_t i = 0; i < N; ++i)
        for (std::size_t j = 0; j < N; ++j)
            sum += matrix[i][j];
    return sum;
}

// Parcours par colonnes (column-major) — saute dans la mémoire
long long sum_col_major() {
    long long sum = 0;
    for (std::size_t j = 0; j < N; ++j)
        for (std::size_t i = 0; i < N; ++i)
            sum += matrix[i][j];
    return sum;
}
```

Les deux fonctions effectuent exactement le même nombre d'additions sur exactement les mêmes données. Seul l'ordre de parcours diffère.

**Résultats typiques** (GCC 15, `-O2`, CPU AMD Zen 4) :

| Parcours | Temps | Cache misses L1 |
|----------|-------|-----------------|
| Row-major (par lignes) | ~18 ms | ~0,3 % |
| Column-major (par colonnes) | ~85 ms | ~22 % |

Le parcours par colonnes est **environ 4,7× plus lent**, alors que l'algorithme est identique. La seule différence : le parcours par colonnes saute de `N × sizeof(int)` = 16 384 octets entre deux accès consécutifs, ce qui dépasse largement la taille d'une cache line (64 octets). Chaque accès provoque un miss. Le parcours par lignes, lui, accède à des adresses consécutives — le préchargement matériel fait le reste.

Ce type de micro-benchmark est reproductible avec Google Benchmark et mesurable avec `perf stat`. Nous y reviendrons en détail dans la sous-section 41.1.2 (*cache lines et false sharing*).

---

## Les trois sous-sections

Cette section 41.1 se décompose en trois sous-sections complémentaires :

### 41.1.1 — Cache L1, L2, L3

Fonctionnement détaillé de chaque niveau de cache : taille, latence, politique d'éviction, cache inclusif vs exclusif. Vous apprendrez à dimensionner vos structures de données en fonction des tailles de cache réelles de votre cible matérielle, et à utiliser `lscpu`, `/sys/devices/system/cpu/` et `perf stat` pour interroger la hiérarchie de votre machine.

### 41.1.2 — Cache lines et false sharing

La cache line de 64 octets est l'unité atomique de transfert entre les niveaux de cache et la RAM. Vous verrez comment l'alignement sur les cache lines affecte les performances, et surtout comment le *false sharing* — deux threads modifiant des variables distinctes mais situées sur la même cache line — peut transformer un programme multi-thread performant en cauchemar de contention. Vous apprendrez à le détecter avec `perf c2c` et à le corriger avec `alignas` et le padding.

### 41.1.3 — Data-oriented design

Le *data-oriented design* (DOD) est une philosophie de conception qui place la donnée — et son organisation mémoire — au centre des décisions d'architecture logicielle. Vous découvrirez la transformation AoS → SoA (*Array of Structures* vers *Structure of Arrays*), l'approche ECS (*Entity-Component-System*) popularisée par l'industrie du jeu vidéo, et comment ces techniques s'appliquent bien au-delà du gaming — serveurs haute performance, traitement de données, simulation scientifique.

---

## Outils de diagnostic rapide

Avant de plonger dans les sous-sections, voici les commandes que vous utiliserez tout au long de cette section pour analyser le comportement cache de vos programmes :

```bash
# Connaître la hiérarchie cache de votre machine
lscpu | grep -i cache
# Résultat typique :
#   L1d cache:   32 KiB  (par cœur)
#   L1i cache:   32 KiB  (par cœur)
#   L2 cache:    512 KiB (par cœur)
#   L3 cache:    32 MiB  (partagé)

# Compteurs cache globaux
perf stat -e cache-references,cache-misses,\  
L1-dcache-loads,L1-dcache-load-misses,\  
LLC-loads,LLC-load-misses ./mon_programme  

# Simulation cache détaillée (plus lent, mais précis)
valgrind --tool=cachegrind ./mon_programme  
cg_annotate cachegrind.out.<pid>  

# Détection de false sharing (Linux 4.x+)
perf c2c record ./mon_programme  
perf c2c report  
```

Ces outils seront détaillés et mis en pratique dans les sous-sections qui suivent.

---


⏭️ [Cache L1, L2, L3](/41-optimisation-cpu-memoire/01.1-niveaux-cache.md)
