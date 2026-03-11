🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 16.6 — Concepts (C++20) pour contraindre les templates ⭐

## Chapitre 16 : Templates et Métaprogrammation · Module 5 : La STL

---

## Introduction

Les quatre sections précédentes ont posé les fondations de la programmation générique en C++ : templates de fonctions et de classes, spécialisation, SFINAE, variadic templates. Ces mécanismes sont puissants, mais ils partagent une faiblesse majeure : les **contraintes sur les types restent implicites**. Rien dans la signature d'un template ne dit explicitement ce que le type `T` doit savoir faire. Si `T` ne satisfait pas les attentes du template, le compilateur produit une erreur — souvent au plus profond de l'instanciation, dans un message incompréhensible qui mentionne des détails d'implémentation internes plutôt que le vrai problème.

SFINAE et `std::enable_if` apportent une réponse technique, mais au prix d'une syntaxe lourde et fragile (section 16.4). Pendant des années, la communauté C++ a cherché un moyen de rendre les contraintes **explicites, lisibles et composables**. Ce moyen existe depuis C++20 : les **Concepts**.

Un Concept est une **contrainte nommée** sur un ou plusieurs paramètres template. Il exprime de manière déclarative les exigences qu'un type doit satisfaire pour être utilisé avec un template donné. Les Concepts transforment fondamentalement la façon d'écrire, de lire et de déboguer du code générique.

> 📎 *La section 12.4 offre un premier aperçu des Concepts dans le cadre du survol des nouveautés C++20. Cette section fournit la couverture complète et approfondie, avec les trois syntaxes d'utilisation, les concepts standard, la création de concepts personnalisés et les patterns avancés.*

---

## Le problème que les Concepts résolvent

### Des contraintes invisibles

Considérons un template apparemment simple :

```cpp
template <typename T>  
T maximum(T a, T b) {  
    return (a > b) ? a : b;
}
```

Pour que ce template fonctionne, `T` doit supporter l'opérateur `>`. Mais cette exigence n'apparaît nulle part dans la signature. Elle est enfouie dans le corps de la fonction. Si un utilisateur tente d'appeler `maximum` avec un type sans `operator>`, le compilateur produit une erreur pointant vers **l'intérieur** du template, pas vers le point d'appel.

### Messages d'erreur catastrophiques

Prenons un exemple plus réaliste avec la STL :

```cpp
struct Point { int x, y; };

std::vector<Point> points = {{3,1}, {1,2}, {2,3}};  
std::sort(points.begin(), points.end());  // ERREUR  
```

`std::sort` requiert que les éléments soient comparables avec `<`. `Point` ne l'est pas. Sur GCC ou Clang sans Concepts, l'erreur produit typiquement **des dizaines de lignes** pointant vers les profondeurs de l'implémentation de `std::sort`, mentionnant des types internes comme `__gnu_cxx::__ops::_Iter_less_iter` et des instanciations imbriquées sur cinq niveaux.

Avec les Concepts activés (et les versions récentes de la STL qui les utilisent), le message devient :

```
error: no matching function for call to 'sort'  
note: constraints not satisfied  
note: the expression 'a < b' is not valid for type 'Point'  
```

Clair, concis, actionnable.

### SFINAE : puissant mais illisible

La section 16.4 a montré comment SFINAE et `enable_if` permettent de contraindre les templates. Le résultat est fonctionnel mais difficilement lisible :

```cpp
// SFINAE : contrainte sur T — doit être un type entier
template <typename T,
          std::enable_if_t<std::is_integral_v<T>, int> = 0>
T doubler(T valeur) {
    return valeur * 2;
}
```

L'équivalent avec un Concept :

```cpp
// Concept : même contrainte, lisible comme de la prose
template <std::integral T>  
T doubler(T valeur) {  
    return valeur * 2;
}
```

La contrainte est **dans la signature**, pas dans un mécanisme technique caché dans un paramètre template par défaut. L'intention est immédiatement visible.

---

## Qu'est-ce qu'un Concept ?

Un Concept est un **prédicat nommé** évalué à la compilation, qui prend un ou plusieurs paramètres de type et retourne `true` ou `false`. Il décrit un ensemble d'exigences qu'un type doit satisfaire.

### Définition formelle

```cpp
template <typename T>  
concept NomDuConcept = /* expression booléenne constante impliquant T */;  
```

Le mot-clé `concept` introduit la définition. L'expression à droite du `=` est une **contrainte** — une expression booléenne évaluée à la compilation. Si elle vaut `true` pour un type donné, le type **satisfait** le concept ; sinon, il ne le satisfait pas.

### Un premier concept simple

```cpp
template <typename T>  
concept Numeric = std::is_arithmetic_v<T>;  
```

Ce concept `Numeric` est satisfait par `int`, `double`, `float`, `char`, `bool`, etc. — tous les types pour lesquels `std::is_arithmetic_v` vaut `true`. Il est rejeté par `std::string`, `std::vector<int>`, ou tout type défini par l'utilisateur non-arithmétique.

```cpp
static_assert(Numeric<int>);           // true  
static_assert(Numeric<double>);        // true  
static_assert(!Numeric<std::string>);  // false  
```

### Un concept avec clause `requires`

Les concepts deviennent vraiment expressifs lorsqu'ils utilisent une **clause `requires`**, qui permet de tester directement si des expressions sont valides pour un type donné :

```cpp
template <typename T>  
concept Addable = requires(T a, T b) {  
    { a + b } -> std::convertible_to<T>;  // a + b doit être valide et convertible en T
};
```

Ce concept exige que pour deux valeurs de type `T`, l'expression `a + b` soit valide et que son résultat soit convertible en `T`. La syntaxe `requires` et ses variantes sont couvertes en détail dans la sous-section 16.6.1.

---

## Les trois syntaxes d'utilisation

Une fois un concept défini, il peut être appliqué à un template de trois manières équivalentes. Chaque syntaxe a le même effet : si le type fourni ne satisfait pas le concept, la surcharge ou la spécialisation est écartée (comme SFINAE, mais avec des messages d'erreur clairs).

### Syntaxe 1 : contrainte dans la déclaration du paramètre template

Le concept remplace `typename` ou `class` dans la liste des paramètres template :

```cpp
template <std::integral T>  
T doubler(T valeur) {  
    return valeur * 2;
}
```

C'est la forme la plus concise et la plus lisible pour les contraintes simples portant sur un seul type.

### Syntaxe 2 : clause `requires` après la signature

La clause `requires` est placée après la liste des paramètres template (ou après la signature de la fonction) :

```cpp
template <typename T>
    requires std::integral<T>
T doubler(T valeur) {
    return valeur * 2;
}
```

Cette forme est préférable quand la contrainte est plus complexe, qu'elle combine plusieurs concepts, ou qu'elle porte sur des relations entre plusieurs paramètres :

```cpp
template <typename T, typename U>
    requires std::convertible_to<U, T> && std::integral<T>
T ajouter(T a, U b) {
    return a + static_cast<T>(b);
}
```

### Syntaxe 3 : `auto` contraint (abrégé)

La forme la plus courte utilise `auto` avec un concept dans les paramètres de la fonction, sans même écrire `template` :

```cpp
std::integral auto doubler(std::integral auto valeur) {
    return valeur * 2;
}
```

Chaque paramètre `auto` contraint est un paramètre template implicite. Cette syntaxe est idéale pour les fonctions courtes et les lambdas :

```cpp
auto tripler = [](std::integral auto val) { return val * 3; };
```

### Équivalence des trois formes

Les trois syntaxes produisent exactement le même code compilé. Le choix est une question de style et de clarté :

| Syntaxe | Idéale pour |
|---|---|
| `template <Concept T>` | Contrainte simple sur un type, forme standard |
| `requires Concept<T>` | Contraintes composées, relations entre types |
| `Concept auto param` | Fonctions courtes, lambdas, style concis |

---

## Concepts et résolution de surcharge

Les Concepts participent à la résolution de surcharge de manière naturelle. Le compilateur préfère la surcharge dont les contraintes sont les **plus spécifiques** :

```cpp
template <typename T>  
void traiter(T val) {  
    std::print("type quelconque\n");
}

template <std::integral T>  
void traiter(T val) {  
    std::print("type entier\n");
}

template <std::signed_integral T>  
void traiter(T val) {  
    std::print("type entier signe\n");
}
```

```cpp
traiter(3.14);             // "type quelconque" — double n'est pas integral  
traiter(42u);              // "type entier" — unsigned int est integral mais pas signed  
traiter(42);               // "type entier signe" — int est signed_integral  
```

Le compilateur sait que `std::signed_integral` est plus spécifique que `std::integral` (car `signed_integral` **subsume** `integral` — tout type signed_integral est aussi integral, mais pas l'inverse). Il sélectionne la surcharge la plus contrainte qui correspond au type fourni.

Ce mécanisme de **subsomption** (*subsumption*) est ce qui rend les Concepts supérieurs à SFINAE pour le dispatch : avec SFINAE et `enable_if`, le compilateur ne peut pas comparer la spécificité de deux contraintes et signale une ambiguïté. Avec les Concepts, il comprend la hiérarchie des contraintes et choisit automatiquement la meilleure correspondance.

---

## Concepts et classes template

Les Concepts s'appliquent aussi aux classes template, avec la même clarté :

```cpp
template <std::integral T>  
class Counter {  
public:  
    explicit Counter(T initial = T{0}) : value_{initial} {}

    void increment() { ++value_; }
    void decrement() { --value_; }
    T get() const { return value_; }

private:
    T value_;
};
```

```cpp
Counter<int> c1{10};          // OK : int satisfait std::integral  
Counter<unsigned long> c2;    // OK : unsigned long aussi  

// Counter<double> c3;        // ERREUR : double ne satisfait pas std::integral
// Diagnostic clair : "constraints not satisfied: std::integral<double>"
```

On peut aussi utiliser `requires` pour contraindre des spécialisations partielles :

```cpp
// Template principal : accepte tout type
template <typename T>  
class Wrapper {  
public:  
    void info() const { std::print("Wrapper generique\n"); }
};

// Spécialisation contrainte : uniquement pour les types arithmétiques
template <typename T>
    requires std::is_arithmetic_v<T>
class Wrapper<T> {  
public:  
    void info() const { std::print("Wrapper numerique\n"); }
};
```

---

## Concepts et variadic templates

Les Concepts s'intègrent naturellement avec les variadic templates (section 16.5) pour contraindre chaque élément d'un parameter pack :

```cpp
// Chaque argument doit être affichable via std::print (std::formattable est C++23)
template <typename... Args>
    requires (std::formattable<Args, char> && ...)
void print_all(Args&&... args) {
    (std::print("{} ", args), ...);
    std::print("\n");
}
```

L'expression `(std::formattable<Args, char> && ...)` est un **fold expression** (section 16.7) sur la contrainte : elle vérifie que *chaque* type du pack satisfait le concept `std::formattable`. Si un seul type échoue, toute la contrainte est rejetée avec un message d'erreur indiquant quel type pose problème.

Avec la syntaxe `auto` contraint :

```cpp
void print_all(std::formattable<char> auto&&... args) {
    (std::print("{} ", args), ...);
    std::print("\n");
}
```

---

## Avantages des Concepts : synthèse

Comparons point par point les Concepts avec les approches antérieures :

| Critère | Sans contrainte | SFINAE / `enable_if` | Concepts (C++20) |
|---|---|---|---|
| **Lisibilité** | Contrainte invisible | Lourde, technique | Déclarative, proche du langage naturel |
| **Messages d'erreur** | Profonds et obscurs | Légèrement meilleurs | Clairs et ciblés |
| **Composabilité** | N/A | Laborieuse (`&&`, `\|\|` dans `enable_if`) | Naturelle (`&&`, `\|\|`, subsomption) |
| **Overload ranking** | Pas de dispatch | Ambiguïtés fréquentes | Subsomption automatique |
| **Documentation** | Le code ne dit rien | Les traits servent de doc implicite | Le concept **est** la documentation |
| **Vérification** | À l'instanciation | À l'instanciation | À l'utilisation (avant l'instanciation du corps) |

Le dernier point est particulièrement important. Avec les Concepts, le compilateur vérifie la contrainte **avant** d'instancier le corps du template. L'erreur se produit au point d'appel, pas dans les profondeurs de l'implémentation.

---

## Contenu des sous-sections

Cette section se décompose en trois sous-sections qui approfondissent chaque aspect :

**16.6.1 — Syntaxe `requires`** : la clause `requires` et les `requires`-expressions en détail. Les quatre types d'exigences (*simple requirements*, *type requirements*, *compound requirements*, *nested requirements*). La différence entre clause `requires` et expression `requires`. Les contraintes composées avec `&&` et `||`.

**16.6.2 — Concepts standard de la STL** : tour d'horizon des concepts prédéfinis dans `<concepts>`, `<ranges>`, `<iterator>` et autres headers. Les familles de concepts : core language (`same_as`, `convertible_to`, `integral`...), comparison (`equality_comparable`, `totally_ordered`...), object (`movable`, `copyable`, `regular`...), callable (`invocable`, `predicate`...) et iterator/range.

**16.6.3 — Création de concepts personnalisés** : concevoir des concepts spécifiques à votre domaine. Patterns de composition, bonnes pratiques de nommage, granularité des concepts, tests avec `static_assert`, et intégration dans l'architecture d'un projet.

---

## Prérequis pour cette section

Les Concepts s'appuient sur les mécanismes couverts dans les sections précédentes de ce chapitre :

- les **templates de fonctions** (16.1) et de **classes** (16.2) — les Concepts les contraignent ;
- la **spécialisation** (16.3) — les Concepts peuvent conditionner des spécialisations ;
- **SFINAE** (16.4) — les Concepts en sont le successeur direct ;
- les **variadic templates** (16.5) — les Concepts peuvent contraindre les parameter packs.

Une familiarité avec les traits de type de `<type_traits>` (`std::is_integral_v`, `std::is_same_v`, etc.) est également utile, car de nombreux concepts standard les encapsulent.

---

> **Note pédagogique** 💡 — Les Concepts sont la fonctionnalité C++20 qui a le plus d'impact sur l'écriture quotidienne de code générique. Si vous ne deviez retenir qu'une chose de ce chapitre, ce serait cette section. Les sous-sections qui suivent sont conçues pour être explorées séquentiellement : la syntaxe `requires` d'abord, puis les concepts standard comme vocabulaire, et enfin la création de concepts personnalisés pour votre propre code.

⏭️ [Syntaxe requires](/16-templates-metaprogrammation/06.1-requires.md)
