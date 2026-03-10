🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 7.4 — Classes abstraites et interfaces pures

## Chapitre 7 : Héritage et Polymorphisme · Module 3 : Programmation Orientée Objet

---

## Introduction

Les sections précédentes ont montré comment le mot-clé `virtual` permet le dispatch dynamique et comment `override` sécurise les redéfinitions. Mais dans tous les exemples vus jusqu'ici, la classe de base fournissait une **implémentation par défaut** de chaque méthode virtuelle. Ce n'est pas toujours souhaitable.

Dans de nombreuses situations de conception, la classe de base représente un **concept abstrait** — une forme géométrique, un flux de données, une tâche — dont il est impossible ou insensé de fournir une implémentation générique. Ce qu'on veut exprimer, c'est un **contrat** : toute classe dérivée *doit* fournir sa propre implémentation. C'est exactement le rôle des **fonctions virtuelles pures** et des **classes abstraites**.

Poussé à son terme, ce principe mène au concept d'**interface pure** — une classe abstraite qui ne contient aucune donnée membre et aucune implémentation, uniquement un contrat de méthodes à implémenter. C'est l'équivalent C++ du mot-clé `interface` en Java ou Go, même si C++ ne le formalise pas en tant que construction syntaxique dédiée.

---

## Fonctions virtuelles pures

### Syntaxe

Une fonction virtuelle pure se déclare en ajoutant `= 0` à la fin de sa déclaration :

```cpp
class Forme {  
public:  
    virtual double aire() const = 0;         // virtuelle pure
    virtual double perimetre() const = 0;    // virtuelle pure
    virtual void dessiner() const = 0;       // virtuelle pure

    virtual ~Forme() = default;
};
```

Le `= 0` n'est pas une affectation. C'est une syntaxe spéciale qui indique au compilateur que cette méthode **n'a pas d'implémentation obligatoire dans cette classe** et que toute classe dérivée concrète devra la redéfinir.

### Effet sur la classe

Une classe qui contient au moins une fonction virtuelle pure (directement ou par héritage) est une **classe abstraite**. Elle ne peut pas être instanciée :

```cpp
Forme f;                           // ❌ Erreur : Forme est abstraite  
auto p = std::make_unique<Forme>();// ❌ Erreur : ne peut pas instancier Forme  
```

Le message de GCC :

```
error: cannot declare variable 'f' to be of abstract type 'Forme'  
note: because the following virtual functions are pure within 'Forme':  
note:     'virtual double Forme::aire() const'  
note:     'virtual double Forme::perimetre() const'  
note:     'virtual void Forme::dessiner() const'  
```

En revanche, on peut déclarer des **pointeurs et des références** vers une classe abstraite — c'est d'ailleurs tout l'intérêt :

```cpp
std::unique_ptr<Forme> f;           // ✅ Pointeur (pas d'instanciation)  
std::vector<std::unique_ptr<Forme>> formes;  // ✅ Collection polymorphique  

void afficher(Forme const& f);      // ✅ Référence comme paramètre
```

### Redéfinition dans les classes dérivées

Une classe dérivée devient **concrète** (instanciable) uniquement si elle redéfinit **toutes** les fonctions virtuelles pures héritées :

```cpp
class Cercle : public Forme {
    double rayon_;
public:
    explicit Cercle(double rayon) : rayon_{rayon} {}

    double aire() const override {
        return std::numbers::pi * rayon_ * rayon_;
    }

    double perimetre() const override {
        return 2.0 * std::numbers::pi * rayon_;
    }

    void dessiner() const override {
        std::println("○ (rayon = {})", rayon_);
    }
};
```

`Cercle` redéfinit les trois méthodes pures : elle est concrète et peut être instanciée.

Si une classe dérivée ne redéfinit pas **toutes** les fonctions pures, elle reste abstraite :

```cpp
class FormePartielle : public Forme {  
public:  
    double aire() const override { return 0.0; }
    // perimetre() et dessiner() ne sont PAS redéfinies
};

// FormePartielle est toujours abstraite — ne peut pas être instanciée
// FormePartielle fp;  // ❌ Erreur
```

Cela permet de construire des **hiérarchies à plusieurs niveaux** où l'abstraction est raffinée progressivement.

---

## Fonction virtuelle pure avec implémentation

Un fait souvent méconnu : une fonction virtuelle pure **peut** avoir une implémentation dans la classe de base. La classe reste abstraite (elle ne peut pas être instanciée), mais l'implémentation est disponible et peut être appelée explicitement par les classes dérivées via l'opérateur de portée :

```cpp
class Connexion {  
public:  
    virtual void fermer() = 0;   // pure — mais avec implémentation
    virtual ~Connexion() = default;
};

// Implémentation fournie en dehors de la déclaration de la classe
void Connexion::fermer() {
    std::println("Nettoyage générique de la connexion");
}

class ConnexionTCP : public Connexion {  
public:  
    void fermer() override {
        std::println("Fermeture du socket TCP");
        Connexion::fermer();   // appel explicite à l'implémentation de base
    }
};

class ConnexionUDP : public Connexion {  
public:  
    void fermer() override {
        std::println("Fermeture du socket UDP");
        Connexion::fermer();   // réutilise le nettoyage commun
    }
};
```

```
Fermeture du socket TCP  
Nettoyage générique de la connexion  
```

Ce pattern est utile quand il existe une **logique commune de nettoyage** que toutes les classes dérivées doivent exécuter, mais qu'on veut forcer chaque dérivée à fournir aussi sa propre logique spécifique. Le `= 0` garantit que la méthode sera redéfinie (la classe dérivée ne peut pas se contenter de l'héritage par défaut), tandis que l'implémentation de base offre un point de réutilisation accessible via `Base::methode()`.

> 💡 Le destructeur virtuel pur avec implémentation est un cas particulier courant. Il est la seule façon de rendre une classe abstraite quand aucune autre méthode ne se prête naturellement au `= 0` :  
>
> ```cpp
> class AbstractBase {
> public:
>     virtual ~AbstractBase() = 0;   // rend la classe abstraite
> };
>
> // L'implémentation est OBLIGATOIRE — le destructeur est toujours appelé
> AbstractBase::~AbstractBase() = default;
> ```
>  
> Sans l'implémentation, le linker échouera car le destructeur de la base est toujours appelé dans la chaîne de destruction.

---

## Le concept d'interface pure en C++

### Qu'est-ce qu'une interface ?

En Java, `interface` est un mot-clé du langage qui définit un type contenant uniquement des signatures de méthodes, sans données ni implémentation. C++ n'a pas de mot-clé `interface`, mais le concept se modélise par une classe abstraite répondant aux critères suivants :

1. **Aucune donnée membre** (pas d'état).
2. **Toutes les méthodes sont virtuelles pures** (aucune implémentation par défaut).
3. **Un destructeur virtuel** (par défaut, `= default`).

```cpp
class ISerializable {  
public:  
    virtual std::string to_json() const = 0;
    virtual void from_json(std::string_view json) = 0;
    virtual ~ISerializable() = default;
};

class IPrintable {  
public:  
    virtual void print(std::ostream& out) const = 0;
    virtual ~IPrintable() = default;
};
```

### Convention de nommage

Il n'existe pas de convention universelle pour nommer les interfaces en C++. Les styles les plus répandus sont :

- **Préfixe `I`** : `ISerializable`, `IPrintable`, `IObserver` — inspiré de la tradition COM/Java/.NET. Courant dans les projets d'entreprise et les bases de code Microsoft.  
- **Suffixe descriptif** : `Serializable`, `Printable`, `Observer` — plus proche de l'idiome C++ standard. La bibliothèque standard n'utilise pas de préfixe.  
- **Suffixe `Interface`** : `SerializableInterface` — explicite mais verbeux.

L'important est d'être **cohérent** au sein d'un projet. Cette formation utilise le préfixe `I` pour distinguer clairement les interfaces des classes concrètes dans les exemples.

### Implémentation de plusieurs interfaces

L'héritage multiple prend tout son sens avec les interfaces pures. Puisqu'une interface n'a pas de données membres, il n'y a ni duplication mémoire ni problème du diamant (sauf si deux interfaces héritent d'une interface commune — un cas rare qu'on résout par héritage virtuel si nécessaire) :

```cpp
class Rapport : public ISerializable, public IPrintable {
    std::string titre_;
    std::string contenu_;

public:
    Rapport(std::string titre, std::string contenu)
        : titre_{std::move(titre)}, contenu_{std::move(contenu)} {}

    // Implémentation de ISerializable
    std::string to_json() const override {
        return std::format(R"({{"titre":"{}","contenu":"{}"}})", titre_, contenu_);
    }

    void from_json(std::string_view json) override {
        // parsing simplifié...
    }

    // Implémentation de IPrintable
    void print(std::ostream& out) const override {
        out << titre_ << "\n" << contenu_;
    }
};
```

`Rapport` implémente deux contrats distincts. Le code client peut manipuler un `Rapport` à travers l'une ou l'autre interface selon le contexte :

```cpp
void sauvegarder(ISerializable const& obj) {
    auto json = obj.to_json();
    // écriture dans un fichier...
}

void afficher(IPrintable const& obj) {
    obj.print(std::cout);
}

int main() {
    Rapport r{"Q1 2026", "Résultats positifs"};
    sauvegarder(r);   // vu comme ISerializable
    afficher(r);       // vu comme IPrintable
}
```

Chaque fonction ne voit que l'interface qui l'intéresse. C'est le **principe de ségrégation des interfaces** (Interface Segregation Principle — le "I" de SOLID).

---

## Interface vs classe abstraite avec implémentation partielle

La distinction entre interface pure et classe abstraite avec implémentation partielle est une question de **design**, pas de syntaxe. Les deux utilisent le même mécanisme (`virtual ... = 0`), mais l'intention diffère :

| Caractéristique | Interface pure | Classe abstraite partielle |  
|---|---|---|  
| Données membres | Aucune | Possibles |  
| Méthodes implémentées | Aucune (sauf destructeur) | Certaines (comportement par défaut) |  
| Rôle | Définir un **contrat** | Fournir un **squelette** réutilisable |  
| Héritage multiple | Sans risque de diamant | Risque si plusieurs branches ont un ancêtre commun |  
| Exemples typiques | `IObserver`, `ISerializable` | `AbstractLogger`, `BaseTache` |

En pratique, on utilise souvent les deux dans une même hiérarchie :

```cpp
// Interface pure : le contrat
class ILogger {  
public:  
    virtual void log(std::string_view message) = 0;
    virtual void set_level(int level) = 0;
    virtual ~ILogger() = default;
};

// Classe abstraite partielle : squelette avec logique commune
class AbstractLogger : public ILogger {
    int level_ = 0;
public:
    void set_level(int level) override {
        level_ = level;
    }

    void log(std::string_view message) override {
        if (should_log()) {
            do_log(message);   // délègue la sortie aux sous-classes
        }
    }

protected:
    bool should_log() const { return level_ > 0; }
    virtual void do_log(std::string_view message) = 0;   // point d'extension
};

// Classe concrète : ne fournit que la sortie spécifique
class ConsoleLogger final : public AbstractLogger {  
protected:  
    void do_log(std::string_view message) override {
        std::println("[CONSOLE] {}", message);
    }
};

class FileLogger final : public AbstractLogger {
    std::string chemin_;
protected:
    void do_log(std::string_view message) override {
        std::println("[FILE:{}] {}", chemin_, message);
        // écriture réelle dans le fichier...
    }
public:
    explicit FileLogger(std::string chemin) : chemin_{std::move(chemin)} {}
};
```

Cette architecture à trois niveaux — interface → squelette abstrait → implémentation concrète — est un pattern classique appelé **Template Method** (à ne pas confondre avec les templates C++). L'interface `ILogger` définit le contrat, `AbstractLogger` implémente la logique commune (filtrage par niveau), et chaque logger concret ne fournit que sa spécificité (sortie console, fichier, réseau, etc.).

---

## Classes abstraites et constructeurs

Une classe abstraite ne peut pas être instanciée, mais elle **peut** avoir des constructeurs. Ces constructeurs sont appelés par les classes dérivées via la liste d'initialisation, exactement comme pour l'héritage classique :

```cpp
class AbstractDocument {
    std::string auteur_;
    std::string date_creation_;

public:
    AbstractDocument(std::string auteur, std::string date)
        : auteur_{std::move(auteur)}, date_creation_{std::move(date)} {}

    virtual void exporter() const = 0;

    std::string const& auteur() const { return auteur_; }
    std::string const& date() const { return date_creation_; }

    virtual ~AbstractDocument() = default;
};

class PDF final : public AbstractDocument {  
public:  
    PDF(std::string auteur, std::string date)
        : AbstractDocument{std::move(auteur), std::move(date)} {}

    void exporter() const override {
        std::println("Export PDF par {}, créé le {}", auteur(), date());
    }
};
```

Le constructeur d'`AbstractDocument` initialise l'état commun. Le fait que la classe soit abstraite ne change rien à la mécanique de construction — elle empêche simplement l'instanciation directe.

> 💡 Il est souvent judicieux de rendre le constructeur d'une classe abstraite **`protected`** plutôt que `public`. Puisque la classe ne peut pas être instanciée directement, un constructeur `public` est trompeur. Un constructeur `protected` exprime clairement qu'il n'est destiné qu'aux classes dérivées :  
>
> ```cpp
> class AbstractDocument {
> protected:
>     AbstractDocument(std::string auteur, std::string date)
>         : auteur_{std::move(auteur)}, date_creation_{std::move(date)} {}
> public:
>     virtual void exporter() const = 0;
>     virtual ~AbstractDocument() = default;
>     // ...
> };
> ```

---

## Utilisation avec les smart pointers et les conteneurs

Les classes abstraites et les interfaces trouvent leur utilisation naturelle avec les **smart pointers** (chapitre 9) pour construire des collections polymorphiques :

```cpp
#include <memory>
#include <vector>

std::vector<std::unique_ptr<ILogger>> creer_loggers() {
    std::vector<std::unique_ptr<ILogger>> loggers;
    loggers.push_back(std::make_unique<ConsoleLogger>());
    loggers.push_back(std::make_unique<FileLogger>("/var/log/app.log"));
    return loggers;
}

void log_partout(std::vector<std::unique_ptr<ILogger>> const& loggers,
                 std::string_view message) {
    for (auto const& logger : loggers) {
        logger->log(message);   // dispatch dynamique via l'interface
    }
}
```

Le code client ne connaît que `ILogger`. Il est **découplé** des implémentations concrètes. On peut ajouter un `SyslogLogger`, un `NetworkLogger` ou un `NullLogger` (pour les tests) sans modifier `log_partout()`. C'est le **principe ouvert/fermé** (Open/Closed Principle — le "O" de SOLID) : ouvert à l'extension, fermé à la modification.

---

## Factory functions et classes abstraites

Puisqu'une classe abstraite ne peut pas être instanciée directement, la **construction d'objets concrets** est souvent déléguée à des fonctions factory qui retournent un `unique_ptr` vers l'interface :

```cpp
enum class LogDestination { Console, File, Syslog };

std::unique_ptr<ILogger> creer_logger(LogDestination dest,
                                       std::string const& param = "") {
    switch (dest) {
        case LogDestination::Console:
            return std::make_unique<ConsoleLogger>();
        case LogDestination::File:
            return std::make_unique<FileLogger>(param);
        case LogDestination::Syslog:
            // return std::make_unique<SyslogLogger>(param);
        default:
            return std::make_unique<ConsoleLogger>();
    }
}

int main() {
    auto logger = creer_logger(LogDestination::File, "/tmp/app.log");
    logger->set_level(1);
    logger->log("Application démarrée");
}
```

Le code client ne mentionne jamais les types concrets. La factory est le **seul point** où la décision de quel type instancier est prise. Ce pattern (Factory Method, section 44.1.2) est omniprésent dans les architectures C++ bien conçues.

---

## Différences avec d'autres langages

Pour les développeurs venant d'autres langages, voici comment les concepts se transposent :

| Concept | C++ | Java | Go | Rust |  
|---|---|---|---|---|  
| Interface | Classe avec uniquement des fonctions virtuelles pures | `interface` | `interface` (implicite) | `trait` |  
| Classe abstraite | Classe avec au moins une fonction virtuelle pure | `abstract class` | N/A | N/A (traits avec implémentations par défaut) |  
| Implémentation multiple d'interfaces | Héritage multiple | `implements` (multiple) | Implicite (duck typing) | `impl Trait for Type` |  
| Vérification | Compilation (override) | Compilation | Compilation (mais implicite) | Compilation |  
| Mot-clé dédié | Aucun (`= 0`) | `interface`, `abstract` | `interface` | `trait` |

Le point distinctif de C++ est l'absence de mot-clé dédié pour les interfaces. C'est un **pattern de codage**, pas une construction syntaxique. La discipline revient au développeur, mais le mécanisme sous-jacent (`virtual ... = 0`, destructeur virtuel) est le même que pour toute autre méthode virtuelle.

---

## Bonnes pratiques

**Utilisez des interfaces pour définir les contrats entre composants.** Une interface exprime ce qu'un composant attend de ses dépendances, sans imposer comment elles sont implémentées. C'est la base de l'injection de dépendances et du code testable.

**Déclarez toujours un destructeur virtuel dans une interface.** Même si l'interface n'a pas de données membres, les classes concrètes qui l'implémentent en auront. Sans destructeur virtuel, la suppression via un pointeur d'interface est un comportement indéfini. Le destructeur virtuel par défaut (`virtual ~Interface() = default;`) est la forme recommandée.

**Rendez les constructeurs d'une classe abstraite `protected`.** Cela documente explicitement le fait que la classe n'est pas destinée à être instanciée directement.

**Préférez les interfaces sans état pour l'héritage multiple.** L'héritage multiple de classes concrètes avec données membres est source de complexité (section 7.1.2). L'héritage multiple d'interfaces pures sans données est sûr et idiomatique.

**Envisagez les Concepts C++20 comme alternative aux interfaces.** Les Concepts (section 16.6) offrent une forme de "contrat" vérifiée à la compilation, sans vtable ni surcoût à l'exécution. Pour du code générique (templates), les Concepts sont souvent préférables aux interfaces polymorphiques. Les interfaces restent incontournables quand le type concret n'est pas connu à la compilation (collections hétérogènes, plugins, injection de dépendances).

**Ne créez pas d'interface pour une seule implémentation.** Si votre interface `IDatabase` n'a qu'une seule classe dérivée `PostgresDatabase`, le niveau d'abstraction n'apporte rien. L'abstraction a un coût (indirection, complexité). Introduisez une interface quand vous avez (ou anticipez raisonnablement) au moins deux implémentations, ou quand vous avez besoin de mocks pour les tests.

---

## Résumé

| Concept | Syntaxe | Effet |  
|---|---|---|  
| Fonction virtuelle pure | `virtual void f() = 0;` | Force la redéfinition dans les classes dérivées concrètes |  
| Classe abstraite | Classe avec ≥1 méthode `= 0` | Ne peut pas être instanciée |  
| Pure virtual avec implémentation | `virtual void f() = 0;` + `void Base::f() { ... }` | Classe toujours abstraite, mais implémentation accessible via `Base::f()` |  
| Destructeur virtuel pur | `virtual ~Base() = 0;` + `Base::~Base() = default;` | Rend une classe abstraite même sans autre méthode pure |  
| Interface pure | Classe avec uniquement des méthodes `= 0` et un destructeur virtuel, sans données | Définit un contrat sans implémentation |  
| Template Method pattern | Interface → Squelette abstrait → Implémentation concrète | Factorisation de la logique commune avec points d'extension |

---


⏭️ [Coût du polymorphisme dynamique en performance](/07-heritage-polymorphisme/05-cout-polymorphisme.md)
