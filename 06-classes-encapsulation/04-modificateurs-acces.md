🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 6.4 — Modificateurs d'accès : public, private, protected

## Chapitre 6 : Classes et Encapsulation

---

## Ce que vous allez apprendre

- Comprendre le rôle de chaque modificateur d'accès et ce qu'il autorise ou interdit.  
- Appliquer le **principe du moindre privilège** à la conception de vos classes.  
- Savoir quand utiliser `protected` — et pourquoi c'est plus rare qu'on ne le pense.  
- Maîtriser le mot-clé `friend` et ses cas d'usage légitimes.  
- Organiser les sections d'accès dans une classe pour maximiser la lisibilité.

---

## Le principe : protéger l'invariant

En section 6.1, nous avons défini l'invariant d'une classe comme un ensemble de règles que l'objet respecte à tout instant. Pour `DynArray`, l'invariant est : *si `size_ > 0`, alors `data_` pointe vers un bloc valide de `size_` entiers ; si `size_ == 0`, alors `data_ == nullptr`*.

Les modificateurs d'accès sont le mécanisme qui **protège** cet invariant. Ils contrôlent quel code a le droit de lire ou modifier les données internes de la classe. Sans eux, n'importe quel code extérieur pourrait modifier `data_` sans mettre à jour `size_`, ou mettre `size_` à zéro sans libérer la mémoire — violant l'invariant et corrompant l'objet.

Le C++ propose trois niveaux d'accès :

| Modificateur | Accessible depuis | Usage principal |  
|---|---|---|  
| `public` | Partout | Interface visible par les utilisateurs de la classe |  
| `private` | La classe elle-même uniquement | Données internes, méthodes d'implémentation |  
| `protected` | La classe et ses classes dérivées | Membres destinés à l'extension par héritage |

---

## `public` : l'interface

Les membres `public` forment le **contrat** de la classe avec le monde extérieur. C'est ce que l'utilisateur de la classe voit, appelle, et sur quoi il construit son code. L'interface publique doit être :

- **Minimale** — n'exposer que ce qui est nécessaire.  
- **Stable** — changer l'interface publique casse le code client.  
- **Suffisante** — permettre toutes les opérations légitimes sur l'objet.

```cpp
class DynArray {  
public:  
    // Constructeurs et destructeur — l'utilisateur doit pouvoir créer des objets
    DynArray() = default;
    explicit DynArray(std::size_t size);
    DynArray(std::size_t size, int value);
    DynArray(const DynArray& other);
    DynArray(DynArray&& other) noexcept;
    ~DynArray();

    // Opérations — l'interface que l'utilisateur manipule
    std::size_t size() const;
    bool empty() const;
    int& operator[](std::size_t index);
    const int& operator[](std::size_t index) const;

    // L'utilisateur n'a PAS besoin de savoir comment c'est stocké en interne
};
```

L'utilisateur de `DynArray` sait qu'il peut créer un tableau, connaître sa taille, et accéder à ses éléments. Il ne sait pas — et n'a pas besoin de savoir — que les données sont stockées via un `int*` alloué avec `new[]`. Si demain vous remplacez l'implémentation par un `std::vector` interne ou un allocateur personnalisé, le code client ne change pas.

---

## `private` : l'implémentation

Les membres `private` sont inaccessibles en dehors de la classe. Seules les fonctions membres de la classe (et les `friend`, que nous verrons plus loin) peuvent y accéder.

```cpp
class DynArray {
    // ...
private:
    int* data_ = nullptr;       // Détail d'implémentation
    std::size_t size_ = 0;      // Détail d'implémentation

    void reallocate(std::size_t new_capacity);   // Méthode interne
};
```

Tenter d'accéder à un membre privé depuis l'extérieur produit une erreur de compilation claire :

```cpp
DynArray arr(10);  
arr.data_[0] = 42;     // ERREUR : 'int* DynArray::data_' is private within this context  
arr.size_ = 0;         // ERREUR : idem — l'invariant est protégé  
```

### Ce que `private` protège réellement

Il est important de comprendre que `private` est une protection au moment de la **compilation**, pas à l'exécution. Un développeur déterminé pourrait techniquement contourner les protections d'accès avec des casts ou des manipulations mémoire. Mais ce n'est pas le propos. Le rôle de `private` est de :

- **Documenter l'intention** — "ce membre fait partie de l'implémentation, ne l'utilisez pas directement."  
- **Empêcher les erreurs accidentelles** — le compilateur refuse les accès non autorisés.  
- **Permettre l'évolution** — vous pouvez modifier les membres privés sans casser le code client, puisque personne ne les utilise directement.

### Accès entre instances de la même classe

Un point qui surprend souvent : un objet peut accéder aux membres **privés** d'un autre objet **du même type**. L'accès privé est contrôlé par **classe**, pas par instance :

```cpp
DynArray::DynArray(const DynArray& other)
    : data_(other.size_ > 0 ? new int[other.size_] : nullptr)   // Accède à other.size_
    , size_(other.size_) {                                        // Accède à other.size_
    std::copy_n(other.data_, size_, data_);                       // Accède à other.data_
}
```

C'est indispensable pour implémenter le constructeur de copie, l'opérateur d'affectation, et toute opération qui compare ou combine deux instances. Le raisonnement est le suivant : le code du constructeur de copie fait partie de la classe `DynArray` — il a été écrit par l'auteur de la classe, qui connaît l'invariant et sait comment manipuler les membres privés sans le violer.

---

## `protected` : l'interface d'extension

Les membres `protected` sont accessibles depuis la classe elle-même **et** depuis ses classes dérivées. C'est un niveau intermédiaire entre `public` (tout le monde) et `private` (la classe seule).

```cpp
class Shape {  
public:  
    virtual double area() const = 0;
    virtual ~Shape() = default;

    std::string color() const { return color_; }

protected:
    // Les classes dérivées peuvent lire et modifier la couleur
    void set_color(const std::string& c) { color_ = c; }

private:
    // Même les classes dérivées ne peuvent pas accéder directement à color_
    std::string color_ = "black";
};

class Circle : public Shape {  
public:  
    explicit Circle(double radius) : radius_(radius) {}

    double area() const override {
        return 3.14159265358979 * radius_ * radius_;
    }

    void make_red() {
        set_color("red");    // OK — set_color est protected, Circle dérive de Shape
        // color_ = "red";   // ERREUR — color_ est private dans Shape
    }

private:
    double radius_;
};
```

### Quand utiliser `protected` ?

En pratique, `protected` est **beaucoup moins fréquent** que `public` et `private`. Les *C++ Core Guidelines* (règle C.133) recommandent d'éviter les données membres `protected` :

**Données `protected` — à éviter.** Exposer des données aux classes dérivées brise l'encapsulation presque autant que `public`. La classe de base ne peut plus garantir son invariant, car n'importe quelle classe dérivée peut modifier les données directement. Préférez des données `private` avec des accesseurs `protected` si les classes dérivées ont besoin de lire ou modifier l'état du parent de manière contrôlée.

```cpp
// DÉCONSEILLÉ — données protected
class Base {  
protected:  
    int count_ = 0;   // N'importe quelle dérivée peut mettre count_ à -1
};

// RECOMMANDÉ — méthodes protected, données private
class Base {  
public:  
    int count() const { return count_; }
protected:
    void increment_count() { ++count_; }   // Contrôle sur les modifications
private:
    int count_ = 0;                         // Invariant protégé
};
```

**Méthodes `protected` — légitimes dans deux cas.** Premier cas : les méthodes destinées à être appelées par les classes dérivées, comme dans l'exemple `set_color` ci-dessus. Second cas : les méthodes virtuelles destinées à être redéfinies par les classes dérivées (le pattern *Template Method*). Nous approfondirons ce point au chapitre 7 (Héritage et Polymorphisme).

---

## `friend` : accès contrôlé hors de la classe

Le mot-clé `friend` donne à une fonction ou une classe extérieure l'accès aux membres `private` et `protected` d'une classe. C'est une ouverture ciblée dans l'encapsulation.

### Fonction `friend`

Le cas d'usage le plus courant est la surcharge de l'opérateur `<<` pour l'affichage :

```cpp
class DynArray {  
public:  
    // ... interface publique ...

    // Déclare operator<< comme ami — il peut accéder aux membres privés
    friend std::ostream& operator<<(std::ostream& os, const DynArray& arr);

private:
    int* data_ = nullptr;
    std::size_t size_ = 0;
};

// Définition — a accès à arr.data_ et arr.size_
std::ostream& operator<<(std::ostream& os, const DynArray& arr) {
    os << "[";
    for (std::size_t i = 0; i < arr.size_; ++i) {
        if (i > 0) os << ", ";
        os << arr.data_[i];
    }
    os << "]";
    return os;
}
```

Pourquoi `operator<<` ne peut-il pas être une méthode membre ? Parce que pour écrire `std::cout << arr`, l'opérande gauche doit être un `std::ostream` — pas un `DynArray`. L'opérateur doit donc être une fonction libre, et `friend` lui donne l'accès aux membres privés sans forcer l'ajout d'accesseurs publics superflus.

### Classe `friend`

Une classe entière peut être déclarée amie :

```cpp
class Engine {
    friend class Debugger;   // Debugger a accès à tous les membres privés de Engine

private:
    int internal_state_ = 0;
    void secret_method() {}
};

class Debugger {  
public:  
    void inspect(const Engine& e) {
        std::cout << e.internal_state_ << "\n";   // OK — Debugger est ami de Engine
        // e.secret_method();                       // OK aussi
    }
};
```

### `friend` est-il une violation de l'encapsulation ?

C'est un débat récurrent. La réponse nuancée est la suivante :

`friend` **ne brise pas** l'encapsulation — il l'**étend de manière contrôlée**. C'est la classe elle-même qui décide qui est son ami. Un code extérieur ne peut pas se déclarer ami d'une classe — c'est toujours la classe qui accorde l'accès. De plus, l'amitié n'est ni héritée (les classes dérivées de `Debugger` ne sont pas automatiquement amies de `Engine`), ni transitive (si A est ami de B et B est ami de C, A n'est pas ami de C).

Cela dit, `friend` doit rester **l'exception**. Les cas légitimes sont :

- La surcharge d'opérateurs qui ne peuvent pas être des méthodes membres (`<<`, `>>`, opérateurs binaires symétriques).  
- Les fonctions ou classes étroitement couplées par conception (un itérateur et son conteneur, un builder et l'objet qu'il construit).  
- Les fonctions de test qui ont besoin d'inspecter l'état interne (discutable — souvent un signe que l'interface publique est insuffisante).

Si vous avez besoin de `friend` fréquemment, c'est souvent un signal que votre interface publique manque d'une fonctionnalité, ou que vos classes sont trop couplées.

---

## Organiser les sections d'accès

L'ordre dans lequel vous arrangez les sections `public`, `private` et `protected` dans une classe n'affecte pas le comportement du programme, mais il a un impact majeur sur la lisibilité. La convention la plus répandue, recommandée par le *Google C++ Style Guide* et les *Core Guidelines*, est de placer `public` en premier :

```cpp
class WellOrganized {  
public:  
    // --- Types et alias ---
    using value_type = int;
    using size_type = std::size_t;

    // --- Constructeurs et destructeur ---
    WellOrganized() = default;
    explicit WellOrganized(size_type size);
    WellOrganized(const WellOrganized& other);
    WellOrganized(WellOrganized&& other) noexcept;
    ~WellOrganized();

    // --- Opérateurs d'affectation ---
    WellOrganized& operator=(const WellOrganized& other);
    WellOrganized& operator=(WellOrganized&& other) noexcept;

    // --- Interface publique ---
    size_type size() const;
    bool empty() const;
    value_type& operator[](size_type index);
    const value_type& operator[](size_type index) const;

    // --- Friends ---
    friend std::ostream& operator<<(std::ostream& os, const WellOrganized& obj);

protected:
    // --- Interface d'extension (si héritage) ---
    void on_resize(size_type new_size);

private:
    // --- Données membres ---
    value_type* data_ = nullptr;
    size_type size_ = 0;

    // --- Méthodes internes ---
    void reallocate(size_type new_capacity);
};
```

Le raisonnement est simple : l'utilisateur de la classe s'intéresse d'abord à l'interface publique. Les détails d'implémentation viennent en dernier. C'est comme un document technique : le résumé en haut, les détails en bas.

> 💡 Certaines équipes préfèrent l'ordre inverse (`private` d'abord), argumentant que comprendre les données internes aide à comprendre l'interface. C'est moins courant mais pas faux. L'essentiel est d'être **cohérent** au sein d'un projet.

---

## `struct` revisité : conséquence sur l'accès

Rappelons (section 6.1) que la seule différence entre `struct` et `class` est la visibilité par défaut. Cela a une conséquence directe sur l'encapsulation :

```cpp
struct Point {
    double x;    // public par défaut — aucun invariant à protéger
    double y;    // public par défaut
};

class Circle {
    double radius_;   // private par défaut — invariant protégé (radius >= 0)
public:
    explicit Circle(double r) : radius_(r) {
        if (r < 0) throw std::invalid_argument("Negative radius");
    }
    double radius() const { return radius_; }
};
```

`Point` est un agrégat : toute combinaison de `x` et `y` est valide. Il n'y a pas d'invariant à protéger, donc `public` par défaut (via `struct`) est le bon choix. `Circle` a un invariant (`radius_ >= 0`), donc `private` par défaut (via `class`) est approprié.

---

## Encapsulation et accesseurs : le juste milieu

Un piège fréquent chez les débutants est de mettre toutes les données en `private` puis de créer un getter et un setter pour chaque membre. Cela donne l'illusion de l'encapsulation sans en offrir le bénéfice :

```cpp
// MAUVAIS — getters/setters triviaux pour tout → pas d'encapsulation réelle
class PseudoEncapsulated {  
public:  
    int count() const { return count_; }
    void set_count(int c) { count_ = c; }       // Aucune vérification

    std::string name() const { return name_; }
    void set_name(const std::string& n) { name_ = n; }  // Aucune vérification

private:
    int count_;
    std::string name_;
};
// Autant utiliser un struct avec des membres publics...
```

L'encapsulation a de la valeur quand elle **protège quelque chose**. Si n'importe qui peut écrire n'importe quelle valeur dans un membre via un setter, ce membre est effectivement public.

Les bons accesseurs suivent ces principes :

**Getters : libéralement.** Retourner une valeur en lecture seule ne compromet pas l'invariant. Les getters `const` sont presque toujours légitimes.

**Setters : avec parcimonie et validation.** Un setter ne devrait exister que si la modification du membre de l'extérieur a un sens, et il devrait valider la valeur pour protéger l'invariant :

```cpp
class Temperature {  
public:  
    explicit Temperature(double kelvin) : kelvin_(kelvin) {
        if (kelvin < 0.0) throw std::invalid_argument("Below absolute zero");
    }

    double kelvin() const { return kelvin_; }
    double celsius() const { return kelvin_ - 273.15; }

    void set_kelvin(double k) {
        if (k < 0.0) throw std::invalid_argument("Below absolute zero");
        kelvin_ = k;    // L'invariant est maintenu
    }

    // Pas de set_celsius — on évite les doubles interfaces de modification
    // L'utilisateur convertit lui-même : t.set_kelvin(celsius + 273.15)

private:
    double kelvin_;
};
```

**Opérations plutôt que setters.** Souvent, au lieu d'un setter, une opération de plus haut niveau est préférable. Au lieu de `set_size(n)`, proposez `resize(n)` qui gère aussi la réallocation. Au lieu de `set_balance(x)`, proposez `deposit(amount)` et `withdraw(amount)` qui vérifient les contraintes métier.

---

## Fil conducteur : accès dans `DynArray`

Notre `DynArray` a naturellement adopté les bons choix d'accès tout au long de ce chapitre. Récapitulons :

```cpp
class DynArray {  
public:  
    // Interface publique — ce que l'utilisateur manipule
    DynArray() = default;
    explicit DynArray(std::size_t size);
    DynArray(std::size_t size, int value);
    DynArray(const DynArray& other);
    DynArray(DynArray&& other) noexcept;
    ~DynArray();

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    int& operator[](std::size_t index);
    const int& operator[](std::size_t index) const;

    // Friend pour l'affichage
    friend std::ostream& operator<<(std::ostream& os, const DynArray& arr);

private:
    // Implémentation — invisible depuis l'extérieur
    int* data_ = nullptr;
    std::size_t size_ = 0;
};
```

Aucun setter sur `data_` ou `size_` — ces membres ne sont modifiables qu'à travers les constructeurs, le destructeur, et les opérateurs d'affectation (section 6.5). L'invariant est protégé structurellement. L'utilisateur interagit uniquement via l'interface publique : `size()`, `empty()`, `operator[]`. Il ne sait pas et n'a pas besoin de savoir que les données sont stockées dans un bloc alloué par `new`.

---

## Points clés à retenir

- `public` expose l'**interface** — gardez-la minimale, stable et suffisante. C'est le contrat avec le code client.  
- `private` cache l'**implémentation** — les données internes et les méthodes auxiliaires. Vous pouvez les modifier sans casser le code qui utilise votre classe.  
- `protected` est un niveau intermédiaire pour l'**extension par héritage**. Évitez les données `protected` ; préférez des méthodes `protected` qui contrôlent l'accès.  
- L'accès privé est par **classe**, pas par instance. Un objet peut accéder aux membres privés d'un autre objet du même type.  
- `friend` accorde un accès ciblé à une fonction ou une classe extérieure. C'est la classe qui décide — l'amitié n'est ni héritée ni transitive. Réservez `friend` aux opérateurs et aux couplages structurels.  
- Placez `public` en premier dans la déclaration de classe. L'utilisateur lit l'interface avant les détails.  
- Évitez les getters/setters mécaniques. Un setter sans validation ne protège rien. Préférez des **opérations** de haut niveau qui maintiennent l'invariant.  
- Utilisez `struct` pour les agrégats sans invariant (tout `public`) et `class` pour les types avec invariant (données `private`).

---


⏭️ [La règle des 5 (Rule of Five)](/06-classes-encapsulation/05-rule-of-five.md)
