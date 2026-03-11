🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 16.4 — SFINAE (Substitution Failure Is Not An Error)

## Chapitre 16 : Templates et Métaprogrammation · Module 5 : La STL

---

## Introduction

SFINAE est l'acronyme de *Substitution Failure Is Not An Error* — « un échec de substitution n'est pas une erreur ». Derrière ce nom intimidant se cache un principe simple : lorsque le compilateur tente d'instancier un template et que la substitution d'un paramètre de type produit une expression invalide, **cette candidature est silencieusement écartée** plutôt que de provoquer une erreur de compilation. Le compilateur continue à chercher une autre surcharge ou spécialisation valide.

Ce mécanisme, présent dans le langage depuis C++98, est devenu un outil central de la métaprogrammation. Il permet d'activer ou de désactiver des surcharges de fonctions et des spécialisations de classes **en fonction des propriétés des types**, créant ainsi une forme de dispatch conditionnel à la compilation.

SFINAE a dominé la programmation générique avancée en C++ pendant deux décennies. Depuis C++20, les **Concepts** (section 16.6) offrent une alternative beaucoup plus lisible pour la majorité des cas. Néanmoins, comprendre SFINAE reste indispensable : le mécanisme est omniprésent dans la STL, dans Boost, et dans tout code C++ pré-C++20 que vous rencontrerez en production.

---

## Le principe fondamental

Pour comprendre SFINAE, il faut d'abord comprendre ce qui se passe quand le compilateur résout un appel à une fonction template.

### Le processus de résolution de surcharge

Quand le compilateur rencontre un appel de fonction, il :

1. **Identifie les candidats** — toutes les fonctions et templates de fonctions visibles qui portent le bon nom.
2. **Substitue les types** — pour chaque template candidat, il remplace les paramètres template par les types déduits ou spécifiés.
3. **Élimine les candidats invalides** — si la substitution produit un type ou une expression invalide *dans le contexte immédiat* de la signature, le candidat est écarté (c'est SFINAE).
4. **Sélectionne le meilleur candidat** — parmi les candidats restants, il applique les règles de résolution de surcharge.

L'étape 3 est le cœur de SFINAE : un échec de substitution **dans la signature** du template n'est pas fatal. Le candidat est simplement retiré de l'ensemble, comme s'il n'avait jamais existé.

### Un premier exemple

```cpp
template <typename T>  
typename T::value_type extraire(const T& conteneur) {  
    return *conteneur.begin();
}

template <typename T>  
std::enable_if_t<!std::is_class_v<T>, T> extraire(T valeur) {  
    return valeur;
}
```

```cpp
std::vector<int> v{10, 20, 30};  
int n = 42;  

auto a = extraire(v);   // Appelle la première surcharge (T::value_type existe pour vector)  
auto b = extraire(n);   // Appelle la seconde surcharge  
```

Que se passe-t-il pour `extraire(n)` avec `T = int` ?

1. Le compilateur essaie la première surcharge : il substitue `T = int` et obtient `int::value_type` comme type de retour.
2. `int::value_type` n'existe pas — c'est un échec de substitution.
3. Grâce à SFINAE, ce n'est pas une erreur. La première surcharge est simplement écartée.
4. Le compilateur essaie la seconde surcharge : `T = int`, signature `int extraire(int)`. C'est valide.
5. La seconde surcharge est sélectionnée.

Sans SFINAE, l'échec sur `int::value_type` aurait provoqué une erreur de compilation immédiate, même si une surcharge parfaitement valide existait.

---

## Contexte immédiat vs corps de la fonction

SFINAE ne s'applique que dans le **contexte immédiat** de la substitution, c'est-à-dire dans la signature du template : le type de retour, les paramètres, les paramètres template eux-mêmes et les expressions `requires` (C++20). Les erreurs qui surviennent **dans le corps** de la fonction ne sont pas couvertes par SFINAE et produisent des erreurs de compilation classiques.

```cpp
// SFINAE s'applique ici : l'erreur est dans la signature (type de retour)
template <typename T>  
typename T::value_type fonction_a(T val) {  
    return val[0];  // Le corps n'est compilé que si la signature est valide
}

// SFINAE ne s'applique PAS ici : la signature est valide pour tout T,
// mais le corps peut échouer
template <typename T>  
void fonction_b(T val) {  
    typename T::value_type x = val[0];  // Erreur DURE si T n'a pas value_type
}
```

```cpp
fonction_a(42);  // OK : SFINAE écarte cette surcharge silencieusement  
fonction_b(42);  // ERREUR de compilation : int::value_type dans le corps  
```

Cette distinction est cruciale. Pour exploiter SFINAE, les contraintes doivent être exprimées **dans la signature**, jamais dans le corps.

---

## `std::enable_if` : SFINAE structuré

Écrire des contraintes SFINAE directement dans le type de retour fonctionne, mais la syntaxe devient vite illisible. La bibliothèque standard fournit `std::enable_if` (depuis C++11), un outil qui structure et standardise l'utilisation de SFINAE.

### Fonctionnement de `std::enable_if`

`std::enable_if` est un template de structure défini conceptuellement ainsi :

```cpp
// Implémentation simplifiée
template <bool Condition, typename T = void>  
struct enable_if {};  // Cas général : pas de membre 'type'  

template <typename T>  
struct enable_if<true, T> {  
    using type = T;   // Spécialisation : 'type' existe seulement si Condition == true
};
```

Quand `Condition` est `true`, `enable_if<true, T>::type` est défini et vaut `T`. Quand `Condition` est `false`, le membre `type` n'existe pas, ce qui provoque un échec de substitution — et SFINAE entre en jeu.

L'alias `std::enable_if_t<Condition, T>` (C++14) est un raccourci pour `typename std::enable_if<Condition, T>::type`.

### Utilisation dans le type de retour

```cpp
#include <type_traits>

// Active uniquement pour les types entiers
template <typename T>  
std::enable_if_t<std::is_integral_v<T>, T>  
doubler(T valeur) {  
    return valeur * 2;
}

// Active uniquement pour les types à virgule flottante
template <typename T>  
std::enable_if_t<std::is_floating_point_v<T>, T>  
doubler(T valeur) {  
    return valeur * 2.0;
}
```

```cpp
auto a = doubler(21);      // T = int, is_integral_v<int> == true → première surcharge  
auto b = doubler(1.5);     // T = double, is_floating_point_v<double> == true → seconde  
// doubler("hello");        // ERREUR : aucune surcharge ne correspond
```

Pour `doubler(21)` : le compilateur essaie les deux surcharges. La première produit `enable_if_t<true, int>` = `int` (valide). La seconde produit `enable_if_t<false, int>` — `type` n'existe pas — SFINAE l'écarte. La première est sélectionnée.

### Utilisation comme paramètre template par défaut

Une autre forme courante place `enable_if` dans un paramètre template supplémentaire avec valeur par défaut. Cela laisse le type de retour propre :

```cpp
template <typename T,
          std::enable_if_t<std::is_integral_v<T>, int> = 0>
T doubler(T valeur) {
    return valeur * 2;
}

template <typename T,
          std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
T doubler(T valeur) {
    return valeur * 2.0;
}
```

Le `= 0` est une valeur par défaut pour le paramètre non-type (de type `int`). Sa valeur n'a aucune importance : ce paramètre n'existe que pour activer ou désactiver la surcharge via SFINAE. Si `enable_if_t` ne peut pas être évalué (condition `false`), la substitution échoue et la surcharge est écartée.

### Utilisation dans les templates de classes

`enable_if` peut aussi conditionner des spécialisations partielles de classes :

```cpp
// Template principal : pas de contrainte
template <typename T, typename Enable = void>  
class NumericOps {  
public:  
    static void info() {
        std::print("Type non numerique\n");
    }
};

// Spécialisation partielle : active pour les types arithmétiques
template <typename T>  
class NumericOps<T, std::enable_if_t<std::is_arithmetic_v<T>>> {  
public:  
    static void info() {
        std::print("Type arithmetique (taille : {} octets)\n", sizeof(T));
    }

    static T clamped_add(T a, T b, T max_val) {
        T result = a + b;
        return (result > max_val) ? max_val : result;
    }
};
```

```cpp
NumericOps<int>::info();          // "Type arithmetique (taille : 4 octets)"  
NumericOps<std::string>::info();  // "Type non numerique"  
```

Le mécanisme repose sur le paramètre `Enable`. Pour `int`, `enable_if_t<is_arithmetic_v<int>>` vaut `void`, ce qui correspond au paramètre par défaut `void` du template principal. La spécialisation partielle est donc plus spécifique et elle est sélectionnée. Pour `std::string`, `is_arithmetic_v` est `false`, `enable_if_t` échoue, la spécialisation est écartée par SFINAE, et le template principal est utilisé.

---

## Expressions SFINAE (C++11)

Avant C++11, SFINAE ne s'appliquait qu'aux types présents dans la signature. C++11 a étendu le mécanisme aux **expressions** : une expression invalide dans la signature (type de retour trailing, `decltype`, paramètre par défaut) déclenche aussi SFINAE.

### `decltype` et SFINAE

`decltype` évalue le type d'une expression sans l'exécuter. Combiné avec une expression qui n'est valide que pour certains types, il crée une condition SFINAE :

```cpp
// Active uniquement si T supporte l'opérateur +
template <typename T>  
auto additionner(T a, T b) -> decltype(a + b) {  
    return a + b;
}
```

Pour `additionner(3, 4)`, `decltype(a + b)` avec `a` et `b` de type `int` produit `int` — valide. Pour un type sans `operator+`, `decltype(a + b)` échoue et SFINAE écarte la surcharge.

### `declval` : simuler une valeur sans la créer

`std::declval<T>()` est un outil complémentaire qui produit une « fausse » référence rvalue de type `T` dans un contexte non évalué (comme `decltype`). Il permet de tester des opérations sur un type sans avoir besoin de le construire :

```cpp
#include <utility>  // std::declval

// Teste si T a une méthode .size() retournant quelque chose de convertible en size_t
template <typename T>  
auto taille(const T& obj) -> decltype(static_cast<std::size_t>(obj.size())) {  
    return static_cast<std::size_t>(obj.size());
}

// Fallback : désactivé pour les types classe (qui peuvent avoir .size())
template <typename T,
          std::enable_if_t<!std::is_class_v<T>, int> = 0>
std::size_t taille(const T&) {
    return 0;  // Type sans .size()
}
```

```cpp
std::vector<int> v{1, 2, 3};  
std::print("{}\n", taille(v));   // 3 — première surcharge  
std::print("{}\n", taille(42));  // 0 — fallback  
```

`std::declval` est particulièrement utile dans les traits de type, où il n'est pas possible de construire une instance du type :

```cpp
// Trait : T a-t-il une méthode serialize() ?
template <typename T, typename = void>  
struct has_serialize : std::false_type {};  

template <typename T>  
struct has_serialize<T,  
    std::void_t<decltype(std::declval<T>().serialize())>>
    : std::true_type {};
```

Ce pattern utilise `std::void_t`, que nous allons examiner maintenant.

---

## `std::void_t` : SFINAE sur les expressions (C++17)

`std::void_t` est un alias template remarquablement simple mais extrêmement puissant :

```cpp
template <typename...>  
using void_t = void;  // Produit toujours void, si les arguments sont valides  
```

Son utilité repose entièrement sur SFINAE : si l'un des arguments template est une expression invalide, la substitution échoue *avant* que `void_t` ne puisse produire `void`. Le candidat est écarté.

### Détecter l'existence d'un type membre

```cpp
// Template principal : par défaut, T n'a pas de value_type
template <typename T, typename = void>  
struct has_value_type : std::false_type {};  

// Spécialisation : active si T::value_type est un type valide
template <typename T>  
struct has_value_type<T, std::void_t<typename T::value_type>>  
    : std::true_type {};
```

```cpp
static_assert(has_value_type<std::vector<int>>::value);  // true  
static_assert(has_value_type<std::map<int,int>>::value); // true  
static_assert(!has_value_type<int>::value);               // false  
static_assert(!has_value_type<double>::value);             // false  
```

### Détecter la validité d'une expression

```cpp
// T supporte-t-il l'opérateur << avec un ostream ?
template <typename T, typename = void>  
struct is_printable : std::false_type {};  

template <typename T>  
struct is_printable<T,  
    std::void_t<decltype(std::declval<std::ostream&>() << std::declval<T>())>>
    : std::true_type {};
```

```cpp
static_assert(is_printable<int>::value);          // true  
static_assert(is_printable<std::string>::value);  // true  

struct Opaque {};  
static_assert(!is_printable<Opaque>::value);      // false  
```

### Mécanisme détaillé

Pourquoi ça fonctionne ? Suivons le raisonnement pour `has_value_type<int>` :

1. Le compilateur considère la spécialisation `has_value_type<int, std::void_t<typename int::value_type>>`.
2. Il évalue `typename int::value_type` — `int` n'a pas de type membre `value_type`.
3. L'expression est invalide → SFINAE écarte cette spécialisation.
4. Le compilateur utilise le template principal → `has_value_type<int>` hérite de `std::false_type`.

Pour `has_value_type<std::vector<int>>` :

1. `typename std::vector<int>::value_type` est `int` — valide.
2. `std::void_t<int>` produit `void`.
3. La spécialisation `has_value_type<std::vector<int>, void>` correspond au paramètre par défaut `void` du template principal.
4. La spécialisation est plus spécifique → sélectionnée → hérite de `std::true_type`.

---

## Patterns classiques pré-C++20

Voici un récapitulatif des patterns SFINAE que vous rencontrerez dans du code existant, accompagnés de leur traduction en Concepts C++20 (section 16.6) pour faciliter la comparaison.

### Contraindre une fonction à un ensemble de types

```cpp
// SFINAE (C++11/14)
template <typename T,
          std::enable_if_t<std::is_integral_v<T>, int> = 0>
void traiter(T val) {
    std::print("Entier : {}\n", val);
}

// Concepts (C++20) — équivalent
template <std::integral T>  
void traiter(T val) {  
    std::print("Entier : {}\n", val);
}
```

### Contraindre selon la présence d'une méthode

```cpp
// SFINAE (C++11/14)
template <typename T,
          typename = std::void_t<decltype(std::declval<T>().begin()),
                                  decltype(std::declval<T>().end())>>
void iterer(const T& conteneur) {
    for (const auto& elem : conteneur) {
        std::print("{} ", elem);
    }
}

// Concepts (C++20) — équivalent
template <std::ranges::range T>  
void iterer(const T& conteneur) {  
    for (const auto& elem : conteneur) {
        std::print("{} ", elem);
    }
}
```

### Dispatch conditionnel (tag dispatching)

Avant `if constexpr` et les Concepts, un idiome courant combinait SFINAE avec le **tag dispatching** — utiliser un type vide comme « étiquette » pour router la résolution de surcharge :

```cpp
// Tags
struct integral_tag {};  
struct floating_tag {};  
struct other_tag {};  

// Sélection du tag
template <typename T>  
auto type_tag() {  
    if constexpr (std::is_integral_v<T>) return integral_tag{};
    else if constexpr (std::is_floating_point_v<T>) return floating_tag{};
    else return other_tag{};
}

// Surcharges basées sur le tag
template <typename T>  
void traiter_impl(T val, integral_tag) {  
    std::print("Traitement entier : {}\n", val);
}

template <typename T>  
void traiter_impl(T val, floating_tag) {  
    std::print("Traitement flottant : {}\n", val);
}

template <typename T>  
void traiter_impl(T val, other_tag) {  
    std::print("Traitement autre\n");
}

// Interface publique
template <typename T>  
void traiter(T val) {  
    traiter_impl(val, type_tag<T>());
}
```

Ce pattern est lisible et extensible, mais verbeux. En C++20, un simple `requires` ou un concept nommé remplace l'ensemble du mécanisme.

---

## Les limites de SFINAE

SFINAE est puissant mais souffre de plusieurs faiblesses qui ont motivé l'introduction des Concepts.

### Messages d'erreur obscurs

Quand aucune surcharge ne correspond après élimination par SFINAE, le compilateur affiche la liste des candidats rejetés avec leurs raisons d'échec. Pour des expressions complexes imbriquant `enable_if`, `void_t`, `decltype` et `declval`, ces messages sont souvent longs et pratiquement illisibles :

```
error: no matching function for call to 'traiter'  
note: candidate template ignored: requirement  
  'std::is_integral_v<std::basic_string<char>>' was not satisfied
note: candidate template ignored: substitution failure
  [with T = std::basic_string<char>]:
  type 'std::basic_string<char>' does not provide a member 'value_type'
  ... (20 lignes supplémentaires)
```

Les Concepts produisent des messages directs : « le type `std::string` ne satisfait pas le concept `std::integral` ».

### Fragilité syntaxique

La moindre erreur dans une expression SFINAE (une parenthèse manquante, un `typename` oublié) peut transformer un échec de substitution silencieux en erreur de compilation dure — ou pire, produire un comportement inattendu sans aucun diagnostic.

### Difficulté de composition

Combiner plusieurs contraintes SFINAE nécessite d'imbriquer des `enable_if` avec des opérateurs logiques sur les traits :

```cpp
// Contrainte : T est entier ET non-signé
template <typename T,
          std::enable_if_t<
              std::is_integral_v<T> && std::is_unsigned_v<T>,
          int> = 0>
void traiter_unsigned(T val);
```

C'est fonctionnel mais laborieux. L'équivalent en Concepts est immédiatement lisible :

```cpp
template <typename T>
    requires std::integral<T> && std::unsigned_integral<T>
void traiter_unsigned(T val);
```

### Pas de diagnostic intentionnel

SFINAE ne permet pas d'expliquer *pourquoi* une contrainte n'est pas satisfaite. Un candidat écarté est silencieux. Les Concepts, en revanche, permettent au compilateur de nommer la contrainte violée et de produire un message d'erreur orienté vers l'utilisateur.

---

## SFINAE dans la STL : exemples concrets

La bibliothèque standard utilise SFINAE extensivement. En voici quelques illustrations que vous pouvez observer dans les headers de votre compilateur.

### `std::is_same`

```cpp
// Implémentation simplifiée
template <typename T, typename U>  
struct is_same : std::false_type {};  

template <typename T>  
struct is_same<T, T> : std::true_type {};  // Spécialisation partielle, pas SFINAE,  
                                            // mais souvent combiné avec enable_if
```

### Constructeur conditionnel de `std::optional`

`std::optional<T>` possède un constructeur de conversion depuis `std::optional<U>` qui n'est activé que si `U` est convertible en `T`. En interne, cela utilise SFINAE :

```cpp
// Simplifié — la vraie implémentation est plus complexe
template <typename U,
          std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
optional(const optional<U>& other);
```

### `std::vector::push_back` vs `emplace_back`

Les surcharges de méthodes dans les conteneurs utilisent SFINAE pour activer des overloads selon que le type est copiable, déplaçable, etc.

---

## Écrire du SFINAE maintenable

Si vous devez écrire du code SFINAE (maintenir du code pré-C++20, supporter des compilateurs plus anciens), quelques techniques améliorent la lisibilité.

### Encapsuler les contraintes dans des traits nommés

Plutôt que d'écrire des expressions `void_t` / `decltype` directement dans les signatures, créez des traits nommés qui documentent l'intention :

```cpp
// Trait nommé : est-ce un conteneur itérable ?
template <typename T, typename = void>  
struct is_iterable : std::false_type {};  

template <typename T>  
struct is_iterable<T, std::void_t<  
    decltype(std::declval<T>().begin()),
    decltype(std::declval<T>().end())>>
    : std::true_type {};

template <typename T>  
constexpr bool is_iterable_v = is_iterable<T>::value;  

// Utilisation : propre et lisible
template <typename T,
          std::enable_if_t<is_iterable_v<T>, int> = 0>
void afficher_elements(const T& conteneur) {
    for (const auto& elem : conteneur) {
        std::print("{} ", elem);
    }
    std::print("\n");
}
```

Le trait `is_iterable_v` est réutilisable, testable indépendamment (avec `static_assert`), et son nom documente l'intention.

### Créer des alias pour les enable_if courants

```cpp
// Alias réutilisables
template <typename T>  
using require_integral = std::enable_if_t<std::is_integral_v<T>, int>;  

template <typename T>  
using require_floating = std::enable_if_t<std::is_floating_point_v<T>, int>;  

// Utilisation
template <typename T, require_integral<T> = 0>  
T safe_divide(T a, T b) {  
    if (b == 0) throw std::domain_error("Division par zero");
    return a / b;
}
```

### Commenter les contraintes

Le SFINAE est intrinsèquement opaque pour un lecteur qui ne maîtrise pas la technique. Un commentaire bref sur chaque contrainte aide considérablement la maintenance :

```cpp
// Activée uniquement si T est un type entier signé
template <typename T,
          std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T>, int> = 0>
T valeur_absolue(T val) {
    return (val < 0) ? -val : val;
}
```

---

## Migration SFINAE → Concepts

Pour les projets qui peuvent cibler C++20, chaque pattern SFINAE a un équivalent en Concepts plus lisible et plus sûr. Voici un tableau de correspondance :

| Pattern SFINAE | Équivalent Concepts (C++20) |
|---|---|
| `enable_if_t<is_integral_v<T>>` | `template <std::integral T>` |
| `enable_if_t<is_floating_point_v<T>>` | `template <std::floating_point T>` |
| `void_t<decltype(val.begin())>` | `requires { val.begin(); }` |
| `enable_if_t<is_convertible_v<U, T>>` | `requires std::convertible_to<U, T>` |
| Trait `has_serialize<T>` + `enable_if` | `concept Serializable = requires(T t) { t.serialize(); }` |
| `enable_if_t<cond1 && cond2>` | `requires Concept1<T> && Concept2<T>` |

La migration n'est pas toujours immédiate (certaines expressions SFINAE complexes nécessitent des concepts composés), mais le résultat est systématiquement plus clair. La section 16.6 couvre cette transition en détail.

---

## Bonnes pratiques

**Favorisez les Concepts en C++20.** Si votre projet cible C++20 ou ultérieur, préférez systématiquement les Concepts à SFINAE. Le code est plus lisible, les messages d'erreur sont meilleurs, et les contraintes sont plus faciles à composer.

**Encapsulez le SFINAE dans des traits nommés.** Ne laissez jamais des expressions `void_t` / `decltype` / `enable_if` brutes dans les signatures publiques. Créez des traits avec des noms explicites.

**Placez les contraintes dans les paramètres template.** La forme `enable_if_t<..., int> = 0` en paramètre template est généralement préférée à la forme dans le type de retour, car elle laisse la signature plus propre.

**Testez vos traits avec `static_assert`.** Chaque trait personnalisé devrait être accompagné de `static_assert` vérifiant les cas positifs et négatifs. C'est la façon la plus fiable de valider une expression SFINAE.

**N'utilisez pas SFINAE pour les fonctions si la surcharge suffit.** Comme discuté en section 16.3, la surcharge classique est plus simple et plus prévisible que la spécialisation ou le SFINAE pour les templates de fonctions.

**Documentez l'intention.** SFINAE est intrinsèquement obscur. Un commentaire d'une ligne expliquant la contrainte en langage naturel rend le code accessible aux développeurs qui ne maîtrisent pas la technique.

---

## En résumé

SFINAE est le mécanisme par lequel le compilateur écarte silencieusement un template candidat lorsque la substitution des paramètres produit une expression invalide dans le contexte immédiat de la signature. Combiné avec `std::enable_if`, `std::void_t`, `decltype` et `std::declval`, il permet de conditionner l'activation de surcharges et de spécialisations en fonction des propriétés des types — une forme de dispatch conditionnel à la compilation.

Ce mécanisme est omniprésent dans la STL et dans le code C++ existant. Il reste nécessaire de le comprendre pour lire et maintenir ce code. Mais pour du code nouveau, les **Concepts C++20** (section 16.6) sont l'alternative recommandée : ils offrent la même expressivité avec une syntaxe déclarative, des messages d'erreur clairs et une composabilité naturelle.

La section suivante (16.5) explore les **variadic templates**, qui permettent aux templates d'accepter un nombre arbitraire de paramètres — un autre pilier de la métaprogrammation moderne.

⏭️ [Variadic templates (C++11)](/16-templates-metaprogrammation/05-variadic-templates.md)
