🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 12.5 Ranges (C++20) : Pipelines fonctionnels

## Repenser les algorithmes

Les algorithmes de la STL existent depuis C++98 et constituent l'un des piliers du langage. Mais leur interface — héritée d'une époque où les lambdas n'existaient pas et où la généricité en était à ses débuts — souffre de défauts structurels que vingt ans de pratique ont rendus évidents : l'obligation de passer des paires d'itérateurs, l'impossibilité de composer les algorithmes entre eux, et une évaluation systématiquement immédiate, même quand elle est inutile.

Les Ranges de C++20 répondent à ces trois problèmes simultanément. Ils introduisent une abstraction — le *range* — qui unifie la notion de « séquence d'éléments », et un mécanisme de composition — les *views* et l'opérateur pipe `|` — qui permet de chaîner des transformations en pipelines lisibles et paresseux. C'est un changement de paradigme dans la manière d'écrire des traitements de données en C++.

> 📎 *Cette section couvre les principes fondamentaux des Ranges. La section 15.6 approfondit les views, la lazy evaluation et les pipelines dans le contexte des algorithmes STL.*

## Le problème : les limites des algorithmes classiques

### Paires d'itérateurs : verbeux et fragile

Les algorithmes classiques de `<algorithm>` prennent des paires d'itérateurs `(begin, end)`. Ce design est flexible mais verbeux, et source d'erreurs quand on mélange des itérateurs provenant de conteneurs différents :

```cpp
#include <algorithm>
#include <vector>

std::vector<int> data = {5, 3, 8, 1, 9, 2, 7};

// Paires d'itérateurs : on répète "data" deux fois
std::sort(data.begin(), data.end());

// Erreur silencieuse : itérateurs de conteneurs différents → UB
std::vector<int> other = {10, 20};
// std::sort(data.begin(), other.end());  // Compile, mais comportement indéfini
```

Pour des opérations simples, le pattern `(container.begin(), container.end())` est un bruit syntaxique répétitif qui n'apporte rien à la compréhension du code.

### Composition impossible

Supposons qu'on veuille, à partir d'un vecteur d'entiers, extraire les nombres pairs, les trier, puis les élever au carré. Avec les algorithmes classiques :

```cpp
#include <algorithm>
#include <vector>

std::vector<int> data = {5, 3, 8, 1, 6, 2, 7, 4, 9};

// Étape 1 : copier les pairs dans un nouveau vecteur
std::vector<int> evens;  
std::copy_if(data.begin(), data.end(), std::back_inserter(evens),  
             [](int n) { return n % 2 == 0; });

// Étape 2 : trier
std::sort(evens.begin(), evens.end());

// Étape 3 : transformer (élever au carré)
std::vector<int> result;  
std::transform(evens.begin(), evens.end(), std::back_inserter(result),  
               [](int n) { return n * n; });

// result == {4, 16, 36, 64}
```

Trois étapes, deux conteneurs intermédiaires, une allocation par conteneur. Le flux logique — filtrer → trier → transformer — est noyé dans la mécanique de gestion des itérateurs et des conteneurs temporaires. On ne peut pas enchaîner les algorithmes en un seul pipeline parce que chaque algorithme attend des itérateurs et retourne des itérateurs (ou rien du tout), pas un objet « prêt à être transformé ».

## La solution : les Ranges

### Qu'est-ce qu'un range ?

Un *range* est tout objet qui fournit un début et une fin — c'est-à-dire tout type pour lequel `std::ranges::begin(r)` et `std::ranges::end(r)` sont valides. Tous les conteneurs de la STL (`vector`, `map`, `list`, etc.) sont des ranges, ainsi que les tableaux C natifs et les `std::span`.

Le concept `std::ranges::range` formalise cette idée :

```cpp
#include <ranges>
#include <vector>

static_assert(std::ranges::range<std::vector<int>>);   // true  
static_assert(std::ranges::range<int[10]>);              // true  
static_assert(std::ranges::range<int>);                  // false — un int seul n'est pas un range  
```

### Algorithmes ranges : plus besoin de paires d'itérateurs

C++20 fournit des versions *ranges* de tous les algorithmes classiques dans le namespace `std::ranges`. Leur différence fondamentale : ils acceptent directement un range au lieu d'une paire d'itérateurs :

```cpp
#include <algorithm>
#include <ranges>
#include <vector>

std::vector<int> data = {5, 3, 8, 1, 9, 2, 7};

// Algorithme classique
std::sort(data.begin(), data.end());

// Algorithme ranges — même résultat, moins de bruit
std::ranges::sort(data);
```

C'est un gain immédiat en lisibilité. Mais la vraie puissance des Ranges réside dans les *views*.

## Les views : transformations paresseuses

### Le concept clé : transformer sans matérialiser

Une *view* est un range léger qui décrit une transformation sans l'exécuter immédiatement. Elle n'alloue pas de mémoire, ne copie pas de données, et ne calcule ses éléments que lorsqu'on les consomme — on parle d'**évaluation paresseuse** (*lazy evaluation*).

```cpp
#include <ranges>
#include <vector>
#include <print>

std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

// Créer une view qui filtre les pairs — rien n'est calculé ici
auto evens = data | std::views::filter([](int n) { return n % 2 == 0; });

// Les éléments sont calculés uniquement quand on itère
for (int n : evens) {
    std::print("{} ", n);  // 2 4 6 8 10
}
```

L'objet `evens` ne contient pas les nombres pairs. Il contient une *description* de la transformation : « prendre `data`, ne garder que les éléments où `n % 2 == 0` ». Le filtrage effectif n'a lieu que lors de l'itération dans la boucle `for`.

### Propriétés fondamentales des views

**Non-owning** — Une view ne possède pas les données. Comme `std::span` ou `std::string_view`, elle observe des données possédées par un autre objet. Les mêmes précautions de durée de vie s'appliquent.

**O(1) en construction** — Créer une view est une opération à coût constant, quelle que soit la taille des données sous-jacentes. Pas d'allocation, pas de copie.

**Lazy** — Les éléments ne sont calculés qu'à la demande, pendant l'itération. Si on ne consomme que les 3 premiers éléments d'une view filtrée sur un million d'éléments, seuls ces 3 éléments sont évalués.

**Composable** — Les views peuvent être enchaînées les unes aux autres via l'opérateur `|`.

## L'opérateur pipe : le cœur du paradigme

L'opérateur `|` permet de chaîner des views en pipelines, dans un style qui rappelle les pipes Unix ou les méthodes chaînées de LINQ (C#), Stream (Java) ou les iterators de Rust :

```cpp
#include <ranges>
#include <vector>
#include <print>

std::vector<int> data = {5, 3, 8, 1, 6, 2, 7, 4, 9};

auto result = data
    | std::views::filter([](int n) { return n % 2 == 0; })   // garder les pairs
    | std::views::transform([](int n) { return n * n; })      // élever au carré
    | std::views::take(3);                                     // prendre les 3 premiers

for (int n : result) {
    std::print("{} ", n);  // 64 36 4
}
```

Le pipeline se lit de haut en bas comme une description de traitement : filtrer → transformer → limiter. L'ordre de lecture correspond à l'ordre d'exécution logique. Comparons avec la version classique en trois étapes et deux conteneurs intermédiaires vue plus haut — le gain en clarté est majeur.

### Comment fonctionne le pipe

L'expression `data | std::views::filter(pred)` est équivalente à `std::views::filter(data, pred)`. L'opérateur `|` est une convention syntaxique qui permet d'écrire le range source à gauche et la transformation à droite, autorisant le chaînage naturel.

Quand on écrit :

```cpp
auto pipeline = data
    | std::views::filter(f)
    | std::views::transform(g);
```

Le compilateur construit un objet view composé qui « empile » les transformations. Aucun calcul n'est effectué. L'évaluation n'a lieu que lorsqu'un consommateur (boucle `for`, algorithme, conversion en conteneur) itère sur `pipeline`.

## Views standard les plus utilisées

C++20 et C++23 fournissent un ensemble riche de views dans `<ranges>`. Voici les plus courantes.

### Filtrage et transformation

```cpp
#include <ranges>
#include <vector>
#include <string>
#include <print>

std::vector<int> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

// filter : garder les éléments satisfaisant un prédicat
auto odds = nums | std::views::filter([](int n) { return n % 2 != 0; });
// → 1, 3, 5, 7, 9

// transform : appliquer une fonction à chaque élément
auto squares = nums | std::views::transform([](int n) { return n * n; });
// → 1, 4, 9, 16, 25, 36, 49, 64, 81, 100
```

### Découpage et sélection

```cpp
// take : les N premiers éléments
auto first_five = nums | std::views::take(5);
// → 1, 2, 3, 4, 5

// drop : ignorer les N premiers éléments
auto after_three = nums | std::views::drop(3);
// → 4, 5, 6, 7, 8, 9, 10

// take_while : prendre tant que le prédicat est vrai
auto small = nums | std::views::take_while([](int n) { return n < 6; });
// → 1, 2, 3, 4, 5

// drop_while : ignorer tant que le prédicat est vrai
auto big = nums | std::views::drop_while([](int n) { return n < 6; });
// → 6, 7, 8, 9, 10
```

### Génération

```cpp
// iota : séquence croissante à partir d'une valeur
auto from_one = std::views::iota(1);        // 1, 2, 3, 4, ... (infini !)  
auto range_10 = std::views::iota(1, 11);    // 1, 2, 3, ..., 10 (fini)  

// Combinaison : les 5 premiers carrés parfaits
auto perfect_squares = std::views::iota(1)
    | std::views::transform([](int n) { return n * n; })
    | std::views::take(5);
// → 1, 4, 9, 16, 25
```

`std::views::iota` est remarquable parce qu'il montre la puissance de la lazy evaluation : la séquence infinie `iota(1)` ne pose aucun problème tant qu'un `take` ou un `take_while` en amont limite la consommation.

### Aplatissement et découpage de chaînes

```cpp
#include <ranges>
#include <vector>
#include <string>
#include <string_view>

// reverse : parcours inverse
std::vector<int> v = {1, 2, 3, 4, 5};  
auto reversed = v | std::views::reverse;  
// → 5, 4, 3, 2, 1

// split (C++20) : découper une chaîne
std::string csv = "Alice,Bob,Clara,Dave";  
for (auto word : csv | std::views::split(',')) {  
    // word est un sous-range de caractères
    auto sv = std::string_view(word.begin(), word.end());
    // sv successivement : "Alice", "Bob", "Clara", "Dave"
}

// join : aplatir un range de ranges
std::vector<std::vector<int>> nested = {{1, 2}, {3, 4}, {5}};  
auto flat = nested | std::views::join;  
// → 1, 2, 3, 4, 5
```

### Enrichissement (C++23)

C++23 ajoute plusieurs views particulièrement utiles :

```cpp
#include <ranges>
#include <vector>
#include <string>
#include <print>

std::vector<std::string> names = {"Alice", "Bob", "Clara"};

// enumerate (C++23) : ajoute un index à chaque élément
for (auto [i, name] : std::views::enumerate(names)) {
    std::print("[{}] {}\n", i, name);
}
// [0] Alice
// [1] Bob
// [2] Clara

// zip (C++23) : combine plusieurs ranges en parallèle
std::vector<int> scores = {95, 87, 92};  
for (auto [name, score] : std::views::zip(names, scores)) {  
    std::print("{} : {}\n", name, score);
}
// Alice : 95
// Bob : 87
// Clara : 92

// chunk (C++23) : découper un range en blocs de taille fixe
auto pairs = std::views::iota(1, 11) | std::views::chunk(3);
// → {1,2,3}, {4,5,6}, {7,8,9}, {10}

// slide (C++23) : fenêtre glissante
auto windows = std::views::iota(1, 6) | std::views::slide(3);
// → {1,2,3}, {2,3,4}, {3,4,5}
```

Les views `enumerate` et `zip` remplacent des patterns manuels extrêmement courants — compteur d'index à la main, itération synchronisée sur deux conteneurs — par des abstractions propres, sûres et composables.

## Matérialisation : de la view au conteneur

Les views sont paresseuses et non-owning. Si on a besoin d'un vrai conteneur possédant ses données, il faut *matérialiser* la view. En C++23, `std::ranges::to` simplifie cette conversion :

```cpp
#include <ranges>
#include <vector>
#include <print>

auto squares = std::views::iota(1, 11)
    | std::views::filter([](int n) { return n % 2 == 0; })
    | std::views::transform([](int n) { return n * n; })
    | std::ranges::to<std::vector>();   // C++23 : matérialisation directe

// squares est un std::vector<int> = {4, 16, 36, 64, 100}
```

Avant C++23, la matérialisation nécessitait une construction manuelle :

```cpp
// Pré-C++23 : construction manuelle
auto view = std::views::iota(1, 11)
    | std::views::filter([](int n) { return n % 2 == 0; })
    | std::views::transform([](int n) { return n * n; });

std::vector<int> result(std::ranges::begin(view), std::ranges::end(view));
```

`std::ranges::to` est un ajout majeur de C++23 qui rend le workflow pipeline → conteneur fluide et naturel.

## Exemple complet : traitement de données

Pour illustrer la puissance des pipelines, voici un scénario réaliste — analyser une liste de mesures de température :

```cpp
#include <ranges>
#include <vector>
#include <string>
#include <algorithm>
#include <print>

struct Measurement {
    std::string sensor_id;
    double temperature;
    bool valid;
};

std::vector<Measurement> readings = {
    {"sensor-01", 22.5, true},
    {"sensor-02", -99.0, false},   // invalide
    {"sensor-01", 23.1, true},
    {"sensor-03", 18.7, true},
    {"sensor-02", 21.0, true},
    {"sensor-01", 24.0, true},
    {"sensor-03", -99.0, false},   // invalide
    {"sensor-02", 22.3, true},
};

// Pipeline : filtrer les invalides → extraire les températures → trier → top 5
auto top_temps = readings
    | std::views::filter([](const Measurement& m) { return m.valid; })
    | std::views::transform([](const Measurement& m) { return m.temperature; })
    | std::ranges::to<std::vector>(); // matérialiser pour trier

std::ranges::sort(top_temps, std::greater{});  
auto top_5 = top_temps   
    | std::views::take(5);

std::print("Top 5 températures :\n");  
for (auto [i, temp] : std::views::enumerate(top_5)) {  
    std::print("  {}. {:.1f}°C\n", i + 1, temp);
}
```

Le pipeline décrit clairement le flux de données : filtrer → extraire → trier → limiter → afficher. Chaque étape est une transformation isolée, testable indépendamment, et le tout s'exécute avec un minimum d'allocations.

## Ranges et performances

### La lazy evaluation paie

Parce que les views ne matérialisent pas de conteneurs intermédiaires, un pipeline de views n'effectue qu'un seul passage sur les données (pour les pipelines linéaires). Considérons :

```cpp
auto result = data
    | std::views::filter(pred)
    | std::views::transform(func)
    | std::views::take(10);
```

Lors de l'itération, chaque élément de `data` est testé par `pred` ; s'il passe, il est transformé par `func` ; et dès que 10 éléments ont été produits, l'itération s'arrête. Il n'y a pas de vecteur intermédiaire contenant les éléments filtrés, ni de vecteur intermédiaire contenant les éléments transformés.

Pour un `data` d'un million d'éléments dont seuls 10 passent le filtre dans les 1000 premiers, la version pipeline ne touche que ~1000 éléments. La version classique avec `copy_if` dans un vecteur intermédiaire parcourt le million entier avant de s'arrêter.

### Limites de performance

La lazy evaluation n'est pas toujours optimale. Les views introduisent une couche d'indirection lors de l'itération (chaque appel à `++` sur l'itérateur peut impliquer un appel de prédicat de filtre). Pour des traitements lourds sur des données déjà en mémoire sans filtrage, un algorithme direct peut être marginalement plus rapide.

En pratique, la différence est rarement mesurable. Mais pour du code critique en performance, le profilage (chapitre 31) reste indispensable — comme pour toute décision d'optimisation.

### Quand matérialiser ?

Certaines opérations nécessitent un accès aléatoire ou plusieurs passes sur les données (tri, recherche binaire, suppression de doublons). Dans ces cas, il faut matérialiser la view en conteneur avec `std::ranges::to` ou une construction manuelle, puis appliquer l'algorithme sur le conteneur.

La règle est intuitive : les views sont idéales pour les transformations linéaires (filtre, map, take, drop) ; les algorithmes mutants ou à accès aléatoire (sort, unique, binary_search) nécessitent un conteneur matérialisé.

## Ranges vs algorithmes classiques : lequel choisir ?

Les deux approches coexistent et les algorithmes classiques ne sont pas dépréciés. Voici une grille de décision :

**Préférer les algorithmes ranges (`std::ranges::*`)** dans tous les cas. Ils acceptent directement un conteneur (pas besoin de paires d'itérateurs), ils sont contraints par des concepts (meilleurs messages d'erreur), et ils s'intègrent dans des pipelines. Il n'y a essentiellement aucune raison de préférer `std::sort` à `std::ranges::sort` dans du code nouveau.

**Utiliser les views et l'opérateur `|`** dès qu'il y a deux transformations ou plus à enchaîner, ou quand la lazy evaluation apporte un bénéfice (filtrage avant transformation, limitation du nombre d'éléments, séquences infinies).

**Rester sur les algorithmes classiques** quand on travaille avec du code legacy qui n'a pas migré vers C++20, ou dans les rares cas où une bibliothèque tierce attend explicitement des paires d'itérateurs.

## Bonnes pratiques

**Lire les pipelines de haut en bas.** Organiser chaque étape sur sa propre ligne avec indentation pour que le flux de données soit immédiatement visible. Le pipeline doit se lire comme une phrase : « prendre les données, filtrer les valides, extraire les scores, trier, garder les 10 meilleurs ».

**Matérialiser uniquement quand c'est nécessaire.** Laisser les données dans une view le plus longtemps possible. Ne convertir en conteneur que pour les opérations qui l'exigent (tri, accès aléatoire, stockage à long terme).

**Préférer `std::ranges::to<std::vector>()` (C++23)** à la construction manuelle avec des itérateurs. C'est plus lisible et moins sujet aux erreurs.

**Attention aux durées de vie.** Comme les views sont non-owning, elles deviennent invalides si le conteneur source est détruit ou modifié de manière à invalider ses itérateurs (par exemple, un `push_back` sur un `vector`). Ne pas stocker des views en tant que membres de classe sans garantir la survie du conteneur source.

**Combiner views standard et lambdas.** La force des pipelines vient de la composition de briques simples. Les views standard (`filter`, `transform`, `take`, `drop`, etc.) couvrent 90 % des besoins. Les lambdas fournissent la logique métier spécifique de chaque étape.

---

>  
> 📎 [15.6 Ranges : Simplification des algorithmes — Couverture approfondie](/15-algorithmes-stl/06-ranges.md)

⏭️ [Coroutines (C++20) : Programmation asynchrone](/12-nouveautes-cpp17-26/06-coroutines.md)
