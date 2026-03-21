🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 44.1 — Singleton, Factory, Builder

## Patterns de Création en C++ Moderne

---

## Introduction

Les patterns de création (*creational patterns*) répondent à une question fondamentale : **comment instancier des objets de manière flexible, contrôlée et maintenable ?** Dans un langage comme C++ où la gestion de la mémoire est explicite et où la distinction entre stack et heap a des conséquences directes sur les performances, cette question est loin d'être triviale.

Le Gang of Four identifiait cinq patterns de création : Singleton, Factory Method, Abstract Factory, Builder et Prototype. En C++ moderne, trois d'entre eux restent particulièrement pertinents et fréquemment rencontrés dans le code système, les outils DevOps et les applications cloud native :

- **Singleton** : garantir qu'une classe n'a qu'une seule instance, accessible globalement.  
- **Factory** (englobant Factory Method et Abstract Factory) : déléguer la création d'objets à une logique dédiée, découplant le code client des types concrets.  
- **Builder** : construire pas à pas des objets complexes ayant de nombreux paramètres optionnels.

Le pattern **Prototype** (clonage d'objets) a perdu de sa pertinence en C++ moderne : la sémantique de copie et de mouvement, combinée aux smart pointers, couvre la majorité de ses cas d'usage sans nécessiter un pattern formel. Nous ne le traiterons donc pas dans cette section.

---

## Ce qui a fondamentalement changé

### L'ère du `new` brut est révolue

Dans les implémentations classiques des patterns de création, le mot-clé `new` était omniprésent. Un Factory retournait un `new ConcreteProduct`, un Singleton stockait un `new` dans un pointeur statique, un Builder accumulait des allocations manuelles. Le code client devait ensuite gérer le `delete` correspondant — source inépuisable de fuites mémoire.

En C++ moderne, la règle est claire (cf. section 9.4) : **`new` et `delete` n'apparaissent jamais dans du code applicatif**. Les patterns de création s'appuient désormais sur :

- `std::make_unique<T>()` pour les créations à propriété exclusive (cas le plus fréquent).  
- `std::make_shared<T>()` lorsque la propriété est partagée.  
- Les variables `static` locales pour le Singleton (thread-safe depuis C++11).  
- La construction sur la stack chaque fois que possible, le heap n'étant utilisé que lorsque le polymorphisme ou la durée de vie l'exigent.

### Le polymorphisme n'impose plus l'héritage

Les patterns de création GoF reposaient massivement sur des hiérarchies de classes : une interface `Product`, des classes concrètes `ConcreteProductA`, `ConcreteProductB`, une factory abstraite… Cette mécanique reste pertinente dans certains contextes (plugins, systèmes extensibles à l'exécution), mais le C++ moderne offre des alternatives plus légères :

- **`std::variant`** remplace avantageusement les hiérarchies fermées (quand l'ensemble des types est connu à la compilation). Un `std::variant<Circle, Rectangle, Triangle>` offre un polymorphisme sans vtable, sans allocation heap, avec un pattern matching via `std::visit`.  
- **Les templates et concepts** permettent un polymorphisme statique où le Factory n'a même pas besoin de retourner un pointeur polymorphique — il peut retourner le type concret directement.  
- **`std::function` et lambdas** permettent d'encapsuler des logiques de création sans définir de classes dédiées.

### Le Builder profite de la sémantique de mouvement

L'API fluent du Builder (chaînage d'appels : `builder.setX(…).setY(…).build()`) bénéficie directement de la move semantics. L'objet construit peut être transféré hors du Builder sans copie, et les membres `std::string`, `std::vector` ou autres types lourds sont déplacés plutôt que copiés. Combiné avec `std::optional` pour les champs facultatifs, le Builder moderne est à la fois expressif et performant.

---

## Factory à base de `std::variant` : un avant-goût

Pour illustrer concrètement la rupture avec l'approche classique, comparons deux implémentations d'un même Factory.

### Approche classique (héritage + `new`)

```cpp
// Hiérarchie classique
class Shape {  
public:  
    virtual ~Shape() = default;
    virtual double area() const = 0;
};

class Circle : public Shape {
    double r_;
public:
    explicit Circle(double r) : r_(r) {}
    double area() const override { return std::numbers::pi * r_ * r_; }
};

class Rectangle : public Shape {
    double w_, h_;
public:
    Rectangle(double w, double h) : w_(w), h_(h) {}
    double area() const override { return w_ * h_; }
};

// Factory classique
std::unique_ptr<Shape> create_shape(std::string_view type, double a, double b) {
    if (type == "circle")    return std::make_unique<Circle>(a);
    if (type == "rectangle") return std::make_unique<Rectangle>(a, b);
    throw std::invalid_argument("Unknown shape type");
}
```

Cette approche fonctionne et reste valide quand la hiérarchie est **ouverte** (nouveaux types ajoutés sans modifier le code existant, plugins, etc.). Mais elle impose une allocation heap systématique et une indirection via vtable.

### Approche moderne (`std::variant`, zéro allocation)

```cpp
// Types valeurs, pas de hiérarchie
struct Circle    { double radius; };  
struct Rectangle { double width, height; };  

using Shape = std::variant<Circle, Rectangle>;

// Factory retournant un variant
Shape create_shape(std::string_view type, double a, double b) {
    if (type == "circle")    return Circle{a};
    if (type == "rectangle") return Rectangle{a, b};
    throw std::invalid_argument("Unknown shape type");
}

// Utilisation via std::visit
double area(const Shape& s) {
    return std::visit([](const auto& shape) -> double {
        using T = std::decay_t<decltype(shape)>;
        if constexpr (std::is_same_v<T, Circle>)
            return std::numbers::pi * shape.radius * shape.radius;
        else if constexpr (std::is_same_v<T, Rectangle>)
            return shape.width * shape.height;
    }, s);
}
```

Ici, aucune allocation heap, aucune vtable, et le compilateur vérifie à la compilation que tous les cas du variant sont traités. L'ajout d'un nouveau type au variant provoque une erreur de compilation dans chaque `std::visit` incomplet — c'est la **sécurité par exhaustivité**.

Le choix entre les deux approches n'est pas dogmatique. Il dépend du contexte :

| Critère | Héritage + `unique_ptr` | `std::variant` |
|---|---|---|
| Ensemble de types | Ouvert (extensible) | Fermé (connu à la compilation) |
| Allocation | Heap (indirection) | Stack ou inline (zéro alloc) |
| Performance | Indirection vtable | Dispatch statique, cache-friendly |
| Ajout d'un type | Transparent pour le code existant | Recompilation nécessaire |
| Cas d'usage typique | Plugins, systèmes extensibles | Types connus, code système/perf |

---

## Anti-patterns à éviter

Avant de plonger dans chaque pattern, identifions les erreurs les plus fréquentes dans leur mise en œuvre :

### Singleton abusif

Le Singleton est probablement le pattern le plus surutilisé. Il est souvent employé comme un déguisement pour des **variables globales**, introduisant un couplage fort, des difficultés de test (mocking impossible) et des problèmes d'ordre d'initialisation entre unités de compilation. La section 44.1.1 détaille les cas où il est véritablement justifié — ils sont plus rares qu'on ne le pense.

### Factory over-engineering

Créer une hiérarchie Abstract Factory / Concrete Factory / Product / Concrete Product pour un système qui ne comptera jamais plus de deux ou trois types concrets est du surengineering. Si les types sont connus à la compilation, un simple `std::variant` ou une fonction template suffit. Le Factory formel se justifie quand le **découplage binaire** est nécessaire (bibliothèques dynamiques, plugins) ou quand l'ensemble des types est réellement ouvert et évolue indépendamment du code client.

### Builder sans valeur ajoutée

Un Builder n'a de sens que si l'objet à construire possède **de nombreux paramètres, dont certains sont optionnels, avec des valeurs par défaut et potentiellement des contraintes de validation**. Pour une classe à deux ou trois paramètres obligatoires, un constructeur classique (éventuellement avec des *named parameters* simulés via des structs d'options) est plus simple et tout aussi lisible.

---

## Plan de la section

Les sous-sections suivantes détaillent chacun des trois patterns :

- **44.1.1 — Singleton thread-safe** : le Meyers' Singleton, les dangers du Singleton classique, les cas d'usage légitimes (logger, pool de connexions, configuration), et les alternatives (`std::optional` paresseux, injection de dépendances).  
- **44.1.2 — Factory et Abstract Factory** : Factory functions, Factory avec registry (enregistrement dynamique), Factory à base de `std::variant`, Abstract Factory avec templates.  
- **44.1.3 — Builder fluent** : API de chaînage, validation dans `build()`, interaction avec `std::optional` et move semantics, comparaison avec le *named parameter idiom*.

Chaque sous-section présente l'implémentation complète, analyse les compromis de conception, et indique clairement quand le pattern est le bon outil — et quand il ne l'est pas.

⏭️ [Singleton thread-safe](/44-design-patterns/01.1-singleton.md)
