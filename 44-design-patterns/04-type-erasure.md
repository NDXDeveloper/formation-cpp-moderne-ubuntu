🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 44.4 — Type Erasure et `std::any`

## Réconcilier polymorphisme statique et dynamique

---

## Définition et intention

Le type erasure (*effacement de type*) est une technique qui permet de manipuler des objets de **types différents à travers une interface uniforme**, sans imposer de relation d'héritage entre ces types. L'information sur le type concret est "effacée" derrière une façade non-template, tout en préservant le comportement spécifique de chaque type sous le capot.

C'est la réponse à une limitation fondamentale du C++ : les templates offrent un polymorphisme puissant mais entièrement statique (les types doivent être connus à la compilation), tandis que l'héritage offre un polymorphisme dynamique mais impose un couplage via une classe de base commune. Le type erasure combine les avantages des deux : **interface uniforme à l'exécution, aucune hiérarchie imposée aux types concrets**.

Si cette description semble abstraite, il y a de bonnes chances que vous utilisiez déjà du type erasure au quotidien sans le savoir. `std::function`, `std::any`, `std::format` et les itérateurs de ranges en sont des exemples dans la bibliothèque standard.

---

## Le type erasure dans la bibliothèque standard

Avant d'implémenter notre propre type erasure, observons ceux que le standard fournit. Ils illustrent trois niveaux de "spécificité" dans l'effacement de type.

### `std::any` : effacement total

`std::any` peut contenir **n'importe quelle valeur** de n'importe quel type copiable. L'interface ne fait aucune hypothèse sur le type stocké — il faut explicitement extraire la valeur avec `std::any_cast` en spécifiant le type attendu :

```cpp
#include <any>

std::any value = 42;                          // Contient un int  
value = std::string("hello");                 // Maintenant un string  
value = std::vector<double>{1.0, 2.0, 3.0};  // Maintenant un vector  

// Extraction — il faut connaître le type
auto& vec = std::any_cast<std::vector<double>&>(value);  
std::print("Taille : {}\n", vec.size());  // 3  

// Mauvais type → exception std::bad_any_cast
try {
    auto n = std::any_cast<int>(value);
} catch (const std::bad_any_cast& e) {
    std::print(stderr, "Erreur : {}\n", e.what());
}

// Vérification du type avant extraction
if (value.type() == typeid(std::vector<double>)) {
    // Safe
}
```

`std::any` est l'effacement le plus radical : il ne conserve **aucune interface** sur le type stocké. C'est un conteneur opaque. Il est utile quand on doit stocker des valeurs hétérogènes sans connaître leur type à l'avance (propriétés dynamiques, configurations génériques, systèmes de plugins), mais il offre peu de sécurité — l'extraction repose sur la connaissance du type exact à l'exécution.

### `std::function` : effacement avec interface callable

`std::function<R(Args...)>` peut contenir **n'importe quel callable** dont la signature est compatible — lambda, pointeur de fonction, functor, méthode liée. Le type concret est effacé, mais l'interface est préservée : on peut appeler l'objet stocké via `operator()` :

```cpp
#include <functional>

// Trois types complètement différents...
auto lambda  = [](int x) { return x * 2; };  
int  free_fn(int x) { return x + 10; }  

struct Multiplier {
    int factor;
    int operator()(int x) const { return x * factor; }
};

// ... stockés dans le même type
std::function<int(int)> fn;

fn = lambda;  
std::print("{}\n", fn(5));    // 10  

fn = free_fn;  
std::print("{}\n", fn(5));    // 15  

fn = Multiplier{3};  
std::print("{}\n", fn(5));    // 15  
```

`std::function` est un type erasure **ciblé** : il efface le type concret mais préserve la capacité d'appel. C'est le modèle le plus courant de type erasure en pratique.

### `std::variant` : pas du type erasure

Il est important de distinguer `std::variant` du type erasure. Un `std::variant<int, string, double>` connaît à la compilation la **liste exhaustive** des types qu'il peut contenir. Il n'y a pas d'effacement : le type est toujours là, encodé dans le discriminant du variant. Le code client traite chaque cas via `std::visit`. C'est du polymorphisme fermé, pas du type erasure.

Le type erasure, à l'inverse, accepte **n'importe quel type** satisfaisant un ensemble de contraintes (une interface), sans que ces types soient listés à l'avance.

---

## Pourquoi implémenter son propre type erasure ?

`std::any` est trop générique (aucune interface) et `std::function` est limité aux callables. Entre les deux, il existe un vaste espace de besoins : stocker dans une même collection des objets qui partagent une **interface spécifique** (dessiner, sérialiser, compresser…) sans leur imposer d'hériter d'une classe de base commune.

Scénarios concrets :

- Un `std::vector<Drawable>` contenant des cercles, rectangles et textes qui n'ont aucun ancêtre commun mais qui implémentent tous une méthode `draw()`.  
- Un pipeline de transformations où chaque étape a un type différent mais expose `transform(Data&)`.  
- Un système d'événements où les handlers sont des objets métier hétérogènes exposant `handle(const Event&)`.

Le type erasure custom répond à ces besoins.

---

## Anatomie d'un type erasure custom

L'implémentation repose sur trois composants :

1. **Un concept (ou contrainte)** définissant l'interface requise.
2. **Une classe de base abstraite interne** (le *concept holder*) exposant cette interface via des fonctions virtuelles.
3. **Un template interne** (le *model*) qui enveloppe le type concret et implémente les fonctions virtuelles en déléguant au type enveloppé.
4. **Une classe externe non-template** (le *wrapper*) qui possède un pointeur vers le concept holder et expose l'interface publique.

Le terme consacré pour cette architecture est **concept-model idiom** (à ne pas confondre avec les `concept` C++20, bien que les deux se combinent naturellement).

### Implémentation pas à pas : `AnyDrawable`

Commençons par un type erasure pour des objets dessinables :

```cpp
#include <memory>
#include <string>
#include <print>

class AnyDrawable {  
public:  
    // ─── Construction depuis n'importe quel type "dessinable" ───

    template<typename T>
    AnyDrawable(T obj)
        : pimpl_(std::make_unique<Model<T>>(std::move(obj)))
    {}

    // ─── Copie (deep copy via clone) ───

    AnyDrawable(const AnyDrawable& other)
        : pimpl_(other.pimpl_ ? other.pimpl_->clone() : nullptr)
    {}

    AnyDrawable& operator=(const AnyDrawable& other) {
        if (this != &other) {
            pimpl_ = other.pimpl_ ? other.pimpl_->clone() : nullptr;
        }
        return *this;
    }

    // ─── Déplacement ───

    AnyDrawable(AnyDrawable&&) noexcept = default;
    AnyDrawable& operator=(AnyDrawable&&) noexcept = default;

    // ─── Interface publique (non-template) ───

    void draw() const {
        pimpl_->draw();
    }

    std::string describe() const {
        return pimpl_->describe();
    }

private:
    // ─── Concept : interface abstraite interne ───

    struct Concept {
        virtual ~Concept() = default;
        virtual void draw() const = 0;
        virtual std::string describe() const = 0;
        virtual std::unique_ptr<Concept> clone() const = 0;
    };

    // ─── Model : enveloppe template qui délègue au type concret ───

    template<typename T>
    struct Model final : Concept {
        T obj_;

        explicit Model(T obj) : obj_(std::move(obj)) {}

        void draw() const override {
            obj_.draw();
        }

        std::string describe() const override {
            return obj_.describe();
        }

        std::unique_ptr<Concept> clone() const override {
            return std::make_unique<Model>(obj_);
        }
    };

    std::unique_ptr<Concept> pimpl_;
};
```

### Types concrets — aucun héritage requis

```cpp
struct Circle {
    double x, y, radius;

    void draw() const {
        std::print("Drawing circle at ({:.1f}, {:.1f}) r={:.1f}\n", x, y, radius);
    }

    std::string describe() const {
        return std::format("Circle(r={:.1f})", radius);
    }
};

struct Rectangle {
    double x, y, width, height;

    void draw() const {
        std::print("Drawing rect at ({:.1f}, {:.1f}) {}x{}\n",
                   x, y, width, height);
    }

    std::string describe() const {
        return std::format("Rect({}x{})", width, height);
    }
};

struct Text {
    double x, y;
    std::string content;

    void draw() const {
        std::print("Drawing '{}' at ({:.1f}, {:.1f})\n", content, x, y);
    }

    std::string describe() const {
        return std::format("Text('{}')", content);
    }
};
```

### Utilisation : collection hétérogène sans héritage

```cpp
// Trois types sans aucun ancêtre commun, dans un même vector
std::vector<AnyDrawable> scene;

scene.emplace_back(Circle{100, 100, 50});  
scene.emplace_back(Rectangle{200, 150, 80, 40});  
scene.emplace_back(Text{300, 200, "Hello"});  

// Interface uniforme
for (const auto& drawable : scene) {
    drawable.draw();
    std::print("  → {}\n", drawable.describe());
}
```

Sortie :

```
Drawing circle at (100.0, 100.0) r=50.0
  → Circle(r=50.0)
Drawing rect at (200.0, 150.0) 80x40
  → Rect(80x40)
Drawing 'Hello' at (300.0, 200.0)
  → Text('Hello')
```

`Circle`, `Rectangle` et `Text` n'héritent de rien. Ils n'ont même pas besoin de connaître l'existence de `AnyDrawable`. Le seul contrat est structurel : ils doivent exposer `draw()` et `describe()` avec les bonnes signatures.

---

## Pourquoi pas simplement `std::unique_ptr<Interface>` ?

La question légitime : pourquoi cette mécanique complexe plutôt qu'une interface virtuelle classique ?

```cpp
// Approche classique — impose l'héritage
class Drawable {  
public:  
    virtual ~Drawable() = default;
    virtual void draw() const = 0;
};

class Circle : public Drawable { /* ... */ };
```

Les différences sont fondamentales :

| Critère | `unique_ptr<Interface>` | Type erasure |
|---|---|---|
| Couplage | Fort : chaque type **doit** hériter de l'interface | Aucun : le type n'a aucune connaissance du wrapper |
| Types tiers | Impossible d'adapter un type de bibliothèque externe | ✅ Fonctionne avec n'importe quel type compatible |
| Sémantique de valeur | Sémantique de pointeur (heap, indirection) | Sémantique de valeur (copie, déplacement) |
| Composition | Héritage simple ou multiple | Structurelle (duck typing) |
| Intrusion | Modifie la définition du type | Non-intrusif |

Le point critique est le **caractère non-intrusif**. Si vous utilisez une bibliothèque tierce dont les types exposent `draw()` mais n'héritent pas de votre `Drawable`, le type erasure les accepte tels quels. L'héritage exige que vous contrôliez la définition du type — ce qui n'est pas toujours possible.

---

## Type erasure + Concepts C++20 : contraintes explicites

Sans contrainte, le constructeur template de `AnyDrawable` accepte n'importe quel type. L'erreur n'apparaît qu'à l'instanciation du `Model`, profondément dans le template — avec un message cryptique. Les concepts C++20 rendent le contrat explicite :

```cpp
template<typename T>  
concept Drawable = requires(const T& obj) {  
    { obj.draw() } -> std::same_as<void>;
    { obj.describe() } -> std::convertible_to<std::string>;
};

class AnyDrawable {  
public:  
    // Le concept contraint le constructeur
    template<Drawable T>
    AnyDrawable(T obj)
        : pimpl_(std::make_unique<Model<T>>(std::move(obj)))
    {}

    // ... reste identique
};
```

Désormais, passer un type qui n'expose pas `draw()` ou `describe()` produit une erreur claire au site d'appel :

```cpp
struct NotDrawable { int x; };

AnyDrawable d = NotDrawable{42};  
// ❌ Erreur : NotDrawable ne satisfait pas le concept Drawable
//    note: 'draw()' n'est pas trouvé
```

---

## Sémantique de valeur : copie et déplacement

Un aspect crucial du type erasure est qu'il donne une **sémantique de valeur** à des objets polymorphiques. Contrairement à `unique_ptr<Base>` qui impose une sémantique de pointeur (pas de copie, indirection obligatoire), un type erasure bien conçu se copie, se déplace et se stocke comme un `int` ou un `std::string` :

```cpp
AnyDrawable a = Circle{0, 0, 10};  
AnyDrawable b = a;                     // Deep copy — b contient un Circle indépendant  

b.draw();                               // Fonctionne — b a sa propre copie

std::vector<AnyDrawable> scene;  
scene.push_back(a);                    // Copie dans le vector  
scene.push_back(std::move(a));         // Déplacement — a est vidé  
```

Cette sémantique est rendue possible par la méthode `clone()` dans le `Concept` interne, qui effectue une copie polymorphique (deep copy du type concret enveloppé).

### Type erasure move-only

Si la copie n'est pas nécessaire (ou si les types enveloppés ne sont pas copiables), on peut supprimer la copie et ne garder que le déplacement :

```cpp
class AnyProcessor {  
public:  
    template<typename T>
    AnyProcessor(T obj)
        : pimpl_(std::make_unique<Model<T>>(std::move(obj)))
    {}

    // Move only — pas de clone nécessaire
    AnyProcessor(AnyProcessor&&) noexcept = default;
    AnyProcessor& operator=(AnyProcessor&&) noexcept = default;

    AnyProcessor(const AnyProcessor&)            = delete;
    AnyProcessor& operator=(const AnyProcessor&) = delete;

    void process(std::span<const std::byte> data) const {
        pimpl_->process(data);
    }

private:
    struct Concept {
        virtual ~Concept() = default;
        virtual void process(std::span<const std::byte> data) const = 0;
        // Pas de clone() — copie supprimée
    };

    template<typename T>
    struct Model final : Concept {
        T obj_;
        explicit Model(T obj) : obj_(std::move(obj)) {}
        void process(std::span<const std::byte> data) const override {
            obj_.process(data);
        }
    };

    std::unique_ptr<Concept> pimpl_;
};
```

C'est plus simple et plus efficace. En pratique, la version move-only est suffisante dans la majorité des cas (pipelines de traitement, handlers d'événements, commandes).

---

## Small Buffer Optimization (SBO)

L'implémentation avec `std::unique_ptr` implique une **allocation heap** à chaque construction du wrapper. Pour les petits types (quelques dizaines d'octets), cette allocation est un coût disproportionné. La *Small Buffer Optimization* (SBO) stocke les petits objets **inline** dans le wrapper, évitant le heap :

```cpp
class AnyDrawable {
    static constexpr std::size_t BufferSize  = 64;
    static constexpr std::size_t BufferAlign = alignof(std::max_align_t);

    // Buffer interne pour les petits objets
    alignas(BufferAlign) std::byte buffer_[BufferSize];
    bool on_heap_ = false;

    struct Concept { /* ... comme avant ... */ };

    template<typename T>
    struct Model final : Concept { /* ... comme avant ... */ };

    Concept* ptr() {
        return on_heap_
            ? reinterpret_cast<Concept*>(*reinterpret_cast<Concept**>(buffer_))
            : reinterpret_cast<Concept*>(buffer_);
    }

public:
    template<Drawable T>
    AnyDrawable(T obj) {
        if constexpr (sizeof(Model<T>) <= BufferSize
                   && alignof(Model<T>) <= BufferAlign
                   && std::is_nothrow_move_constructible_v<T>) {
            // Placement new dans le buffer interne
            ::new (buffer_) Model<T>(std::move(obj));
            on_heap_ = false;
        } else {
            // Trop gros → heap
            auto p = std::make_unique<Model<T>>(std::move(obj));
            ::new (buffer_) std::unique_ptr<Concept>(std::move(p));
            on_heap_ = true;
        }
    }

    // ... destructeur, move, copie doivent gérer les deux cas
};
```

C'est exactement le mécanisme que `std::function` utilise en interne. Les petites lambdas (quelques captures) sont stockées inline ; les grosses sont allouées sur le heap.

> **En pratique** : implémenter la SBO manuellement est complexe et source de bugs (alignement, exception safety, destructeur conditionnel). Si les performances le justifient, préférer utiliser une bibliothèque éprouvée comme `folly::Function` (Facebook) ou `inplace_function` plutôt que de réimplémenter le mécanisme.

---

## Exemple complet : pipeline de transformations

Un cas d'usage réaliste combinant type erasure, move semantics et composition :

```cpp
template<typename T>  
concept DataTransform = requires(const T& t, std::vector<std::byte>& data) {  
    { t.transform(data) } -> std::same_as<void>;
    { t.name() } -> std::convertible_to<std::string_view>;
};

class AnyTransform {  
public:  
    template<DataTransform T>
    AnyTransform(T obj)
        : pimpl_(std::make_unique<Model<T>>(std::move(obj)))
    {}

    AnyTransform(AnyTransform&&) noexcept = default;
    AnyTransform& operator=(AnyTransform&&) noexcept = default;

    void transform(std::vector<std::byte>& data) const {
        pimpl_->transform(data);
    }

    std::string_view name() const {
        return pimpl_->name();
    }

private:
    struct Concept {
        virtual ~Concept() = default;
        virtual void transform(std::vector<std::byte>& data) const = 0;
        virtual std::string_view name() const = 0;
    };

    template<typename T>
    struct Model final : Concept {
        T obj_;
        explicit Model(T obj) : obj_(std::move(obj)) {}
        void transform(std::vector<std::byte>& data) const override {
            obj_.transform(data);
        }
        std::string_view name() const override { return obj_.name(); }
    };

    std::unique_ptr<Concept> pimpl_;
};
```

Types concrets indépendants — aucun héritage :

```cpp
struct CompressStep {
    std::string algorithm;

    void transform(std::vector<std::byte>& data) const {
        // Compression...
        std::print("  [compress/{}] {} bytes\n", algorithm, data.size());
    }
    std::string_view name() const { return "compress"; }
};

struct EncryptStep {
    std::string cipher;

    void transform(std::vector<std::byte>& data) const {
        // Chiffrement...
        std::print("  [encrypt/{}] {} bytes\n", cipher, data.size());
    }
    std::string_view name() const { return "encrypt"; }
};

struct Base64EncodeStep {
    void transform(std::vector<std::byte>& data) const {
        // Encodage base64...
        std::print("  [base64] {} bytes\n", data.size());
    }
    std::string_view name() const { return "base64"; }
};
```

Pipeline composé dynamiquement :

```cpp
class Pipeline {  
public:  
    template<DataTransform T>
    Pipeline& add(T step) {
        steps_.emplace_back(std::move(step));
        return *this;
    }

    void execute(std::vector<std::byte>& data) const {
        std::print("Pipeline ({} étapes) :\n", steps_.size());
        for (const auto& step : steps_) {
            step.transform(data);
        }
        std::print("Pipeline terminé.\n");
    }

private:
    std::vector<AnyTransform> steps_;
};
```

```cpp
auto pipeline = Pipeline();  
pipeline.add(CompressStep{"zstd"})  
        .add(EncryptStep{"aes-256-gcm"})
        .add(Base64EncodeStep{});

std::vector<std::byte> data(1024);  
pipeline.execute(data);  
```

Sortie :

```
Pipeline (3 étapes) :
  [compress/zstd] 1024 bytes
  [encrypt/aes-256-gcm] 1024 bytes
  [base64] 1024 bytes
Pipeline terminé.
```

Les étapes du pipeline sont des types complètement indépendants, assemblés dynamiquement. Ajouter une nouvelle étape ne modifie ni `AnyTransform` ni `Pipeline` — il suffit de définir un nouveau type avec `transform()` et `name()`.

---

## `std::any` en pratique : quand l'utiliser

Malgré son manque d'interface, `std::any` a sa place dans des contextes spécifiques.

### Propriétés dynamiques (property map)

Un dictionnaire de propriétés hétérogènes est un cas d'usage naturel :

```cpp
class PropertyBag {  
public:  
    template<typename T>
    void set(const std::string& key, T value) {
        props_[key] = std::move(value);
    }

    template<typename T>
    const T& get(const std::string& key) const {
        return std::any_cast<const T&>(props_.at(key));
    }

    template<typename T>
    std::optional<T> try_get(const std::string& key) const {
        auto it = props_.find(key);
        if (it == props_.end()) return std::nullopt;
        auto* ptr = std::any_cast<T>(&it->second);
        if (!ptr) return std::nullopt;
        return *ptr;
    }

    bool contains(const std::string& key) const {
        return props_.contains(key);
    }

private:
    std::unordered_map<std::string, std::any> props_;
};
```

```cpp
PropertyBag config;  
config.set("host", std::string("0.0.0.0"));  
config.set("port", 8080);  
config.set("debug", true);  
config.set("tags", std::vector<std::string>{"web", "api"});  

auto host = config.get<std::string>("host");    // "0.0.0.0"  
auto port = config.get<int>("port");            // 8080  

// Extraction sûre
if (auto debug = config.try_get<bool>("debug")) {
    std::print("Debug : {}\n", *debug);
}
```

### Quand `std::any` est le mauvais choix

- **Si les types sont connus à la compilation** → `std::variant` est plus sûr (exhaustivité vérifiée, pas de `bad_any_cast`).  
- **Si les objets partagent une interface** → un type erasure custom est plus expressif (on appelle des méthodes sans connaître le type).  
- **Si la performance est critique** → `std::any` alloue sur le heap pour les types dépassant la SBO interne (typiquement 16-32 bytes selon l'implémentation).

`std::any` est un outil de dernier recours quand ni `std::variant` (types connus) ni le type erasure custom (interface commune) ni les templates (résolution compile-time) ne conviennent.

---

## Relations entre les approches

Le spectre des solutions polymorphiques en C++ moderne, du plus contraint au plus ouvert :

```
Plus de sécurité                                    Plus de flexibilité  
compile-time                                              runtime  
    │                                                        │
    ▼                                                        ▼
Templates    std::variant    Type erasure    virtual    std::any
(statique)    (fermé)        (interface)    (héritage)  (opaque)
    │              │              │              │           │
    │              │              │              │           │
  Types          Types        N'importe       Types      N'importe
  connus à      listés à     quel type       héritant    quel type
  la compil.    la compil.   satisfaisant    d'une       copiable
                             un concept      base
```

Chaque approche se situe à un point différent du compromis sécurité/flexibilité :

- **Templates** : sécurité maximale, zéro overhead, mais tout doit être résolu à la compilation.  
- **`std::variant`** : ensemble fermé de types, dispatch statique via `std::visit`, vérification d'exhaustivité.  
- **Type erasure custom** : interface définie, types ouverts et non-intrusifs, sémantique de valeur.  
- **Héritage virtuel** : interface définie, types ouverts mais intrusifs (héritage requis), sémantique de pointeur.  
- **`std::any`** : aucune interface, aucune contrainte — flexibilité maximale mais sécurité minimale.

---

## Anti-patterns et pièges

### Type erasure pour un seul type

Si le wrapper ne contiendra jamais qu'un seul type concret, le type erasure est un overhead inutile. Utiliser directement le type concret ou un template.

### `std::any_cast` sans vérification

```cpp
// ❌ Lance std::bad_any_cast si le type est mauvais
auto value = std::any_cast<int>(some_any);

// ✅ Vérification préalable avec pointeur
if (auto* ptr = std::any_cast<int>(&some_any)) {
    // Utiliser *ptr en toute sécurité
}
```

La version par pointeur retourne `nullptr` au lieu de lancer une exception. C'est la forme à privilégier quand le type n'est pas garanti.

### Oublier `clone()` dans un type erasure copiable

Sans `clone()`, la copie du wrapper copierait le `unique_ptr` — ce qui est interdit. Si la copie est nécessaire, chaque `Model` doit implémenter un `clone()` qui crée une copie polymorphique. Si la copie n'est pas nécessaire, supprimer explicitement le constructeur de copie et documenter que le wrapper est move-only.

### Concept interne trop large

Un `Concept` avec dix méthodes virtuelles est le signe d'une interface trop grosse. Le type erasure fonctionne mieux avec des interfaces **étroites et focalisées** (une à trois méthodes). Pour les interfaces larges, l'héritage classique avec `unique_ptr<Base>` est souvent plus clair.

---

## Points clés à retenir

- Le type erasure permet de manipuler des **types hétérogènes sans héritage commun** à travers une interface uniforme non-template. C'est le pont entre le polymorphisme statique (templates) et dynamique (virtual).  
- L'architecture **Concept-Model** est le cœur du pattern : une interface abstraite interne (`Concept`) et un template d'enveloppe (`Model`) qui délègue au type concret. La classe externe non-template expose l'interface publique.  
- **`std::function`** est un type erasure pour les callables. **`std::any`** est un type erasure sans interface. Un **type erasure custom** offre le meilleur compromis : interface définie, types ouverts, non-intrusif.  
- Les **concepts C++20** contraignent le constructeur template du wrapper et produisent des erreurs claires quand un type ne satisfait pas l'interface requise.  
- La **sémantique de valeur** (copie via `clone()`, déplacement) distingue le type erasure du pattern `unique_ptr<Base>` qui impose une sémantique de pointeur. La version **move-only** (sans `clone()`) est plus simple et souvent suffisante.  
- La **Small Buffer Optimization** évite l'allocation heap pour les petits types, mais sa mise en œuvre manuelle est complexe. Préférer une bibliothèque éprouvée si la performance l'exige.  
- `std::any` est un outil de **dernier recours**. Si les types sont connus → `std::variant`. S'ils partagent une interface → type erasure custom. Si ni l'un ni l'autre → `std::any`.

⏭️ [Dependency Injection en C++](/44-design-patterns/05-dependency-injection.md)
