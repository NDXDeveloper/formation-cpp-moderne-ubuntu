🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 16.7 — Fold expressions (C++17)

## Chapitre 16 : Templates et Métaprogrammation · Module 5 : La STL

---

## Introduction

La section 16.5 a présenté les variadic templates et la technique classique pour traiter un parameter pack : la récursion de templates. Chaque élément est séparé du reste (head/tail), traité individuellement, puis la récursion se poursuit jusqu'à un cas de base. Cette approche fonctionne, mais elle est verbeuse, génère de nombreuses instanciations et peut atteindre les limites de profondeur du compilateur.

Les **fold expressions**, introduites en C++17, résolvent ces problèmes en offrant une syntaxe concise pour appliquer un opérateur binaire sur l'ensemble des éléments d'un parameter pack, **en une seule expression**. Pas de récursion, pas de cas de base séparé, pas d'instanciations intermédiaires.

Si les variadic templates sont le moteur, les fold expressions sont la boîte de vitesses automatique : elles rendent le mécanisme beaucoup plus simple à utiliser au quotidien.

---

## Principe fondamental

Une fold expression « replie » (*fold*) un parameter pack en appliquant un opérateur binaire entre chaque élément, de manière analogue à `std::accumulate` mais **à la compilation** et avec une syntaxe intégrée au langage.

Pour un pack `args...` contenant les éléments `a, b, c, d` et un opérateur `+`, un fold produit :

```
a + b + c + d
```

Le compilateur génère cette expression étendue directement, sans récursion ni instanciations intermédiaires.

---

## Les quatre formes syntaxiques

Il existe exactement quatre formes de fold expressions, selon deux axes : la présence ou l'absence d'une **valeur initiale**, et le sens de l'**associativité** (gauche ou droite).

Toutes les formes sont encadrées par des **parenthèses obligatoires**.

### 1. Unary right fold

```cpp
(pack op ...)
```

Expansion : `a op (b op (c op d))`  — associativité droite.

```cpp
template <typename... Args>  
auto somme(Args... args) {  
    return (args + ...);
}

somme(1, 2, 3, 4);  // 1 + (2 + (3 + 4)) = 10
```

### 2. Unary left fold

```cpp
(... op pack)
```

Expansion : `((a op b) op c) op d`  — associativité gauche.

```cpp
template <typename... Args>  
auto somme(Args... args) {  
    return (... + args);
}

somme(1, 2, 3, 4);  // ((1 + 2) + 3) + 4 = 10
```

### 3. Binary right fold

```cpp
(pack op ... op init)
```

Expansion : `a op (b op (c op (d op init)))` — associativité droite, avec valeur initiale.

```cpp
template <typename... Args>  
auto somme(Args... args) {  
    return (args + ... + 0);  // 0 est la valeur initiale
}

somme();             // 0 (pack vide → retourne init)  
somme(1, 2, 3);     // 1 + (2 + (3 + 0)) = 6  
```

### 4. Binary left fold

```cpp
(init op ... op pack)
```

Expansion : `(((init op a) op b) op c) op d` — associativité gauche, avec valeur initiale.

```cpp
template <typename... Args>  
auto somme(Args... args) {  
    return (0 + ... + args);
}

somme();             // 0  
somme(1, 2, 3);     // ((0 + 1) + 2) + 3 = 6  
```

### Tableau récapitulatif

| Forme | Syntaxe | Expansion (a, b, c) | Pack vide |
|---|---|---|---|
| Unary right fold | `(args op ...)` | `a op (b op c)` | Erreur (sauf `&&`, `\|\|`, `,`) |
| Unary left fold | `(... op args)` | `(a op b) op c` | Erreur (sauf `&&`, `\|\|`, `,`) |
| Binary right fold | `(args op ... op init)` | `a op (b op (c op init))` | `init` |
| Binary left fold | `(init op ... op args)` | `((init op a) op b) op c` | `init` |

> **Règle clé** — Les folds **unaires** échouent à la compilation sur un pack vide, sauf pour les opérateurs `&&` (valeur par défaut `true`), `||` (valeur par défaut `false`) et `,` (valeur par défaut `void()`). Les folds **binaires** retournent la valeur initiale quand le pack est vide, ce qui les rend plus sûrs.

---

## Opérateurs supportés

Les fold expressions supportent la grande majorité des opérateurs binaires de C++ :

**Arithmétiques** : `+`, `-`, `*`, `/`, `%`

**Logiques** : `&&`, `||`

**Binaires (bitwise)** : `&`, `|`, `^`

**Décalage** : `<<`, `>>`

**Comparaison** : `==`, `!=`, `<`, `>`, `<=`, `>=`

**Affectation** : `=`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=`

**Virgule** : `,`

**Pointeur vers membre** : `.*`, `->*`

L'opérateur **virgule** (`,`) mérite une attention particulière : il permet d'exécuter une expression **pour chaque élément du pack**, sans accumuler de résultat. C'est la forme la plus fréquemment utilisée en pratique.

---

## Cas d'usage concrets

### Somme et produit

La forme la plus directe — accumuler une valeur :

```cpp
template <typename... Args>  
auto somme(Args... args) {  
    return (0 + ... + args);   // Binary left fold, safe sur pack vide
}

template <typename... Args>  
auto produit(Args... args) {  
    return (1 * ... * args);   // Élément neutre = 1
}
```

```cpp
std::print("{}\n", somme(1, 2, 3, 4, 5));     // 15  
std::print("{}\n", produit(2, 3, 4));           // 24  
std::print("{}\n", somme());                     // 0  
std::print("{}\n", produit());                   // 1  
```

### Affichage de tous les éléments

L'opérateur virgule exécute chaque sous-expression dans l'ordre :

```cpp
template <typename... Args>  
void print_all(Args&&... args) {  
    (std::print("{} ", args), ...);
    std::print("\n");
}
```

```cpp
print_all(1, "hello", 3.14, true);
// 1 hello 3.14 true
```

L'expansion produit :

```cpp
std::print("{} ", 1), (std::print("{} ", "hello"), (std::print("{} ", 3.14), std::print("{} ", true)));
```

Chaque `std::print` est exécuté séquentiellement grâce à l'opérateur virgule.

### Vérification universelle (`&&`) et existentielle (`||`)

```cpp
// Tous les arguments sont-ils vrais ?
template <typename... Args>  
bool tous_vrais(Args... args) {  
    return (... && args);   // Unary left fold avec &&
}

// Au moins un argument est-il vrai ?
template <typename... Args>  
bool au_moins_un(Args... args) {  
    return (... || args);   // Unary left fold avec ||
}
```

```cpp
std::print("{}\n", tous_vrais(true, true, true));    // true  
std::print("{}\n", tous_vrais(true, false, true));   // false  
std::print("{}\n", au_moins_un(false, false, true));  // true  
```

Les folds unaires sur `&&` et `||` sont les seules qui acceptent un pack vide : `&&` retourne `true` (neutre de la conjonction) et `||` retourne `false` (neutre de la disjonction).

### Vérifier qu'une valeur appartient à un ensemble

```cpp
template <typename T, typename... Options>  
bool est_parmi(const T& valeur, const Options&... options) {  
    return (... || (valeur == options));
}
```

```cpp
std::print("{}\n", est_parmi(3, 1, 2, 3, 4));        // true  
std::print("{}\n", est_parmi("b", "a", "b", "c"));   // true (const char* comparison)  
std::print("{}\n", est_parmi(7, 1, 2, 3));            // false  
```

L'expansion pour `est_parmi(3, 1, 2, 3, 4)` produit :

```cpp
((3 == 1) || (3 == 2)) || (3 == 3)) || (3 == 4)
```

Le court-circuit de `||` s'applique : dès qu'une comparaison retourne `true`, les suivantes ne sont pas évaluées.

### Insertion dans un flux ou un conteneur

L'opérateur `<<` s'utilise en left fold pour chaîner les insertions dans un flux :

```cpp
template <typename... Args>  
void log(std::ostream& os, Args&&... args) {  
    (os << ... << args);
    os << '\n';
}
```

```cpp
log(std::cout, "User ", "Alice", " logged in at ", 1709123456);
// User Alice logged in at 1709123456
```

L'expansion produit : `((((os << "User ") << "Alice") << " logged in at ") << 1709123456)`.

Pour insérer dans un conteneur, l'opérateur virgule avec `push_back` :

```cpp
template <typename Container, typename... Args>  
void push_all(Container& c, Args&&... args) {  
    (c.push_back(std::forward<Args>(args)), ...);
}
```

```cpp
std::vector<int> v;  
push_all(v, 1, 2, 3, 4, 5);  
// v = {1, 2, 3, 4, 5}
```

---

## Fold expressions et Concepts

Les fold expressions se combinent avec les Concepts (section 16.6) pour contraindre les packs de manière expressive :

### Contraindre chaque élément du pack

```cpp
template <typename... Args>
    requires (std::integral<Args> && ...)
auto somme_entiers(Args... args) {
    return (0 + ... + args);
}
```

L'expression `(std::integral<Args> && ...)` est un fold unaire right sur `&&` : elle exige que **chaque** type du pack satisfasse `std::integral`. Si un seul type échoue, la contrainte entière est rejetée.

```cpp
auto s = somme_entiers(1, 2, 3);         // OK : int, int, int
// auto e = somme_entiers(1, 2.0, 3);    // ERREUR : double ne satisfait pas integral
```

### Contraindre l'existence d'une opération entre éléments

```cpp
template <typename T, typename... Args>
    requires (std::convertible_to<Args, T> && ...)
T construire_depuis(Args&&... args) {
    // Utilise le premier argument convertible, ou une logique personnalisée
    T result{};
    ((result = static_cast<T>(args)), ...);  // Le dernier argument "gagne"
    return result;
}
```

### Fold dans une définition de concept

Un concept lui-même peut utiliser un fold pour exprimer une contrainte sur un pack :

```cpp
template <typename... Ts>  
concept AllRegular = (std::regular<Ts> && ...);  

template <typename... Ts>
    requires AllRegular<Ts...>
class MultiStore {
    std::tuple<Ts...> data_;
};
```

---

## Fold expressions avec des patterns complexes

Le « pattern » replié peut être n'importe quelle expression impliquant un élément du pack, pas seulement l'élément brut.

### Transformation avant fold

```cpp
template <typename... Args>  
auto somme_carres(Args... args) {  
    return (0 + ... + (args * args));
}
```

```cpp
std::print("{}\n", somme_carres(1, 2, 3, 4));  // 1 + 4 + 9 + 16 = 30
```

### Appel de fonction sur chaque élément

```cpp
template <typename F, typename... Args>  
void appliquer(F&& f, Args&&... args) {  
    (f(std::forward<Args>(args)), ...);
}
```

```cpp
appliquer([](auto x) { std::print("{} ", x * 2); }, 1, 2, 3);
// 2 4 6
```

### Combinaison avec des index

En combinant avec `std::index_sequence` (section 16.5), on peut accéder à la position de chaque élément :

```cpp
template <typename Tuple, std::size_t... Is>  
void afficher_indexed_impl(const Tuple& t, std::index_sequence<Is...>) {  
    ((std::print("[{}] = {} ", Is, std::get<Is>(t))), ...);
    std::print("\n");
}

template <typename... Types>  
void afficher_indexed(const std::tuple<Types...>& t) {  
    afficher_indexed_impl(t, std::index_sequence_for<Types...>{});
}
```

```cpp
auto t = std::make_tuple(42, 3.14, "hello"s);  
afficher_indexed(t);  
// [0] = 42 [1] = 3.14 [2] = hello
```

---

## Associativité : quand ça compte

Pour les opérateurs commutatifs et associatifs (`+`, `*`, `&&`, `||`), le choix entre left fold et right fold ne change pas le résultat. Mais pour les opérateurs non-associatifs, l'ordre d'évaluation compte :

### Soustraction

```cpp
template <typename... Args>  
auto sub_left(Args... args) {  
    return (... - args);      // Left fold : ((a - b) - c) - d
}

template <typename... Args>  
auto sub_right(Args... args) {  
    return (args - ...);      // Right fold : a - (b - (c - d))
}
```

```cpp
std::print("{}\n", sub_left(10, 3, 2, 1));   // ((10 - 3) - 2) - 1 = 4  
std::print("{}\n", sub_right(10, 3, 2, 1));  // 10 - (3 - (2 - 1)) = 8  
```

### Opérateur `<<` avec les flux

L'opérateur `<<` est naturellement associatif à gauche (`os << a << b << c`), donc le **left fold** est la forme correcte :

```cpp
// CORRECT : left fold
(os << ... << args);
// Produit : ((os << a) << b) << c

// INCORRECT : right fold
(args << ... << os);
// Produirait : a << (b << (c << os)) — sens inversé, types incompatibles
```

### Règle générale

Utilisez le **left fold** (`(... op args)` ou `(init op ... op args)`) pour les opérateurs qui s'enchaînent de gauche à droite dans l'usage normal (`<<`, `-`, `/`, opérateurs d'affectation). Utilisez le **right fold** pour les cas rares où l'associativité droite est souhaitée (construction récursive de structures, par exemple).

En pratique, le left fold binaire `(init op ... op args)` est la forme la plus fréquente et la plus sûre (gestion du pack vide incluse).

---

## Fold expressions vs récursion : comparaison

Reprenons l'exemple d'affichage de la section 16.5 et comparons les deux approches :

### Récursion (C++11)

```cpp
// Cas de base
void afficher() {
    std::print("\n");
}

// Cas récursif
template <typename T, typename... Rest>  
void afficher(T premier, Rest... reste) {  
    std::print("{} ", premier);
    afficher(reste...);
}
```

Trois entités : deux surcharges + N instanciations récursives.

### Fold expression (C++17)

```cpp
template <typename... Args>  
void afficher(Args... args) {  
    (std::print("{} ", args), ...);
    std::print("\n");
}
```

Une seule entité, une seule instanciation, pas de récursion.

### Comparaison

| Critère | Récursion | Fold expression |
|---|---|---|
| **Lignes de code** | ~10 (2 surcharges) | ~4 (1 fonction) |
| **Instanciations** | N+1 (une par niveau) | 1 |
| **Profondeur de templates** | N | 1 |
| **Temps de compilation** | Croît avec N | Constant |
| **Lisibilité** | Familière mais verbeuse | Concise, idiomatique en C++17 |
| **Flexibilité** | Accès à la position, logique complexe | Limitée à un opérateur binaire |

Le dernier point est important : la récursion reste nécessaire quand chaque élément nécessite un traitement différent selon sa position, ou quand la logique ne se réduit pas à un opérateur binaire simple. Les fold expressions couvrent la majorité des cas courants ; la récursion gère les cas restants.

---

## Limites

**Un seul opérateur par fold.** On ne peut pas enchaîner deux opérateurs différents dans une même fold expression. Pour des opérations composites, il faut soit imbriquer des folds, soit revenir à la récursion.

**Pas d'accès à l'index.** Contrairement à la récursion ou aux index sequences, une fold expression ne connaît pas la position de l'élément courant dans le pack.

**Opérateurs surchargés.** Si un opérateur est surchargé de manière inattendue pour un type donné, la fold expression produira un résultat surprenant. Les Concepts (section 16.6) permettent de protéger les fold expressions en contraignant les types du pack.

**Packs vides avec folds unaires.** En dehors de `&&`, `||` et `,`, un fold unaire sur un pack vide est une erreur de compilation. Préférez les folds binaires avec une valeur initiale appropriée pour les fonctions qui peuvent recevoir zéro argument.

---

## Bonnes pratiques

**Préférez les fold expressions à la récursion pour les opérations simples.** Somme, produit, affichage, vérification de prédicat, insertion : toutes ces opérations s'expriment en une ligne avec un fold.

**Utilisez les folds binaires pour gérer les packs vides.** `(0 + ... + args)` est toujours sûr. `(... + args)` échoue sur un pack vide. Sauf raison précise, la forme binaire est plus robuste.

**Choisissez l'associativité en conscience.** Left fold pour les opérateurs qui s'enchaînent naturellement de gauche à droite (`<<`, `-`, `/`). Right fold uniquement quand l'associativité droite est sémantiquement nécessaire.

**Combinez avec les Concepts pour la sûreté.** Contraignez les types du pack avant d'appliquer un fold : `requires (std::integral<Args> && ...)` évite les surprises avec des types incompatibles.

**Gardez les expressions de fold lisibles.** Une fold expression avec un pattern de trois lignes imbriquant des appels de fonction perd tout avantage de concision. Si le pattern devient complexe, extrayez-le dans une lambda ou revenez à la récursion.

**Documentez l'opérateur neutre.** Dans une fold binaire, la valeur initiale est l'élément neutre de l'opérateur : `0` pour `+`, `1` pour `*`, `true` pour `&&`, `false` pour `||`. Un choix incorrect produit des résultats silencieusement faux.

---

## En résumé

Les fold expressions offrent une syntaxe concise et efficace pour appliquer un opérateur binaire sur tous les éléments d'un parameter pack. Quatre formes existent, combinant associativité gauche/droite et présence/absence d'une valeur initiale. Elles éliminent la récursion de templates pour la majorité des opérations courantes : accumulation, vérification de prédicats, application d'une action à chaque élément, insertion dans un flux ou un conteneur.

Combinées avec les Concepts pour la sûreté des types et le perfect forwarding pour la préservation des catégories de valeur, les fold expressions complètent la boîte à outils de la programmation générique moderne en C++.

---

## Conclusion du chapitre 16

Ce chapitre a parcouru l'ensemble des mécanismes de programmation générique en C++, des templates de fonctions les plus simples (16.1) jusqu'aux fold expressions (16.7), en passant par les templates de classes (16.2), la spécialisation (16.3), SFINAE (16.4), les variadic templates (16.5) et les Concepts C++20 (16.6).

Ces mécanismes forment un ensemble cohérent : les templates sont le fondement, la spécialisation et SFINAE permettent l'adaptation, les variadic templates gèrent le nombre arbitraire de paramètres, les Concepts rendent les contraintes explicites, et les fold expressions simplifient les opérations sur les packs. Ensemble, ils constituent le socle sur lequel repose toute la STL — et sur lequel vous pouvez bâtir vos propres abstractions génériques, performantes et maintenables.

⏭️ [Module 6 : Gestion des Erreurs et Robustesse](/module-06-gestion-erreurs.md)
