🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 7.3 — Polymorphisme dynamique : `virtual`, `override`, `final`

## Chapitre 7 : Héritage et Polymorphisme · Module 3 : Programmation Orientée Objet

---

## Introduction

La section précédente a décortiqué le mécanisme interne du dispatch dynamique — vtables, vptrs, double indirection. Cette section adopte le point de vue du **développeur** : comment écrire du code polymorphique correct, maintenable et résistant aux erreurs de refactoring.

C++ fournit trois mots-clés pour contrôler le polymorphisme dynamique :

- **`virtual`** (C++98) — déclare qu'une méthode doit être dispatchée dynamiquement.  
- **`override`** (C++11) — indique explicitement qu'une méthode dans une classe dérivée redéfinit une méthode virtuelle de la base, et demande au compilateur de le **vérifier**.  
- **`final`** (C++11) — interdit toute redéfinition ultérieure d'une méthode, ou tout héritage ultérieur d'une classe.

Le trio `virtual`/`override`/`final` forme un système cohérent qui, utilisé correctement, élimine des catégories entières de bugs silencieux. L'enjeu de cette section est de montrer pourquoi `override` devrait être **systématique** dans tout code C++ moderne, et quand `final` apporte une valeur ajoutée réelle.

---

## `virtual` : déclarer le polymorphisme

### Syntaxe et sémantique

Le mot-clé `virtual` se place devant le type de retour d'une méthode dans la classe de base :

```cpp
class Transport {  
public:  
    virtual void deplacer() const {
        std::println("Transport générique");
    }

    virtual double cout_km() const {
        return 0.0;
    }

    virtual ~Transport() = default;
};
```

Une fois qu'une méthode est déclarée `virtual`, elle l'est **implicitement dans toute la hiérarchie descendante**. Il n'est pas nécessaire — ni recommandé — de répéter `virtual` dans les classes dérivées :

```cpp
class Train : public Transport {  
public:  
    // virtual est implicite ici — inutile de le réécrire
    void deplacer() const override {
        std::println("Le train roule sur les rails");
    }

    double cout_km() const override {
        return 0.12;
    }
};
```

### Ce que `virtual` ne fait pas

`virtual` ne rend pas une méthode obligatoirement redéfinie. La classe de base fournit une implémentation par défaut qui sera héritée par toute classe dérivée qui ne la redéfinit pas. Pour forcer la redéfinition, il faut utiliser les **fonctions virtuelles pures** (`= 0`), couvertes en section 7.4.

`virtual` ne s'applique qu'aux **méthodes membres non statiques**. On ne peut pas déclarer `virtual` un constructeur, une fonction libre, une méthode statique ou un template de méthode (les templates de méthodes ne peuvent pas être virtuels car leur instanciation se fait à la compilation, incompatible avec le mécanisme de vtable).

---

## `override` : sécuriser les redéfinitions

### Le problème que `override` résout

Avant C++11, rien ne vérifiait qu'une méthode dans une classe dérivée redéfinissait effectivement une méthode virtuelle de la base. Une faute de frappe, un changement de signature lors d'un refactoring ou un oubli de `const` créaient silencieusement une **nouvelle méthode** au lieu de redéfinir l'existante :

```cpp
class Forme {  
public:  
    virtual void dessiner() const { /* ... */ }
    virtual ~Forme() = default;
};

class Cercle : public Forme {  
public:  
    // ⚠️ BUG SILENCIEUX : "desiner" au lieu de "dessiner"
    virtual void desiner() const { /* ... */ }

    // ⚠️ BUG SILENCIEUX : manque "const"
    virtual void dessiner() { /* ... */ }
};
```

Dans les deux cas, le compilateur accepte le code sans broncher. `Cercle` déclare de nouvelles méthodes au lieu de redéfinir `dessiner() const`. Le dispatch dynamique appellera `Forme::dessiner()` alors que le développeur pense avoir fourni une implémentation spécifique. Ce type de bug est particulièrement pernicieux car il ne provoque ni erreur de compilation ni crash — simplement un comportement incorrect, potentiellement découvert des semaines plus tard.

### La solution : `override`

Le spécificateur `override` se place **après** la liste des paramètres (et après `const`/`noexcept` le cas échéant). Il demande au compilateur de **vérifier** que la méthode redéfinit bien une méthode virtuelle d'une classe de base avec une signature identique :

```cpp
class Cercle : public Forme {  
public:  
    void desiner() const override;    // ❌ Erreur de compilation :
                                       //    ne redéfinit aucune méthode de Forme

    void dessiner() override;          // ❌ Erreur de compilation :
                                       //    signature différente (manque const)

    void dessiner() const override;    // ✅ Compilation réussie :
                                       //    redéfinit bien Forme::dessiner() const
};
```

Les erreurs de GCC sont claires :

```
error: 'void Cercle::desiner() const' marked 'override', but does not override  
error: 'void Cercle::dessiner()' marked 'override', but does not override  
```

### `override` protège aussi contre les changements en amont

Imaginons que la classe de base est modifiée lors d'un refactoring — la signature d'une méthode change :

```cpp
// Avant :
class Forme {  
public:  
    virtual void dessiner() const;
};

// Après refactoring :
class Forme {  
public:  
    virtual void dessiner(bool antialiasing = false) const;  // signature changée
};
```

Sans `override`, toutes les classes dérivées qui redéfinissaient `dessiner() const` compilent toujours — mais elles ne redéfinissent plus rien. Avec `override`, chaque classe dérivée provoque une erreur de compilation, forçant le développeur à mettre à jour toute la hiérarchie. C'est un filet de sécurité inestimable dans les grandes bases de code.

### Règles d'utilisation

`override` n'est pas un qualificateur de type comme `const` — c'est un **spécificateur contextuel** (*context-sensitive keyword*). Il n'est reconnu comme mot-clé que dans le contexte d'une déclaration de méthode après la liste de paramètres. En dehors de ce contexte, `override` peut être utilisé comme identifiant (pour la rétrocompatibilité avec du code pré-C++11), bien que ce soit déconseillé.

L'ordre des spécificateurs après la liste de paramètres est :

```cpp
void methode() const noexcept override;
//                   ↑         ↑         ↑
//                 cv-qual   except    virt-spec
```

`override` se place toujours en **dernier**, après `const` et `noexcept`.

### Faut-il écrire `virtual` et `override` ensemble ?

Non. Quand une méthode est marquée `override`, le `virtual` est redondant. La pratique recommandée en C++ moderne est :

```cpp
// ✅ Recommandé : virtual dans la base, override dans les dérivées
class Base {  
public:  
    virtual void action();        // virtual : déclare le polymorphisme
    virtual ~Base() = default;
};

class Derivee : public Base {  
public:  
    void action() override;       // override seul : vérifie la redéfinition
};
```

```cpp
// ❌ Déconseillé : virtual + override est redondant
class Derivee : public Base {  
public:  
    virtual void action() override;   // virtual est superflu ici
};
```

Le `virtual` dans la dérivée n'apporte aucune information supplémentaire et brouille la lisibilité : il laisse penser que c'est cette classe qui *introduit* le polymorphisme, alors qu'elle ne fait que redéfinir.

---

## `final` : verrouiller la hiérarchie

### `final` sur une méthode

Le spécificateur `final` interdit à toute classe dérivée de redéfinir la méthode. Il se place au même endroit que `override` :

```cpp
class Forme {  
public:  
    virtual void dessiner() const = 0;
    virtual std::string type() const = 0;
    virtual ~Forme() = default;
};

class Cercle : public Forme {  
public:  
    void dessiner() const override {
        std::println("○");
    }

    // Cette implémentation de type() ne peut plus être redéfinie
    std::string type() const override final {
        return "Cercle";
    }
};

class CercleColore : public Cercle {  
public:  
    void dessiner() const override {       // ✅ OK — dessiner() n'est pas final
        std::println("○ (en couleur)");
    }

    // std::string type() const override {  // ❌ Erreur : type() est final dans Cercle
    //     return "CercleColore";
    // }
};
```

L'utilisation de `final` sur une méthode exprime un **contrat architectural** : cette implémentation est définitive, et aucune sous-classe ne doit la modifier. C'est utile quand une méthode contient de la logique critique dont la redéfinition pourrait compromettre les invariants de la classe.

### `final` sur une classe

`final` peut aussi être appliqué à une **classe entière**, interdisant tout héritage :

```cpp
class Singleton final {
    static Singleton& instance() {
        static Singleton s;
        return s;
    }
    // ...
};

// class MonSingleton : public Singleton {};  // ❌ Erreur : Singleton est final
```

Une classe `final` ne peut avoir aucune classe dérivée. C'est un outil puissant pour exprimer l'intention de conception : cette classe est une feuille dans la hiérarchie, et elle est conçue pour être utilisée telle quelle.

### `final` comme aide à l'optimisation

Au-delà de l'intention de design, `final` fournit une information précieuse au compilateur pour la **dévirtualisation**. Quand le compilateur sait qu'une classe est `final` ou qu'une méthode est `final`, il peut remplacer les appels virtuels par des appels directs, éliminant le surcoût du dispatch dynamique :

```cpp
class Forme {  
public:  
    virtual void dessiner() const = 0;
    virtual ~Forme() = default;
};

class Cercle final : public Forme {  
public:  
    void dessiner() const override {
        std::println("○");
    }
};

void rendu(Cercle const& c) {
    c.dessiner();   // Le compilateur sait que c est un Cercle final
                    // → appel direct, inlinable, pas de vtable
}
```

Sans `final`, le compilateur devrait supposer que `c` pourrait être un objet d'une sous-classe de `Cercle` avec une implémentation différente de `dessiner()`. Avec `final`, cette possibilité est exclue et le compilateur peut optimiser en toute sécurité.

Cet effet est mesurable dans du code à haute fréquence d'appel. La section 7.5 quantifie les gains de la dévirtualisation.

---

## Combinaisons possibles et syntaxe complète

Voici toutes les combinaisons valides des spécificateurs virtuels :

```cpp
class Base {  
public:  
    virtual void a();                    // virtuelle, redéfinissable
    virtual void b() = 0;               // virtuelle pure, doit être redéfinie
    virtual void c() final;             // virtuelle, ne peut plus être redéfinie
    virtual ~Base() = default;
};

class Milieu : public Base {  
public:  
    void a() override;                  // redéfinit Base::a()
    void b() override final;            // redéfinit Base::b(), verrouille ensuite
};

class Feuille final : public Milieu {  
public:  
    void a() override;                  // redéfinit Milieu::a()
    // void b() override;              // ❌ Erreur : b() est final dans Milieu
};

// class SousFeuille : public Feuille {};  // ❌ Erreur : Feuille est final
```

Le tableau suivant résume les spécificateurs et leur position dans la hiérarchie :

| Spécificateur | Où l'utiliser | Effet |  
|---|---|---|  
| `virtual` | **Classe de base uniquement** | Introduit le dispatch dynamique pour cette méthode |  
| `override` | **Classes dérivées** | Vérifie que la méthode redéfinit bien une méthode virtuelle de la base |  
| `final` (méthode) | **Toute classe de la hiérarchie** | Interdit la redéfinition dans les sous-classes |  
| `final` (classe) | **Déclaration de la classe** | Interdit tout héritage de cette classe |  
| `= 0` | **Classe de base** | Rend la méthode virtuelle pure (pas d'implémentation par défaut obligatoire) |

---

## Pattern courant : le polymorphisme en pratique

Voici un exemple complet qui illustre l'utilisation idiomatique de `virtual`, `override` et `final` dans un contexte réaliste :

```cpp
#include <memory>
#include <vector>
#include <print>

class Tache {
    std::string nom_;
public:
    explicit Tache(std::string nom) : nom_{std::move(nom)} {}

    virtual void executer() = 0;

    virtual std::string description() const {
        return nom_;
    }

    std::string const& nom() const { return nom_; }

    virtual ~Tache() = default;
};

class TacheSysteme : public Tache {
    std::string commande_;
public:
    TacheSysteme(std::string nom, std::string cmd)
        : Tache{std::move(nom)}, commande_{std::move(cmd)} {}

    void executer() override {
        std::println("Exécution système : {}", commande_);
        // std::system(commande_.c_str());
    }

    std::string description() const override {
        return std::format("{} [cmd: {}]", nom(), commande_);
    }
};

class TacheLog final : public Tache {
    std::string message_;
public:
    TacheLog(std::string nom, std::string message)
        : Tache{std::move(nom)}, message_{std::move(message)} {}

    void executer() override {
        std::println("[LOG] {}", message_);
    }

    // description() n'est pas redéfinie → utilise Tache::description()
};

void executer_pipeline(std::vector<std::unique_ptr<Tache>>& pipeline) {
    for (auto const& tache : pipeline) {
        std::println("--- {} ---", tache->description());
        tache->executer();    // dispatch dynamique → bonne implémentation
    }
}

int main() {
    std::vector<std::unique_ptr<Tache>> pipeline;
    pipeline.push_back(std::make_unique<TacheSysteme>("Backup", "tar czf backup.tar.gz /data"));
    pipeline.push_back(std::make_unique<TacheLog>("Notification", "Pipeline démarré"));
    pipeline.push_back(std::make_unique<TacheSysteme>("Deploy", "kubectl apply -f deploy.yaml"));

    executer_pipeline(pipeline);
}
```

```
--- Backup [cmd: tar czf backup.tar.gz /data] ---
Exécution système : tar czf backup.tar.gz /data
--- Notification ---
[LOG] Pipeline démarré
--- Deploy [cmd: kubectl apply -f deploy.yaml] ---
Exécution système : kubectl apply -f deploy.yaml
```

Les éléments clés de cet exemple :

- `Tache` est la classe de base avec un destructeur virtuel, une méthode virtuelle pure (`executer()`) et une méthode virtuelle avec implémentation par défaut (`description()`).  
- `TacheSysteme` redéfinit les deux méthodes avec `override`.  
- `TacheLog` est `final` — personne ne peut en hériter. Elle redéfinit `executer()` mais réutilise l'implémentation par défaut de `description()`.  
- `executer_pipeline()` manipule un vecteur de `unique_ptr<Tache>` — le dispatch dynamique appelle la bonne implémentation de `executer()` et `description()` pour chaque tâche.

---

## Erreurs fréquentes et diagnostic

### Erreur 1 : signature incompatible avec `override`

```cpp
class Base {  
public:  
    virtual void traiter(int x) const;
};

class Derivee : public Base {  
public:  
    void traiter(int x) override;   // ❌ manque const
};
```

```
error: 'void Derivee::traiter(int)' marked 'override', but does not override
```

La correction : ajouter `const` pour correspondre à la signature de la base.

### Erreur 2 : redéfinir une méthode `final`

```cpp
class A {  
public:  
    virtual void f() final;
};

class B : public A {  
public:  
    void f() override;   // ❌ f() est final dans A
};
```

```
error: virtual function 'virtual void B::f()' overriding final function
```

### Erreur 3 : hériter d'une classe `final`

```cpp
class Feuille final {};

class Tentative : public Feuille {};   // ❌ Feuille est final
```

```
error: cannot derive from 'final' base 'Feuille'
```

### Erreur 4 : oublier `override` et créer un name hiding accidentel

```cpp
class Base {  
public:  
    virtual void process(int x);
};

class Derivee : public Base {  
public:  
    void process(double x);   // ⚠️ Pas d'override → c'est du name hiding
                               // process(int) de Base est masquée
};
```

Pas d'erreur de compilation, mais `Derivee::process(double)` **ne redéfinit pas** `Base::process(int)`. L'option `-Wsuggest-override` (GCC) ou `-Wmissing-override` (Clang) détecte ce cas :

```
warning: 'process' overrides a member function but is not marked 'override'
```

---

## Options de compilation recommandées

Pour exploiter pleinement la sécurité offerte par `override` et `final`, activez les warnings suivants :

```bash
# GCC 15
g++ -std=c++23 -Wall -Wextra -Wpedantic \
    -Wsuggest-override \
    -Wnon-virtual-dtor \
    -Woverloaded-virtual \
    -o programme source.cpp

# Clang 20
clang++ -std=c++23 -Wall -Wextra -Wpedantic \
    -Wmissing-override \
    -Wnon-virtual-dtor \
    -Woverloaded-virtual \
    -o programme source.cpp
```

| Option | Détecte |  
|---|---|  
| `-Wsuggest-override` (GCC) | Méthodes qui redéfinissent une virtuelle sans `override` |  
| `-Wmissing-override` (Clang) | Équivalent Clang du précédent |  
| `-Wnon-virtual-dtor` | Classes avec des fonctions virtuelles mais sans destructeur virtuel |  
| `-Woverloaded-virtual` | Méthodes dans la dérivée qui masquent (name hiding) des surcharges virtuelles de la base |

Combinées avec `-Werror`, ces options transforment des bugs silencieux en erreurs de compilation — exactement ce que vous voulez.

---

## Bonnes pratiques

**Utilisez `override` systématiquement, sans exception.** C'est la règle la plus importante de cette section. Chaque méthode qui redéfinit une méthode virtuelle doit porter `override`. Pas "quand on y pense", pas "sauf pour les cas simples" — **toujours**. Le coût est nul (un mot de huit lettres) et le bénéfice est considérable.

**Ne répétez pas `virtual` dans les classes dérivées.** `virtual` introduit le polymorphisme dans la base. Dans les dérivées, `override` suffit à exprimer l'intention et à garantir la vérification par le compilateur.

**Utilisez `final` quand l'architecture l'exige.** Marquez `final` les classes qui ne sont pas conçues pour être dérivées et les méthodes dont la redéfinition violerait des invariants. Cela documente l'intention, protège la conception et aide le compilateur à optimiser.

**Activez `-Wsuggest-override` / `-Wmissing-override` dans votre build.** Ces warnings détectent les méthodes qui devraient porter `override` mais ne le font pas. Dans un projet existant sans `override`, activez-les progressivement pour migrer le code.

**Ne confondez pas `final` et conception fermée.** `final` est un outil de design, pas une mesure de sécurité. Il exprime que l'extension par héritage n'est pas le bon mécanisme d'extension pour cette classe. Si votre design a besoin d'extensibilité, préférez des points d'extension explicites (callbacks, templates, injection de dépendances) plutôt que de retirer `final`.

---

## Résumé

| Mot-clé | Introduit en | Rôle | Vérification |  
|---|---|---|---|  
| `virtual` | C++98 | Active le dispatch dynamique | Aucune (confiance au développeur) |  
| `override` | C++11 | Déclare une redéfinition | Erreur de compilation si la signature ne correspond à aucune méthode virtuelle de la base |  
| `final` (méthode) | C++11 | Interdit la redéfinition | Erreur de compilation si une sous-classe tente de redéfinir |  
| `final` (classe) | C++11 | Interdit l'héritage | Erreur de compilation si une classe tente d'hériter |

La convention moderne en C++ peut se résumer en trois règles :

1. **`virtual`** dans la classe de base pour introduire le polymorphisme.
2. **`override`** dans chaque classe dérivée pour sécuriser la redéfinition.
3. **`final`** aux points de la hiérarchie où l'extension doit s'arrêter.

---


⏭️ [Classes abstraites et interfaces pures](/07-heritage-polymorphisme/04-classes-abstraites.md)
