🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 44 — Patterns de Conception en C++

## Module 16 : Patterns et Architecture · Niveau Expert

---

## Introduction

Les patterns de conception (design patterns) sont des solutions éprouvées à des problèmes récurrents de conception logicielle. Popularisés par le livre *Design Patterns: Elements of Reusable Object-Oriented Software* du Gang of Four (GoF) en 1994, ces patterns ont traversé les décennies — mais leur **implémentation en C++ moderne a profondément évolué**.

Là où les patterns GoF s'appuyaient largement sur l'héritage, les pointeurs nus et les interfaces virtuelles, le C++ contemporain (C++17 → C++26) offre des mécanismes qui modifient radicalement la manière de les exprimer : templates, lambdas, `std::variant`, `std::function`, concepts, `constexpr`, sémantique de mouvement… Certains patterns deviennent triviaux, d'autres changent de forme, et quelques-uns perdent en pertinence.

Ce chapitre ne se contente pas de cataloguer les patterns classiques. Il les revisite à travers le prisme du C++ moderne, en insistant sur trois axes :

- **Quand un pattern est-il réellement utile ?** L'application mécanique de patterns sans justification (le *pattern abuse*) est un anti-pattern en soi. Chaque pattern présenté sera accompagné de ses conditions d'applicabilité concrètes.  
- **Comment l'exprimer idiomatiquement en C++ moderne ?** L'implémentation d'un Singleton en 2026 n'a rien à voir avec celle de 2005. Nous privilégierons systématiquement les approches type-safe, RAII-compatibles et thread-safe.  
- **Quelles alternatives le langage offre-t-il nativement ?** Dans bien des cas, une feature du langage (concepts, `std::variant`, lambdas génériques) remplace avantageusement un pattern formel.

---

## Patterns classiques vs C++ moderne : ce qui a changé

Le tableau suivant résume l'évolution des principaux patterns couverts dans ce chapitre et les mécanismes modernes qui les transforment :

| Pattern | Approche classique (GoF) | Approche C++ moderne | Section |
|---|---|---|---|
| **Singleton** | Instance statique + mutex manuel | `static` local (Meyers' Singleton), thread-safe garanti depuis C++11 | 44.1.1 |
| **Factory** | Héritage + `new` brut | `std::unique_ptr`, `std::variant`, templates | 44.1.2 |
| **Builder** | Chaînage de setters avec pointeurs | API fluent avec références, move semantics, `std::optional` | 44.1.3 |
| **Observer** | Interfaces virtuelles + raw pointers | `std::function`, signaux/slots, `std::weak_ptr` | 44.2 |
| **Strategy** | Hiérarchie de classes + virtual dispatch | Lambdas, `std::function`, templates (policy-based design) | 44.2 |
| **Command** | Classes encapsulant des actions | `std::function`, lambdas capturantes | 44.2 |
| **CRTP** | Héritage template récursif | Polymorphisme statique, mixin patterns, `deducing this` (C++23) | 44.3 |
| **Type Erasure** | `void*` et casts manuels | `std::any`, `std::function`, custom erasure | 44.4 |
| **Dependency Injection** | Frameworks lourds, interfaces | Templates, concepts, constructor injection | 44.5 |

---

## Deux familles de polymorphisme

Un fil conducteur de ce chapitre est la distinction entre **polymorphisme dynamique** et **polymorphisme statique**, car le choix entre les deux conditionne la forme que prendra chaque pattern.

### Polymorphisme dynamique (runtime)

C'est l'approche classique, fondée sur les fonctions virtuelles et la vtable :

```cpp
class Shape {  
public:  
    virtual ~Shape() = default;
    virtual double area() const = 0;
};

class Circle : public Shape {
    double radius_;
public:
    explicit Circle(double r) : radius_(r) {}
    double area() const override { return std::numbers::pi * radius_ * radius_; }
};
```

**Avantages** : collections hétérogènes, découplage binaire (ABI stable), extension sans recompilation.  
**Coûts** : indirection via vtable, allocation heap fréquente, cache misses potentiels.  

### Polymorphisme statique (compile-time)

Fondé sur les templates, les concepts et le CRTP :

```cpp
template<typename T>  
concept ShapeLike = requires(const T& s) {  
    { s.area() } -> std::convertible_to<double>;
};

template<ShapeLike S>  
void print_area(const S& shape) {  
    std::print("Aire : {:.2f}\n", shape.area());
}
```

**Avantages** : zéro overhead à l'exécution, inlining agressif, vérification des contraintes à la compilation.  
**Coûts** : temps de compilation plus long, pas de collections hétérogènes directes, code template dans les headers.  

### Quand choisir l'un ou l'autre ?

Le polymorphisme dynamique s'impose lorsque les types concrets ne sont pas connus à la compilation (plugins, configurations runtime, objets provenant de fichiers). Le polymorphisme statique est préférable lorsque les types sont connus à la compilation et que la performance est critique — c'est le cas dans de nombreux contextes système et DevOps.

En pratique, les patterns modernes mélangent souvent les deux approches. Le type erasure (section 44.4), par exemple, offre une interface runtime non-template tout en préservant la flexibilité du polymorphisme statique sous le capot.

---

## Principes directeurs

Avant d'aborder chaque pattern individuellement, gardons à l'esprit ces principes qui guident une conception C++ moderne de qualité :

### Préférer la composition à l'héritage

L'héritage crée un couplage fort entre classes. Dans la majorité des cas, injecter un comportement via un membre, une lambda ou un template parameter produit un design plus flexible et plus testable.

### RAII partout

Chaque pattern doit respecter le principe RAII (cf. section 6.3). Un Factory qui retourne un `new` brut est un pattern mal implémenté. Les smart pointers (`std::unique_ptr`, `std::shared_ptr`) sont la norme pour tout transfert de propriété.

### Minimiser les indirections inutiles

Un pattern doit justifier son coût. Si un `std::function` suffit là où un pattern Strategy classique nécessiterait trois classes et une interface, la lambda est le bon choix. Le C++ moderne permet souvent d'éliminer des couches d'abstraction sans perdre en clarté.

### Exploiter le système de types

Les concepts (C++20), `std::variant`, `std::optional` et `std::expected` (C++23) permettent de déplacer des vérifications du runtime vers la compilation. Un bon pattern en C++ moderne rend les états invalides **irreprésentables** plutôt que de les vérifier à l'exécution.

---

## Organisation du chapitre

Ce chapitre est structuré en cinq sections progressives :

- **Section 44.1 — Singleton, Factory, Builder** : les patterns de création, revisités avec les smart pointers, `std::variant` et les API fluent modernes.  
- **Section 44.2 — Observer, Strategy, Command** : les patterns comportementaux, transformés par les lambdas, `std::function` et le policy-based design.  
- **Section 44.3 — CRTP (Curiously Recurring Template Pattern)** : le polymorphisme statique poussé à son maximum, avec un aperçu de `deducing this` (C++23) qui simplifie considérablement le pattern.  
- **Section 44.4 — Type Erasure et `std::any`** : la technique qui réconcilie polymorphisme dynamique et statique, au cœur de `std::function`, `std::any` et de nombreuses bibliothèques modernes.  
- **Section 44.5 — Dependency Injection en C++** : les stratégies d'injection de dépendances sans framework lourd, en s'appuyant sur les templates, les concepts et le constructor injection.

Chaque section présente le problème résolu par le pattern, son implémentation idiomatique en C++ moderne, ses pièges courants et les situations où il vaut mieux s'en passer.

---

## Prérequis

Ce chapitre suppose une bonne maîtrise des concepts suivants, couverts dans les chapitres précédents :

- **Smart pointers** (chapitre 9) : `std::unique_ptr`, `std::shared_ptr`, `std::weak_ptr`  
- **Sémantique de mouvement** (chapitre 10) : move constructors, `std::move`, perfect forwarding  
- **Lambdas et `std::function`** (chapitre 11) : captures, lambdas génériques, callable objects  
- **Templates et concepts** (chapitre 16) : templates de classes/fonctions, `requires`, concepts standard  
- **`std::variant`, `std::optional`, `std::any`** (section 12.2) : types somme et alternatives aux hiérarchies  
- **Héritage et polymorphisme** (chapitre 7) : fonctions virtuelles, vtable, `override`, `final`

⏭️ [Singleton, Factory, Builder](/44-design-patterns/01-creational-patterns.md)
