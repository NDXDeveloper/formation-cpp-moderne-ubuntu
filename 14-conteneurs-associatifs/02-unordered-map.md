🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 14.2 — std::unordered_map : Tables de hachage

## Module 5 — La Bibliothèque Standard (STL) | Niveau Intermédiaire

---

## Introduction

La section précédente a montré que `std::map` garantit des opérations en O(log n) grâce à son arbre Red-Black. C'est excellent, mais quand la seule question est *« quelle valeur correspond à cette clé ? »* — sans besoin de parcours ordonné ni de requêtes par plage —, on peut faire mieux.

`std::unordered_map`, introduit en C++11, repose sur une **table de hachage** (*hash table*). Il offre une recherche, une insertion et une suppression en **O(1) amorti**, soit une complexité constante indépendante du nombre d'éléments. Pour les cas d'usage dominés par la recherche pure, c'est le conteneur associatif le plus performant de la STL standard.

Le compromis est clair : en échange de cette vitesse, `std::unordered_map` ne maintient **aucun ordre** sur ses éléments. Le parcours produit les clés dans un ordre arbitraire qui peut changer après chaque insertion ou suppression.

```cpp
#include <unordered_map>
#include <print>

std::unordered_map<std::string, int> population {
    {"Paris",     2'161'000},
    {"Lyon",        522'969},
    {"Marseille",   873'076},
    {"Toulouse",    504'078}
};

// Recherche en O(1) amorti
if (auto it = population.find("Lyon"); it != population.end()) {
    std::print("Lyon : {} habitants\n", it->second);
}

// Parcours — ordre NON garanti
for (const auto& [ville, pop] : population) {
    std::print("{} : {}\n", ville, pop);
}
// Peut afficher dans n'importe quel ordre
```

---

## Le principe en un coup d'œil

Le fonctionnement d'une table de hachage repose sur une idée simple : transformer la clé en un **indice numérique** (le *hash*) qui désigne directement l'emplacement de stockage. Au lieu de parcourir un arbre ou une liste pour trouver un élément, on calcule son adresse.

```
Clé "Lyon"  →  hash("Lyon")  →  indice 3  →  bucket[3]  →  {"Lyon", 522969}
```

Quand deux clés produisent le même indice — une **collision** —, le conteneur doit gérer le conflit. La STL utilise le **chaînage séparé** (*separate chaining*) : chaque bucket contient une liste chaînée d'éléments qui partagent le même indice.

La performance du conteneur dépend donc de deux facteurs :

- la **qualité de la fonction de hachage** — une bonne distribution minimise les collisions,
- le **facteur de charge** (*load factor*) — le ratio entre le nombre d'éléments et le nombre de buckets.

> La sous-section 14.2.1 détaille le fonctionnement interne des hash tables (structure des buckets, gestion des collisions, rehashing). La sous-section 14.2.2 couvre l'écriture de fonctions de hachage personnalisées.

---

## API essentielle

L'interface de `std::unordered_map` est volontairement proche de celle de `std::map`. Un développeur familier avec `std::map` retrouve les mêmes méthodes de base, ce qui facilite le passage d'un conteneur à l'autre.

### Insertion

```cpp
std::unordered_map<std::string, double> prix;

// operator[] — crée ou écrase
prix["café"] = 1.50;

// insert — n'écrase pas si la clé existe
auto [it, ok] = prix.insert({"thé", 2.00});

// insert_or_assign (C++17) — insert ou mise à jour explicite
prix.insert_or_assign("café", 1.80);

// emplace — construction en place
prix.emplace("jus", 3.50);

// try_emplace (C++17) — ne construit la valeur que si la clé est absente
prix.try_emplace("eau", 0.50);
```

Les mêmes méthodes que `std::map`, les mêmes sémantiques, les mêmes pièges — en particulier `operator[]` qui insère silencieusement une entrée par défaut en lecture.

### Recherche

```cpp
// find — retourne un itérateur
auto it = prix.find("café");  
if (it != prix.end()) {  
    std::print("Café : {:.2f}€\n", it->second);
}

// contains (C++20) — test d'existence
if (prix.contains("thé")) {
    std::print("Le thé est au menu\n");
}

// at — accès avec exception si absent
try {
    double p = prix.at("limonade");
} catch (const std::out_of_range& e) {
    std::print("Non trouvé : {}\n", e.what());
}

// count — 0 ou 1 (pas de doublons dans unordered_map)
if (prix.count("jus") > 0) { /* ... */ }
```

### Suppression

```cpp
prix.erase("thé");           // Par clé — retourne le nombre d'éléments supprimés  
prix.erase(prix.find("jus")); // Par itérateur  
```

### Parcours

```cpp
for (const auto& [produit, p] : prix) {
    std::print("{} → {:.2f}€\n", produit, p);
}
// L'ordre d'affichage est imprévisible et peut changer entre deux exécutions
```

---

## Ce qui distingue unordered_map de map

Malgré une API très similaire, les deux conteneurs diffèrent sur des points fondamentaux qu'il faut bien comprendre pour choisir le bon.

### Pas de parcours ordonné

C'est la différence la plus visible. `std::unordered_map` ne dispose ni de `lower_bound`, ni de `upper_bound`. Il possède `equal_range`, mais celle-ci retourne simplement les éléments ayant exactement la clé demandée (0 ou 1 pour `unordered_map`, potentiellement plusieurs pour `unordered_multimap`) — pas une plage ordonnée. Si le parcours dans l'ordre des clés ou les requêtes par plage sont nécessaires, `std::map` ou `std::flat_map` sont les seuls choix.

### Pas d'itérateurs stables face au rehashing

Quand le nombre d'éléments dépasse le seuil défini par le facteur de charge, le conteneur effectue un **rehashing** : il alloue un nouveau tableau de buckets, recalcule les hash de tous les éléments et les redistribue. Pendant cette opération, **tous les itérateurs sont invalidés**. Les références et pointeurs vers les éléments restent en revanche valides (la STL le garantit).

```cpp
std::unordered_map<int, int> m;  
m.reserve(100); // Pré-allouer pour éviter le rehashing prématuré  

auto it = m.emplace(42, 99).first;
// ... insertions massives qui déclenchent un rehashing ...
// it est désormais potentiellement invalide !
```

### Exigences sur le type de clé

`std::map` exige que la clé supporte l'opérateur `<` (ou un comparateur personnalisé). `std::unordered_map` exige deux choses différentes :

- une **fonction de hachage** — fournie par `std::hash<Key>` pour les types standard,
- un **opérateur d'égalité** — `operator==` pour vérifier si deux clés dans le même bucket sont identiques.

Les types standard (`int`, `double`, `std::string`, `std::string_view`, pointeurs) ont des spécialisations de `std::hash` dans la bibliothèque standard. Pour un type personnalisé, il faut fournir les deux soi-même — c'est l'objet de la sous-section 14.2.2.

---

## Contrôle du comportement interne

`std::unordered_map` expose des méthodes qui permettent d'observer et d'influencer le fonctionnement de la table de hachage. C'est un niveau de contrôle que `std::map` ne propose pas.

### Buckets

```cpp
std::unordered_map<std::string, int> m {
    {"a", 1}, {"b", 2}, {"c", 3}, {"d", 4}, {"e", 5}
};

std::print("Nombre de buckets : {}\n", m.bucket_count());  
std::print("Nombre d'éléments : {}\n", m.size());  

// Dans quel bucket se trouve une clé ?
std::print("'c' est dans le bucket {}\n", m.bucket("c"));

// Combien d'éléments dans un bucket donné ?
for (std::size_t i = 0; i < m.bucket_count(); ++i) {
    if (m.bucket_size(i) > 0) {
        std::print("Bucket {} : {} éléments\n", i, m.bucket_size(i));
    }
}
```

### Facteur de charge

```cpp
std::print("Load factor actuel : {:.2f}\n", m.load_factor());  
std::print("Load factor max    : {:.2f}\n", m.max_load_factor());  

// Réduire le seuil pour déclencher le rehashing plus tôt
// (moins de collisions, plus de mémoire)
m.max_load_factor(0.5f);

// Forcer un rehashing immédiat
m.rehash(100);   // Demande au moins 100 buckets  
m.reserve(1000); // Pré-alloue pour accueillir 1000 éléments sans rehashing  
```

Le facteur de charge par défaut est **1.0** dans la plupart des implémentations, ce qui signifie qu'un rehashing est déclenché quand le nombre d'éléments atteint le nombre de buckets.

### reserve — la bonne pratique pour les insertions en masse ⭐

Quand on connaît à l'avance le nombre approximatif d'éléments, `reserve` évite les rehashings successifs pendant le remplissage :

```cpp
std::unordered_map<int, std::string> lookup;  
lookup.reserve(10'000); // Un seul rehashing au lieu de plusieurs  

for (int i = 0; i < 10'000; ++i) {
    lookup.emplace(i, std::to_string(i));
}
```

Sans `reserve`, la table subit typiquement 10 à 15 rehashings pour atteindre 10 000 éléments, chacun impliquant la réallocation du tableau de buckets et le recalcul de tous les hash. Avec `reserve`, un seul redimensionnement a lieu au début.

---

## Hachage des types standard

La bibliothèque standard fournit des spécialisations de `std::hash<T>` pour les types courants :

| Type | Spécialisation `std::hash` |
|---|---|
| `int`, `long`, `unsigned`, etc. | Identité ou mélange de bits |
| `float`, `double` | Hash de la représentation binaire |
| `std::string` | Hash du contenu (FNV-1a ou similaire) |
| `std::string_view` | Compatible avec `std::string` |
| Pointeurs bruts | Hash de l'adresse |
| `std::unique_ptr`, `std::shared_ptr` | Hash du pointeur sous-jacent |
| Énumérations | Hash de la valeur sous-jacente |

Les types **non couverts** par défaut incluent : `std::pair`, `std::tuple`, les types utilisateur, les conteneurs (`std::vector<T>` comme clé). Pour ces types, il faut fournir une fonction de hachage personnalisée — c'est le sujet de la sous-section 14.2.2.

---

## std::unordered_multimap

Comme `std::multimap` est à `std::map`, `std::unordered_multimap` est à `std::unordered_map` : il autorise **plusieurs éléments à partager la même clé**.

```cpp
#include <unordered_map>

std::unordered_multimap<std::string, int> scores {
    {"Alice", 95}, {"Bob", 87}, {"Alice", 88}, {"Alice", 92}
};

// count retourne le nombre d'éléments avec cette clé
std::print("Scores d'Alice : {}\n", scores.count("Alice")); // 3

// equal_range retourne la plage d'éléments
auto [begin, end] = scores.equal_range("Alice");  
for (auto it = begin; it != end; ++it) {  
    std::print("  {}\n", it->second);
}
```

Comme pour `std::multimap`, il n'y a ni `operator[]` ni `at()`. Et comme pour `std::multimap`, l'alternative `std::unordered_map<Key, std::vector<Value>>` est souvent plus pratique et plus performante en pratique.

---

## Comparaison rapide : map vs unordered_map

| Critère | `std::map` | `std::unordered_map` |
|---|---|---|
| **Recherche** | O(log n) | O(1) amorti |
| **Insertion** | O(log n) | O(1) amorti |
| **Pire cas** | O(log n) | O(n) — collisions massives |
| **Parcours ordonné** | Oui (garanti) | Non |
| **Requêtes par plage** | `lower_bound`, `upper_bound` | Non disponible |
| **Exigence sur la clé** | `operator<` | `std::hash` + `operator==` |
| **Invalidation itérateurs** | Stable | Invalidés par rehashing |
| **Overhead mémoire** | ~48-64 octets/nœud | Buckets + listes chaînées |
| **Localité cache** | Faible (nœuds dispersés) | Faible (chaînage séparé) |

Le choix est généralement simple :

- **Besoin d'ordre ou de plages** → `std::map`
- **Recherche pure sans ordre** → `std::unordered_map`
- **Les deux à la fois** → maintenir les deux conteneurs ou envisager un vecteur trié (`std::flat_map` en C++23)

---

## Quand std::unordered_map n'est pas le meilleur choix

Malgré son O(1) théorique, `std::unordered_map` n'est pas toujours le conteneur le plus rapide en pratique. Plusieurs situations peuvent le désavantager :

- **Petits jeux de données** (quelques dizaines d'éléments) : l'overhead de la table de hachage — allocation des buckets, calcul du hash, résolution des collisions — peut rendre un simple `std::vector` trié avec `std::lower_bound` plus rapide, grâce à la localité mémoire.
- **Clés coûteuses à hasher** : si la fonction de hachage est lente (par exemple sur de très longues chaînes), la comparaison directe en O(log n) de `std::map` peut être compétitive.
- **Patterns d'accès séquentiels** : un parcours complet de `std::unordered_map` est plus lent qu'un parcours de `std::vector` ou `std::flat_map` à cause de la dispersion mémoire des buckets et du chaînage.

> La section 14.5 confronte ces conteneurs dans des benchmarks mesurés pour quantifier ces différences.

---

## Ce que couvrent les sous-sections

Les deux sous-sections suivantes approfondissent les mécanismes internes :

- **14.2.1 — Fonctionnement des hash tables** : structure des buckets, chaînage séparé vs adressage ouvert, processus de rehashing, impact du facteur de charge, et pourquoi le pire cas est en O(n).
- **14.2.2 — Custom hash functions** : écriture de fonctions de hachage pour types personnalisés, techniques de combinaison de hash, qualité de distribution et pièges courants.

---

## En bref

`std::unordered_map` est le conteneur associatif de choix quand la recherche par clé est l'opération dominante et que l'ordre des éléments n'importe pas. Son O(1) amorti en fait le conteneur le plus rapide de la STL pour ce cas d'usage. En contrepartie, il impose une fonction de hachage sur la clé, ne garantit aucun ordre de parcours, et ses itérateurs sont invalidés par le rehashing. Pré-allouer avec `reserve` et surveiller le facteur de charge sont les deux leviers essentiels pour en tirer les meilleures performances.

⏭️ [Fonctionnement des hash tables](/14-conteneurs-associatifs/02.1-hash-tables.md)
