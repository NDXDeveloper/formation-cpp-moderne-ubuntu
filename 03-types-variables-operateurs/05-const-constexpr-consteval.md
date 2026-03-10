🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 3.5 — `const`, `constexpr` et `consteval` (C++20)

> **Chapitre 3 — Types, Variables et Opérateurs** · Section 5 sur 5  
> Prérequis : [3.4 — Portée des variables et durée de vie](/03-types-variables-operateurs/04-portee-duree-vie.md)

---

## Introduction

L'un des principes fondamentaux du développement logiciel est de rendre explicites les choses qui ne doivent pas changer. En C++, cette idée prend une forme particulièrement riche : le langage offre non pas un, mais **trois niveaux** d'immutabilité et de calcul anticipé, chacun avec ses garanties et ses domaines d'application.

- **`const`** — la valeur ne peut pas être modifiée **à l'exécution**. C'est une promesse faite au compilateur et aux autres développeurs : « cette donnée ne changera pas après son initialisation ».
- **`constexpr`** — la valeur ou le résultat d'une fonction **peut** être calculé à la compilation. Le compilateur est autorisé (et encouragé) à effectuer le calcul avant même que le programme ne s'exécute.
- **`consteval`** (C++20) — la fonction **doit** être évaluée à la compilation. Si elle est appelée dans un contexte qui ne le permet pas, c'est une erreur de compilation.

Ces trois qualificateurs forment un spectre progressif — de la simple protection contre les modifications accidentelles jusqu'à la garantie que tout le calcul est résolu avant l'exécution. Comprendre leurs différences et savoir quand utiliser chacun est l'un des marqueurs d'un code C++ moderne, performant et expressif.

---

## Pourquoi l'immutabilité est importante

### Prévention des bugs

La cause la plus élémentaire de bugs est la modification inattendue d'une donnée. Si une variable est déclarée `const`, toute tentative de la modifier provoque une erreur de compilation — le bug est éliminé avant même d'exister :

```cpp
const int max_retries = 5;  
max_retries = 10; // ❌ Erreur de compilation  
```

Dans un programme de plusieurs milliers de lignes, où une variable peut être lue et potentiellement modifiée dans des dizaines d'endroits, cette garantie statique est inestimable. Elle transforme une catégorie entière de bugs potentiels en erreurs de compilation détectées instantanément.

### Documentation du code

`const` communique une intention au lecteur humain. Quand un paramètre de fonction est déclaré `const`, le lecteur sait immédiatement que la fonction ne le modifiera pas — sans avoir besoin de lire le corps de la fonction :

```cpp
double compute_average(const std::vector<int>& data);
// Le const garantit que data ne sera pas modifié
```

Cette forme de documentation est supérieure aux commentaires : elle est vérifiée par le compilateur et ne peut pas devenir obsolète.

### Optimisation par le compilateur

Quand le compilateur sait qu'une valeur ne changera pas, il peut effectuer des optimisations impossibles autrement. Une variable `const` peut être placée en mémoire en lecture seule, sa valeur peut être propagée directement dans le code machine (*constant folding*), et les accès répétés peuvent être éliminés.

Avec `constexpr` et `consteval`, les optimisations vont encore plus loin : le calcul entier est effectué à la compilation. Le binaire résultant contient directement le résultat, sans aucune instruction de calcul à l'exécution. C'est comme si le programmeur avait écrit le résultat en dur — mais avec la maintenabilité du code source original.

---

## Le spectre de l'évaluation

Pour comprendre les trois qualificateurs, il est utile de les positionner sur un spectre allant de l'exécution pure à la compilation pure :

```
Exécution                                                        Compilation  
pure                                                               pure  
  │                                                                  │
  │    variable       const          constexpr         consteval     │
  │    ordinaire    (runtime)    (compile-time si     (compile-time  │
  │                              possible, runtime    obligatoire)   │
  │                              sinon)                              │
  ▼                                                                  ▼
```

**Variable ordinaire** — modifiable, évaluée à l'exécution. Aucune garantie d'immutabilité.

**`const`** — immutable après initialisation, mais l'initialisation elle-même peut se produire à l'exécution. La valeur n'est pas nécessairement connue à la compilation :

```cpp
const int x = compute_something(); // La valeur est fixée au runtime
// x ne changera plus, mais le compilateur ne connaît pas sa valeur à la compilation
```

**`constexpr`** — si le contexte le permet, le calcul est effectué à la compilation. Sinon, il est effectué à l'exécution. C'est un **mécanisme opportuniste** :

```cpp
constexpr int square(int n) { return n * n; }

constexpr int a = square(5);     // ✅ Évalué à la compilation → a vaut 25  
int runtime_val = get_input();  
int b = square(runtime_val);      // ✅ Évalué à l'exécution — c'est aussi valide  
```

**`consteval`** — le calcul est **obligatoirement** effectué à la compilation. Aucun appel runtime n'est possible :

```cpp
consteval int cube(int n) { return n * n * n; }

constexpr int a = cube(3);       // ✅ Évalué à la compilation → a vaut 27  
int runtime_val = get_input();  
int b = cube(runtime_val);        // ❌ Erreur de compilation : argument non constant  
```

---

## L'évolution historique

La progression de `const` vers `consteval` reflète l'évolution du langage sur trois décennies :

**C++ originel (années 1980)** — `const` est introduit comme alternative aux `#define` du préprocesseur C. L'objectif est de fournir une constante avec un vrai type, une vraie portée et une vérification à la compilation :

```cpp
// C : macro sans type ni portée
#define MAX_SIZE 1024

// C++ : constante typée avec portée
const int max_size = 1024;
```

**C++11** — `constexpr` est introduit pour permettre des calculs à la compilation au-delà de ce que `const` offre. Dans sa première version, les fonctions `constexpr` sont très limitées (un seul `return`, pas de boucles, pas de variables locales).

**C++14** — Les restrictions sur les fonctions `constexpr` sont largement levées : boucles, variables locales, `if/else`, plusieurs instructions. `constexpr` devient un outil de programmation réellement utilisable.

**C++17** — `if constexpr` est introduit, permettant de choisir des branches de code à la compilation en fonction de conditions constantes — un outil puissant pour la métaprogrammation.

**C++20** — `consteval` est introduit pour les cas où l'évaluation à la compilation n'est pas juste souhaitée mais **obligatoire**. `constinit` est également ajouté pour résoudre le problème d'ordre d'initialisation des variables statiques. Les fonctions `constexpr` gagnent encore en expressivité : allocation dynamique (`new`/`delete`) à la compilation, `std::vector` et `std::string` en contexte `constexpr`.

**C++23** — Poursuite de l'expansion de `constexpr` à la bibliothèque standard. De plus en plus de fonctions de la STL sont marquées `constexpr`, rendant possible l'évaluation à la compilation de code utilisant `<algorithm>`, `<numeric>`, etc.

Cette progression illustre une tendance lourde du C++ moderne : **déplacer un maximum de travail de l'exécution vers la compilation**. Chaque calcul effectué à la compilation est un calcul qui ne coûte rien à l'exécution, qui ne peut pas échouer au runtime, et qui est vérifié par le compilateur.

---

## `const`, `constexpr`, `consteval` : tableau comparatif

| Caractéristique | `const` | `constexpr` | `consteval` |
|---|---|---|---|
| Introduit en | C++ originel | C++11 | C++20 |
| Immutabilité | Oui | Oui (implicitement `const`) | Oui (implicitement `const`) |
| Valeur connue à la compilation | Pas garanti | Oui si l'initialiseur est constant | Oui (garanti) |
| Applicable aux variables | Oui | Oui | Non (fonctions uniquement) |
| Applicable aux fonctions | Non* | Oui | Oui |
| Évaluation runtime possible | Oui | Oui (selon le contexte) | Non (erreur de compilation) |
| Utilisable comme taille de tableau | Pas garanti** | Oui | N/A |
| Utilisable comme paramètre de template | Pas garanti** | Oui | N/A |

\* `const` s'applique aux méthodes de classe pour indiquer qu'elles ne modifient pas l'objet — c'est un usage différent, couvert au chapitre 6.

\** Une variable `const` initialisée avec un littéral peut être utilisée dans ces contextes (le compilateur traite cela comme une expression constante), mais ce n'est pas garanti pour toutes les initialisations `const`.

---

## Quand utiliser quoi ?

Avant d'entrer dans le détail de chaque qualificateur dans les sous-sections suivantes, voici les heuristiques de choix :

**Utilisez `const` pour toute variable dont la valeur ne doit pas changer après initialisation**, même si cette valeur n'est connue qu'à l'exécution. C'est la forme d'immutabilité la plus courante et elle devrait être votre réflexe par défaut :

```cpp
const auto config = load_config(path);   // Valeur fixée au runtime  
const auto size = container.size();       // Immutable après initialisation  
```

**Utilisez `constexpr` quand la valeur peut raisonnablement être calculée à la compilation** — constantes mathématiques, dimensions de tableaux, paramètres de configuration connus à la compilation, fonctions utilitaires pures :

```cpp
constexpr double pi = 3.14159265358979323846;  
constexpr int buffer_size = 4096;  
constexpr int factorial(int n) { return n <= 1 ? 1 : n * factorial(n - 1); }  
```

**Utilisez `consteval` quand l'évaluation à la compilation est une exigence**, pas un souhait — génération de lookup tables, validation de paramètres à la compilation, calculs qui n'ont de sens qu'au moment du build :

```cpp
consteval uint32_t compile_time_hash(const char* str) {
    uint32_t hash = 0;
    while (*str) {
        hash = hash * 31 + static_cast<uint32_t>(*str++);
    }
    return hash;
}

// L'appel avec une chaîne dynamique serait une erreur de conception
constexpr auto id = compile_time_hash("user_created"); // ✅ Compile-time
```

---

## Ce que nous allons couvrir

Les trois sous-sections suivantes explorent chaque qualificateur en profondeur :

- **3.5.1 — `const` : Immutabilité à l'exécution.** `const` sur les variables, les pointeurs (`const int*` vs `int* const`), les références, les paramètres de fonctions. La const-correctness comme philosophie de programmation.

- **3.5.2 — `constexpr` : Calcul à la compilation.** Variables `constexpr`, fonctions `constexpr` (règles et limitations par standard), `if constexpr` (C++17), `constexpr` dans la STL, `constinit` (C++20) pour les variables statiques.

- **3.5.3 — `consteval` : Fonctions obligatoirement compile-time.** Différences avec `constexpr`, cas d'usage (lookup tables, validation, code generation), interaction avec `constexpr` et `if consteval` (C++23).

---


⏭️ [const : Immutabilité à l'exécution](/03-types-variables-operateurs/05.1-const.md)
