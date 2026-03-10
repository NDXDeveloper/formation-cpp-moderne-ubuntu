🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 4.4 — Surcharge de fonctions (Function Overloading)

## Chapitre 4 · Structures de Contrôle et Fonctions · Niveau Débutant

---

## Introduction

En C, chaque fonction doit porter un nom unique. Si l'on veut calculer la valeur absolue d'un entier et d'un flottant, il faut deux fonctions avec des noms différents — `abs` et `fabs`, par exemple. C++ lève cette limitation grâce à la **surcharge de fonctions** (*function overloading*) : plusieurs fonctions peuvent porter **le même nom** à condition que leurs **listes de paramètres** diffèrent.

```cpp
#include <iostream>
#include <string>

void afficher(int x)                 { std::cout << "Entier : " << x << "\n"; }  
void afficher(double x)              { std::cout << "Flottant : " << x << "\n"; }  
void afficher(const std::string& x)  { std::cout << "Chaîne : " << x << "\n"; }  

int main() {
    afficher(42);
    afficher(3.14);
    afficher(std::string("hello"));
}
```

```
Entier : 42  
Flottant : 3.14  
Chaîne : hello  
```

Le compilateur choisit automatiquement la bonne version en fonction des **types des arguments** fournis à l'appel. Ce mécanisme s'appelle la **résolution de surcharge** (*overload resolution*) et se produit entièrement à la compilation — il n'y a aucun coût à l'exécution.

---

## Ce qui distingue deux surcharges

Le compilateur différencie les surcharges en se basant sur la **signature** de la fonction, qui inclut le nombre de paramètres, leurs types et leur ordre. En revanche, certains éléments **ne font pas** partie de la signature.

### Ce qui compte : les paramètres

```cpp
// ✅ Surcharges valides — signatures différentes

void f(int x);              // 1 paramètre int  
void f(double x);           // 1 paramètre double  
void f(int x, int y);       // 2 paramètres int  
void f(int x, double y);    // int puis double  
void f(double x, int y);    // double puis int (ordre différent)  
```

### Ce qui ne compte pas : le type de retour

Le type de retour **n'est pas pris en compte** pour distinguer deux surcharges. Deux fonctions qui ne diffèrent que par leur type de retour sont considérées comme la même fonction — le compilateur produit une erreur.

```cpp
int    calculer(int x);  
double calculer(int x);  // ❌ Erreur — même signature (int), seul le retour diffère  
```

La raison est simple : au site d'appel, le compilateur ne sait pas toujours quel type de retour l'appelant attend, notamment quand le résultat n'est pas capturé ou est utilisé dans une expression ambiguë.

### Ce qui ne compte pas : `const` de premier niveau sur un paramètre par valeur

Le `const` appliqué directement à un paramètre passé par valeur est ignoré pour la résolution de surcharge, car il n'a aucun impact du point de vue de l'appelant — la fonction travaille sur une copie dans les deux cas.

```cpp
void process(int x);  
void process(const int x);  // ❌ Erreur — même signature que la ligne précédente  
```

En revanche, `const` sur une référence ou un pointeur **fait** partie de la signature, car il change ce que la fonction peut faire avec l'argument :

```cpp
void process(int& x);        // Référence vers int modifiable  
void process(const int& x);  // Référence vers int constant — surcharge valide ✅  
```

---

## Résolution de surcharge : comment le compilateur choisit

Quand le compilateur rencontre un appel à une fonction surchargée, il applique un algorithme en trois étapes pour déterminer quelle version appeler.

### Étape 1 : identifier les candidates

Le compilateur collecte toutes les fonctions qui portent le nom appelé et qui sont visibles dans la portée courante.

### Étape 2 : éliminer les non-viables

Parmi les candidates, seules celles dont les paramètres peuvent correspondre aux arguments fournis (éventuellement après conversion) sont conservées.

### Étape 3 : classer par qualité de correspondance

Le compilateur classe les fonctions viables selon la qualité de la correspondance entre chaque argument et le paramètre attendu. Il existe une hiérarchie de correspondances, de la meilleure à la moins bonne :

| Rang | Type de correspondance | Exemple |  
|------|----------------------|---------|  
| 1 | **Correspondance exacte** | `int` → `int` |  
| 2 | **Promotion** | `char` → `int`, `float` → `double` |  
| 3 | **Conversion standard** | `int` → `double`, `double` → `int` |  
| 4 | **Conversion définie par l'utilisateur** | `const char*` → `std::string` |

Le compilateur choisit la surcharge qui offre la **meilleure correspondance globale**. Si deux surcharges sont à égalité (aucune n'est strictement meilleure que l'autre sur tous les arguments), l'appel est **ambigu** et le compilateur produit une erreur.

### Exemple pas à pas

```cpp
void f(int x)    { std::cout << "int\n"; }  
void f(double x) { std::cout << "double\n"; }  

int main() {
    f(42);     // Correspondance exacte avec f(int) → appelle f(int)
    f(3.14);   // Correspondance exacte avec f(double) → appelle f(double)
    f('A');    // 'A' est un char → promotion vers int → appelle f(int)
    f(3.14f);  // 3.14f est un float → promotion vers double → appelle f(double)
}
```

La promotion `char` → `int` est préférée à la conversion `char` → `double`, donc `f('A')` appelle `f(int)`. De même, la promotion `float` → `double` est préférée à la conversion `float` → `int`.

---

## Cas d'ambiguïté

Quand le compilateur ne peut pas départager deux surcharges, il refuse de compiler. Comprendre ces cas permet de concevoir des interfaces qui les évitent.

### Deux conversions de même rang

```cpp
void f(double x) { std::cout << "double\n"; }  
void f(long x)   { std::cout << "long\n"; }  

int main() {
    f(42);    // ❌ Ambigu : int → double et int → long sont des conversions standard
}
```

`int` peut être converti en `double` ou en `long` — les deux sont des conversions standard de même rang. Le compilateur ne peut pas choisir.

**Solutions possibles :**

```cpp
f(42.0);            // Force le type double → correspondance exacte avec f(double)  
f(42L);             // Force le type long → correspondance exacte avec f(long)  
f(static_cast<double>(42));  // Cast explicite  
```

On peut aussi ajouter une surcharge pour `int` afin de résoudre l'ambiguïté à la source :

```cpp
void f(int x)    { std::cout << "int\n"; }   // ← nouvelle surcharge  
void f(double x) { std::cout << "double\n"; }  
void f(long x)   { std::cout << "long\n"; }  

int main() {
    f(42);  // ✅ Correspondance exacte avec f(int)
}
```

### Référence vs valeur

```cpp
void g(int x)  { std::cout << "valeur\n"; }  
void g(int& x) { std::cout << "référence\n"; }  

int main() {
    int n = 5;
    g(n);  // ❌ Ambigu — n correspond aux deux
}
```

Le compilateur ne sait pas si l'appelant veut une copie ou un alias. Ce design est à éviter — en pratique, une fonction n'offre pas les deux alternatives simultanément.

### `const &` vs valeur pour les petits types

```cpp
void h(int x)        { std::cout << "valeur\n"; }  
void h(const int& x) { std::cout << "const ref\n"; }  

int main() {
    int n = 5;
    h(n);   // ❌ Ambigu
    h(42);  // ❌ Ambigu
}
```

Pour les types primitifs, ne proposez pas les deux formes. Choisissez le passage par valeur (recommandé pour les petits types) et tenez-vous-y.

---

## Surcharge et conversions implicites

Les conversions implicites du C++ (promotions, conversions numériques, conversions utilisateur) interagissent avec la surcharge et peuvent produire des résultats inattendus.

### Promotion `bool` → `int`

```cpp
void f(int x)          { std::cout << "int\n"; }  
void f(const char* s)  { std::cout << "string\n"; }  

int main() {
    f(true);  // Appelle f(int) — bool est promu en int (1)
}
```

Si l'intention était d'appeler une surcharge spécifique pour `bool`, il faut la définir explicitement :

```cpp
void f(bool b)         { std::cout << "bool\n"; }  
void f(int x)          { std::cout << "int\n"; }  
void f(const char* s)  { std::cout << "string\n"; }  

int main() {
    f(true);  // ✅ Correspondance exacte avec f(bool)
}
```

### `nullptr` et les surcharges numériques

```cpp
void f(int x)    { std::cout << "int\n"; }  
void f(int* ptr) { std::cout << "pointeur\n"; }  

int main() {
    f(0);        // Appelle f(int) — 0 est un int
    f(NULL);     // ⚠️ Dépend de l'implémentation — NULL peut être 0 (int) ou 0L (long)
    f(nullptr);  // ✅ Appelle f(int*) — sans ambiguïté
}
```

C'est une raison supplémentaire de toujours utiliser `nullptr` au lieu de `NULL` ou `0` pour les pointeurs nuls.

### Conversions utilisateur

Quand une classe définit un constructeur à un paramètre (non-`explicit`) ou un opérateur de conversion, ces conversions participent à la résolution de surcharge :

```cpp
struct Distance {
    double metres;
    Distance(double m) : metres(m) {}  // Conversion implicite double → Distance
};

void mesurer(Distance d)              { std::cout << d.metres << " m\n"; }  
void mesurer(const std::string& label) { std::cout << "Label : " << label << "\n"; }  

int main() {
    mesurer(3.14);                    // Conversion utilisateur double → Distance → mesurer(Distance)
    mesurer(std::string("parcours")); // Correspondance exacte avec mesurer(const std::string&)
}
```

Si cette conversion implicite est indésirable, le constructeur peut être marqué `explicit` pour interdire les conversions automatiques :

```cpp
struct Distance {
    double metres;
    explicit Distance(double m) : metres(m) {}
};

void mesurer(Distance d) { /* ... */ }

int main() {
    // mesurer(3.14);                // ❌ Erreur — conversion implicite interdite
    mesurer(Distance(3.14));         // ✅ Conversion explicite
}
```

Le mot-clé `explicit` sera approfondi au chapitre 6 (Classes et Encapsulation).

---

## Surcharge et portée

Les surcharges doivent être déclarées dans la **même portée** pour être considérées ensemble. Une déclaration dans une portée intérieure **masque** (plutôt qu'elle ne surcharge) les fonctions du même nom dans la portée extérieure :

```cpp
#include <iostream>

void f(int x)    { std::cout << "int global\n"; }  
void f(double x) { std::cout << "double global\n"; }  

namespace app {
    void f(const std::string& s) { std::cout << "string app\n"; }

    void test() {
        // f(42);  // ❌ Erreur — seul f(const std::string&) est visible ici
        f(std::string("ok"));   // ✅
        ::f(42);                // ✅ Accès explicite à la portée globale
    }
}
```

Dans le namespace `app`, la déclaration de `f(const std::string&)` **masque** les deux `f` globaux. Ce comportement est souvent inattendu. Pour rendre les surcharges globales visibles à côté de la surcharge locale, on utilise une **directive `using`** :

```cpp
namespace app {
    using ::f;  // Rend les f globaux visibles dans cette portée
    void f(const std::string& s) { std::cout << "string app\n"; }

    void test() {
        f(42);                  // ✅ Appelle ::f(int)
        f(3.14);                // ✅ Appelle ::f(double)
        f(std::string("ok"));   // ✅ Appelle app::f(const std::string&)
    }
}
```

Ce mécanisme de masquage se retrouve aussi entre classes de base et classes dérivées — un sujet couvert au chapitre 7.

---

## Le *name mangling* : comment le compilateur distingue les surcharges

En C, le nom d'une fonction correspond directement à un symbole dans le fichier objet. En C++, ce n'est plus suffisant puisque plusieurs fonctions peuvent porter le même nom. Le compilateur résout ce problème par le **name mangling** (ou *name decoration*) : il encode la signature de la fonction dans le nom du symbole.

Par exemple, pour ces deux fonctions :

```cpp
void f(int x);  
void f(double x);  
```

GCC pourrait générer les symboles `_Z1fi` et `_Z1fd`, où `i` représente `int` et `d` représente `double`. Le format exact dépend du compilateur, mais le principe est universel.

On peut observer les symboles avec l'outil `nm` :

```bash
$ g++ -c surcharge.cpp -o surcharge.o
$ nm surcharge.o | grep " T "
0000000000000000 T _Z1fi
0000000000000010 T _Z1fd
```

Et les démangler avec `c++filt` :

```bash
$ nm surcharge.o | c++filt
0000000000000000 T f(int)
0000000000000010 T f(double)
```

C'est la raison pour laquelle `extern "C"` désactive le name mangling — indispensable pour exposer des fonctions C++ à du code C (chapitre 43).

---

## Surcharge en pratique : exemples idiomatiques

### API avec plusieurs types d'entrée

```cpp
class Logger {  
public:  
    void log(const std::string& message);
    void log(const char* message);
    void log(int error_code);
};
```

L'appelant utilise toujours `logger.log(...)` sans se soucier du type — l'interface est unifiée.

### Constructeurs de classe

La surcharge est omniprésente dans les constructeurs, qui portent tous le même nom (celui de la classe) :

```cpp
class Point {  
public:  
    Point();                          // Constructeur par défaut
    Point(double x, double y);        // Constructeur avec coordonnées
    Point(const Point& other);        // Constructeur de copie
};
```

Ce sujet est traité en détail au chapitre 6 (section 6.2).

### Opérateurs surchargés

La surcharge d'opérateurs (chapitre 8) est un cas particulier de surcharge de fonctions — `operator+`, `operator==`, etc. sont des fonctions dont le nom est un opérateur :

```cpp
Point operator+(const Point& a, const Point& b);  
Point operator*(const Point& p, double facteur);  
Point operator*(double facteur, const Point& p);  // Ordre inversé  
```

---

## Bonnes pratiques

**Les surcharges doivent faire la même chose.** Si `afficher(int)` et `afficher(std::string)` ont des comportements fondamentalement différents, le nom commun est trompeur. La surcharge est un outil pour **adapter l'implémentation** au type, pas pour regrouper des fonctions sans rapport sous un même nom.

**Éviter les surcharges qui ne diffèrent que par une conversion implicite étroite.** Une surcharge `f(int)` / `f(long)` / `f(double)` crée un champ de mines d'ambiguïtés. Préférez des noms distincts si les types sont proches et que la distinction est sémantiquement significative.

**Limiter le nombre de surcharges.** Une fonction avec dix surcharges est difficile à maintenir et à comprendre. Au-delà de trois ou quatre variantes, considérez un template (chapitre 16) ou un paramètre `std::variant` (section 12.2).

**Tester les cas limites.** Les conversions implicites et les promotions peuvent produire des appels inattendus. Vérifiez le comportement avec des arguments comme `0`, `true`, `nullptr`, `'A'`, `3.14f` — ce sont les cas qui surprennent le plus souvent.

**Documenter les surcharges dans l'en-tête.** Un commentaire bref sur chaque variante aide le lecteur à comprendre pourquoi plusieurs versions existent et quand utiliser laquelle.

---

## Résumé

| Aspect | Détail |  
|--------|--------|  
| Définition | Plusieurs fonctions portant le même nom avec des signatures différentes |  
| Distinction | Nombre, types et ordre des paramètres |  
| Type de retour | Ne participe **pas** à la distinction |  
| Résolution | À la compilation — aucun coût à l'exécution |  
| Ambiguïté | Erreur de compilation si deux surcharges sont à égalité |  
| Lien avec les templates | Les templates généralisent la surcharge à un nombre infini de types |

---

## Ce qui suit

La section 4.5, dernière du chapitre, aborde les **valeurs par défaut** et les **fonctions `inline`** — deux mécanismes qui simplifient les interfaces et interagissent directement avec la surcharge : une fonction avec des paramètres par défaut peut parfois remplacer plusieurs surcharges.

⏭️ [Valeurs par défaut et fonctions inline](/04-structures-controle-fonctions/05-valeurs-defaut-inline.md)
