🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 44.3 — CRTP (Curiously Recurring Template Pattern)

## Polymorphisme statique poussé à son maximum

---

## Définition et intention

Le CRTP (*Curiously Recurring Template Pattern*) est un idiome C++ où une classe dérive d'un template paramétré **par elle-même** :

```cpp
template<typename Derived>  
class Base {  
    // Base connaît le type réel de la classe dérivée
};

class Concrete : public Base<Concrete> {
    // Concrete hérite de Base<Concrete>
};
```

Cette récursion apparente n'est ni un bug ni une curiosité : c'est un mécanisme puissant qui permet à la classe de base d'appeler des méthodes de la classe dérivée **sans fonctions virtuelles**, sans vtable et sans aucune indirection à l'exécution. Le dispatch est résolu entièrement à la compilation.

Le CRTP est au cœur de nombreuses bibliothèques C++ majeures : Eigen (algèbre linéaire), Boost.Operators, les itérateurs CRTP de la STL, et d'innombrables frameworks internes dans l'industrie. En C++23, le mécanisme `deducing this` offre une alternative plus lisible pour certains cas d'usage — mais le CRTP reste indispensable dans d'autres. Cette section couvre les deux.

---

## Le mécanisme fondamental

### Comment Base accède à Derived

La classe de base effectue un `static_cast<Derived*>(this)` pour accéder aux méthodes et données de la classe dérivée. Ce cast est **sûr** car, par construction, `this` est toujours de type `Derived` (puisque `Derived` hérite de `Base<Derived>`) :

```cpp
template<typename Derived>  
class Shape {  
public:  
    double area() const {
        // Appel de la méthode de la classe dérivée — résolu à la compilation
        return self().area_impl();
    }

    void describe() const {
        std::print("Forme d'aire {:.2f}\n", self().area_impl());
    }

private:
    // Helper : cast vers le type dérivé
    const Derived& self() const {
        return static_cast<const Derived&>(*this);
    }

    Derived& self() {
        return static_cast<Derived&>(*this);
    }
};
```

```cpp
class Circle : public Shape<Circle> {
    double radius_;
public:
    explicit Circle(double r) : radius_(r) {}

    // Implémentation concrète — pas de virtual, pas d'override
    double area_impl() const {
        return std::numbers::pi * radius_ * radius_;
    }
};

class Rectangle : public Shape<Rectangle> {
    double w_, h_;
public:
    Rectangle(double w, double h) : w_(w), h_(h) {}

    double area_impl() const {
        return w_ * h_;
    }
};
```

```cpp
auto c = Circle(5.0);  
c.describe();  // "Forme d'aire 78.54" — aucune vtable, appel inline  

auto r = Rectangle(3.0, 4.0);  
r.describe();  // "Forme d'aire 12.00"  
```

### Pourquoi le `static_cast` est sûr

Au moment où `Base<Derived>::area()` est appelée, `this` pointe vers un objet `Derived` (puisque `Derived` hérite de `Base<Derived>`). Le `static_cast` vers `Derived&` est un *downcast* le long de la hiérarchie d'héritage, garanti valide par construction. C'est un cast à coût zéro — il ne génère aucune instruction machine.

Le seul scénario dangereux serait de passer un mauvais type au template :

```cpp
class Evil : public Shape<Circle> {  // ❌ Evil prétend être Circle
    // static_cast vers Circle alors que this est Evil → comportement indéfini
};
```

Ce piège peut être détecté à la compilation (voir la section "Garde contre les abus" plus bas).

---

## Cas d'usage 1 : Polymorphisme statique (Static Dispatch)

C'est l'utilisation fondamentale du CRTP : obtenir un **polymorphisme sans vtable**, avec inlining complet par le compilateur.

### Comparaison avec le polymorphisme dynamique

```cpp
// Polymorphisme dynamique — vtable, indirection
class ShapeVirtual {  
public:  
    virtual ~ShapeVirtual() = default;
    virtual double area() const = 0;
};

// Polymorphisme statique — CRTP, zéro overhead
template<typename Derived>  
class ShapeCRTP {  
public:  
    double area() const {
        return static_cast<const Derived&>(*this).area_impl();
    }
};
```

| Aspect | `virtual` (dynamique) | CRTP (statique) |
|---|---|---|
| Dispatch | Runtime (vtable lookup) | Compile-time (inliné) |
| Overhead par appel | ~1-3 ns (indirection) | 0 ns (appel direct) |
| Taille de l'objet | +8 bytes (vptr) | Pas de surcoût |
| Collections hétérogènes | `vector<unique_ptr<Base>>` | ❌ Pas directement |
| Ajout d'un type | Pas de recompilation | Recompilation |
| Optimisations compilateur | Limitées (opacité du vtable) | Complètes (inlining, constexpr) |

Le CRTP excelle dans les bibliothèques où les types sont connus à la compilation et les opérations sont appelées des millions de fois dans des boucles critiques — algèbre linéaire, traitement de signal, moteurs de jeu, parsers.

---

## Cas d'usage 2 : Injection de fonctionnalités (Mixin Pattern)

Le CRTP permet d'injecter des **fonctionnalités génériques** dans une classe dérivée, en s'appuyant sur les méthodes que la dérivée fournit. C'est le *mixin pattern* : la classe de base ajoute des capacités sans que la classe dérivée n'ait besoin de les implémenter.

### Exemple : opérateurs de comparaison automatiques

Avant C++20 et l'opérateur spaceship `<=>` (section 8.5), le CRTP était la manière idiomatique de générer automatiquement les opérateurs de comparaison à partir d'un seul opérateur `<` :

```cpp
template<typename Derived>  
class Comparable {  
public:  
    friend bool operator>(const Derived& a, const Derived& b) {
        return b < a;
    }
    friend bool operator<=(const Derived& a, const Derived& b) {
        return !(b < a);
    }
    friend bool operator>=(const Derived& a, const Derived& b) {
        return !(a < b);
    }
    friend bool operator==(const Derived& a, const Derived& b) {
        return !(a < b) && !(b < a);
    }
    friend bool operator!=(const Derived& a, const Derived& b) {
        return (a < b) || (b < a);
    }
};

class Temperature : public Comparable<Temperature> {
    double celsius_;
public:
    explicit Temperature(double c) : celsius_(c) {}

    // Seul operator< est implémenté — les autres sont injectés par CRTP
    friend bool operator<(const Temperature& a, const Temperature& b) {
        return a.celsius_ < b.celsius_;
    }
};
```

```cpp
auto t1 = Temperature(20.0);  
auto t2 = Temperature(25.0);  

// Tous ces opérateurs fonctionnent grâce au CRTP
bool r1 = t1 < t2;   // true — implémenté manuellement  
bool r2 = t1 > t2;   // false — généré par Comparable  
bool r3 = t1 <= t2;  // true — généré par Comparable  
bool r4 = t1 == t2;  // false — généré par Comparable  
```

> **Note historique** : depuis C++20, l'opérateur spaceship `<=>` remplace avantageusement ce pattern pour les comparaisons. Mais le principe du mixin CRTP reste pertinent pour d'autres fonctionnalités.

### Exemple : sérialisation automatique

Un mixin CRTP qui ajoute la capacité de sérialisation JSON à n'importe quelle classe, à condition qu'elle fournisse une méthode `to_json_fields()` :

```cpp
template<typename Derived>  
class JsonSerializable {  
public:  
    std::string to_json() const {
        auto fields = static_cast<const Derived&>(*this).to_json_fields();
        nlohmann::json j;
        for (const auto& [key, value] : fields) {
            j[key] = value;
        }
        return j.dump();
    }

    void save_to_file(const std::filesystem::path& path) const {
        std::ofstream out(path);
        out << to_json();
    }
};

class ServerConfig : public JsonSerializable<ServerConfig> {
    std::string host_ = "0.0.0.0";
    uint16_t    port_ = 8080;
    int         threads_ = 4;
public:
    // Le CRTP injecte to_json() et save_to_file()
    // La classe ne fournit que les champs
    auto to_json_fields() const {
        return std::vector<std::pair<std::string, nlohmann::json>>{
            {"host", host_},
            {"port", port_},
            {"threads", threads_}
        };
    }
};
```

```cpp
ServerConfig config;  
config.save_to_file("/tmp/config.json");  
// Écrit : {"host":"0.0.0.0","port":8080,"threads":4}
```

La classe dérivée se concentre sur ses données ; le CRTP injecte les comportements transversaux.

### Héritage multiple de mixins

Le CRTP permet de **combiner** plusieurs mixins indépendants, chacun injectant une fonctionnalité orthogonale :

```cpp
template<typename Derived>  
class Printable {  
public:  
    void print() const {
        std::print("{}\n", static_cast<const Derived&>(*this).to_string());
    }
};

template<typename Derived>  
class Cloneable {  
public:  
    std::unique_ptr<Derived> clone() const {
        return std::make_unique<Derived>(
            static_cast<const Derived&>(*this)
        );
    }
};

// Composition de mixins
class Sensor : public Printable<Sensor>,
               public Cloneable<Sensor>,
               public JsonSerializable<Sensor> {
    std::string name_;
    double      value_;
public:
    Sensor(std::string name, double val)
        : name_(std::move(name)), value_(val) {}

    std::string to_string() const {
        return std::format("{}={:.2f}", name_, value_);
    }

    auto to_json_fields() const {
        return std::vector<std::pair<std::string, nlohmann::json>>{
            {"name", name_}, {"value", value_}
        };
    }
};
```

```cpp
auto sensor = Sensor("temperature", 23.5);  
sensor.print();                          // Printable → "temperature=23.50"  
auto copy = sensor.clone();              // Cloneable → copie indépendante  
sensor.save_to_file("/tmp/sensor.json"); // JsonSerializable → fichier JSON  
```

Chaque mixin est indépendant, testable isolément, et composable librement. C'est la **composition par héritage template** — plus flexible que l'héritage classique car il n'y a aucun couplage entre les mixins.

---

## Cas d'usage 3 : Compteur d'instances

Un pattern classique : compter le nombre d'instances vivantes d'un type donné, utile pour le monitoring et la détection de fuites logiques :

```cpp
template<typename Derived>  
class InstanceCounter {  
public:  
    static int live_count() { return count_; }

protected:
    InstanceCounter()  { ++count_; }
    ~InstanceCounter() { --count_; }

    // Copie et déplacement : une nouvelle instance est une nouvelle instance
    InstanceCounter(const InstanceCounter&)            { ++count_; }
    InstanceCounter& operator=(const InstanceCounter&) { return *this; }  // Pas de changement
    InstanceCounter(InstanceCounter&&)                 { ++count_; }
    InstanceCounter& operator=(InstanceCounter&&)      { return *this; }

private:
    static inline int count_ = 0;  // inline (C++17) : une seule instance
};

class Connection : public InstanceCounter<Connection> {
    // ...
};

class Request : public InstanceCounter<Request> {
    // ...
};
```

```cpp
{
    Connection c1, c2, c3;
    std::print("Connections : {}\n", Connection::live_count());  // 3
    std::print("Requests : {}\n", Request::live_count());        // 0

    Request r1;
    std::print("Requests : {}\n", Request::live_count());        // 1
}
std::print("Connections : {}\n", Connection::live_count());      // 0
```

Sans le CRTP, un seul compteur serait partagé entre tous les types dérivés. Le paramètre template `Derived` garantit que **chaque type a son propre compteur statique** — `InstanceCounter<Connection>::count_` et `InstanceCounter<Request>::count_` sont deux variables distinctes.

---

## Garde contre les abus : empêcher le mauvais type

Le piège le plus dangereux du CRTP est de passer un type incorrect au template :

```cpp
class A : public Shape<B> {};  // ❌ A dit être B — comportement indéfini
```

En C++20, un `static_assert` dans le constructeur de la base verrouille cette erreur :

```cpp
template<typename Derived>  
class Shape {  
protected:  
    Shape() {
        static_assert(std::is_base_of_v<Shape<Derived>, Derived>,
                      "Le paramètre CRTP doit être la classe dérivée elle-même");
    }
};
```

Une protection plus complète utilise un constructeur privé accessible uniquement par la classe dérivée via `friend` :

```cpp
template<typename Derived>  
class CRTPBase {  
    // Constructeur privé → seul Derived peut construire
    CRTPBase() = default;
    friend Derived;

protected:
    // Helpers d'accès sécurisés
    const Derived& self() const { return static_cast<const Derived&>(*this); }
    Derived&       self()       { return static_cast<Derived&>(*this); }
};
```

Avec ce design, `class Evil : public CRTPBase<SomeOtherClass>` ne compile pas car `Evil` n'est pas `friend` de `CRTPBase<SomeOtherClass>` — seul `SomeOtherClass` l'est.

---

## Limitations du CRTP classique

Malgré sa puissance, le CRTP souffre de défauts structurels qu'il faut connaître.

### Pas de collections hétérogènes

`Shape<Circle>` et `Shape<Rectangle>` sont **deux types complètement distincts**. Ils ne partagent pas de classe de base commune. Impossible de les stocker dans un même `std::vector` :

```cpp
// ❌ Ne compile pas — types incompatibles
std::vector<Shape<???>> shapes;
```

Pour les collections hétérogènes, il faut combiner le CRTP avec du type erasure (section 44.4) ou utiliser le polymorphisme dynamique classique.

### Lisibilité et complexité

La syntaxe `class Derived : public Base<Derived>` est déroutante à la première lecture. Dans un code avec plusieurs niveaux de CRTP et des mixins composés, la compréhension peut devenir difficile. Le code CRTP doit être documenté avec soin.

### Verbosité du `static_cast`

Chaque accès à la classe dérivée nécessite un `static_cast<const Derived&>(*this)`. Le helper `self()` atténue le problème mais reste un pattern à répéter dans chaque base CRTP.

### Messages d'erreur

Quand l'interface requise par le CRTP n'est pas respectée (méthode `area_impl()` manquante, mauvaise signature), les messages d'erreur du compilateur sont souvent cryptiques, pointant vers l'intérieur du template plutôt que vers la classe dérivée fautive. Les concepts C++20 améliorent considérablement ce point (voir plus bas).

---

## CRTP + Concepts (C++20) : messages d'erreur clairs

Les concepts permettent d'exprimer explicitement les exigences du CRTP sur la classe dérivée. Le compilateur produit alors des messages d'erreur ciblés au lieu de cascades d'erreurs dans les templates :

```cpp
// Concept : ce que la classe dérivée doit fournir
template<typename T>  
concept HasAreaImpl = requires(const T& t) {  
    { t.area_impl() } -> std::convertible_to<double>;
};

template<HasAreaImpl Derived>  
class Shape {  
public:  
    double area() const {
        return self().area_impl();
    }

    void describe() const {
        std::print("Forme d'aire {:.2f}\n", area());
    }

private:
    const Derived& self() const {
        return static_cast<const Derived&>(*this);
    }
};
```

Si une classe dérivée oublie `area_impl()` :

```cpp
class Broken : public Shape<Broken> {};  // ❌ Erreur claire du compilateur
```

Le compilateur indique que `Broken` ne satisfait pas le concept `HasAreaImpl` parce que `area_impl()` est absente — au lieu d'un mur d'erreurs dans les entrailles de `Shape`.

---

## `deducing this` (C++23) : l'évolution majeure

C++23 introduit les **explicit object parameters** (communément appelés *deducing this*), qui permettent à une méthode de recevoir son propre objet `this` comme paramètre explicite dont le type est déduit. Cette fonctionnalité résout plusieurs cas d'usage du CRTP de manière plus élégante.

### Syntaxe de base

```cpp
struct Base {
    // 'this' est un paramètre explicite — son type est déduit
    template<typename Self>
    void describe(this const Self& self) {
        std::print("Aire : {:.2f}\n", self.area());
    }
};

struct Circle : Base {
    double radius;
    double area() const { return std::numbers::pi * radius * radius; }
};

struct Rectangle : Base {
    double width, height;
    double area() const { return width * height; }
};
```

```cpp
auto c = Circle{{}, 5.0};       // {} initialise la base Base (vide)  
c.describe();  // Self est déduit comme Circle → appelle Circle::area()  

auto r = Rectangle{{}, 3.0, 4.0};  
r.describe();  // Self est déduit comme Rectangle → appelle Rectangle::area()  
```

### Ce qui change par rapport au CRTP

Le `deducing this` élimine trois irritants du CRTP classique :

**Plus de paramètre template récursif :**

```cpp
// CRTP classique
class Circle : public Shape<Circle> { /* ... */ };

// Deducing this — héritage simple, non paramétré
class Circle : public Shape { /* ... */ };
```

**Plus de `static_cast` :**

```cpp
// CRTP classique
const Derived& self() const {
    return static_cast<const Derived&>(*this);
}

// Deducing this — le type est déjà le bon
void method(this const auto& self) {
    self.derived_method();  // Appel direct, pas de cast
}
```

**Plus de risque de passer le mauvais type :**

```cpp
// CRTP classique — erreur silencieuse possible
class Evil : public Shape<WrongType> {};

// Deducing this — le type est déduit, pas spécifié
class Evil : public Shape {};  // Self sera toujours Evil — correct par construction
```

### Mixin avec `deducing this`

Le pattern mixin devient plus naturel :

```cpp
struct Printable {
    template<typename Self>
    void print(this const Self& self) {
        std::print("{}\n", self.to_string());
    }
};

struct Cloneable {
    template<typename Self>
    auto clone(this const Self& self) {
        return std::make_unique<Self>(self);
    }
};

// Héritage simple, non paramétré
struct Sensor : Printable, Cloneable {
    std::string name;
    double value;

    std::string to_string() const {
        return std::format("{}={:.2f}", name, value);
    }
};
```

```cpp
auto s = Sensor{"temp", 22.5};  
s.print();              // Printable::print() déduit Self = Sensor  
auto copy = s.clone();  // Cloneable::clone() déduit Self = Sensor  
```

Comparé au CRTP :

```cpp
// CRTP classique — verbose
struct Sensor : Printable<Sensor>, Cloneable<Sensor> { /* ... */ };

// Deducing this — propre
struct Sensor : Printable, Cloneable { /* ... */ };
```

### Chaînage fluent avec `deducing this`

Un cas d'usage élégant : le chaînage de méthodes dans une hiérarchie. Avec le CRTP classique, un Builder hérité doit retourner `Derived&` pour que le chaînage fonctionne avec le type dérivé. Avec `deducing this`, c'est automatique :

```cpp
struct WidgetBuilder {
    std::string name_;
    int width_ = 100, height_ = 100;

    template<typename Self>
    Self& set_name(this Self& self, std::string name) {
        self.name_ = std::move(name);
        return self;
    }

    template<typename Self>
    Self& set_size(this Self& self, int w, int h) {
        self.width_ = w;
        self.height_ = h;
        return self;
    }
};

struct ButtonBuilder : WidgetBuilder {
    std::string label_;

    template<typename Self>
    Self& set_label(this Self& self, std::string label) {
        self.label_ = std::move(label);
        return self;
    }
};
```

```cpp
// Le chaînage fonctionne sans perdre le type dérivé
auto builder = ButtonBuilder{}
    .set_name("ok_btn")      // Retourne ButtonBuilder& (pas WidgetBuilder&)
    .set_size(200, 50)        // Toujours ButtonBuilder&
    .set_label("OK");         // ButtonBuilder&
```

Sans `deducing this`, `set_name()` retournerait `WidgetBuilder&` et l'appel à `set_label()` échouerait car `WidgetBuilder` n'a pas de `set_label()`. Le CRTP classique pouvait résoudre ce problème, mais au prix d'une complexité template significative.

### Limites de `deducing this`

Le `deducing this` ne remplace pas le CRTP dans **tous** les cas :

**Variables statiques par type dérivé.** Le compteur d'instances (présenté plus haut) repose sur `static inline int count_` dans `Base<Derived>`. Chaque instanciation du template produit une variable distincte. Avec `deducing this`, la base n'est plus paramétrée — il n'y a qu'une seule classe de base et donc qu'une seule variable statique partagée. Le CRTP reste nécessaire pour ce cas d'usage.

**Spécialisation par type dérivé.** Si la base doit avoir un comportement différent (spécialisation template) selon le type dérivé, le CRTP avec ses instanciations distinctes est irremplaçable.

**Support compilateur (mars 2026).** Le `deducing this` est supporté par GCC 14+, Clang 18+ et MSVC 19.37+. Le support est mature pour les cas simples, mais certaines interactions avec les concepts ou les templates variadiques peuvent encore présenter des bugs edge-case selon le compilateur. Vérifier sur Compiler Explorer (godbolt.org) en cas de doute.

---

## CRTP vs `deducing this` : guide de décision

| Besoin | CRTP classique | `deducing this` (C++23) |
|---|---|---|
| Polymorphisme statique (dispatch) | ✅ | ✅ Plus simple |
| Mixins / injection de fonctionnalités | ✅ | ✅ Plus lisible |
| Chaînage fluent dans une hiérarchie | ✅ Verbose | ✅ Élégant |
| Variables statiques par type dérivé | ✅ Nécessaire | ❌ Impossible |
| Spécialisation template par dérivé | ✅ Nécessaire | ❌ Impossible |
| Lisibilité | ⚠️ Déroutant | ✅ Naturel |
| Compatibilité (pré-C++23) | ✅ C++98+ | ❌ C++23 requis |

**Recommandation** : sur un projet en C++23 ou supérieur, préférer `deducing this` pour le polymorphisme statique et les mixins. Conserver le CRTP pour les cas nécessitant des données statiques par type ou une spécialisation template.

---

## Points clés à retenir

- Le CRTP permet le **polymorphisme statique** : dispatch à la compilation, zéro overhead, inlining complet. Il remplace les fonctions virtuelles quand les types sont connus à la compilation.  
- Le **mixin pattern** via CRTP injecte des fonctionnalités (sérialisation, comparaison, clonage, comptage d'instances) dans les classes dérivées par composition d'héritages template.  
- Le helper `self()` encapsule le `static_cast` et améliore la lisibilité. Le constructeur privé + `friend Derived` empêche les abus.  
- Les **concepts C++20** contraignent les exigences du CRTP sur la classe dérivée et produisent des messages d'erreur exploitables.  
- **`deducing this` (C++23)** simplifie radicalement la majorité des cas d'usage du CRTP : plus de paramètre template récursif, plus de `static_cast`, plus de risque de mauvais type. Il est préférable au CRTP classique pour le polymorphisme statique et les mixins.  
- Le CRTP reste **irremplaçable** pour les variables statiques par type dérivé et la spécialisation template — deux cas que `deducing this` ne couvre pas.  
- Le CRTP ne permet **pas** les collections hétérogènes. Pour combiner polymorphisme statique et collections runtime, le type erasure (section 44.4) est la réponse.

⏭️ [Type erasure et std::any](/44-design-patterns/04-type-erasure.md)
