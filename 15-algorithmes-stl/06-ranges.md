🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 15.6 — Ranges (C++20) : Simplification des algorithmes ⭐

## Chapitre 15 — Algorithmes de la STL

---

## Introduction

Depuis les sections précédentes, nous avons régulièrement montré les versions Ranges des algorithmes en complément des versions classiques. Cette section rassemble et approfondit le sujet : qu'est-ce que la bibliothèque Ranges, pourquoi elle a été introduite, et en quoi elle transforme fondamentalement la manière d'écrire du code avec la STL.

La bibliothèque Ranges (C++20) n'est pas une réécriture cosmétique. Elle résout trois catégories de problèmes qui ont pesé sur la STL pendant plus de vingt ans :

- La **verbosité** des paires d'itérateurs, source de répétition et d'erreurs de mismatch.
- L'**impossibilité de composer** les algorithmes — chaîner un filtre, une transformation et un tri nécessitait des conteneurs intermédiaires.
- L'**absence de contraintes explicites** — les erreurs de template produisaient des messages incompréhensibles.

```cpp
#include <ranges>
#include <algorithm>
#include <vector>
#include <string>
```

---

## Le problème : vingt ans de paires d'itérateurs

Considérons un code classique pré-C++20 :

```cpp
std::vector<int> v = {5, 3, 8, 1, 9, 2, 7};

// Trier
std::sort(v.begin(), v.end());

// Trouver un élément
auto it = std::find(v.begin(), v.end(), 8);

// Copier les pairs dans un autre vector
std::vector<int> evens;  
std::copy_if(v.begin(), v.end(), std::back_inserter(evens),  
    [](int x) { return x % 2 == 0; });

// Transformer
std::vector<int> doubled(v.size());  
std::transform(v.begin(), v.end(), doubled.begin(),  
    [](int x) { return x * 2; });
```

Chaque appel répète `v.begin(), v.end()`. C'est verbeux, et surtout c'est fragile : rien n'empêche de passer `v.begin()` avec `other_vector.end()`, une erreur qui compile mais provoque un comportement indéfini. Sur un algorithme à deux séquences comme `std::transform` binaire ou `std::mismatch`, les possibilités de mismatch se multiplient.

Au-delà de la verbosité, le vrai problème est la **composition**. Supposons qu'on veuille filtrer les éléments pairs, les mettre au carré, puis calculer leur somme. En STL classique :

```cpp
std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

// Étape 1 : filtrer les pairs dans un conteneur temporaire
std::vector<int> evens;  
std::copy_if(v.begin(), v.end(), std::back_inserter(evens),  
    [](int x) { return x % 2 == 0; });

// Étape 2 : mettre au carré dans un autre conteneur temporaire
std::vector<int> squared(evens.size());  
std::transform(evens.begin(), evens.end(), squared.begin(),  
    [](int x) { return x * x; });

// Étape 3 : sommer
int total = std::accumulate(squared.begin(), squared.end(), 0);
// total == 220  (4 + 16 + 36 + 64 + 100)
```

Trois étapes, deux conteneurs intermédiaires, des allocations mémoire inutiles. Le code exprime *comment* faire plutôt que *quoi* faire. C'est exactement le type de situation que les Ranges résolvent.

---

## La solution Ranges : trois piliers

La bibliothèque Ranges s'articule autour de trois composants complémentaires :

### 1. Algorithmes contraints (namespace std::ranges)

Les algorithmes du namespace `std::ranges` sont des versions améliorées des algorithmes classiques. Ils acceptent directement un **range** (tout objet possédant `begin()` et `end()`) au lieu d'une paire d'itérateurs :

```cpp
std::vector<int> v = {5, 3, 8, 1, 9, 2, 7};

// Classique
std::sort(v.begin(), v.end());

// Ranges — passage direct du conteneur
std::ranges::sort(v);
```

C'est plus concis, et surtout **impossible de se tromper** : on ne peut pas mélanger les itérateurs de deux conteneurs différents.

Les algorithmes `std::ranges` existent pour la quasi-totalité des algorithmes classiques : `std::ranges::find`, `std::ranges::count_if`, `std::ranges::copy`, `std::ranges::transform`, `std::ranges::remove_if`, `std::ranges::unique`, `std::ranges::reverse`, `std::ranges::partition`, etc.

### 2. Views (vues paresseuses)

Les views sont des adaptateurs légers qui **décrivent** une transformation ou un filtrage sans l'exécuter. Elles ne possèdent pas de données, ne font aucune allocation, et ne calculent les résultats que quand on les parcourt (lazy evaluation). C'est le composant qui permet la composition sans conteneurs intermédiaires.

```cpp
auto view = v | std::views::filter([](int x) { return x % 2 == 0; })
              | std::views::transform([](int x) { return x * x; });
```

Cette ligne ne fait **rien** : elle construit un objet view qui décrit l'opération. Le calcul n'a lieu que quand on itère :

```cpp
for (int x : view) {
    std::print("{} ", x);  // C'est ici que le filtre et la transformation s'exécutent
}
```

Les views sont couvertes en détail dans la sous-section 15.6.1.

### 3. Concepts (contraintes explicites)

Les algorithmes Ranges utilisent les **concepts** C++20 pour exprimer leurs exigences. Quand on passe un type inadapté, le message d'erreur est clair :

```cpp
std::list<int> lst = {3, 1, 4};

// std::ranges::sort(lst);
// Erreur : "constraint not satisfied: std::sortable<std::list<int>::iterator>"
// Le message indique clairement que sort nécessite des itérateurs sortable
// (qui implique random_access_iterator)
```

Comparez avec l'erreur classique sans Ranges, qui produit typiquement des dizaines de lignes de template instantiation incompréhensibles.

---

## Projections : la fonctionnalité qui change tout

Les **projections** sont peut-être l'apport le plus immédiatement utile des algorithmes Ranges au quotidien. Chaque algorithme `std::ranges` accepte un paramètre optionnel de projection — un callable appliqué à chaque élément **avant** la comparaison ou l'opération.

### Le problème sans projection

En STL classique, pour trier des objets selon un de leurs champs, il faut écrire un comparateur lambda :

```cpp
struct Server {
    std::string hostname;
    int cpu_load;
    int memory_mb;
};

std::vector<Server> servers = {
    {"web-01", 72, 4096},
    {"db-01", 91, 16384},
    {"cache-01", 23, 2048},
    {"web-02", 45, 8192}
};

// Classique : lambda comparateur
std::sort(servers.begin(), servers.end(),
    [](const Server& a, const Server& b) {
        return a.cpu_load < b.cpu_load;
    }
);
```

La lambda est verbeuse et répétitive : on écrit deux fois `cpu_load`, on déclare deux paramètres typés, on explicite la comparaison. C'est le même pattern pour chaque champ et chaque algorithme.

### La solution avec projection

```cpp
// Ranges : projection directe
std::ranges::sort(servers, {}, &Server::cpu_load);
```

Le `{}` indique le comparateur par défaut (`std::ranges::less`). `&Server::cpu_load` est la projection : chaque élément est « projeté » sur son champ `cpu_load` avant la comparaison. Une ligne au lieu de quatre.

La projection fonctionne avec **tout callable** — un pointeur vers membre, une lambda, une fonction libre :

```cpp
// Trier par nom de host, décroissant
std::ranges::sort(servers, std::ranges::greater{}, &Server::hostname);

// Trier par un calcul dérivé
std::ranges::sort(servers, {}, [](const Server& s) {
    return s.cpu_load * 100 + s.memory_mb;  // score composite
});
```

### Projections sur tous les algorithmes

Les projections ne sont pas limitées au tri. Elles fonctionnent sur tous les algorithmes Ranges :

```cpp
// Trouver le serveur nommé "db-01"
auto it = std::ranges::find(servers, "db-01", &Server::hostname);

// Compter les serveurs avec une charge > 50%
auto n = std::ranges::count_if(servers,
    [](int load) { return load > 50; },
    &Server::cpu_load    // projeter sur cpu_load AVANT d'appliquer le prédicat
);

// Le prédicat reçoit un int (le cpu_load), pas un Server

// Min et max par mémoire
auto [min_it, max_it] = std::ranges::minmax_element(servers, {}, &Server::memory_mb);  
std::print("Moins de RAM : {} ({}MB)\n", min_it->hostname, min_it->memory_mb);  
std::print("Plus de RAM : {} ({}MB)\n", max_it->hostname, max_it->memory_mb);  
```

Le mécanisme est toujours le même : la projection transforme l'élément *avant* que l'algorithme ne l'utilise. Le prédicat ou le comparateur reçoit la valeur projetée, pas l'élément original. Cela élimine la majorité des lambdas comparateurs écrites à la main dans le code STL classique.

### Projection avec count_if et find_if : attention à l'ordre des paramètres

L'ordre des paramètres dans les algorithmes Ranges diffère parfois du classique. La projection est toujours le **dernier** paramètre :

```cpp
// std::ranges::count_if(range, prédicat, projection)
auto n = std::ranges::count_if(servers,
    [](int load) { return load > 50; },  // prédicat (reçoit la valeur projetée)
    &Server::cpu_load                     // projection
);

// std::ranges::find_if(range, prédicat, projection)
auto it = std::ranges::find_if(servers,
    [](int mem) { return mem > 10000; },
    &Server::memory_mb
);
```

---

## Algorithmes Ranges vs Classiques : différences clés

Au-delà des projections et du passage direct de conteneurs, les algorithmes `std::ranges` présentent quelques différences supplémentaires avec leurs homologues classiques.

### Valeur de retour enrichie

Plusieurs algorithmes Ranges renvoient des types de retour plus riches que leurs versions classiques. Par exemple, `std::ranges::copy` renvoie une struct contenant à la fois un itérateur vers la fin de la source consommée et un itérateur vers la fin de la destination :

```cpp
std::vector<int> src = {1, 2, 3, 4, 5};  
std::vector<int> dst(5);  

auto [in_it, out_it] = std::ranges::copy(src, dst.begin());
// in_it  == src.end()
// out_it == dst.end()
```

`std::ranges::remove` renvoie un sous-range des éléments à effacer, permettant une syntaxe plus directe que le remove-erase idiom :

```cpp
std::vector<int> v = {1, 2, 3, 2, 5, 2, 7};

// Le sous-range renvoyé est directement passable à erase
v.erase(std::ranges::remove(v, 2).begin(), v.end());
// Mais std::erase(v, 2) de C++20 reste plus simple
```

### Pas de politique d'exécution

Les algorithmes `std::ranges` ne supportent **pas** les politiques d'exécution parallèle (`std::execution::par`, etc.) dans le standard actuel (C++20/C++23). Pour la parallélisation, il faut utiliser les versions classiques avec paires d'itérateurs :

```cpp
// ✅ Parallélisation avec algorithme classique
std::sort(std::execution::par, v.begin(), v.end());

// ❌ Pas de surcharge parallèle pour ranges
// std::ranges::sort(std::execution::par, v);  // N'existe pas
```

C'est une limitation significative. En pratique, cela signifie qu'on utilise les algorithmes Ranges pour le code séquentiel expressif, et les algorithmes classiques quand la parallélisation est nécessaire. L'intégration des politiques d'exécution dans les Ranges est envisagée pour un futur standard.

### Dangling iterator protection

Les algorithmes Ranges détectent à la **compilation** les situations où un itérateur renvoyé pointerait vers un objet temporaire détruit :

```cpp
// Classique : compile, mais l'itérateur est dangling (comportement indéfini)
// auto it = std::find(get_vector().begin(), get_vector().end(), 42);
// Le vector temporaire est détruit, it est invalide — aucun warning

// Ranges : erreur de compilation !
// auto it = std::ranges::find(get_vector(), 42);
// Renvoie std::ranges::dangling au lieu d'un itérateur
// Toute tentative de déréférencement de dangling est une erreur de compilation
```

Le mécanisme repose sur le concept `std::ranges::borrowed_range`. Si le range passé est un temporaire qui ne survit pas à l'appel, l'algorithme renvoie `std::ranges::dangling` au lieu d'un itérateur, empêchant l'utilisation accidentelle d'un itérateur invalide.

```cpp
// Solution : stocker le vector dans une variable
std::vector<int> v = get_vector();  
auto it = std::ranges::find(v, 42);  // ✅ v survit, l'itérateur est valide  
```

---

## Sous-ranges : opérer sur une partie d'un conteneur

Les algorithmes Ranges acceptent des conteneurs entiers, mais on a parfois besoin de travailler sur un sous-ensemble. `std::ranges::subrange` crée un range à partir d'une paire d'itérateurs :

```cpp
std::vector<int> v = {9, 7, 5, 3, 1, 8, 6, 4, 2, 0};

// Trier uniquement les 5 premiers éléments
std::ranges::sort(std::ranges::subrange(v.begin(), v.begin() + 5));
// v == {1, 3, 5, 7, 9, 8, 6, 4, 2, 0}
```

On peut aussi utiliser la syntaxe itérateur-sentinelle directe sur les algorithmes Ranges :

```cpp
// Équivalent : passer begin et end comme arguments séparés
std::ranges::sort(v.begin(), v.begin() + 5);
```

Les algorithmes Ranges acceptent donc à la fois un range complet **et** une paire itérateur-sentinelle, offrant le meilleur des deux mondes.

---

## Sentinelles : généraliser la notion de fin

En STL classique, le « début » et la « fin » d'une séquence sont des itérateurs du **même type**. Les Ranges relâchent cette contrainte : la fin peut être une **sentinelle** — un objet d'un type différent qui sait se comparer à un itérateur pour indiquer la fin de la séquence.

Le cas le plus courant est un pointeur nul terminant une chaîne C :

```cpp
struct NullTerminator {
    bool operator==(const char* p) const { return *p == '\0'; }
};

const char* msg = "Hello, Ranges!";

// Compter les caractères sans connaître la longueur à l'avance
auto n = std::ranges::count_if(
    msg, NullTerminator{},
    [](char c) { return c == 'l'; }
);
// n == 2
```

La sentinelle évite d'avoir à calculer la longueur au préalable (un premier parcours) pour ensuite faire le vrai travail (un second parcours). Avec une sentinelle, tout se fait en un seul parcours.

`std::ranges::unreachable_sentinel` est une sentinelle prédéfinie qui ne correspond **jamais** — utile quand on sait par invariant que la condition de fin sera atteinte avant la fin réelle du conteneur :

```cpp
// On sait que 42 existe dans v — pas besoin de vérifier v.end()
auto it = std::ranges::find(v.begin(), std::unreachable_sentinel, 42);
// Légèrement plus rapide : la boucle ne teste pas it != v.end() à chaque itération
```

---

## Quand utiliser les algorithmes Ranges vs Classiques

| Critère | Algorithme Ranges | Algorithme Classique |
|---|---|---|
| Passage de conteneur direct | Oui | Non (paires d'itérateurs) |
| Projections | Oui | Non |
| Messages d'erreur clairs | Oui (concepts) | Non (erreurs template) |
| Protection dangling | Oui | Non |
| Politiques d'exécution parallèle | Non | Oui |
| Compatibilité code existant | C++20+ | Tout standard |
| Composition via views | Oui | Non |

En pratique, pour du code C++20 et ultérieur :

- Utilisez **`std::ranges`** comme choix par défaut — plus sûr, plus lisible, projections.
- Basculez sur les **algorithmes classiques** quand vous avez besoin de parallélisation (`std::execution::par`).
- Les deux coexistent parfaitement dans le même programme.

---

## Plan des sous-sections

Les deux sous-sections suivantes approfondissent les deux mécanismes de composition que les Ranges rendent possibles :

- **15.6.1** — **Views et lazy evaluation** : les adaptateurs paresseux (`filter`, `transform`, `take`, `drop`, `reverse`…), leur coût zéro en mémoire, et la manière dont ils transforment le style de programmation.
- **15.6.2** — **Pipelines avec l'opérateur `|`** : la syntaxe de composition qui enchaîne les views en un flux lisible de gauche à droite, et comment elle remplace les conteneurs intermédiaires.

Ensemble, ces mécanismes permettent de réécrire l'exemple d'introduction — filtrer, transformer et sommer — en une seule expression déclarative sans allocation intermédiaire.

⏭️ [Views et lazy evaluation](/15-algorithmes-stl/06.1-views-lazy.md)
