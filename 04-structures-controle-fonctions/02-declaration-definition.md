🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 4.2 — Déclaration et définition de fonctions

## Chapitre 4 · Structures de Contrôle et Fonctions · Niveau Débutant

---

## Introduction

Dans un programme C++ non trivial, le code est découpé en **fonctions** — des unités logiques autonomes qui reçoivent des données en entrée, effectuent un traitement et renvoient éventuellement un résultat. Bien comprendre comment on déclare, définit et organise les fonctions est un préalable indispensable avant d'aborder les modes de passage de paramètres (section 4.3) et la surcharge (section 4.4).

Cette section couvre trois aspects fondamentaux : la distinction entre **déclaration** (prototype) et **définition** (implémentation), la **séparation en fichiers** `.h`/`.cpp` qui structure tout projet C++ professionnel, et la **One Definition Rule** (ODR), une règle du langage dont la violation produit des erreurs souvent difficiles à diagnostiquer.

---

## Anatomie d'une fonction C++

Une fonction en C++ se compose de cinq éléments :

```
type_retour nom_fonction(liste_de_paramètres) {
    // corps de la fonction
    return valeur;
}
```

Prenons un exemple concret :

```cpp
double calculer_moyenne(int a, int b) {
    return static_cast<double>(a + b) / 2.0;
}
```

Le **type de retour** (`double`) indique le type de la valeur renvoyée par `return`. Le **nom** (`calculer_moyenne`) identifie la fonction. La **liste de paramètres** (`int a, int b`) décrit les données que la fonction reçoit, chacune avec un type et un nom. Le **corps** (entre accolades) contient les instructions exécutées lors de l'appel. Enfin, l'instruction `return` transmet le résultat à l'appelant.

### Fonctions sans valeur de retour : `void`

Le type `void` indique qu'une fonction ne renvoie rien. L'instruction `return` est alors facultative (mais peut être utilisée sans valeur pour sortir de la fonction de manière anticipée).

```cpp
void afficher_banniere(const std::string& titre) {
    std::cout << "=== " << titre << " ===\n";
}

void process(int x) {
    if (x < 0) {
        return;  // Sortie anticipée, aucune valeur renvoyée
    }
    // traitement normal...
}
```

### La fonction `main`

Tout programme C++ possède exactement une fonction `main`. Elle est le point d'entrée du programme et a deux signatures autorisées par le standard :

```cpp
int main() {
    // ...
    return 0;  // 0 signale un succès au système d'exploitation
}

int main(int argc, char* argv[]) {
    // argc = nombre d'arguments en ligne de commande
    // argv = tableau de chaînes C (argv[0] = nom du programme)
    return 0;
}
```

Le `return 0` à la fin de `main` est optionnel depuis C++98 — le compilateur l'ajoute implicitement. En revanche, pour toute autre fonction retournant un type non-`void`, oublier le `return` est un **comportement indéfini**.

---

## Déclaration vs définition

C++ distingue clairement deux concepts : la **déclaration** d'une fonction (qui annonce son existence au compilateur) et sa **définition** (qui fournit son implémentation).

### La déclaration (prototype / forward declaration)

Une déclaration indique au compilateur la **signature** de la fonction : son type de retour, son nom et ses types de paramètres. Elle se termine par un point-virgule et ne contient pas de corps.

```cpp
// Déclarations (prototypes)
double calculer_moyenne(int a, int b);  
void afficher_banniere(const std::string& titre);  
int fibonacci(int n);  
```

Les noms des paramètres sont optionnels dans une déclaration — seuls les types comptent pour le compilateur :

```cpp
// Équivalent — sans noms de paramètres
double calculer_moyenne(int, int);
```

En pratique, on garde les noms pour la lisibilité, surtout dans les fichiers d'en-tête qui servent de documentation.

### La définition (implémentation)

La définition fournit le **corps** de la fonction. Elle inclut implicitement une déclaration.

```cpp
// Définition — contient le corps
double calculer_moyenne(int a, int b) {
    return static_cast<double>(a + b) / 2.0;
}
```

### Pourquoi cette distinction existe

Le compilateur C++ traite chaque fichier source (`.cpp`) **indépendamment**, de haut en bas. Quand il rencontre un appel à une fonction, il a besoin de connaître sa signature (type de retour, nombre et types des paramètres) pour vérifier que l'appel est correct et générer le code machine approprié. Mais il n'a pas besoin du corps à ce stade — celui-ci peut se trouver plus loin dans le fichier, dans un autre fichier source, ou même dans une bibliothèque externe.

La déclaration répond exactement à ce besoin : elle donne au compilateur les informations suffisantes pour valider un appel, **sans fournir l'implémentation**.

### Exemple : l'ordre compte dans un fichier unique

Dans un fichier unique, le compilateur lit de haut en bas. Sans déclaration préalable, appeler une fonction avant sa définition provoque une erreur :

```cpp
#include <iostream>

int main() {
    // ❌ Erreur : 'carre' n'a pas été déclaré à ce point
    std::cout << carre(5) << "\n";
    return 0;
}

int carre(int x) {
    return x * x;
}
```

Deux solutions sont possibles. La première est de placer la définition **avant** l'appel :

```cpp
#include <iostream>

int carre(int x) {
    return x * x;
}

int main() {
    std::cout << carre(5) << "\n";  // ✅ 'carre' est déjà défini
}
```

La seconde, plus propre et plus scalable, est d'utiliser une **déclaration anticipée** (*forward declaration*) :

```cpp
#include <iostream>

// Déclaration (prototype) — annonce l'existence de la fonction
int carre(int x);

int main() {
    std::cout << carre(5) << "\n";  // ✅ Le compilateur connaît la signature
}

// Définition — fournit l'implémentation
int carre(int x) {
    return x * x;
}
```

Cette seconde approche est le fondement de l'organisation en fichiers multiples.

---

## Séparation `.h` / `.cpp` : l'organisation professionnelle

Dans tout projet dépassant quelques centaines de lignes, les fonctions sont réparties dans plusieurs fichiers. La convention universelle en C++ est de séparer les **déclarations** (fichiers d'en-tête `.h` ou `.hpp`) des **définitions** (fichiers sources `.cpp`).

### Le fichier d'en-tête (`.h` ou `.hpp`)

Il contient les déclarations : prototypes de fonctions, définitions de classes, constantes, et tout ce qui doit être visible par les autres fichiers qui l'incluent.

```cpp
// math_utils.h
#ifndef MATH_UTILS_H
#define MATH_UTILS_H

double calculer_moyenne(int a, int b);  
int    fibonacci(int n);  
bool   est_premier(int n);  

#endif // MATH_UTILS_H
```

### Le fichier source (`.cpp`)

Il contient les définitions (implémentations). Il inclut son propre en-tête pour garantir la cohérence entre déclarations et définitions.

```cpp
// math_utils.cpp
#include "math_utils.h"

double calculer_moyenne(int a, int b) {
    return static_cast<double>(a + b) / 2.0;
}

int fibonacci(int n) {
    if (n <= 1) return n;
    int a = 0, b = 1;
    for (int i = 2; i <= n; ++i) {
        int tmp = a + b;
        a = b;
        b = tmp;
    }
    return b;
}

bool est_premier(int n) {
    if (n < 2) return false;
    for (int i = 2; i * i <= n; ++i) {
        if (n % i == 0) return false;
    }
    return true;
}
```

### Le fichier utilisateur

Tout fichier `.cpp` qui a besoin de ces fonctions inclut l'en-tête :

```cpp
// main.cpp
#include <iostream>
#include "math_utils.h"

int main() {
    std::cout << "Moyenne : " << calculer_moyenne(10, 20) << "\n";
    std::cout << "Fib(10) : " << fibonacci(10) << "\n";
    std::cout << "Premier ? " << est_premier(17) << "\n";
}
```

### Compilation séparée

Chaque fichier `.cpp` est compilé indépendamment en un fichier objet (`.o`). L'éditeur de liens (*linker*) assemble ensuite les fichiers objet en un exécutable :

```bash
# Compilation séparée
g++ -std=c++17 -Wall -c math_utils.cpp -o math_utils.o  
g++ -std=c++17 -Wall -c main.cpp -o main.o  

# Édition de liens
g++ math_utils.o main.o -o programme
```

Ou en une seule commande (le compilateur gère les étapes intermédiaires) :

```bash
g++ -std=c++17 -Wall math_utils.cpp main.cpp -o programme
```

Ce modèle a un avantage majeur : quand on modifie `math_utils.cpp`, seul ce fichier est recompilé. Les outils de build comme CMake ou Ninja exploitent cette propriété pour ne recompiler que le strict nécessaire — un gain de temps considérable sur les gros projets.

---

## Gardes d'inclusion (*include guards*)

Un fichier d'en-tête peut être inclus par plusieurs fichiers `.cpp`, et certains en-têtes s'incluent mutuellement. Sans protection, les déclarations seraient dupliquées, provoquant des erreurs de compilation.

### La solution classique : `#ifndef` / `#define` / `#endif`

```cpp
// config.h
#ifndef CONFIG_H
#define CONFIG_H

struct Config {
    int timeout;
    int max_retries;
};

Config load_config(const std::string& path);

#endif // CONFIG_H
```

Quand le compilateur rencontre `#include "config.h"` pour la première fois, `CONFIG_H` n'est pas défini : le contenu est inclus et le symbole est défini. Lors d'une inclusion ultérieure, `CONFIG_H` est déjà défini : tout le bloc est ignoré.

### L'alternative moderne : `#pragma once`

```cpp
// config.h
#pragma once

struct Config {
    int timeout;
    int max_retries;
};

Config load_config(const std::string& path);
```

`#pragma once` est supporté par GCC, Clang, MSVC et la quasi-totalité des compilateurs actuels. Il est plus concis et évite le risque de collision de noms de macros. Bien qu'il ne fasse pas partie du standard ISO C++, son utilisation est devenue courante dans les projets modernes.

---

## La One Definition Rule (ODR)

L'ODR est l'une des règles les plus importantes (et les plus violées) du langage C++. Elle s'énonce en deux volets.

### Volet 1 : dans une unité de traduction

Au sein d'un même fichier `.cpp` (et de tout ce qu'il inclut), chaque fonction, variable et classe ne peut être **définie qu'une seule fois**.

```cpp
// ❌ Erreur de compilation — double définition dans le même fichier
int carre(int x) { return x * x; }  
int carre(int x) { return x * x; }  
```

### Volet 2 : dans l'ensemble du programme

À travers tous les fichiers `.cpp` du programme, chaque fonction (non-`inline`) ne peut être **définie qu'une seule fois**. Si deux fichiers `.cpp` contiennent chacun une définition de la même fonction, l'éditeur de liens produit une erreur de *multiple definition*.

```cpp
// fichier_a.cpp
int helper() { return 1; }

// fichier_b.cpp
int helper() { return 2; }  // ❌ Erreur à l'édition de liens
```

C'est exactement la raison pour laquelle les **définitions** vont dans les fichiers `.cpp` (compilés une seule fois) et les **déclarations** dans les fichiers `.h` (inclus par plusieurs `.cpp`).

### L'erreur classique : définir une fonction dans un `.h`

Si on place une définition (pas seulement une déclaration) dans un en-tête, et que cet en-tête est inclus par deux fichiers `.cpp` différents, la fonction est définie deux fois dans le programme final :

```cpp
// utils.h — ❌ Problème potentiel
#pragma once

int helper() {      // Définition, pas juste une déclaration
    return 42;
}
```

```cpp
// a.cpp
#include "utils.h"  // helper() est défini dans a.o
```

```cpp
// b.cpp
#include "utils.h"  // helper() est aussi défini dans b.o → erreur ODR
```

Le linker voit deux définitions de `helper()` et refuse de lier le programme. Les gardes d'inclusion ne protègent **pas** contre ce problème — ils empêchent l'inclusion multiple *dans un même fichier*, pas *à travers les fichiers*.

---

## Exceptions à l'ODR : `inline`, `constexpr`, templates

Certaines constructions sont **exemptées** de la règle « une seule définition dans le programme ». Elles peuvent (et parfois doivent) être définies dans les en-têtes.

### Fonctions `inline`

Le mot-clé `inline` indique au compilateur que la fonction peut être définie dans plusieurs unités de traduction, à condition que toutes les définitions soient **identiques**. Le linker en choisira une et ignorera les autres.

```cpp
// utils.h — ✅ Correct grâce à inline
#pragma once

inline int helper() {
    return 42;
}
```

`inline` ne signifie pas « inliner le code de la fonction à chaque appel ». C'est une suggestion d'optimisation que le compilateur est libre d'ignorer. Sa sémantique principale est de **relâcher l'ODR** pour permettre la définition dans un en-tête.

### Fonctions `constexpr` et `consteval`

Les fonctions `constexpr` (C++11) et `consteval` (C++20) sont implicitement `inline`. Elles peuvent donc être définies dans les en-têtes sans violer l'ODR :

```cpp
// geometry.h
#pragma once

constexpr double pi() {
    return 3.14159265358979323846;
}

constexpr double cercle_aire(double rayon) {
    return pi() * rayon * rayon;
}
```

### Templates

Les templates de fonctions et de classes sont également définis dans les en-têtes. Le compilateur a besoin du code complet du template dans chaque unité de traduction qui l'utilise pour l'instancier avec les types concrets :

```cpp
// algorithms.h
#pragma once

template <typename T>  
T maximum(T a, T b) {  
    return (a > b) ? a : b;
}
```

Les templates seront couverts en détail au chapitre 16.

### Résumé des règles de placement

| Construction | Où la définir ? | Raison |  
|-------------|----------------|--------|  
| Fonction ordinaire | `.cpp` uniquement | ODR — une seule définition autorisée |  
| Fonction `inline` | `.h` autorisé | `inline` relâche l'ODR |  
| Fonction `constexpr` / `consteval` | `.h` (obligatoire en pratique) | Implicitement `inline` |  
| Template de fonction | `.h` (obligatoire) | Le compilateur a besoin du corps pour l'instanciation |  
| Déclaration (prototype) | `.h` | C'est le rôle de l'en-tête |

---

## Déduction du type de retour avec `auto` (C++14)

À partir de C++14, on peut laisser le compilateur déduire le type de retour d'une fonction en utilisant `auto` :

```cpp
auto carre(int x) {
    return x * x;  // Le compilateur déduit int
}

auto construire_message(const std::string& nom) {
    return "Bonjour, " + nom;  // Déduit std::string
}
```

Cette syntaxe est pratique pour les fonctions courtes ou les fonctions templates dont le type de retour est complexe à écrire. En revanche, pour les fonctions publiques d'une API (en-têtes), un type de retour explicite reste préférable — il sert de documentation et permet au lecteur de comprendre la fonction sans lire son corps.

### Trailing return type (C++11)

C++11 avait déjà introduit une syntaxe alternative avec le type de retour **après** la liste de paramètres, précédé de `->` :

```cpp
auto calculer_moyenne(int a, int b) -> double {
    return static_cast<double>(a + b) / 2.0;
}
```

Cette forme est surtout utile quand le type de retour dépend des types des paramètres (fréquent dans les templates) :

```cpp
template <typename A, typename B>  
auto additionner(A a, B b) -> decltype(a + b) {  
    return a + b;
}
```

En C++14 et au-delà, la déduction automatique avec `auto` rend souvent le trailing return type superflu, mais il reste courant dans le code générique pour sa clarté.

---

## Attribut `[[nodiscard]]` (C++17)

L'attribut `[[nodiscard]]` signale au compilateur qu'ignorer la valeur de retour d'une fonction est probablement une erreur. Le compilateur émet alors un warning si l'appelant ne capture pas le résultat.

```cpp
[[nodiscard]] int calculer_checksum(const std::vector<char>& data) {
    int sum = 0;
    for (char c : data) sum += c;
    return sum;
}

int main() {
    std::vector<char> data = {'a', 'b', 'c'};

    calculer_checksum(data);    // ⚠️ Warning : valeur de retour ignorée
    int cs = calculer_checksum(data);  // ✅ OK
}
```

C'est une pratique recommandée pour toute fonction dont le résultat **doit** être utilisé — fonctions d'erreur, allocations, résultats de calculs. On le retrouve abondamment dans la STL (`std::async`, `std::launder`, les méthodes `empty()` des conteneurs, etc.).

C++20 permet d'ajouter un message explicatif :

```cpp
[[nodiscard("Le code d'erreur doit être vérifié")]]
int save_to_disk(const std::string& path, const Data& data);
```

---

## Bonnes pratiques

**Une fonction, une responsabilité.** Une fonction qui fait trop de choses est difficile à tester, à nommer et à réutiliser. Si le nom de la fonction contient « et » (« parse_and_validate »), c'est souvent le signe qu'elle devrait être découpée.

**Des noms explicites.** Le nom d'une fonction doit décrire ce qu'elle fait, pas comment elle le fait. Préférez `calculer_moyenne` à `func1`, et `est_premier` à `check`. Les verbes à l'infinitif (ou en anglais, à l'impératif) sont la convention la plus répandue.

**Toujours inclure l'en-tête correspondant en premier dans le `.cpp`.** Cela garantit que l'en-tête est auto-suffisant — qu'il n'a pas de dépendance cachée sur un `#include` qui serait placé avant lui par hasard :

```cpp
// math_utils.cpp
#include "math_utils.h"   // ← en premier : vérifie l'auto-suffisance
#include <cmath>
#include <algorithm>
```

**Utiliser `[[nodiscard]]`** sur toute fonction dont le résultat ne doit pas être ignoré. C'est un filet de sécurité gratuit.

**Déclarer les paramètres `const` quand c'est possible.** Même pour les paramètres passés par valeur, `const` documente l'intention et permet au compilateur de détecter les modifications accidentelles dans le corps de la fonction :

```cpp
int carre(const int x) {
    // x = 10;  // ❌ Erreur — const empêche la modification
    return x * x;
}
```

Cette pratique sera développée en détail dans la section 4.3 (passage de paramètres).

---

## Liens avec la suite

Cette section a posé les bases : ce qu'est une fonction, comment elle est déclarée, définie et organisée dans un projet multi-fichiers. Les sections suivantes complètent le tableau :

- **Section 4.3** — Comment les données transitent entre l'appelant et la fonction (passage par valeur, référence, pointeur).  
- **Section 4.4** — Comment C++ permet d'avoir plusieurs fonctions portant le même nom mais avec des signatures différentes (surcharge).  
- **Section 4.5** — Comment les valeurs par défaut et le mot-clé `inline` simplifient les interfaces.  
- **Chapitre 6** — Les fonctions membres dans le contexte des classes et de l'encapsulation.  
- **Chapitre 46** — L'organisation professionnelle en répertoires `src/`, `include/`, `tests/`.

⏭️ [Passage de paramètres](/04-structures-controle-fonctions/03-passage-parametres.md)
