🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 8.2 — Opérateurs d'affectation (`=` et `+=`, `-=`, etc.)

## Chapitre 8 : Surcharge d'Opérateurs et Conversions · Module 3 : Programmation Orientée Objet

---

## Introduction

L'opérateur d'affectation `=` occupe une place à part parmi les opérateurs surchargeables. C'est le seul opérateur qui est **généré automatiquement** par le compilateur (avec le constructeur de copie, le constructeur de déplacement et le destructeur), et il est directement lié à la **Rule of Five** (section 6.5). Mal implémenté, il provoque des bugs graves — double `delete`, fuites mémoire, corruption silencieuse de données.

Les opérateurs d'affectation composés (`+=`, `-=`, `*=`, `/=`, `%=`, etc.) sont quant à eux les **briques de base** sur lesquelles reposent les opérateurs arithmétiques binaires, comme vu en section 8.1. Leur implémentation est plus simple que celle de `operator=`, mais les conventions de signature et de retour méritent d'être formalisées.

Cette section traite les deux familles ensemble, car elles partagent une caractéristique fondamentale : elles **modifient l'objet courant** et **retournent une référence vers `*this`**.

---

## `operator=` : affectation par copie

### Version générée par le compilateur

Si vous ne définissez pas `operator=`, le compilateur en génère un qui effectue une **copie membre à membre** (*memberwise copy*) :

```cpp
class Point {
    double x_, y_;
public:
    Point(double x, double y) : x_{x}, y_{y} {}
    // operator= généré implicitement :
    // copie x_ depuis rhs.x_, y_ depuis rhs.y_
};

Point a{1.0, 2.0};  
Point b{3.0, 4.0};  
a = b;   // a.x_ = b.x_, a.y_ = b.y_ — correct  
```

Pour les types qui ne gèrent pas de ressources directement (pas de pointeur brut, pas de handle système), cette version générée est correcte. C'est le cas de la majorité des types en C++ moderne, car les ressources sont gérées par des smart pointers ou des conteneurs STL qui implémentent eux-mêmes la copie correctement.

### Quand faut-il écrire `operator=` manuellement ?

Dès que votre classe gère une **ressource brute** — pointeur brut vers de la mémoire allouée, descripteur de fichier, handle réseau — la copie membre à membre ne suffit plus. C'est la situation décrite par la Rule of Five (section 6.5) : si vous devez écrire un destructeur personnalisé, vous devez probablement aussi écrire le constructeur de copie, l'opérateur d'affectation par copie, le constructeur de déplacement et l'opérateur d'affectation par déplacement.

Voici un type `Buffer` qui illustre le problème et sa solution :

```cpp
#include <algorithm>
#include <cstddef>

class Buffer {
    std::size_t taille_;
    int* donnees_;

public:
    explicit Buffer(std::size_t taille)
        : taille_{taille}
        , donnees_{new int[taille]{}} {}

    ~Buffer() {
        delete[] donnees_;
    }

    std::size_t taille() const { return taille_; }
    int& operator[](std::size_t i) { return donnees_[i]; }
    int const& operator[](std::size_t i) const { return donnees_[i]; }

    // Sans operator= personnalisé :
    // a = b copie le POINTEUR donnees_, pas les données
    // → deux objets pointent vers la même mémoire
    // → double delete au moment de la destruction
};
```

### Implémentation naïve de l'affectation par copie

Une première tentative pourrait ressembler à ceci :

```cpp
Buffer& operator=(Buffer const& rhs) {
    delete[] donnees_;                              // libérer l'ancien
    taille_ = rhs.taille_;
    donnees_ = new int[taille_];                    // allouer le nouveau
    std::copy_n(rhs.donnees_, taille_, donnees_);   // copier les données
    return *this;
}
```

Cette version fonctionne dans le cas général, mais elle a un **défaut fatal** : la **self-assignment**. Si l'on écrit `buffer = buffer;` (ce qui peut arriver indirectement, par exemple via `tab[i] = tab[j]` quand `i == j`), on détruit les données **avant** de les copier. Le comportement est indéfini.

### Protection contre la self-assignment

La correction classique ajoute un test d'identité :

```cpp
Buffer& operator=(Buffer const& rhs) {
    if (this != &rhs) {
        delete[] donnees_;
        taille_ = rhs.taille_;
        donnees_ = new int[taille_];
        std::copy_n(rhs.donnees_, taille_, donnees_);
    }
    return *this;
}
```

C'est correct, mais cette version souffre d'un autre problème : si `new int[taille_]` lève une exception, l'objet est dans un **état invalide** — `donnees_` a été détruit mais le nouveau n'a pas été alloué. L'objet n'est plus utilisable ni destructible proprement. C'est une violation de la **garantie forte d'exception** (*strong exception safety*).

---

## Le copy-and-swap idiom

L'idiom **copy-and-swap** résout élégamment les deux problèmes (self-assignment et exception safety) en une seule technique :

```cpp
#include <utility>   // std::swap

class Buffer {
    std::size_t taille_;
    int* donnees_;

public:
    explicit Buffer(std::size_t taille)
        : taille_{taille}
        , donnees_{new int[taille]{}} {}

    // Constructeur de copie
    Buffer(Buffer const& other)
        : taille_{other.taille_}
        , donnees_{new int[taille_]} {
        std::copy_n(other.donnees_, taille_, donnees_);
    }

    // Destructeur
    ~Buffer() {
        delete[] donnees_;
    }

    // Fonction swap membre (noexcept)
    void swap(Buffer& other) noexcept {
        std::swap(taille_, other.taille_);
        std::swap(donnees_, other.donnees_);
    }

    // operator= par copy-and-swap
    Buffer& operator=(Buffer other) {   // ← paramètre par VALEUR
        swap(other);                     // échange avec la copie locale
        return *this;
    }                                    // other (ancienne donnée) est détruit ici
};
```

### Comment ça fonctionne

1. Le paramètre `other` est passé **par valeur**. La copie est effectuée à l'entrée de la fonction, via le constructeur de copie. Si `new` lève une exception pendant cette copie, `*this` n'a pas été modifié — la garantie forte est respectée.

2. `swap(other)` échange les entrailles de `*this` avec celles de la copie locale. `*this` contient maintenant les nouvelles données, et `other` contient les anciennes.

3. À la fin de la fonction, `other` est détruit. Le destructeur libère les anciennes données.

### Pourquoi c'est safe

**Self-assignment** : si `a = a`, le constructeur de copie crée une copie indépendante de `a`. Le swap échange `a` avec cette copie. Le destructeur détruit la copie. L'objet `a` est inchangé — c'est le comportement attendu.

**Exception safety** : si le constructeur de copie échoue (exception lors de `new`), la fonction n'a pas été entrée et `*this` est intact. Si le swap échoue — mais il est `noexcept` car il n'échange que des pointeurs et des entiers, donc il ne peut pas échouer.

### Avantage supplémentaire : gestion unifiée copie + déplacement

Quand le paramètre est passé par valeur, le compilateur choisit automatiquement le **constructeur de copie** ou le **constructeur de déplacement** selon le contexte d'appel :

```cpp
Buffer a{100};  
Buffer b{200};  

a = b;              // b est un lvalue → constructeur de copie → swap  
a = Buffer{300};    // temporaire (rvalue) → constructeur de déplacement → swap  
```

Un seul `operator=` gère les deux cas. Il n'est pas nécessaire d'écrire séparément un `operator=(Buffer&&)`.

> 💡 Certains développeurs préfèrent écrire deux opérateurs séparés (copie et déplacement) pour un contrôle plus fin. C'est un choix valide, couvert dans le paragraphe suivant. Le copy-and-swap est l'approche la plus **simple et la plus sûre**, recommandée quand on ne cherche pas une optimisation maximale.

---

## `operator=` : affectation par déplacement

### Approche séparée

Quand on souhaite un contrôle explicite sur la copie et le déplacement, on écrit deux opérateurs distincts :

```cpp
class Buffer {
    std::size_t taille_;
    int* donnees_;

public:
    // ... constructeurs, destructeur ...

    // Affectation par copie
    Buffer& operator=(Buffer const& rhs) {
        if (this != &rhs) {
            delete[] donnees_;
            taille_ = rhs.taille_;
            donnees_ = new int[taille_];
            std::copy_n(rhs.donnees_, taille_, donnees_);
        }
        return *this;
    }

    // Affectation par déplacement
    Buffer& operator=(Buffer&& rhs) noexcept {
        if (this != &rhs) {
            delete[] donnees_;           // libérer l'ancien
            taille_ = rhs.taille_;       // voler les ressources
            donnees_ = rhs.donnees_;
            rhs.taille_ = 0;            // laisser rhs dans un état valide
            rhs.donnees_ = nullptr;
        }
        return *this;
    }
};
```

L'opérateur de déplacement est marqué **`noexcept`** car il ne fait que transférer des pointeurs et des entiers — aucune allocation, aucune exception possible. Ce `noexcept` est important : il permet aux conteneurs STL (comme `std::vector`) d'utiliser le déplacement lors des réallocations plutôt que la copie, ce qui améliore significativement les performances.

### État de l'objet déplacé (*moved-from state*)

Après un déplacement, l'objet source (`rhs`) doit rester dans un **état valide mais indéterminé**. Cela signifie :

- Le destructeur doit pouvoir s'exécuter sans problème (d'où `rhs.donnees_ = nullptr` — `delete[] nullptr` est garanti sans effet).
- L'objet peut être réaffecté ou détruit, mais son contenu n'est pas garanti.

La convention est de laisser l'objet dans un état "vide" ou "par défaut", comparable à celui d'un objet construit par défaut.

---

## `= default` et `= delete`

Pour les types simples qui n'ont pas besoin d'une gestion manuelle, vous pouvez demander explicitement au compilateur de générer l'opérateur ou de le supprimer :

```cpp
class Config {
    std::string nom_;
    int version_;
public:
    Config(std::string nom, int v) : nom_{std::move(nom)}, version_{v} {}

    // Demander explicitement la génération
    Config& operator=(Config const&) = default;
    Config& operator=(Config&&) = default;
};

class Connexion {
    int socket_fd_;
public:
    // Interdire la copie (une connexion ne se copie pas)
    Connexion& operator=(Connexion const&) = delete;

    // Permettre le déplacement
    Connexion& operator=(Connexion&& other) noexcept {
        if (this != &other) {
            close(socket_fd_);
            socket_fd_ = other.socket_fd_;
            other.socket_fd_ = -1;
        }
        return *this;
    }
};
```

Utiliser `= default` est préférable au silence — cela documente explicitement l'intention et garantit que le compilateur ne supprimera pas implicitement l'opérateur si vous ajoutez plus tard un destructeur ou un constructeur de copie.

> ⚠️ **Rappel de la Rule of Five** (section 6.5) : si vous définissez *l'un quelconque* des cinq (destructeur, constructeur de copie, opérateur d'affectation par copie, constructeur de déplacement, opérateur d'affectation par déplacement), définissez-les **tous les cinq** — ou marquez-les explicitement `= default` / `= delete`.

---

## Opérateurs d'affectation composés : `+=`, `-=`, `*=`, `/=`, `%=`

Les opérateurs composés sont plus simples que `operator=` car ils ne soulèvent ni la question de la self-assignment destructrice ni celle de la gestion de ressources. Ils modifient `*this` selon l'opération et retournent une référence vers `*this`.

### Signature canonique

```cpp
class Vec2 {
    double x_, y_;
public:
    Vec2(double x, double y) : x_{x}, y_{y} {}

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

    Vec2& operator*=(double scalaire) {
        x_ *= scalaire;
        y_ *= scalaire;
        return *this;
    }

    Vec2& operator/=(double scalaire) {
        // Gestion de la division par zéro selon votre politique d'erreur
        x_ /= scalaire;
        y_ /= scalaire;
        return *this;
    }
};
```

Les conventions sont constantes :

| Aspect | Convention |
|---|---|
| Type de fonction | Membre |
| Retour | `T&` (référence vers `*this`) |
| Paramètre | `T const&` (ou type scalaire par valeur) |
| Qualification `const` | **Non** — la méthode modifie `*this` |

### Pourquoi des fonctions membres ?

Les opérateurs composés **doivent** être des fonctions membres (c'est une exigence du standard pour `operator=`, et une forte convention pour `+=`, `-=`, etc.). La raison est sémantique : ces opérateurs modifient l'opérande gauche, qui doit donc être un objet modifiable. En tant que fonction membre, l'opérande gauche est implicitement `*this`.

### Chaînage

Le retour de `*this` par référence permet le chaînage, bien que celui-ci soit rarement utilisé avec les opérateurs composés :

```cpp
Vec2 v{1.0, 2.0};
(v += Vec2{1.0, 0.0}) *= 2.0;
// v est maintenant {4.0, 4.0}
```

Ce style est peu lisible et déconseillé, mais le supporter est la convention — tout comme `a = b = c` est supporté pour `operator=`.

---

## Relation entre `operator=` et les opérateurs composés

Il est important de distinguer deux familles d'opérateurs qui utilisent le signe `=` mais qui n'ont **rien en commun** techniquement :

| Opérateur | Catégorie | Rôle | Lien avec la Rule of Five |
|---|---|---|---|
| `operator=` | Affectation simple | Remplacer le contenu de l'objet | **Direct** — fait partie des Big Five |
| `operator+=`, `-=`, etc. | Affectation composée | Modifier le contenu existant | **Aucun** — indépendant de la Rule of Five |

L'opérateur `a += b` n'est **pas** équivalent à `a = a + b`. Conceptuellement le résultat est le même, mais mécaniquement :

- `a += b` modifie `a` en place. Pas de création de temporaire, pas de copie.
- `a = a + b` crée un temporaire (`a + b`), puis affecte ce temporaire à `a`. Potentiellement plus coûteux, sauf si le compilateur optimise (RVO/copy elision).

C'est pourquoi le pattern canonique définit `operator+` **en termes de** `operator+=` (section 8.1) — pour garantir que la logique est identique tout en offrant les deux syntaxes.

---

## Opérateurs composés avec types hétérogènes

Les opérateurs composés peuvent accepter un type différent de la classe :

```cpp
class Matrix {
    // ...
public:
    // Multiplication matrice × scalaire
    Matrix& operator*=(double scalaire) {
        for (auto& val : donnees_) val *= scalaire;
        return *this;
    }

    // Multiplication matrice × matrice
    Matrix& operator*=(Matrix const& rhs) {
        *this = *this * rhs;   // délègue à operator* (cas particulier)
        return *this;
    }
};
```

Le cas `operator*=(Matrix const&)` est particulier : contrairement à l'addition ou la soustraction, la multiplication matricielle ne peut pas facilement s'effectuer en place (le résultat dépend de toute la matrice d'origine). On crée donc un temporaire via `operator*` et on affecte le résultat. C'est l'exception au pattern habituel "le composé est la brique de base du binaire" — ici c'est l'inverse.

---

## Opérateurs d'affectation pour les types bit-à-bit : `|=`, `&=`, `^=`, `<<=`, `>>=`

Les opérateurs bit-à-bit composés suivent exactement les mêmes conventions. Ils sont particulièrement utiles pour les types qui représentent des **flags** ou des **bitmasks** :

```cpp
#include <cstdint>
#include <iostream>

class Permissions {
    std::uint8_t bits_;

public:
    enum Flag : std::uint8_t {
        Lecture  = 0b001,
        Ecriture = 0b010,
        Execution = 0b100
    };

    explicit Permissions(std::uint8_t bits = 0) : bits_{bits} {}

    Permissions& operator|=(Flag f) {
        bits_ |= f;
        return *this;
    }

    Permissions& operator&=(std::uint8_t mask) {
        bits_ &= mask;
        return *this;
    }

    bool a(Flag f) const { return (bits_ & f) != 0; }

    friend Permissions operator|(Permissions p, Flag f) {
        p |= f;
        return p;
    }

    friend std::ostream& operator<<(std::ostream& os, Permissions const& p) {
        os << (p.a(Lecture) ? "r" : "-")
           << (p.a(Ecriture) ? "w" : "-")
           << (p.a(Execution) ? "x" : "-");
        return os;
    }
};
```

```cpp
Permissions p;  
p |= Permissions::Lecture;  
p |= Permissions::Execution;  
std::cout << "Permissions : " << p << "\n";   // r-x  (via operator<<)  
```

---

## Exemple complet : `String` simplifiée

Voici un type `String` simplifié qui illustre l'interaction entre `operator=` (Rule of Five), `operator+=` (concaténation) et le copy-and-swap idiom :

```cpp
#include <algorithm>
#include <cstring>
#include <iostream>
#include <utility>

class String {
    char* data_;
    std::size_t len_;

    void alloc_and_copy(char const* src, std::size_t n) {
        data_ = new char[n + 1];
        std::memcpy(data_, src, n);
        data_[n] = '\0';
        len_ = n;
    }

public:
    // --- Constructeurs ---

    String() : data_{new char[1]{'\0'}}, len_{0} {}

    String(char const* s)
        : data_{nullptr}, len_{0} {
        alloc_and_copy(s, std::strlen(s));
    }

    // Constructeur de copie
    String(String const& other)
        : data_{nullptr}, len_{0} {
        alloc_and_copy(other.data_, other.len_);
    }

    // Constructeur de déplacement
    String(String&& other) noexcept
        : data_{other.data_}, len_{other.len_} {
        other.data_ = nullptr;
        other.len_ = 0;
    }

    // --- Destructeur ---

    ~String() { delete[] data_; }

    // --- swap ---

    void swap(String& other) noexcept {
        std::swap(data_, other.data_);
        std::swap(len_, other.len_);
    }

    // --- operator= (copy-and-swap) ---

    String& operator=(String other) {   // par valeur → copie ou déplacement
        swap(other);
        return *this;
    }

    // --- operator+= (concaténation) ---

    String& operator+=(String const& rhs) {
        char* nouveau = new char[len_ + rhs.len_ + 1];
        std::memcpy(nouveau, data_, len_);
        std::memcpy(nouveau + len_, rhs.data_, rhs.len_);
        nouveau[len_ + rhs.len_] = '\0';
        delete[] data_;
        data_ = nouveau;
        len_ += rhs.len_;
        return *this;
    }

    String& operator+=(char const* rhs) {
        return *this += String{rhs};   // réutilise la version String
    }

    // --- Accesseurs ---

    char const* c_str() const { return data_; }
    std::size_t taille() const { return len_; }

    // --- Flux ---

    friend std::ostream& operator<<(std::ostream& os, String const& s) {
        return os << s.data_;
    }
};

// --- operator+ (fonction libre) ---

String operator+(String lhs, String const& rhs) {
    lhs += rhs;
    return lhs;
}

String operator+(String lhs, char const* rhs) {
    lhs += rhs;
    return lhs;
}
```

L'utilisation montre la cohérence entre les différents opérateurs :

```cpp
int main() {
    String a{"Hello"};
    String b{" World"};

    String c = a + b;      // operator+ → copie de a, puis +=
    std::cout << c << "\n";  // Hello World

    a += "!";              // operator+= avec char const*
    std::cout << a << "\n";  // Hello!

    String d;
    d = a;                 // operator= par copie (copy-and-swap)
    d = std::move(b);      // operator= par déplacement (move + swap)
}
```

Cet exemple illustre la Rule of Five complète (destructeur, copie, déplacement, `operator=` unifié via copy-and-swap) et l'articulation avec `operator+=`/`operator+`.

---

## Bonnes pratiques

**Utilisez le copy-and-swap idiom pour `operator=` quand vous gérez des ressources.** C'est l'approche la plus sûre : exception-safe, self-assignment-safe, et elle unifie copie et déplacement en un seul opérateur.

**Marquez `operator=(T&&)` comme `noexcept` quand c'est possible.** Le déplacement ne devrait jamais lever d'exception si vous ne faites que transférer des pointeurs et des scalaires. Le `noexcept` permet aux conteneurs STL d'utiliser le déplacement lors des réallocations.

**Laissez l'objet déplacé dans un état valide et destructible.** Mettez les pointeurs à `nullptr`, les tailles à 0, les handles à une valeur invalide. Le destructeur doit pouvoir s'exécuter sans erreur.

**Préférez `= default` quand la copie membre à membre suffit.** Si tous vos membres sont des types qui gèrent eux-mêmes leurs ressources (`std::string`, `std::vector`, `std::unique_ptr`), le `operator=` généré est correct. L'écrire manuellement est non seulement inutile mais risqué.

**Appliquez la Rule of Five ou la Rule of Zero.** Soit vous gérez une ressource et vous implémentez les cinq fonctions spéciales, soit vous n'en gérez aucune et vous les laissez toutes en `= default` (implicitement ou explicitement). Les états intermédiaires (définir un destructeur mais pas `operator=`) sont des recettes pour des bugs.

**Implémentez les opérateurs composés comme briques de base.** `operator+=` contient la logique, `operator+` délègue. Ne dupliquez jamais le code entre les deux.

---

## Résumé

| Opérateur | Type | Signature | Self-assignment | Exception safety |
|---|---|---|---|---|
| `operator=(T const&)` | Copie | `T& operator=(T const&)` | À gérer (`if (this != &rhs)`) | Attention à l'allocation |
| `operator=(T&&)` | Déplacement | `T& operator=(T&&) noexcept` | À gérer | Garanti `noexcept` |
| `operator=(T)` (copy-and-swap) | Unifié | `T& operator=(T other)` | Sûr par construction | Garantie forte |
| `operator+=`, `-=`, etc. | Composé | `T& operator+=(T const&)` | Généralement non-problématique | Dépend de l'implémentation |

| Idiom | Principe | Avantage |
|---|---|---|
| **Copy-and-swap** | Copier par valeur, échanger, laisser le destructeur nettoyer | Exception-safe + self-assignment-safe |
| **Rule of Five** | Définir les 5 fonctions spéciales ensemble | Cohérence et correction |
| **Rule of Zero** | Ne rien définir (tout en `= default`) | Simplicité maximale |

---


⏭️ [Opérateurs de conversion (conversion operators)](/08-surcharge-operateurs/03-operateurs-conversion.md)
