🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 13 : Conteneurs Séquentiels

## Module 5 : La Bibliothèque Standard (STL) — Niveau Intermédiaire

---

## Présentation

Les conteneurs séquentiels constituent la première grande famille de conteneurs de la Standard Template Library (STL). Leur caractéristique commune est de stocker les éléments dans un **ordre déterminé par le programmeur** : l'ordre d'insertion. Contrairement aux conteneurs associatifs (que nous verrons au chapitre 14), ils n'imposent aucun tri automatique et ne reposent sur aucune clé de recherche.

Choisir le bon conteneur séquentiel est l'une des décisions les plus impactantes en C++ moderne. Un mauvais choix peut dégrader les performances d'un facteur 10 ou plus, non pas à cause de la complexité algorithmique théorique, mais à cause du comportement réel du matériel — en particulier le cache CPU.

---

## Pourquoi un chapitre entier sur les conteneurs séquentiels ?

En C++ moderne, le conteneur que vous choisissez détermine bien plus que la complexité asymptotique de vos opérations. Il influence directement :

- **La localité mémoire** : des éléments contigus en mémoire (comme dans `std::vector`) exploitent le cache CPU de manière optimale, tandis que des éléments dispersés (comme dans `std::list`) provoquent des *cache misses* systématiques.  
- **Le coût des allocations** : chaque nœud d'une `std::list` nécessite une allocation dynamique individuelle, alors qu'un `std::vector` alloue un seul bloc contigu qu'il redimensionne au besoin.  
- **L'invalidation des itérateurs** : certaines opérations (insertion, suppression, redimensionnement) invalident les itérateurs existants. Les règles varient selon le conteneur et constituent une source classique de bugs.  
- **La compatibilité avec les algorithmes STL et les Ranges (C++20)** : certains algorithmes requièrent des catégories d'itérateurs spécifiques (*random access*, *bidirectional*, *forward*).

---

## Les conteneurs séquentiels de la STL

La bibliothèque standard propose six conteneurs séquentiels, auxquels s'ajoute `std::span` (C++20) qui n'est pas un conteneur à proprement parler, mais une **vue non-propriétaire** sur des données contiguës :

| Conteneur | Mémoire | Accès | Insertion/Suppression | Cas d'usage principal |  
|---|---|---|---|---|  
| `std::vector` | Contiguë | O(1) par index | O(1) amorti en fin, O(n) ailleurs | Conteneur par défaut |  
| `std::array` | Contiguë (stack) | O(1) par index | Impossible (taille fixe) | Taille connue à la compilation |  
| `std::deque` | Blocs contigus | O(1) par index | O(1) en début et fin | Files, insertion aux deux extrémités |  
| `std::list` | Nœuds dispersés | O(n) séquentiel | O(1) n'importe où (avec itérateur) | Insertions/suppressions fréquentes au milieu |  
| `std::forward_list` | Nœuds dispersés | O(n) séquentiel (avant uniquement) | O(1) après un itérateur | Liste simplement chaînée, mémoire minimale |  
| `std::span` (C++20) | Vue (non-propriétaire) | O(1) par index | Non applicable | Interface unifiée sur données contiguës |

> 💡 **Règle d'or** : commencez toujours par `std::vector`. Ne changez de conteneur que si un profiling démontre un problème de performance, ou si la sémantique d'un autre conteneur correspond exactement à votre besoin (taille fixe → `std::array`, file double → `std::deque`).

---

## Le principe de contiguïté mémoire

Pour comprendre pourquoi `std::vector` est le conteneur par défaut recommandé, il faut saisir l'importance fondamentale de la **contiguïté mémoire** sur les architectures modernes.

Lorsque le processeur accède à un élément en mémoire, il ne charge pas uniquement cet élément : il charge une **ligne de cache** entière (typiquement 64 octets sur x86_64). Si les éléments suivants se trouvent dans cette même ligne, leur accès sera quasi instantané. C'est exactement ce qui se passe avec `std::vector` et `std::array`.

À l'inverse, dans une `std::list`, chaque nœud est alloué indépendamment sur le heap. Les nœuds sont dispersés en mémoire, et chaque accès au nœud suivant provoque potentiellement un *cache miss* — un accès à la mémoire principale qui est environ 100 fois plus lent qu'un accès au cache L1.

```
std::vector<int> — Mémoire contiguë :
┌─────┬─────┬─────┬─────┬─────┬──────┐
│  10 │  20 │  30 │  40 │  50 │  60  │  ← un seul bloc, cache-friendly
└─────┴─────┴─────┴─────┴─────┴──────┘

std::list<int> — Nœuds dispersés :
┌───────────┐     ┌───────────┐     ┌───────────┐
│ prev │ 10 │────▶│ prev │ 20 │────▶│ prev │ 30 │  ← allocations séparées
│ next │    │◀────│ next │    │◀────│ next │    │     cache misses probables
└───────────┘     └───────────┘     └───────────┘
   0x7f1a...        0x7f3c...        0x7f8e...
```

Cette réalité matérielle explique pourquoi, en pratique, un `std::vector` dans lequel on insère au milieu (opération théoriquement O(n)) est souvent **plus rapide** qu'une `std::list` pour la même opération (théoriquement O(1)) — le coût du déplacement d'éléments contigus en mémoire est largement compensé par la localité de cache.

---

## Critères de choix d'un conteneur séquentiel

Voici les questions à se poser pour choisir le bon conteneur :

**La taille est-elle connue à la compilation ?**
Si oui, `std::array` offre une allocation sur la stack sans surcoût dynamique. C'est le choix le plus performant pour les tableaux de taille fixe.

**Avez-vous besoin d'un accès par index en O(1) ?**
Seuls `std::vector`, `std::array` et `std::deque` offrent un accès aléatoire. Si vous parcourez vos données séquentiellement sans jamais accéder par index, une liste peut être envisagée — mais le point suivant s'applique presque toujours.

**Les insertions/suppressions au milieu sont-elles fréquentes et mesurées comme un goulot ?**
Seulement dans ce cas, et après profiling, une `std::list` peut se justifier. Dans la très grande majorité des situations, `std::vector` reste plus performant même pour des insertions au milieu, grâce à la localité de cache.

**Avez-vous besoin d'insertion en O(1) aux deux extrémités ?**
C'est le cas d'usage classique de `std::deque`. Il offre un bon compromis entre accès par index et insertion efficace en début et fin de séquence.

**Exposez-vous une vue sur des données existantes sans transfert de propriété ?**
`std::span` (C++20) est conçu exactement pour cela. Il remplace les anciennes signatures `(T* ptr, size_t count)` par une interface moderne, type-safe, et sans allocation.

---

## Fil conducteur du chapitre

Ce chapitre explore chaque conteneur séquentiel en détail. Nous commencerons par `std::vector`, le conteneur que tout développeur C++ doit maîtriser parfaitement, puis nous examinerons les alternatives pour comprendre quand et pourquoi s'en écarter.

Nous terminerons par une analyse de la complexité algorithmique (Big O) appliquée au choix de conteneur, en rappelant que la théorie ne suffit pas : seul le profiling sur des données réelles tranche les débats de performance.

---

## Prérequis

Avant d'aborder ce chapitre, assurez-vous de maîtriser :

- Les **templates** de base (chapitre 16, sections 16.1 et 16.2) — les conteneurs STL sont des templates.  
- La **gestion de la mémoire** (chapitre 5) — comprendre stack vs heap, allocation dynamique.  
- La **sémantique de mouvement** (chapitre 10) — `std::move`, move constructors — essentielle pour insérer efficacement des objets dans les conteneurs.  
- Les **smart pointers** (chapitre 9) — en particulier `std::unique_ptr`, car stocker des `unique_ptr` dans un conteneur nécessite la sémantique de mouvement.

---

## Ce que vous saurez faire à la fin de ce chapitre

À l'issue de ce chapitre, vous serez capable de :

- Utiliser `std::vector` avec confiance, en comprenant son fonctionnement interne (capacité, réallocation, amortissement).  
- Choisir le conteneur séquentiel adapté à votre problème en vous appuyant sur des critères concrets.  
- Identifier et éviter les pièges classiques liés à l'invalidation des itérateurs.  
- Utiliser `std::span` pour écrire des interfaces génériques et performantes.  
- Raisonner en complexité algorithmique tout en gardant à l'esprit les réalités matérielles (cache, allocations).

⏭️ [std::vector : Le conteneur par défaut](/13-conteneurs-sequentiels/01-vector.md)
