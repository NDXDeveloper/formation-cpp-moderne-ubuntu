🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 14.1 — std::map et std::multimap : Arbres binaires ordonnés

## Module 5 — La Bibliothèque Standard (STL) | Niveau Intermédiaire

---

## Introduction

`std::map` est le conteneur associatif ordonné le plus utilisé de la STL. Il stocke des paires **clé → valeur** dans un ordre trié permanent, et garantit des opérations de recherche, d'insertion et de suppression en **O(log n)**. Son cousin `std::multimap` offre la même interface, mais autorise plusieurs entrées à partager la même clé.

Derrière cette interface se cache une structure de données classique de l'informatique : l'**arbre binaire de recherche auto-équilibré**, presque toujours implémenté sous forme de **Red-Black Tree** dans les bibliothèques standard majeures (libstdc++, libc++, MSVC STL).

---

## Structure interne : le Red-Black Tree

Un Red-Black Tree est un arbre binaire de recherche dans lequel chaque nœud porte une couleur (rouge ou noir) et respecte un ensemble d'invariants qui garantissent que l'arbre reste approximativement équilibré. En conséquence, la hauteur de l'arbre ne dépasse jamais **2 × log₂(n + 1)**, ce qui borne les opérations de recherche et de modification à **O(log n)** dans tous les cas — y compris le pire cas.

Chaque nœud de l'arbre contient :

- la **paire clé/valeur** (`std::pair<const Key, Value>`),
- un pointeur vers le **parent**,
- un pointeur vers le fils **gauche**,
- un pointeur vers le fils **droit**,
- un **bit de couleur** (rouge ou noir).

Cette organisation a une conséquence importante sur l'utilisation mémoire : chaque élément inséré provoque une **allocation individuelle** sur le heap. Pour un `std::map<int, int>`, le surcoût par élément (pointeurs + couleur + overhead d'allocation) est largement supérieur à la taille utile de la paire elle-même. C'est le prix structurel de la flexibilité offerte par les arbres.

```
            [  15:B  ]
           /          \
      [ 10:R ]      [ 20:R ]
      /     \        /     \
   [ 5:B ] [12:B] [17:B] [25:B]

   B = Noir (Black), R = Rouge (Red)
   Clé gauche < Clé parent < Clé droite
```

L'ordre trié est maintenu en permanence : le parcours **in-order** (gauche → nœud → droite) produit les clés dans l'ordre croissant. C'est ce qui rend le parcours ordonné via itérateurs naturellement efficace.

---

## Déclarer et initialiser un std::map

Le header nécessaire est `<map>`. La signature de base est :

```cpp
#include <map>

// map<Key, Value, Compare, Allocator>
// Compare est par défaut std::less<Key>
```

### Initialisation par liste

```cpp
std::map<std::string, int> population {
    {"Paris",    2'161'000},
    {"Lyon",       522'969},
    {"Marseille",  873'076},
    {"Toulouse",   504'078}
};
// Les éléments sont automatiquement triés par clé (ordre alphabétique)
```

### Initialisation vide puis insertion progressive

```cpp
std::map<int, std::string> codes_http;  
codes_http[200] = "OK";  
codes_http[404] = "Not Found";  
codes_http[500] = "Internal Server Error";  
```

### Initialisation depuis une plage d'itérateurs

```cpp
std::vector<std::pair<std::string, double>> raw_data {
    {"alpha", 1.1}, {"gamma", 3.3}, {"beta", 2.2}
};

std::map<std::string, double> sorted_data(raw_data.begin(), raw_data.end());
// sorted_data est trié : alpha, beta, gamma
```

---

## Insertion d'éléments

`std::map` offre plusieurs méthodes d'insertion, chacune avec des nuances importantes.

### operator[] — accès ou création implicite

```cpp
std::map<std::string, int> scores;

scores["Alice"] = 95;   // Crée la paire {"Alice", 95}  
scores["Alice"] = 100;  // Écrase la valeur existante  

int val = scores["Bob"]; // Attention : crée {"Bob", 0} si "Bob" n'existe pas !
```

L'opérateur `[]` est pratique mais dangereux en lecture : il **insère un élément par défaut** si la clé n'existe pas. C'est une source fréquente de bugs subtils, surtout dans des boucles de recherche.

> ⚠️ **Règle pratique** : utilisez `[]` uniquement quand vous *voulez* créer ou écraser une entrée. Pour la recherche, préférez `find()` ou `contains()`.

### insert — insertion sans écrasement

```cpp
std::map<std::string, int> config;

auto [it, success] = config.insert({"timeout", 30});
// success == true : la paire a été insérée

auto [it2, success2] = config.insert({"timeout", 60});
// success2 == false : "timeout" existait déjà, la valeur 30 est conservée
```

`insert` retourne un `std::pair<iterator, bool>` depuis C++17 avec structured bindings. Le booléen indique si l'insertion a eu lieu. Si la clé existe déjà, **rien n'est modifié** — comportement opposé à `operator[]`.

### insert_or_assign (C++17) — insert ou mise à jour explicite

```cpp
auto [it, inserted] = config.insert_or_assign("timeout", 60);
// Si "timeout" existe : met à jour la valeur → 60, inserted == false
// Si "timeout" n'existe pas : insère {"timeout", 60}, inserted == true
```

`insert_or_assign` combine le meilleur des deux mondes : il est explicite dans son intention et retourne l'information d'insertion, sans le piège de la construction par défaut de `operator[]`.

### emplace — construction en place

```cpp
std::map<std::string, std::vector<int>> data;

// Construit la paire directement dans l'arbre, sans copie intermédiaire
data.emplace("mesures", std::vector<int>{10, 20, 30});
```

`emplace` transmet ses arguments au constructeur de `std::pair<const Key, Value>` pour éviter les copies inutiles. C'est la méthode privilégiée lorsque la valeur est coûteuse à copier.

### try_emplace (C++17) — emplace sans effet de bord ⭐

```cpp
std::map<std::string, std::string> cache;

// Si "key1" n'existe pas : construit la valeur et insère
// Si "key1" existe : ne fait RIEN (y compris ne pas construire la valeur)
cache.try_emplace("key1", "valeur_couteuse_a_construire");
```

La différence avec `emplace` est subtile mais importante : `emplace` peut construire l'objet valeur même si l'insertion échoue (clé déjà présente), alors que `try_emplace` garantit qu'aucune construction n'a lieu si la clé existe déjà. C'est un gain de performance quand la construction de la valeur est coûteuse.

---

## Recherche et accès

### find — la méthode de recherche standard

```cpp
std::map<std::string, int> population { /* ... */ };

auto it = population.find("Lyon");  
if (it != population.end()) {  
    std::print("Lyon : {} habitants\n", it->second);
} else {
    std::print("Ville non trouvée\n");
}
```

`find` retourne un itérateur vers l'élément trouvé, ou `end()` si la clé n'existe pas. La complexité est **O(log n)**.

### contains (C++20) — test d'existence simplifié

```cpp
if (population.contains("Marseille")) {
    std::print("Marseille est dans la map\n");
}
```

Avant C++20, on écrivait `population.count("Marseille") > 0` ou `population.find("Marseille") != population.end()`. La méthode `contains` est plus lisible et exprime mieux l'intention.

### at — accès avec vérification

```cpp
try {
    int pop = population.at("Bordeaux"); // Lance std::out_of_range si absent
} catch (const std::out_of_range& e) {
    std::print("Erreur : {}\n", e.what());
}
```

Contrairement à `operator[]`, `at` ne crée jamais d'entrée. Si la clé n'existe pas, elle lance une exception. C'est un accès sûr mais avec le coût potentiel de la gestion d'exception — à réserver aux cas où l'absence est véritablement exceptionnelle.

---

## Parcours et itération

L'un des atouts majeurs de `std::map` est que le parcours via itérateurs produit les éléments **dans l'ordre croissant des clés**. Cet ordre est garanti par le standard.

### Parcours ordonné classique

```cpp
std::map<std::string, int> population {
    {"Paris", 2'161'000}, {"Lyon", 522'969},
    {"Marseille", 873'076}, {"Toulouse", 504'078}
};

for (const auto& [ville, pop] : population) {
    std::print("{} : {}\n", ville, pop);
}
// Affiche dans l'ordre alphabétique :
// Lyon : 522969
// Marseille : 873076
// Paris : 2161000
// Toulouse : 504078
```

Les structured bindings (C++17) rendent le parcours d'une map élégant et lisible. Chaque élément est un `std::pair<const Key, Value>` — la clé est toujours `const` car la modifier invaliderait l'invariant de tri de l'arbre.

### Parcours inverse

```cpp
for (auto it = population.rbegin(); it != population.rend(); ++it) {
    std::print("{} : {}\n", it->first, it->second);
}
// Toulouse, Paris, Marseille, Lyon
```

### Requêtes par plage : lower_bound et upper_bound

C'est ici que `std::map` se distingue véritablement des conteneurs non ordonnés. Les méthodes `lower_bound` et `upper_bound` permettent d'effectuer des **requêtes par plage** en O(log n).

```cpp
std::map<int, std::string> events {
    {1789, "Révolution française"},
    {1804, "Premier Empire"},
    {1848, "Deuxième République"},
    {1870, "Troisième République"},
    {1914, "Première Guerre mondiale"},
    {1939, "Seconde Guerre mondiale"},
    {1958, "Cinquième République"}
};

// Événements du XIXe siècle [1800, 1900)
auto from = events.lower_bound(1800); // Premier élément >= 1800  
auto to   = events.lower_bound(1900); // Premier élément >= 1900  

for (auto it = from; it != to; ++it) {
    std::print("{} : {}\n", it->first, it->second);
}
// 1804 : Premier Empire
// 1848 : Deuxième République
// 1870 : Troisième République
```

`equal_range` combine les deux en un seul appel :

```cpp
auto [begin, end] = events.equal_range(1848);
// Itère sur tous les éléments dont la clé == 1848
// Pour std::map, c'est au plus un élément
// Pour std::multimap, ce peut être plusieurs
```

---

## Suppression d'éléments

### erase par clé

```cpp
std::size_t removed = population.erase("Toulouse");
// removed == 1 si la clé existait, 0 sinon
```

### erase par itérateur

```cpp
auto it = population.find("Lyon");  
if (it != population.end()) {  
    population.erase(it); // O(1) amorti une fois l'itérateur obtenu
}
```

### erase par plage

```cpp
// Supprimer tous les événements avant 1900
events.erase(events.begin(), events.lower_bound(1900));
```

### extract (C++17) — retirer un nœud sans le détruire

```cpp
std::map<std::string, int> source {{"Alice", 95}, {"Bob", 87}};  
std::map<std::string, int> dest;  

// Extraire le nœud "Alice" de source
auto node = source.extract("Alice");  
if (!node.empty()) {  
    node.key() = "Alicia"; // On peut modifier la clé !
    dest.insert(std::move(node));
}
// source contient {"Bob", 87}
// dest contient {"Alicia", 95}
```

`extract` est remarquable : il permet de **retirer un nœud de l'arbre sans désallocation** et de le réinsérer ailleurs, éventuellement avec une clé modifiée. C'est la seule manière de modifier une clé sans supprimer/recréer la paire.

### merge (C++17) — fusion de maps

```cpp
std::map<int, std::string> m1 {{1, "a"}, {3, "c"}, {5, "e"}};  
std::map<int, std::string> m2 {{2, "b"}, {3, "x"}, {4, "d"}};  

m1.merge(m2);
// m1 contient {1:"a", 2:"b", 3:"c", 4:"d", 5:"e"}
// m2 contient {3:"x"} — la clé 3 n'a pas pu être transférée (doublon)
```

Comme `extract`, `merge` transfère les nœuds sans copie ni allocation. Les clés qui provoqueraient un doublon restent dans la map source.

---

## Comparaison personnalisée

Par défaut, `std::map` trie ses clés avec `std::less<Key>`, ce qui correspond à l'opérateur `<`. On peut fournir un comparateur personnalisé.

### Ordre décroissant

```cpp
std::map<int, std::string, std::greater<int>> desc_map {
    {1, "un"}, {3, "trois"}, {2, "deux"}
};

for (const auto& [k, v] : desc_map) {
    std::print("{}: {}\n", k, v);
}
// 3: trois
// 2: deux
// 1: un
```

### Comparaison insensible à la casse

```cpp
struct CaseInsensitiveCompare {
    bool operator()(const std::string& a, const std::string& b) const {
        return std::lexicographical_compare(
            a.begin(), a.end(),
            b.begin(), b.end(),
            [](char ca, char cb) {
                return std::tolower(static_cast<unsigned char>(ca))
                     < std::tolower(static_cast<unsigned char>(cb));
            }
        );
    }
};

std::map<std::string, int, CaseInsensitiveCompare> ci_map;  
ci_map["Hello"] = 1;  
ci_map["hello"] = 2; // Écrase la précédente (même clé au sens du comparateur)  
// ci_map.size() == 1
```

> Le comparateur doit définir un **ordre faible strict** (*strict weak ordering*) : irréflexif (`!(a < a)`), asymétrique, transitif, et transitivité de l'incomparabilité. Un comparateur incorrect provoque un **comportement indéfini** — les symptômes classiques sont des crashs aléatoires ou des éléments "fantômes" introuvables.

---

## Transparent comparators et recherche hétérogène (C++14)

Un problème classique avec `std::map<std::string, T>` : chaque appel à `find("hello")` construit un `std::string` temporaire à partir du `const char*`, même si la comparaison pourrait se faire directement.

Les **transparent comparators** éliminent cette allocation :

```cpp
// std::less<> (sans paramètre) est un transparent comparator
std::map<std::string, int, std::less<>> config {
    {"timeout", 30},
    {"retries", 3},
    {"buffer_size", 4096}
};

// Recherche directe avec un const char* — pas de std::string temporaire
auto it = config.find("timeout");

// Fonctionne aussi avec std::string_view
std::string_view key = "retries";  
auto it2 = config.find(key);  
```

Le mécanisme repose sur le type membre `is_transparent` défini dans `std::less<void>`. Quand ce type est présent, les méthodes `find`, `count`, `lower_bound`, `upper_bound`, `equal_range` et `contains` acceptent tout type comparable à la clé.

> ⭐ **Bonne pratique** : pour les maps dont la clé est `std::string`, déclarez systématiquement `std::less<>` comme comparateur. C'est un gain de performance gratuit qui évite des allocations inutiles à chaque recherche.

---

## std::multimap : plusieurs valeurs par clé

`std::multimap` partage la même structure interne et les mêmes garanties de complexité que `std::map`, mais il autorise **plusieurs éléments à avoir la même clé**.

```cpp
#include <map>

std::multimap<std::string, std::string> index {
    {"fruit",  "pomme"},
    {"fruit",  "banane"},
    {"fruit",  "cerise"},
    {"légume", "carotte"},
    {"légume", "poireau"}
};
```

### Différences fondamentales avec std::map

`std::multimap` n'a **pas d'opérateur `[]`** ni de méthode `at()`. La raison est logique : avec plusieurs valeurs possibles pour une même clé, l'accès par clé unique n'a pas de sens.

L'insertion réussit **toujours** — il n'y a pas de notion de doublon à rejeter :

```cpp
index.insert({"fruit", "fraise"});  // Toujours accepté  
index.insert({"fruit", "fraise"});  // Aussi accepté — doublon exact autorisé  
```

### Accéder aux éléments d'une clé

La méthode standard est `equal_range`, qui retourne une paire d'itérateurs délimitant tous les éléments ayant la clé demandée :

```cpp
auto [begin, end] = index.equal_range("fruit");

for (auto it = begin; it != end; ++it) {
    std::print("{}\n", it->second);
}
// pomme, banane, cerise, fraise, fraise (ordre d'insertion pour clés égales)
```

`count` retourne le nombre d'éléments ayant une clé donnée :

```cpp
std::size_t nb_fruits = index.count("fruit"); // 5
```

### Suppression ciblée

`erase` par clé supprime **tous** les éléments ayant cette clé :

```cpp
index.erase("fruit"); // Supprime les 5 entrées "fruit"
```

Pour supprimer un seul élément parmi les doublons, il faut passer par un itérateur :

```cpp
auto it = index.find("légume"); // Pointe sur le premier "légume"  
if (it != index.end()) {  
    index.erase(it); // Supprime uniquement cet élément
}
```

---

## Quand utiliser std::multimap

En pratique, `std::multimap` est moins courant que `std::map`. Avant de l'utiliser, il vaut la peine de considérer les alternatives :

- `std::map<Key, std::vector<Value>>` est souvent plus simple à manipuler, plus efficace en mémoire (un seul nœud par clé, valeurs contiguës dans le vecteur), et offre un accès direct à toutes les valeurs d'une clé via le vecteur.
- `std::unordered_multimap` offre la même sémantique multi-clés mais avec une recherche en O(1) amorti, quand l'ordre n'importe pas.

`std::multimap` reste pertinent quand on a besoin à la fois de clés dupliquées **et** d'un parcours ordonné, ou quand les éléments sont insérés et supprimés individuellement de manière très fréquente (le modèle `map<Key, vector<Value>>` impose alors de gérer la suppression dans le vecteur interne).

---

## Considérations de performance

### Complexités garanties

| Opération | Complexité |
|---|---|
| `find`, `contains`, `count` | O(log n) |
| `insert`, `emplace`, `try_emplace` | O(log n) |
| `erase` (par itérateur) | O(1) amorti |
| `erase` (par clé) | O(log n + k) — k = nombre d'éléments supprimés |
| `lower_bound`, `upper_bound` | O(log n) |
| Parcours complet | O(n) |

### Impact mémoire et cache

Chaque nœud d'un `std::map` est alloué séparément sur le heap. Pour un `std::map<int, int>` typique sur une plateforme 64 bits, chaque nœud consomme environ 48 à 64 octets (4 pour la clé, 4 pour la valeur, 24 pour les trois pointeurs, le reste pour la couleur, le padding d'alignement et l'overhead d'allocation), alors que la donnée utile ne fait que 8 octets.

Ce surcoût a deux conséquences :

- **Empreinte mémoire élevée** : un `std::map<int, int>` de 1 million d'éléments consomme environ 50 Mo, là où un `std::vector<std::pair<int, int>>` trié n'en consommerait qu'environ 8 Mo.
- **Localité mémoire faible** : les nœuds sont dispersés dans le heap. Le parcours séquentiel génère des *cache misses* fréquents, ce qui peut rendre un `std::map` significativement plus lent qu'un vecteur trié pour les lectures en boucle serrée.

> Pour les cas d'usage *read-heavy* sur des jeux de données de taille modérée, `std::flat_map` (C++23) ou un `std::vector` trié avec `std::lower_bound` offrent souvent de meilleures performances mesurées. La section 14.4 et la section 14.5 reviennent en détail sur ces comparaisons.

---

## Résumé des méthodes essentielles

| Méthode | Description | Particularité |
|---|---|---|
| `operator[]` | Accès/création par clé | Crée une entrée si absente — map uniquement |
| `at()` | Accès avec exception | Lance `out_of_range` si absent — map uniquement |
| `insert()` | Insertion sans écrasement | Retourne `{iterator, bool}` |
| `insert_or_assign()` | Insert ou mise à jour (C++17) | Écrase si existant |
| `emplace()` | Construction en place | Peut construire même si insertion échoue |
| `try_emplace()` | Emplace conditionnel (C++17) | Ne construit rien si clé existe |
| `find()` | Recherche par clé | Retourne `end()` si absent |
| `contains()` | Test d'existence (C++20) | Retourne `bool` |
| `count()` | Nombre d'occurrences | 0 ou 1 pour map, 0..n pour multimap |
| `lower_bound()` | Premier >= clé | Requêtes par plage |
| `upper_bound()` | Premier > clé | Requêtes par plage |
| `equal_range()` | Plage d'éléments == clé | Essentiel pour multimap |
| `erase()` | Suppression | Par clé, itérateur ou plage |
| `extract()` | Extraction de nœud (C++17) | Permet de modifier la clé |
| `merge()` | Fusion de maps (C++17) | Transfert sans copie |

---

## En bref

`std::map` est le conteneur associatif de référence quand l'ordre des clés compte. Son implémentation en Red-Black Tree garantit des performances logarithmiques prévisibles et offre des capacités de requêtes par plage que les conteneurs non ordonnés ne peuvent pas fournir. Depuis C++17, `try_emplace`, `insert_or_assign`, `extract` et `merge` en font un outil encore plus expressif et performant.

Son point faible — la dispersion mémoire inhérente aux arbres — justifie de considérer `std::unordered_map` (section 14.2) quand l'ordre n'est pas nécessaire, ou `std::flat_map` (section 14.4) quand les lectures dominent le profil d'accès.

⏭️ [std::unordered_map : Tables de hachage](/14-conteneurs-associatifs/02-unordered-map.md)
