🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 8.1 — Surcharge des opérateurs arithmétiques et de comparaison

## Chapitre 8 : Surcharge d'Opérateurs et Conversions · Module 3 : Programmation Orientée Objet

---

## Introduction

Les opérateurs arithmétiques (`+`, `-`, `*`, `/`, `%`) et de comparaison (`==`, `!=`, `<`, `>`, `<=`, `>=`) sont ceux que l'on surcharge le plus fréquemment. Ils permettent à des types utilisateur — vecteurs mathématiques, fractions, montants monétaires, dates — de se manipuler avec une syntaxe aussi naturelle que les types primitifs.

Mais surcharger ces opérateurs correctement demande de respecter un ensemble de conventions idiomatiques qui ne sont pas toujours évidentes au premier abord : quelle signature adopter, fonction membre ou fonction libre, comment gérer la symétrie, dans quel ordre implémenter les opérateurs entre eux. Cette section détaille le **pattern canonique** pour chaque famille d'opérateurs, tel qu'il est pratiqué dans le code C++ professionnel et dans la bibliothèque standard.

> 💡 L'opérateur spaceship `<=>` (C++20) offre une alternative puissante pour les opérateurs de comparaison, couverte en section 8.5. Cette section présente d'abord l'approche classique (pré-C++20), indispensable à comprendre pour le code existant et pour les cas où le spaceship ne suffit pas.

---

## Type fil rouge : `Vec2`

Tout au long de cette section, les exemples s'appuient sur un type `Vec2` représentant un vecteur 2D — un cas classique où la surcharge d'opérateurs est parfaitement justifiée :

```cpp
#include <cmath>

class Vec2 {
    double x_;
    double y_;

public:
    Vec2() : x_{0.0}, y_{0.0} {}
    Vec2(double x, double y) : x_{x}, y_{y} {}

    double x() const { return x_; }
    double y() const { return y_; }
    double norme() const { return std::sqrt(x_ * x_ + y_ * y_); }
};
```

Nous allons progressivement enrichir ce type avec des opérateurs.

---

## Opérateurs arithmétiques : le pattern canonique

### Étape 1 : implémenter `operator+=` comme membre

Le point de départ est toujours l'opérateur d'affectation composé (`+=`, `-=`, `*=`, etc.), implémenté comme **fonction membre**. C'est logique : l'opération modifie l'objet de gauche, elle a donc besoin d'un accès direct à ses membres.

```cpp
class Vec2 {
    // ... (données et constructeurs)

public:
    Vec2& operator+=(Vec2 const& rhs) {
        x_ += rhs.x_;
        y_ += rhs.y_;
        return *this;
    }

    Vec2& operator-=(Vec2 const& rhs) {
        x_ -= rhs.x_;
        y_ -= rhs.y_;
        return *this;
    }

    // Multiplication par un scalaire
    Vec2& operator*=(double scalaire) {
        x_ *= scalaire;
        y_ *= scalaire;
        return *this;
    }
};
```

Points clés de la signature :

- **Retour par référence** (`Vec2&`) vers `*this`. Cela permet le chaînage : `a += b += c` (bien que ce style soit rarement utilisé, la convention est de le supporter).
- **Paramètre par référence constante** (`Vec2 const&`). L'opérande de droite n'est pas modifié.
- **Non `const`** : la méthode modifie l'objet courant.

### Étape 2 : implémenter `operator+` comme fonction libre en termes de `+=`

L'opérateur binaire `+` ne modifie aucun de ses opérandes — il retourne un **nouvel objet**. Il est implémenté comme fonction libre et délègue le travail à `+=` :

```cpp
Vec2 operator+(Vec2 lhs, Vec2 const& rhs) {
    lhs += rhs;
    return lhs;
}

Vec2 operator-(Vec2 lhs, Vec2 const& rhs) {
    lhs -= rhs;
    return lhs;
}
```

Ce pattern mérite une explication détaillée, car la signature est surprenante au premier abord :

**Le premier paramètre est passé par valeur** (`Vec2 lhs`), pas par référence constante. C'est intentionnel. Puisque `operator+` doit retourner un nouvel objet, il a besoin d'une copie de l'opérande gauche. En le passant par valeur, la copie est faite à l'entrée de la fonction, et le compilateur peut appliquer la **copy elision** ou la sémantique de mouvement (chapitre 10) pour l'optimiser.

**Le second paramètre est passé par référence constante** (`Vec2 const& rhs`), car il n'est pas modifié.

**Le retour est par valeur** (`Vec2`). C'est le résultat de l'opération — un objet temporaire. La Return Value Optimization (RVO, section 10.5) élimine la copie de retour dans la majorité des cas.

### Pourquoi une fonction libre plutôt qu'un membre ?

Si `operator+` était un membre de `Vec2`, l'opérande gauche devrait obligatoirement être un `Vec2`. Cela empêcherait les conversions implicites sur l'opérande gauche. Avec une fonction libre, les deux opérandes sont traités symétriquement :

```cpp
// Si operator+ est membre :
Vec2 a{1, 2};  
Vec2 r1 = a + Vec2{3, 4};     // ✅ a.operator+(Vec2{3, 4})  
// Vec2 r2 = Vec2{3, 4} + a;  // Dépend du contexte — problème de symétrie

// Avec une fonction libre :
Vec2 r3 = a + Vec2{3, 4};     // ✅ operator+(a, Vec2{3, 4})  
Vec2 r4 = Vec2{3, 4} + a;     // ✅ operator+(Vec2{3, 4}, a) — symétrique  
```

La symétrie est importante pour respecter l'intuition mathématique : si `a + b` est valide, `b + a` doit l'être aussi (quand l'addition est commutative).

### Multiplication par un scalaire : les deux sens

Pour la multiplication d'un vecteur par un scalaire, il faut supporter les deux ordres (`vec * scalaire` et `scalaire * vec`) :

```cpp
// vec * scalaire — délègue à operator*=
Vec2 operator*(Vec2 v, double scalaire) {
    v *= scalaire;
    return v;
}

// scalaire * vec — commutativité
Vec2 operator*(double scalaire, Vec2 v) {
    v *= scalaire;
    return v;
}
```

Sans la seconde surcharge, `2.0 * v` serait une erreur de compilation. Ce serait surprenant pour quiconque a fait de l'algèbre linéaire.

---

## Le pattern complet : résumé visuel

Le pattern canonique pour les opérateurs arithmétiques suit toujours la même structure :

```
                    ┌──────────────────────┐
                    │  operator+=  (membre)│
                    │  modifie *this       │
                    │  retourne *this      │
                    └──────────┬───────────┘
                               │
                        délègue à +=
                               │
                    ┌──────────▼───────────┐
                    │  operator+  (libre)  │
                    │  crée un nouvel objet│
                    │  retourne par valeur │
                    └──────────────────────┘
```

Ce pattern s'applique à toutes les paires :

| Opérateur composé (membre) | Opérateur binaire (libre) |
|---|---|
| `operator+=` | `operator+` |
| `operator-=` | `operator-` |
| `operator*=` | `operator*` |
| `operator/=` | `operator/` |
| `operator%=` | `operator%` |

L'avantage est la **non-duplication** : la logique de l'opération n'est écrite qu'une seule fois dans l'opérateur composé. L'opérateur binaire n'est qu'un wrapper.

---

## Opérateur unaire : `-` (négation)

L'opérateur unaire de négation est un cas à part — il ne modifie pas l'objet et retourne un nouvel objet. Il s'implémente comme fonction membre `const` :

```cpp
class Vec2 {
    // ...
public:
    Vec2 operator-() const {
        return Vec2{-x_, -y_};
    }

    Vec2 operator+() const {
        return *this;   // identité — rarement utile mais parfois attendu
    }
};
```

L'utilisation est naturelle :

```cpp
Vec2 v{3.0, 4.0};  
Vec2 neg = -v;       // Vec2{-3.0, -4.0}  
```

---

## Opérateurs de comparaison : approche pré-C++20

### `operator==` et `operator!=`

L'opérateur d'égalité est fondamental. En pré-C++20, il faut implémenter **les deux** :

```cpp
// Fonction libre — symétrie garantie
bool operator==(Vec2 const& lhs, Vec2 const& rhs) {
    return lhs.x() == rhs.x() && lhs.y() == rhs.y();
}

bool operator!=(Vec2 const& lhs, Vec2 const& rhs) {
    return !(lhs == rhs);   // défini en termes de ==
}
```

`operator!=` est défini en termes de `operator==` — un seul point de logique, cohérence garantie.

> ⚠️ **Attention aux flottants.** Comparer des `double` avec `==` est problématique à cause des erreurs d'arrondi. Pour un type `Vec2` mathématique utilisé dans des tests, une comparaison approchée (`std::abs(a - b) < epsilon`) est souvent préférable. L'`operator==` strict reste néanmoins la convention pour les comparaisons structurelles.

### `operator<` et les opérateurs d'ordre

Pour les opérateurs d'ordre, le pattern est similaire — on implémente `<` et on dérive les autres :

```cpp
bool operator<(Vec2 const& lhs, Vec2 const& rhs) {
    // Ordre lexicographique : d'abord x, puis y en cas d'égalité
    if (lhs.x() != rhs.x()) return lhs.x() < rhs.x();
    return lhs.y() < rhs.y();
}

bool operator>(Vec2 const& lhs, Vec2 const& rhs) {
    return rhs < lhs;
}

bool operator<=(Vec2 const& lhs, Vec2 const& rhs) {
    return !(rhs < lhs);
}

bool operator>=(Vec2 const& lhs, Vec2 const& rhs) {
    return !(lhs < rhs);
}
```

Les quatre opérateurs découlent de `operator<` par inversion et négation. La logique n'est écrite qu'une seule fois.

### Le problème de la verbosité pré-C++20

Pour un type `Vec2`, il faut donc implémenter **six** opérateurs de comparaison : `==`, `!=`, `<`, `>`, `<=`, `>=`. Même avec le pattern de dérivation, c'est fastidieux et source d'erreurs de copier-coller. C'est précisément le problème que l'opérateur spaceship `<=>` (section 8.5) résout de manière élégante.

---

## Approche C++20 : `operator==` et `operator<=>` simplifiés

Depuis C++20, le compilateur peut **générer automatiquement** les opérateurs de comparaison. Pour les cas simples, une seule ligne suffit :

```cpp
class Vec2 {
    double x_;
    double y_;

public:
    Vec2(double x, double y) : x_{x}, y_{y} {}

    bool operator==(Vec2 const&) const = default;    // == et != auto-générés
    auto operator<=>(Vec2 const&) const = default;   // <, >, <=, >= auto-générés
};
```

Avec ces deux lignes :

- `a == b` compare `x_` puis `y_` membre par membre.
- `a != b` est automatiquement déduit comme la négation de `==`.
- `a < b`, `a > b`, `a <= b`, `a >= b` sont automatiquement déduits de `<=>`.

C'est la forme recommandée pour tout type dont la comparaison est une comparaison membre par membre. La section 8.5 couvre en profondeur les cas où une implémentation personnalisée de `<=>` est nécessaire.

> 💡 Même en C++20, comprendre le pattern canonique pré-C++20 reste important pour trois raisons : la **maintenance de code existant**, les **cas non triviaux** où `= default` ne suffit pas, et la **compréhension de ce que le compilateur génère** quand on écrit `= default`.

---

## Accès aux membres privés : `friend` ou accesseurs ?

Les opérateurs libres n'ont pas accès aux membres privés de la classe. Deux approches sont possibles :

### Approche 1 : via les accesseurs publics

```cpp
Vec2 operator+(Vec2 lhs, Vec2 const& rhs) {
    lhs += rhs;   // operator+= est public — pas besoin d'accès privé
    return lhs;
}
```

C'est l'approche recommandée quand le pattern canonique est respecté (l'opérateur libre délègue à l'opérateur composé membre). Les accesseurs suffisent.

### Approche 2 : fonction `friend`

Quand l'opérateur libre a besoin d'un accès direct aux membres (par exemple pour des raisons de performance, ou parce que le pattern composé n'est pas applicable), on utilise `friend` :

```cpp
class Vec2 {
    double x_, y_;
public:
    // ...

    // Déclaration friend — donne accès aux membres privés
    friend bool operator==(Vec2 const& lhs, Vec2 const& rhs);
};

bool operator==(Vec2 const& lhs, Vec2 const& rhs) {
    return lhs.x_ == rhs.x_ && lhs.y_ == rhs.y_;   // accès direct à x_, y_
}
```

### Approche 3 : `friend` inline (hidden friend)

Une variante élégante et de plus en plus courante est le **hidden friend** — la fonction `friend` est définie directement à l'intérieur de la classe :

```cpp
class Vec2 {
    double x_, y_;
public:
    Vec2(double x, double y) : x_{x}, y_{y} {}

    friend Vec2 operator+(Vec2 lhs, Vec2 const& rhs) {
        lhs.x_ += rhs.x_;
        lhs.y_ += rhs.y_;
        return lhs;
    }

    friend bool operator==(Vec2 const& lhs, Vec2 const& rhs) {
        return lhs.x_ == rhs.x_ && lhs.y_ == rhs.y_;
    }
};
```

Le hidden friend présente un avantage technique important : la fonction n'est trouvée que par **ADL** (*Argument-Dependent Lookup*). Elle n'est pas visible comme une fonction libre ordinaire, ce qui réduit le risque de surcharges ambiguës et accélère la résolution de noms dans les grandes bases de code. C'est le style recommandé par les C++ Core Guidelines et de plus en plus adopté dans les bibliothèques modernes.

---

## Opérateurs de flux : `<<` et `>>`

Les opérateurs d'insertion (`<<`) et d'extraction (`>>`) pour les flux (`std::ostream`, `std::istream`) sont techniquement des opérateurs arithmétiques (décalage de bits), mais leur surcharge pour les flux est un usage si courant qu'il mérite d'être traité ici.

### Insertion dans un flux (`<<`)

```cpp
#include <ostream>

class Vec2 {
    double x_, y_;
public:
    // ...

    friend std::ostream& operator<<(std::ostream& os, Vec2 const& v) {
        os << "(" << v.x_ << ", " << v.y_ << ")";
        return os;
    }
};
```

L'opérateur **doit** être une fonction libre (ou un friend), car l'opérande gauche est un `std::ostream` — un type que vous ne pouvez pas modifier. Il retourne une référence vers le flux pour permettre le chaînage :

```cpp
Vec2 a{1.0, 2.0}, b{3.0, 4.0};  
std::cout << "a = " << a << ", b = " << b << "\n";  
// a = (1, 2), b = (3, 4)
```

### Extraction depuis un flux (`>>`)

```cpp
#include <istream>

class Vec2 {
    // ...

    friend std::istream& operator>>(std::istream& is, Vec2& v) {
        is >> v.x_ >> v.y_;
        return is;
    }
};
```

Le second paramètre est une **référence non-const** car l'extraction modifie l'objet. En production, il faut aussi gérer les erreurs de flux (vérifier `is.fail()`).

### Interaction avec `std::print` (C++23)

Depuis C++23, `std::print` utilise `std::format` plutôt que les flux. Pour rendre un type compatible avec `std::print`, il faut spécialiser `std::formatter` plutôt que surcharger `operator<<`. Ce sujet est couvert en section 12.7. L'opérateur `<<` reste cependant pertinent pour la compatibilité avec le code existant et les bibliothèques qui utilisent les flux.

---

## Exemple complet : la classe `Fraction`

Voici un exemple plus riche qui illustre l'ensemble des patterns sur un type `Fraction` :

```cpp
#include <numeric>   // std::gcd
#include <ostream>
#include <stdexcept>

class Fraction {
    int num_;
    int den_;

    void simplifier() {
        if (den_ < 0) { num_ = -num_; den_ = -den_; }
        int g = std::gcd(std::abs(num_), den_);
        num_ /= g;
        den_ /= g;
    }

public:
    Fraction(int num, int den = 1) : num_{num}, den_{den} {
        if (den == 0) throw std::invalid_argument{"Dénominateur nul"};
        simplifier();
    }

    int numerateur() const { return num_; }
    int denominateur() const { return den_; }

    // --- Opérateurs composés (membres) ---

    Fraction& operator+=(Fraction const& rhs) {
        num_ = num_ * rhs.den_ + rhs.num_ * den_;
        den_ = den_ * rhs.den_;
        simplifier();
        return *this;
    }

    Fraction& operator-=(Fraction const& rhs) {
        num_ = num_ * rhs.den_ - rhs.num_ * den_;
        den_ = den_ * rhs.den_;
        simplifier();
        return *this;
    }

    Fraction& operator*=(Fraction const& rhs) {
        num_ *= rhs.num_;
        den_ *= rhs.den_;
        simplifier();
        return *this;
    }

    Fraction& operator/=(Fraction const& rhs) {
        if (rhs.num_ == 0) throw std::invalid_argument{"Division par zéro"};
        num_ *= rhs.den_;
        den_ *= rhs.num_;
        simplifier();
        return *this;
    }

    // --- Unaire ---

    Fraction operator-() const {
        return Fraction{-num_, den_};
    }

    // --- Comparaison (hidden friends) ---

    friend bool operator==(Fraction const& a, Fraction const& b) {
        return a.num_ == b.num_ && a.den_ == b.den_;  // fractions déjà simplifiées
    }

    friend bool operator<(Fraction const& a, Fraction const& b) {
        return a.num_ * b.den_ < b.num_ * a.den_;     // dénominateurs toujours > 0
    }

    friend bool operator!=(Fraction const& a, Fraction const& b) { return !(a == b); }
    friend bool operator>(Fraction const& a, Fraction const& b)  { return b < a; }
    friend bool operator<=(Fraction const& a, Fraction const& b) { return !(b < a); }
    friend bool operator>=(Fraction const& a, Fraction const& b) { return !(a < b); }

    // --- Flux ---

    friend std::ostream& operator<<(std::ostream& os, Fraction const& f) {
        os << f.num_;
        if (f.den_ != 1) os << "/" << f.den_;
        return os;
    }
};

// --- Opérateurs binaires (fonctions libres) ---

Fraction operator+(Fraction lhs, Fraction const& rhs) { lhs += rhs; return lhs; }  
Fraction operator-(Fraction lhs, Fraction const& rhs) { lhs -= rhs; return lhs; }  
Fraction operator*(Fraction lhs, Fraction const& rhs) { lhs *= rhs; return lhs; }  
Fraction operator/(Fraction lhs, Fraction const& rhs) { lhs /= rhs; return lhs; }  
```

L'utilisation est complètement naturelle :

```cpp
#include <iostream>

int main() {
    Fraction a{1, 3};
    Fraction b{2, 5};

    std::cout << "a     = " << a << "\n";         // 1/3   (via operator<<)
    std::cout << "b     = " << b << "\n";         // 2/5
    std::cout << "a + b = " << (a + b) << "\n";   // 11/15
    std::cout << "a * b = " << (a * b) << "\n";   // 2/15
    std::cout << "-a    = " << (-a) << "\n";       // -1/3
    std::cout << std::boolalpha;
    std::cout << "a < b = " << (a < b) << "\n";   // true
    std::cout << "a == a = " << (a == a) << "\n";  // true

    // Fonctionne avec des entiers grâce au constructeur implicite Fraction(int)
    Fraction c = a + 2;     // a + Fraction{2, 1} = 1/3 + 2 = 7/3
    Fraction d = 3 * b;     // Fraction{3} * b = 6/5

    std::cout << "c = " << c << "\n";   // 7/3
    std::cout << "d = " << d << "\n";   // 6/5
}
```

Le constructeur `Fraction(int, int = 1)` permet la conversion implicite `int → Fraction`. C'est ce qui rend `a + 2` et `3 * b` possibles sans surcharges dédiées. Cette conversion implicite est un choix de design — la section 8.3 discute quand il est judicieux de la bloquer avec `explicit`.

---

## Bonnes pratiques

**Implémentez l'opérateur composé comme membre, l'opérateur binaire comme fonction libre.** C'est le pattern canonique. Il évite la duplication de code et garantit la symétrie.

**Retournez par valeur pour les opérateurs qui créent un nouvel objet.** `operator+`, `operator-`, `operator*`, `operator/` et les unaires de négation retournent un nouvel objet. Ne retournez jamais une référence vers un temporaire — c'est un comportement indéfini.

**Ne modifiez jamais les opérandes d'un opérateur binaire.** `a + b` ne doit modifier ni `a` ni `b`. C'est garanti par le pattern canonique (passage par valeur pour le premier opérande, par référence constante pour le second).

**Définissez les comparaisons en termes d'un opérateur primaire.** `!=` en termes de `==`, les opérateurs d'ordre en termes de `<`. En C++20, préférez `= default` sur `<=>` et `==` (section 8.5).

**Préférez les hidden friends pour les opérateurs de comparaison et de flux.** Ils limitent la visibilité aux contextes ADL, réduisent les risques d'ambiguïté et gardent l'implémentation proche de la classe.

**Surchargez un opérateur uniquement si sa sémantique est évidente.** `Vec2 + Vec2` est évident. `Employe + Employe` ne l'est pas. Dans le doute, utilisez une méthode nommée (`fusionner()`, `combiner()`, `concatener()`).

---

## Résumé

| Opérateur | Type | Implémentation recommandée | Signature canonique |
|---|---|---|---|
| `+=`, `-=`, `*=`, `/=` | Affectation composée | Fonction **membre** | `T& operator+=(T const& rhs)` |
| `+`, `-`, `*`, `/` | Arithmétique binaire | Fonction **libre** | `T operator+(T lhs, T const& rhs)` |
| `-` (unaire) | Négation | Fonction **membre** `const` | `T operator-() const` |
| `==`, `!=` | Égalité | Hidden **friend** | `friend bool operator==(T const&, T const&)` |
| `<`, `>`, `<=`, `>=` | Ordre | Hidden **friend** | `friend bool operator<(T const&, T const&)` |
| `<<` | Insertion flux | Fonction **libre** / friend | `friend ostream& operator<<(ostream&, T const&)` |
| `>>` | Extraction flux | Fonction **libre** / friend | `friend istream& operator>>(istream&, T&)` |

---


⏭️ [Opérateurs d'affectation (= et +=, -=, etc.)](/08-surcharge-operateurs/02-operateurs-affectation.md)
