🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 4.5 — Valeurs par défaut et fonctions `inline`

## Chapitre 4 · Structures de Contrôle et Fonctions · Niveau Débutant

---

## Valeurs par défaut des paramètres

### Principe

C++ permet d'attribuer une **valeur par défaut** à un ou plusieurs paramètres d'une fonction. Si l'appelant omet ces arguments lors de l'appel, le compilateur utilise automatiquement les valeurs par défaut spécifiées dans la déclaration.

```cpp
#include <iostream>
#include <string>

void connecter(const std::string& hote, int port = 8080, bool ssl = false) {
    std::cout << (ssl ? "https" : "http") << "://" << hote << ":" << port << "\n";
}

int main() {
    connecter("example.com");               // port=8080, ssl=false
    connecter("example.com", 443);          // ssl=false
    connecter("example.com", 443, true);    // tout est spécifié
}
```

```
http://example.com:8080  
http://example.com:443  
https://example.com:443  
```

La fonction `connecter` se comporte comme si trois versions existaient — une avec un paramètre, une avec deux, une avec trois — mais une seule définition suffit. C'est un outil de simplification d'interface puissant.

### Règle d'ordre : de droite à gauche

Les paramètres avec valeurs par défaut doivent être **à la fin** de la liste des paramètres. On ne peut pas intercaler un paramètre sans défaut après un paramètre avec défaut :

```cpp
// ✅ Correct — les défauts sont à droite
void f(int a, int b = 10, int c = 20);

// ❌ Erreur de compilation — paramètre sans défaut après un paramètre avec défaut
// void f(int a = 5, int b, int c = 20);
```

La raison est logique : les arguments sont résolus **de gauche à droite**. Si `b` avait un défaut mais pas `c`, le compilateur ne saurait pas si un deuxième argument fourni correspond à `b` ou à `c`.

En conséquence, les paramètres les plus susceptibles d'être omis doivent être placés **en dernier** dans la liste.

### Où déclarer les valeurs par défaut

Les valeurs par défaut doivent être spécifiées dans la **déclaration** (prototype), pas dans la définition. C'est la déclaration que l'appelant voit — c'est là qu'il a besoin de connaître les défauts.

```cpp
// math_utils.h — déclaration avec défauts
double puissance(double base, int exposant = 2);

// math_utils.cpp — définition SANS répéter les défauts
double puissance(double base, int exposant) {
    double resultat = 1.0;
    for (int i = 0; i < exposant; ++i) {
        resultat *= base;
    }
    return resultat;
}
```

Répéter les valeurs par défaut dans la définition provoque une erreur de compilation :

```cpp
// ❌ Erreur — défaut déjà spécifié dans la déclaration
double puissance(double base, int exposant = 2) {
    // ...
}
```

Dans le cas d'une fonction définie dans un seul fichier (sans séparation `.h`/`.cpp`), la définition sert aussi de déclaration — les défauts y sont alors spécifiés directement.

### Ajout progressif de valeurs par défaut

C++ autorise l'ajout de valeurs par défaut **dans des déclarations successives**, à condition de ne jamais redéfinir un défaut déjà spécifié :

```cpp
void f(int a, int b, int c);         // Première déclaration — aucun défaut  
void f(int a, int b, int c = 30);    // ✅ Ajoute un défaut pour c  
void f(int a, int b = 20, int c);    // ✅ Ajoute un défaut pour b (c a déjà le sien)  

// void f(int a, int b = 99, int c);  // ❌ Erreur — b a déjà un défaut (20)
```

Ce mécanisme est rarement utilisé en pratique, mais il est bon de savoir qu'il existe — on le rencontre parfois dans des en-têtes qui évoluent entre versions d'une bibliothèque.

### Types de valeurs autorisées

La valeur par défaut peut être toute expression évaluable au moment de l'appel : une constante littérale, une variable globale, un appel de fonction, une expression constante :

```cpp
int get_default_port();

// Constante littérale
void f(int x = 42);

// Appel de fonction
void g(int port = get_default_port());

// Expression constexpr
constexpr int MAX_RETRIES = 3;  
void h(int retries = MAX_RETRIES);  

// Valeur construite
void k(std::string prefix = std::string("LOG"));
```

L'expression est évaluée **à chaque appel** où le défaut est utilisé, pas une seule fois à la déclaration. Pour `g`, `get_default_port()` est appelé chaque fois que `g` est invoqué sans argument.

---

## Valeurs par défaut vs surcharge

Les valeurs par défaut et la surcharge de fonctions (section 4.4) résolvent parfois le même problème : permettre d'appeler une fonction avec un nombre variable d'arguments. Les deux approches ont des forces différentes.

### Quand les défauts suffisent

Si les variantes d'une fonction diffèrent uniquement par l'**omission** de certains paramètres qui prennent alors une valeur conventionnelle, les défauts sont l'outil le plus simple :

```cpp
// ✅ Un seul point de maintenance — une seule fonction
std::string formater_date(int jour, int mois, int annee = 2026,
                          char separateur = '/') {
    // ...
}
```

L'alternative par surcharge nécessiterait trois fonctions distinctes avec du code dupliqué ou délégué.

### Quand la surcharge est préférable

Si les variantes ont des **types de paramètres différents** ou des **implémentations fondamentalement différentes**, la surcharge est le bon choix :

```cpp
// Surcharge — les types et les implémentations diffèrent
void envoyer(const std::string& message);  
void envoyer(const std::vector<char>& donnees_binaires);  
void envoyer(int code_erreur);  
```

Les valeurs par défaut ne peuvent pas changer le type d'un paramètre — elles ne font que fournir une valeur conventionnelle pour un paramètre existant.

### Le piège : conflit entre défauts et surcharge

Combiner les deux mécanismes peut créer des ambiguïtés :

```cpp
void traiter(int x, int y = 0);  
void traiter(int x);  

int main() {
    traiter(42);  // ❌ Ambigu — les deux correspondent
}
```

Le compilateur ne sait pas si l'appelant invoque `traiter(int)` ou `traiter(int, int)` avec le défaut. Ce design est à éviter. En règle générale, **ne combinez pas surcharge et valeurs par défaut sur le même nom de fonction** si leurs signatures se chevauchent quand les défauts s'appliquent.

---

## Fonctions `inline`

### Le problème : le coût de l'appel de fonction

Chaque appel de fonction implique un overhead minimal mais non nul : sauvegarde des registres, empilement des arguments, saut vers le code de la fonction, puis retour. Pour des fonctions très courtes appelées des millions de fois (par exemple dans une boucle de calcul intensif), ce coût peut devenir significatif par rapport au travail réel effectué dans la fonction.

### L'idée : substituer le corps au site d'appel

Le mot-clé `inline` **suggère** au compilateur de remplacer l'appel de fonction par le **corps de la fonction** directement au site d'appel — comme si le code avait été copié-collé. C'est ce qu'on appelle l'*inlining*.

```cpp
inline int carre(int x) {
    return x * x;
}

int main() {
    int r = carre(5);
    // Le compilateur peut transformer ceci en :
    // int r = 5 * 5;
}
```

En éliminant le saut et le retour, l'inlining peut améliorer la performance pour les fonctions courtes. Mais il augmente la taille du code généré (chaque site d'appel contient une copie du corps), ce qui peut dégrader la performance du cache d'instructions si c'est fait de manière excessive.

### `inline` en 2026 : une sémantique surtout liée au linker

En pratique, les compilateurs modernes (GCC, Clang) **ignorent largement** la suggestion d'inlining portée par le mot-clé `inline`. Ils disposent de leurs propres heuristiques, basées sur la taille de la fonction, la fréquence d'appel et le contexte d'optimisation (`-O2`, `-O3`), pour décider quoi inliner — avec ou sans le mot-clé.

Le rôle principal de `inline` en C++ moderne est **sémantique, pas optimisant** : il relâche la One Definition Rule (section 4.2) et permet de définir une fonction dans un en-tête sans provoquer d'erreur de *multiple definition* à l'édition de liens.

```cpp
// utils.h — ✅ Grâce à inline, cette définition peut apparaître dans plusieurs .cpp
#pragma once

inline int maximum(int a, int b) {
    return (a > b) ? a : b;
}
```

Sans `inline`, inclure cette définition dans deux fichiers `.cpp` différents produirait une erreur de *multiple definition* au linking. Avec `inline`, le linker sait qu'il peut rencontrer plusieurs définitions identiques et n'en conserver qu'une.

### Fonctions implicitement `inline`

Certaines fonctions sont `inline` **par défaut**, sans qu'on ait besoin d'écrire le mot-clé :

**Fonctions définies dans le corps d'une classe :**

```cpp
class Rectangle {
    double largeur_, hauteur_;
public:
    // Implicitement inline — définie dans la classe
    double aire() const { return largeur_ * hauteur_; }
};
```

**Fonctions `constexpr` et `consteval` :**

```cpp
// Implicitement inline
constexpr int factorielle(int n) {
    return (n <= 1) ? 1 : n * factorielle(n - 1);
}
```

**Fonctions template :**

Les templates sont définis dans les en-têtes par nécessité (le compilateur a besoin du corps pour l'instanciation). L'ODR les traite de manière similaire à `inline`.

### Variables `inline` (C++17)

C++17 étend le concept aux variables : une variable `inline` peut être définie dans un en-tête et partagée entre plusieurs unités de traduction. C'est la solution moderne au problème des constantes globales partagées :

```cpp
// config.h
#pragma once

inline constexpr int MAX_CONNECTIONS = 100;  
inline const std::string DEFAULT_HOST = "localhost";  
```

Avant C++17, définir une constante non-`constexpr` dans un en-tête imposait de la déclarer `extern` dans le `.h` et de la définir dans un `.cpp` — un processus fastidieux.

---

## `inline` vs macros du préprocesseur

Le préprocesseur C offrait une alternative primitive à l'inlining via les macros `#define`. Bien que cette technique soit encore rencontrée dans du code legacy, elle est **fortement déconseillée** en C++ moderne.

```cpp
// ❌ Macro — pas de typage, pas de portée, effets de bord
#define CARRE(x) ((x) * (x))

int a = 5;  
int r = CARRE(a++);  // Comportement indéfini — a++ évalué deux fois  

// ✅ Fonction inline — typée, sûre, débogable
inline int carre(int x) {
    return x * x;
}

int s = carre(a++);  // OK — a++ évalué une seule fois
```

| Critère | Macro `#define` | Fonction `inline` |  
|---------|----------------|-------------------|  
| Vérification des types | Non | Oui |  
| Respect de la portée | Non (substitution textuelle) | Oui (portée normale) |  
| Évaluation des arguments | Multiple (effets de bord) | Une seule fois |  
| Débogage | Difficile (noms remplacés) | Normal (visible dans le debugger) |  
| Surcharge possible | Non | Oui |

La seule raison légitime d'utiliser des macros en C++ moderne est les gardes d'inclusion (`#ifndef`/`#define`) et quelques cas de compilation conditionnelle (`#ifdef DEBUG`). Pour tout le reste, les fonctions `inline`, `constexpr` et les templates les remplacent avantageusement.

---

## Quand utiliser `inline` explicitement

Étant donné que le compilateur décide seul quoi inliner en termes d'optimisation, le mot-clé `inline` n'est utile que dans un seul contexte : **définir une fonction (non-template, non-`constexpr`, non-membre de classe) dans un fichier d'en-tête**.

```cpp
// Dans un .h — inline nécessaire pour éviter la violation ODR
inline void log_debug(const std::string& msg) {
    #ifdef DEBUG
    std::cerr << "[DEBUG] " << msg << "\n";
    #endif
}
```

En dehors de ce cas, il est inutile (et potentiellement trompeur) d'ajouter `inline` à des fonctions définies dans un `.cpp` dans l'espoir d'améliorer les performances. Le compilateur sait mieux que le développeur quelles fonctions méritent d'être inlinées.

### L'attribut `[[gnu::always_inline]]` et `__forceinline`

Pour les cas extrêmes (code critique en performance, intrinsics), GCC et Clang offrent `__attribute__((always_inline))` et MSVC offre `__forceinline`. Ces directives **forcent** l'inlining, contrairement à `inline` qui n'est qu'une suggestion. Leur usage est réservé à l'optimisation bas niveau et n'a pas sa place dans du code applicatif courant.

---

## Bonnes pratiques

**Utilisez les valeurs par défaut pour simplifier les interfaces.** Une fonction avec des paramètres par défaut bien choisis est plus agréable à utiliser qu'un ensemble de surcharges qui se délèguent mutuellement.

**Placez les paramètres les plus stables à gauche, les plus optionnels à droite.** Les paramètres que l'appelant fournira presque toujours doivent être en tête ; ceux qu'il omettra souvent doivent être à la fin avec une valeur par défaut.

**Documentez les valeurs par défaut dans l'en-tête.** Les défauts font partie de l'interface publique — ils doivent être visibles et compréhensibles par l'utilisateur de la fonction.

**N'écrivez pas `inline` pour optimiser — écrivez-le pour le linker.** Le seul usage pertinent de `inline` en C++ moderne est de permettre la définition dans un en-tête. Laissez le compilateur décider ce qu'il inline réellement.

**Préférez `constexpr` à `inline` quand c'est possible.** Si une fonction peut être évaluée à la compilation, `constexpr` (section 3.5.2) est plus expressif qu'`inline` et apporte des garanties supplémentaires. `constexpr` implique `inline`, donc les deux bénéfices sont réunis.

---

## Résumé du chapitre

Ce chapitre a couvert les briques fondamentales du flux de contrôle et de l'organisation du code en C++ :

| Section | Sujet | Point clé à retenir |  
|---------|-------|---------------------|  
| 4.1 | Conditionnelles et boucles | `if constexpr` (C++17) pour le compile-time, range-based `for` comme forme par défaut |  
| 4.2 | Déclaration et définition | Séparer `.h` (déclarations) et `.cpp` (définitions), respecter l'ODR |  
| 4.3 | Passage de paramètres | `const T&` par défaut pour les types non primitifs, par valeur pour les types primitifs |  
| 4.4 | Surcharge de fonctions | Même nom, signatures différentes, résolution à la compilation |  
| 4.5 | Valeurs par défaut et `inline` | Simplifier les interfaces, `inline` pour le linker (pas pour l'optimisation) |

Le chapitre suivant (chapitre 5) aborde la **gestion de la mémoire** — stack vs heap, allocation dynamique, et les dangers associés. C'est un sujet fondamental qui éclaire notamment les choix de passage de paramètres vus en section 4.3.

⏭️ [Gestion de la Mémoire (Le cœur du sujet)](/05-gestion-memoire/README.md)
