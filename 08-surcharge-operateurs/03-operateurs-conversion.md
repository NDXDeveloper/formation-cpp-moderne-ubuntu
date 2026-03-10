🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 8.3 — Opérateurs de conversion (*conversion operators*)

## Chapitre 8 : Surcharge d'Opérateurs et Conversions · Module 3 : Programmation Orientée Objet

---

## Introduction

Les sections précédentes ont montré comment surcharger les opérateurs arithmétiques et d'affectation pour manipuler des objets avec une syntaxe naturelle. Mais une autre dimension de l'intégration d'un type dans l'écosystème C++ est la capacité à **se convertir** vers d'autres types — et inversement.

En C++, deux mécanismes permettent les conversions entre types utilisateur et types existants :

- Les **constructeurs de conversion** : un constructeur qui accepte un seul argument d'un autre type. C'est le mécanisme vu avec `Fraction(int)` en section 8.1 — l'entier est converti en `Fraction`.
- Les **opérateurs de conversion** (*conversion operators*) : des fonctions membres spéciales qui permettent à un objet d'être converti **vers** un autre type. C'est ce que cette section couvre.

Ces deux mécanismes sont complémentaires mais asymétriques. Le constructeur de conversion est défini dans le **type cible** (celui qu'on construit). L'opérateur de conversion est défini dans le **type source** (celui qu'on convertit). Quand vous contrôlez les deux types, vous avez le choix. Quand le type cible est un type que vous ne pouvez pas modifier (`int`, `bool`, `double`, `std::string`), l'opérateur de conversion est votre seule option.

La question centrale de cette section est celle du **contrôle** : les conversions implicites sont pratiques mais dangereuses, et le mot-clé `explicit` est l'outil qui permet de trouver le bon équilibre.

---

## Syntaxe d'un opérateur de conversion

Un opérateur de conversion se déclare comme une fonction membre **sans type de retour** (le type de retour est implicitement le type cible) et **sans paramètre** :

```cpp
class Pourcentage {
    double valeur_;   // stocké comme 0.0 à 100.0

public:
    explicit Pourcentage(double val) : valeur_{val} {}

    // Opérateur de conversion vers double
    operator double() const {
        return valeur_ / 100.0;   // retourne la fraction (0.0 à 1.0)
    }
};
```

L'utilisation :

```cpp
Pourcentage tva{20.0};  
double facteur = tva;            // conversion implicite → 0.2  
double prix = 100.0 * tva;      // 100.0 * 0.2 = 20.0  
```

La syntaxe est inhabituelle : pas de type de retour devant le nom de la fonction. Le type cible (`double`) fait partie intégrante du nom de l'opérateur. La méthode est `const` car la conversion ne modifie pas l'objet source.

---

## Conversions implicites : le piège

L'opérateur de conversion tel que défini ci-dessus est **implicite**. Cela signifie que le compilateur l'invoquera automatiquement chaque fois qu'il a besoin d'un `double` et qu'il dispose d'un `Pourcentage`. C'est pratique dans les cas prévus, mais cela ouvre la porte à des conversions **non intentionnelles** et silencieuses :

```cpp
Pourcentage tva{20.0};

// Conversions attendues :
double d = tva;            // ✅ Clair — on veut un double  
if (tva > 0.5) { ... }    // ✅ Comparaison avec un double  

// Conversions surprenantes :
int i = tva;               // ⚠️ Compile — double → int (troncature silencieuse)  
tva + tva;                 // ⚠️ Compile — les deux sont convertis en double  
                           //    le résultat est un double, pas un Pourcentage
```

Le dernier cas est particulièrement insidieux : `tva + tva` ne retourne pas un `Pourcentage` mais un `double`, parce que le compilateur convertit les deux opérandes en `double` et applique l'addition native. Si l'utilisateur s'attend à obtenir un `Pourcentage`, il obtient un résultat de type différent sans aucun avertissement.

### L'effet boule de neige

Les conversions implicites se composent. Si `Pourcentage` a un `operator double()` implicite et qu'il existe ailleurs une fonction qui accepte un `int`, le compilateur peut enchaîner `Pourcentage → double → int` sans que le développeur en soit conscient. Plus un type a de conversions implicites, plus les résolutions de surcharge deviennent imprévisibles.

---

## `explicit` : reprendre le contrôle

Le mot-clé `explicit` (appliqué aux opérateurs de conversion depuis C++11) interdit la conversion implicite. Le compilateur ne l'invoquera que dans un contexte de **cast explicite** ou dans certains contextes booléens spéciaux :

```cpp
class Pourcentage {
    double valeur_;

public:
    explicit Pourcentage(double val) : valeur_{val} {}

    // Conversion EXPLICITE vers double
    explicit operator double() const {
        return valeur_ / 100.0;
    }
};
```

Maintenant :

```cpp
Pourcentage tva{20.0};

double d1 = tva;                    // ❌ Erreur : conversion implicite interdite  
double d2 = static_cast<double>(tva);  // ✅ Cast explicite — autorisé  
double d3 = double(tva);            // ✅ Cast fonctionnel — autorisé  
auto   d4 = (double)tva;            // ✅ C-style cast — autorisé (mais déconseillé)  

if (tva > 0.5) { }                  // ❌ Erreur : pas de conversion implicite  
if (static_cast<double>(tva) > 0.5) { }  // ✅ Explicite  
```

Le code est plus verbeux dans les cas légitimes, mais il est **sûr** : aucune conversion ne se produit sans que le développeur l'ait explicitement demandée.

---

## Le cas particulier : `explicit operator bool()`

La conversion vers `bool` est le cas d'usage le plus fréquent des opérateurs de conversion. De nombreux types de la bibliothèque standard la définissent : `std::optional`, `std::unique_ptr`, `std::shared_ptr`, `std::ifstream`, `std::expected` (C++23), etc.

### Pourquoi `explicit` et pas implicite ?

Avant C++11, la bibliothèque standard utilisait des contournements tortueux (le "safe bool idiom" via conversion vers pointeur de membre) pour éviter les effets de bord d'un `operator bool()` implicite. Les problèmes étaient réels :

```cpp
// Supposons un operator bool() IMPLICITE :
class Fichier {  
public:  
    operator bool() const { return est_ouvert_; }
    // ...
};

Fichier f1, f2;  
int x = f1;          // ⚠️ Compile : Fichier → bool → int  
f1 + f2;             // ⚠️ Compile : bool + bool = int  
f1 < 42;             // ⚠️ Compile : bool < int  
f1 == "hello";       // ⚠️ Pourrait compiler selon le contexte  
```

Toutes ces expressions sont absurdes, mais elles compilent grâce à la conversion implicite `Fichier → bool`, suivie des promotions et conversions standard de `bool`.

### La solution C++11 : `explicit operator bool()`

```cpp
class Fichier {
    bool est_ouvert_;

public:
    explicit operator bool() const {
        return est_ouvert_;
    }
};
```

Avec `explicit`, les conversions absurdes sont bloquées :

```cpp
Fichier f1, f2;  
int x = f1;          // ❌ Erreur  
f1 + f2;             // ❌ Erreur  
f1 < 42;             // ❌ Erreur  
```

Mais — et c'est la subtilité clé — les **contextes booléens** restent fonctionnels grâce à une exception spéciale du langage appelée *contextual conversion to bool* :

```cpp
Fichier f;

if (f) { }               // ✅ Contexte booléen — conversion autorisée  
while (f) { }             // ✅ Contexte booléen  
bool ok = f ? true : false; // ✅ Contexte booléen (opérateur ternaire)  
bool b = f && true;       // ✅ Contexte booléen (opérateurs logiques)  

if (!f) { }               // ✅ Négation logique — contexte booléen
```

Le standard définit précisément les contextes où la conversion `explicit operator bool()` est autorisée implicitement : conditions de `if`, `while`, `for`, `do-while`, opérandes de `!`, `&&`, `||`, et l'opérateur ternaire `?:`. En dehors de ces contextes, un cast explicite est requis.

### C'est le pattern universel

Quasiment tous les types "nullable" ou "testables" de la bibliothèque standard utilisent `explicit operator bool()` :

```cpp
std::optional<int> opt = trouver_valeur();  
if (opt) {                            // explicit operator bool  
    std::println("Trouvé : {}", *opt);
}

std::unique_ptr<Widget> ptr = creer_widget();  
if (ptr) {                            // explicit operator bool  
    ptr->afficher();
}

std::ifstream fichier{"data.txt"};  
if (fichier) {                        // explicit operator bool  
    // lecture...
}
```

> 💡 **Règle simple : si vous implémentez un `operator bool()`, il doit toujours être `explicit`.** Les cas où un `operator bool()` implicite est justifié sont si rares qu'il est plus sûr de considérer cette règle comme absolue.

---

## Constructeurs de conversion vs opérateurs de conversion

Les deux mécanismes permettent de convertir entre types, mais ils sont définis dans des classes différentes et ont des comportements distincts :

### Constructeur de conversion (défini dans le type cible)

```cpp
class Celsius {
    double temp_;
public:
    // Constructeur de conversion depuis Fahrenheit
    Celsius(Fahrenheit const& f)
        : temp_{(f.valeur() - 32.0) * 5.0 / 9.0} {}
};
```

Le type `Celsius` sait comment se construire à partir d'un `Fahrenheit`. C'est le type **cible** qui contrôle la conversion.

### Opérateur de conversion (défini dans le type source)

```cpp
class Fahrenheit {
    double temp_;
public:
    // Opérateur de conversion vers Celsius
    operator Celsius() const {
        return Celsius{(temp_ - 32.0) * 5.0 / 9.0};
    }
};
```

Le type `Fahrenheit` sait comment se convertir en `Celsius`. C'est le type **source** qui contrôle la conversion.

### Lequel choisir ?

| Situation | Mécanisme recommandé |
|---|---|
| Vous contrôlez le type cible | Constructeur de conversion |
| Vous contrôlez le type source mais pas le type cible | Opérateur de conversion |
| Vous contrôlez les deux | Constructeur de conversion (préféré) |
| Le type cible est un type primitif (`int`, `double`, `bool`) | Opérateur de conversion (seule option) |

En général, le constructeur de conversion est préféré car il centralise la logique dans le type qui est construit, ce qui est plus naturel. L'opérateur de conversion est réservé aux cas où le type cible ne peut pas être modifié.

> ⚠️ **Ne définissez jamais les deux pour la même paire de types.** Si `Fahrenheit` a un `operator Celsius()` et que `Celsius` a un constructeur `Celsius(Fahrenheit)`, le compilateur ne saura pas lequel choisir et signalera une ambiguïté.

---

## Conversions multiples et ambiguïtés

Quand un type possède plusieurs opérateurs de conversion, le compilateur peut se retrouver face à des ambiguïtés. La règle du C++ est que la résolution de surcharge choisit la conversion la **moins coûteuse**, mais quand plusieurs conversions sont de même rang, c'est une erreur :

```cpp
class Valeur {
    double d_;
public:
    explicit Valeur(double d) : d_{d} {}

    operator double() const { return d_; }
    operator int() const { return static_cast<int>(d_); }
};

void traiter(long x) { std::println("long: {}", x); }

Valeur v{3.14};
// traiter(v);   // ❌ Ambiguïté : Valeur → double → long ou Valeur → int → long ?
traiter(static_cast<int>(v));     // ✅ Désambiguïsation explicite  
traiter(static_cast<double>(v));  // ✅ Désambiguïsation explicite  
```

Rendre les conversions `explicit` élimine ce problème car aucune conversion implicite n'est tentée :

```cpp
class Valeur {
    double d_;
public:
    explicit operator double() const { return d_; }
    explicit operator int() const { return static_cast<int>(d_); }
};
```

C'est un argument supplémentaire en faveur de l'utilisation systématique de `explicit`.

---

## Opérateurs de conversion vers des types complexes

Les opérateurs de conversion ne sont pas limités aux types primitifs. On peut convertir vers n'importe quel type, y compris des types de la bibliothèque standard :

```cpp
#include <string>
#include <string_view>

class Identifiant {
    std::string valeur_;

public:
    explicit Identifiant(std::string val) : valeur_{std::move(val)} {}

    // Conversion vers std::string_view (légère, pas de copie)
    operator std::string_view() const noexcept {
        return valeur_;
    }

    // Conversion explicite vers std::string (copie)
    explicit operator std::string() const {
        return valeur_;
    }
};
```

L'asymétrie est intentionnelle ici : la conversion vers `std::string_view` est implicite car elle est légère (pas de copie, juste une vue) et souvent utilisée dans des contextes où une `string_view` est attendue. La conversion vers `std::string` est `explicit` car elle implique une copie — le développeur doit en être conscient.

```cpp
Identifiant id{"ABC-123"};

std::string_view sv = id;                    // ✅ Implicite — léger  
void afficher(std::string_view s);  
afficher(id);                                // ✅ Implicite  

std::string s1 = id;                         // ❌ Erreur — explicit  
std::string s2 = static_cast<std::string>(id); // ✅ Cast explicite  
```

> ⚠️ Attention avec la conversion implicite vers `std::string_view` : si l'objet `Identifiant` est détruit ou déplacé, la `string_view` devient un **dangling reference**. Ce risque est inhérent à toute vue non-owning et doit être documenté.

---

## Conversion vers des pointeurs et `operator*`/`operator->`

Certains types se comportent comme des **wrappers** autour d'un pointeur ou d'une ressource. Plutôt qu'un opérateur de conversion vers un pointeur brut (dangereux), on surcharge les opérateurs de déréférencement :

```cpp
template <typename T>  
class OptionalRef {  
    T* ptr_;

public:
    OptionalRef() : ptr_{nullptr} {}
    explicit OptionalRef(T& ref) : ptr_{&ref} {}

    explicit operator bool() const noexcept {
        return ptr_ != nullptr;
    }

    T& operator*() const {
        return *ptr_;
    }

    T* operator->() const {
        return ptr_;
    }
};
```

Ce pattern est celui utilisé par `std::optional`, `std::unique_ptr` et `std::shared_ptr`. Il combine `explicit operator bool()` pour le test de validité avec `operator*`/`operator->` pour l'accès — sans jamais exposer de conversion implicite vers un pointeur brut.

```cpp
std::string texte = "Hello";  
OptionalRef<std::string> ref{texte};  

if (ref) {                        // explicit operator bool
    std::println("{}", ref->size());  // operator->
    std::println("{}", *ref);         // operator*
}
```

---

## Anti-patterns à éviter

### Anti-pattern 1 : conversion implicite vers un type numérique

```cpp
class Temperature {  
public:  
    operator double() const { return celsius_; }   // ❌ Implicite — dangereux
};
```

Cela permet `Temperature t; int x = t;`, `t + 42`, `t < "hello"` (si une conversion chaînée existe). Utilisez `explicit` ou fournissez un accesseur nommé (`double en_celsius() const`).

### Anti-pattern 2 : multiples conversions implicites

```cpp
class Flexible {  
public:  
    operator int() const;
    operator double() const;
    operator std::string() const;   // ❌ Trop de conversions implicites
};
```

Chaque conversion implicite multiplie les risques d'ambiguïté et de surprises. Rendez-les toutes `explicit` ou remplacez-les par des méthodes nommées (`to_int()`, `to_double()`, `to_string()`).

### Anti-pattern 3 : conversion implicite bidirectionnelle

```cpp
class A {  
public:  
    A(B const&);         // B → A implicite
};

class B {  
public:  
    operator A() const;  // B → A implicite (doublon !)
};
```

Deux chemins de conversion pour la même paire de types. Le compilateur ne saura pas lequel choisir — ambiguïté garantie. Choisissez un seul mécanisme et marquez l'autre `explicit` ou supprimez-le.

---

## Bonnes pratiques

**Marquez vos opérateurs de conversion `explicit` par défaut.** La conversion implicite est l'exception, pas la règle. Commencez toujours par `explicit` et ne retirez le mot-clé que si vous avez une justification solide et que les conséquences ont été analysées.

**`operator bool()` est toujours `explicit`.** C'est la convention universelle depuis C++11, suivie par toute la bibliothèque standard. Ne dérogez jamais à cette règle.

**Préférez les méthodes nommées aux opérateurs de conversion.** `to_string()`, `to_double()`, `as_span()` sont plus lisibles, n'interfèrent pas avec la résolution de surcharge, et expriment clairement l'intention. Réservez les opérateurs de conversion aux cas où l'intégration syntaxique est essentielle (wrapping de pointeurs, types "testables").

**Ne définissez pas de conversion implicite vers un pointeur brut.** C'est une source de bugs mémoire. Utilisez `operator*`/`operator->` pour les types qui encapsulent des pointeurs, et `explicit operator bool()` pour le test de nullité.

**Limitez le nombre de conversions par type.** Un type avec une seule conversion bien choisie est prévisible. Un type avec trois conversions implicites est un piège pour la résolution de surcharge.

**Documentez les conversions non-évidentes.** Si votre type a un `operator std::string_view()` implicite, documentez le risque de dangling reference. Si votre `explicit operator double()` retourne une valeur normalisée plutôt que la valeur brute interne, documentez cette sémantique.

---

## Résumé

| Aspect | Conversion implicite | Conversion `explicit` |
|---|---|---|
| Déclenchement | Automatique par le compilateur | Uniquement sur cast explicite (+ contextes booléens pour `bool`) |
| Risques | Ambiguïtés, conversions chaînées non intentionnelles, surprises | Aucun — le développeur contrôle chaque conversion |
| Cas d'usage | Très rares (vues légères vers `string_view` par ex.) | Règle par défaut pour tout opérateur de conversion |
| `operator bool()` | ❌ Jamais | ✅ Toujours |

| Mécanisme | Défini dans | Syntaxe | Contrôle |
|---|---|---|---|
| Constructeur de conversion | Type **cible** | `Cible(Source const&)` | Le type construit contrôle |
| Opérateur de conversion | Type **source** | `operator Cible() const` | Le type converti contrôle |

---


⏭️ [Opérateur d'appel de fonction (operator())](/08-surcharge-operateurs/04-operateur-appel.md)
