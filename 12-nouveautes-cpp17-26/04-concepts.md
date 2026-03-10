🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 12.4 Concepts (C++20) : Contraintes sur les templates

## Donner un sens aux paramètres de template

Depuis les origines du langage, les templates C++ reposent sur un contrat implicite : le code template compile si — et seulement si — les types fournis supportent les opérations utilisées dans le corps du template. Ce contrat n'est documenté nulle part dans le code, il n'est vérifié qu'au moment de l'instanciation, et sa violation produit des messages d'erreur notoirement incompréhensibles.

Les Concepts de C++20 changent fondamentalement cette situation. Ils permettent d'exprimer des contraintes explicites sur les paramètres de template : « ce type doit être triable », « ce type doit supporter l'addition », « ce type doit être sérialisable ». Ces contraintes sont vérifiées au point d'appel, avant l'instanciation du template, et produisent des messages d'erreur clairs, ciblés et lisibles.

C'est l'une des fonctionnalités les plus attendues de l'histoire du C++ — des propositions existaient dès les années 2000, et il a fallu vingt ans pour aboutir à un design satisfaisant.

> 📎 *Cette section couvre les principes fondamentaux des Concepts. La section 16.6 approfondit la syntaxe `requires`, les concepts standard de la STL et la création de concepts personnalisés.*

## Le problème : des templates sans contrat explicite

### Messages d'erreur illisibles

Considérons une fonction template simple qui trie un conteneur :

```cpp
#include <algorithm>
#include <vector>
#include <list>

template <typename Container>  
void sort_container(Container& c) {  
    std::sort(c.begin(), c.end());
}
```

Cette fonction fonctionne avec `std::vector` mais pas avec `std::list`, car `std::sort` requiert des itérateurs à accès aléatoire (*random access iterators*), et `std::list` ne fournit que des itérateurs bidirectionnels. Que se passe-t-il quand on essaie ?

```cpp
std::list<int> lst = {3, 1, 4, 1, 5};  
sort_container(lst);  // Erreur de compilation  
```

Le compilateur produit une cascade de messages d'erreur qui pointe vers les entrailles de l'implémentation de `std::sort` — des lignes enfouies dans les headers de la bibliothèque standard. Le message mentionne des opérations comme `operator-` entre itérateurs, qui n'ont aucun sens pour l'utilisateur appelant `sort_container`. On ne sait pas immédiatement que le vrai problème est la catégorie d'itérateur.

Avec GCC, ces erreurs peuvent facilement atteindre des dizaines de lignes pour un simple appel invalide. Avec des templates plus complexes (templates de templates, SFINAE, etc.), les messages peuvent dépasser la centaine de lignes et devenir complètement inexploitables.

### Le contrat est invisible

Le problème de fond est que rien dans la signature de `sort_container` n'indique ce qui est attendu de `Container`. Le développeur doit lire le corps de la fonction (ou la documentation, quand elle existe) pour comprendre les exigences. C'est l'antithèse de la programmation par contrat.

Comparons avec une situation familière : les fonctions non-template ont des types explicites dans leur signature. Si une fonction attend un `int`, lui passer une `std::string` produit une erreur claire et immédiate. Les templates, eux, acceptent n'importe quoi syntaxiquement et ne se plaignent qu'au moment de l'instanciation — trop tard, trop profond, trop cryptique.

### SFINAE : le contournement historique

Avant C++20, la technique principale pour contraindre les templates était SFINAE (*Substitution Failure Is Not An Error*). Le principe : créer des surcharges qui « disparaissent » quand les types ne satisfont pas certaines conditions :

```cpp
#include <type_traits>

// Avant C++20 : SFINAE pour contraindre T à être un entier
template <typename T,
          typename = std::enable_if_t<std::is_integral_v<T>>>
T add(T a, T b) {
    return a + b;
}
```

SFINAE fonctionne, mais sa syntaxe est obscure, difficile à maintenir, et les erreurs qu'elle produit ne sont guère meilleures. C'est un hack ingénieux devenu convention, mais qui n'a jamais été conçu pour exprimer des contraintes lisiblement.

## La solution : les Concepts

Un concept est une **contrainte nommée** sur un ou plusieurs types. Il définit un ensemble d'exigences qu'un type doit satisfaire — opérations supportées, types associés, propriétés — et cette contrainte est vérifiée par le compilateur au point d'appel.

### Utiliser un concept existant

La bibliothèque standard de C++20 fournit un riche ensemble de concepts prédéfinis dans `<concepts>` et `<iterator>`. Reprenons l'exemple du tri :

```cpp
#include <algorithm>
#include <concepts>
#include <iterator>
#include <vector>
#include <list>

// Le concept std::sortable exprime exactement ce dont std::sort a besoin
template <std::ranges::random_access_range Container>  
void sort_container(Container& c) {  
    std::sort(c.begin(), c.end());
}
```

Maintenant, l'appel avec `std::list` produit un message d'erreur clair :

```
error: template constraint not satisfied: 'std::list<int>' does not satisfy 'random_access_range'
```

Une ligne. Le problème est nommé explicitement. Le développeur sait immédiatement quoi corriger.

### Quatre syntaxes pour contraindre un template

C++20 offre quatre manières d'appliquer un concept à un paramètre de template. Elles sont équivalentes sémantiquement — le choix est une question de style et de lisibilité.

**1. La clause `requires` après la liste de paramètres de template :**

```cpp
template <typename T>
    requires std::integral<T>
T add(T a, T b) {
    return a + b;
}
```

C'est la forme la plus explicite. La contrainte est clairement séparée de la déclaration du paramètre. Elle est particulièrement lisible quand plusieurs contraintes sont combinées.

**2. Le concept en lieu et place de `typename` :**

```cpp
template <std::integral T>  
T add(T a, T b) {  
    return a + b;
}
```

C'est la forme la plus concise pour les cas simples. Le concept remplace directement le mot-clé `typename` (ou `class`), exprimant que `T` doit satisfaire `std::integral`. C'est la syntaxe recommandée quand il n'y a qu'une seule contrainte simple.

**3. La clause `requires` en fin de signature (trailing) :**

```cpp
template <typename T>  
T add(T a, T b) requires std::integral<T> {  
    return a + b;
}
```

La contrainte est placée après les paramètres de fonction, avant le corps. Cette forme est parfois préférée quand la contrainte dépend des paramètres de la fonction eux-mêmes.

**4. La syntaxe abrégée avec `auto` contraint :**

```cpp
auto add(std::integral auto a, std::integral auto b) {
    return a + b;
}
```

La forme la plus compacte. Chaque paramètre porte sa propre contrainte. Attention : dans cette syntaxe, `a` et `b` peuvent être de types différents (les deux doivent juste satisfaire `std::integral`). Si on veut que `a` et `b` soient du même type, il faut utiliser l'une des trois premières syntaxes avec un seul paramètre `T`.

### Comparaison directe : avant et après les Concepts

Pour mesurer le gain, comparons une même contrainte exprimée avec SFINAE et avec un concept :

```cpp
// ─── SFINAE (avant C++20) ───────────────────────────────────
template <typename T,
          typename = std::enable_if_t<
              std::is_arithmetic_v<T> &&
              !std::is_same_v<T, bool>>>
T multiply(T a, T b) {
    return a * b;
}

// ─── Concepts (C++20) ──────────────────────────────────────
template <typename T>
    requires std::is_arithmetic_v<T> && (!std::same_as<T, bool>)
T multiply(T a, T b) {
    return a * b;
}
```

La version Concepts est plus lisible, mais surtout, les messages d'erreur en cas de violation sont radicalement meilleurs. Le compilateur nomme exactement la contrainte qui n'est pas satisfaite, au lieu de produire un échec de substitution anonyme enfoui dans les templates.

## Définir ses propres concepts

Au-delà des concepts standard, on peut définir des concepts spécifiques à son domaine. Un concept est défini avec le mot-clé `concept` et une expression booléenne à la compilation :

### Concept simple basé sur des traits

```cpp
#include <concepts>
#include <type_traits>

template <typename T>  
concept Numeric = std::is_arithmetic_v<T> && !std::is_same_v<T, bool>;  
```

Ce concept peut maintenant être utilisé exactement comme un concept standard :

```cpp
template <Numeric T>  
T multiply(T a, T b) {  
    return a * b;
}

multiply(3, 4);        // OK  
multiply(2.5, 1.5);    // OK  
multiply(true, false);  // Erreur : bool ne satisfait pas Numeric  
```

### Concept avec requires expression

Pour des contraintes plus riches — vérifier qu'un type supporte certaines opérations ou possède certains types membres — on utilise une *requires expression* :

```cpp
template <typename T>  
concept Printable = requires(T value) {  
    // T doit être formatable avec std::print via std::format
    { std::format("{}", value) } -> std::convertible_to<std::string>;
};
```

La `requires expression` est un bloc qui liste des exigences. Chaque ligne est une contrainte que le compilateur vérifie syntaxiquement et sémantiquement sans exécuter le code. Les accolades `{ expr }` suivies d'une flèche `->` vérifient que le résultat de l'expression satisfait un concept donné.

### Concept composé

Les concepts peuvent être composés par combinaison logique :

```cpp
template <typename T>  
concept Hashable = requires(T a) {  
    { std::hash<T>{}(a) } -> std::convertible_to<std::size_t>;
};

template <typename T>  
concept HashableEquality = Hashable<T> && std::equality_comparable<T>;  
```

Ce `HashableEquality` exprime exactement ce dont un `std::unordered_map` a besoin pour ses clés : un type qui est à la fois hachable et comparable pour l'égalité. L'intention est lisible directement dans le code.

### Un exemple plus complet : Serializable

Voici un concept plus élaboré qui capture l'idée d'un type sérialisable en JSON :

```cpp
#include <concepts>
#include <string>

template <typename T>  
concept Serializable = requires(const T& obj) {  
    // Doit avoir une méthode to_json() retournant un string
    { obj.to_json() } -> std::convertible_to<std::string>;
} && requires(const std::string& json) {
    // Doit avoir une méthode statique from_json()
    { T::from_json(json) } -> std::same_as<T>;
};

// Utilisation :
template <Serializable T>  
void save(const T& obj, const std::string& path) {  
    std::string json = obj.to_json();
    // ... écriture dans le fichier ...
}
```

Si un type ne fournit pas `to_json()` ou `from_json()`, l'erreur à la compilation nomme exactement la contrainte `Serializable` qui n'est pas satisfaite — pas un obscur échec de substitution dans le corps de `save`.

## Les concepts standard de la STL

C++20 fournit une bibliothèque riche de concepts prédéfinis, organisés en catégories. Voici les plus utilisés :

### Concepts fondamentaux (`<concepts>`)

| Concept | Signification |
|---------|--------------|
| `std::same_as<T, U>` | `T` et `U` sont le même type |
| `std::derived_from<T, Base>` | `T` dérive de `Base` |
| `std::convertible_to<T, U>` | `T` est convertible en `U` |
| `std::integral<T>` | `T` est un type entier |
| `std::floating_point<T>` | `T` est un type flottant |
| `std::copyable<T>` | `T` est copiable |
| `std::movable<T>` | `T` est déplaçable |
| `std::regular<T>` | `T` est copiable, comparable avec `==`, et default-constructible |

### Concepts de comparaison (`<concepts>`)

| Concept | Signification |
|---------|--------------|
| `std::equality_comparable<T>` | `T` supporte `==` et `!=` |
| `std::totally_ordered<T>` | `T` supporte `<`, `<=`, `>`, `>=` |

### Concepts d'itérateurs (`<iterator>`)

| Concept | Signification |
|---------|--------------|
| `std::input_iterator<I>` | Itérateur de lecture séquentielle |
| `std::forward_iterator<I>` | Itérateur multi-passe en avant |
| `std::bidirectional_iterator<I>` | Itérateur bidirectionnel |
| `std::random_access_iterator<I>` | Itérateur à accès aléatoire (ex: vector) |

### Concepts de ranges (`<ranges>`)

| Concept | Signification |
|---------|--------------|
| `std::ranges::range<R>` | `R` fournit `begin()` et `end()` |
| `std::ranges::input_range<R>` | Range avec itérateurs d'entrée |
| `std::ranges::random_access_range<R>` | Range avec accès aléatoire |
| `std::ranges::sized_range<R>` | Range dont la taille est connue en O(1) |

> 📎 *La section 16.6.2 détaille l'ensemble des concepts standard avec des exemples d'utilisation pour chaque catégorie.*

## Surcharge et résolution par contraintes

Les concepts s'intègrent au système de surcharge de fonctions de C++. Quand plusieurs surcharges sont disponibles et qu'un type satisfait les contraintes de plusieurs d'entre elles, le compilateur choisit la **surcharge la plus contrainte** :

```cpp
#include <concepts>
#include <print>

// Surcharge générale
template <typename T>  
void describe(const T& value) {  
    std::print("Valeur quelconque\n");
}

// Surcharge pour les types entiers (plus contrainte)
template <std::integral T>  
void describe(const T& value) {  
    std::print("Entier : {}\n", value);
}

// Surcharge pour les types flottants (plus contrainte)
template <std::floating_point T>  
void describe(const T& value) {  
    std::print("Flottant : {}\n", value);
}

describe(42);         // "Entier : 42" — std::integral est plus contraint que typename  
describe(3.14);       // "Flottant : 3.14"  
describe("hello"s);   // "Valeur quelconque" — ni integral ni floating_point  
```

Cette résolution est plus élégante et plus prévisible que la machinerie SFINAE équivalente. Le compilateur comprend les relations de subsomption entre concepts : `std::integral` est « plus spécifique » que `typename`, donc la surcharge contrainte est préférée quand elle s'applique.

### Subsomption : hiérarchie de contraintes

Le compilateur peut ordonner les concepts entre eux quand l'un implique l'autre :

```cpp
template <typename T>  
concept Animal = requires(T a) {  
    { a.name() } -> std::convertible_to<std::string>;
    { a.sound() } -> std::convertible_to<std::string>;
};

template <typename T>  
concept Pet = Animal<T> && requires(T a) {  
    { a.owner() } -> std::convertible_to<std::string>;
};

// Pet subsume Animal — un Pet est toujours un Animal
template <Animal T>  
void interact(const T& a) { /* comportement générique animal */ }  

template <Pet T>  
void interact(const T& p) { /* comportement spécifique animal de compagnie */ }  

// Si Dog satisfait Pet, la surcharge Pet est choisie (plus contrainte)
```

Cette hiérarchie de contraintes est résolue à la compilation sans polymorphisme dynamique, sans vtable, sans aucun coût à l'exécution.

## Concepts et messages d'erreur : la vraie révolution

Au-delà de l'expressivité, le bénéfice le plus tangible des concepts au quotidien est la qualité des messages d'erreur. Comparons un scénario réel.

**Sans concept — appel invalide de std::sort sur une std::list :**

Avec les bibliothèques standard pré-concepts, le compilateur produit une cascade d'erreurs pointant vers l'implémentation interne de `std::sort`, mentionnant des types comme `__gnu_cxx::__normal_iterator` et des opérations comme `operator-` entre itérateurs. Le message réel peut atteindre 50+ lignes et ne mentionne jamais le mot « random access ».

**Avec concept — même appel via les algorithmes ranges :**

```cpp
#include <algorithm>
#include <list>

std::list<int> lst = {3, 1, 2};  
std::ranges::sort(lst);  
```

Le message produit par GCC 15 ou Clang 20 ressemble à :

```
error: no matching call to 'std::ranges::sort(std::list<int>&)'  
note: constraint not satisfied: 'std::list<int>' does not satisfy 'random_access_range'  
```

Deux lignes. Le problème est identifié, nommé, et localisé au bon endroit — au point d'appel, pas dans les entrailles de la bibliothèque.

## Concepts et performances

Les concepts sont une fonctionnalité purement compile-time. Ils n'ajoutent aucune instruction au binaire produit, aucune indirection, aucune vtable. Le code contraint par des concepts génère exactement le même assembleur que le code template non contraint. Les concepts sont un outil de vérification statique, pas un mécanisme runtime.

Cela les différencie fondamentalement du polymorphisme dynamique (fonctions virtuelles, `dynamic_cast`) qui impose un coût à l'exécution. Les concepts offrent le meilleur des deux mondes : la flexibilité de la généricité et la vérification de contrat, sans aucun compromis de performance. On parle parfois de « polymorphisme statique contraint ».

## Bonnes pratiques

**Préférer les concepts standard quand ils existent.** Avant de définir un concept personnalisé, vérifier si `<concepts>`, `<iterator>` ou `<ranges>` ne fournit pas déjà ce qu'on cherche. Les concepts standard sont bien testés, universellement reconnus, et participent correctement à la subsomption.

**Utiliser la syntaxe `template <ConceptName T>` pour les cas simples.** C'est la forme la plus concise et la plus lisible quand il n'y a qu'une seule contrainte directe sur un paramètre.

**Utiliser la clause `requires` pour les contraintes composées.** Dès qu'on combine plusieurs concepts (`requires A<T> && B<T>`) ou qu'on exprime des contraintes complexes, la clause `requires` après le `template` offre la meilleure lisibilité.

**Nommer ses concepts comme des adjectifs.** Un bon concept décrit une capacité : `Sortable`, `Printable`, `Serializable`, `Hashable`. C'est cohérent avec les concepts de la STL (`integral`, `copyable`, `regular`).

**Contraindre les templates dès le premier jour.** Même dans du code interne, les concepts documentent l'intention et produisent de meilleurs messages d'erreur. C'est un investissement minimal pour un bénéfice immédiat en maintenabilité.

**Migrer progressivement depuis SFINAE.** Les concepts et `enable_if` peuvent coexister dans un même projet. La migration peut se faire fonction par fonction, en commençant par les signatures les plus visibles (API publiques, headers partagés).

---

>  
> 📎 [16.6 Concepts pour contraindre les templates — Couverture approfondie](/16-templates-metaprogrammation/06-concepts.md)

⏭️ [Ranges (C++20) : Pipelines fonctionnels](/12-nouveautes-cpp17-26/05-ranges.md)
