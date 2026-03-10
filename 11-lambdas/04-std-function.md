🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 11.4 — `std::function` et callable objects

## Le problème : stocker et transmettre des callables

Les lambdas sont puissantes, mais elles ont une particularité gênante pour certains usages : chaque lambda a un **type unique et anonyme**, connu du compilateur seul. On ne peut ni l'écrire explicitement, ni déclarer un paramètre de fonction, un membre de classe ou un conteneur avec ce type.

```cpp
auto greet = []() { std::print("Hello\n"); };
auto farewell = []() { std::print("Goodbye\n"); };

// decltype(greet) ≠ decltype(farewell), même signature pourtant identique

// Comment stocker l'un OU l'autre dans la même variable ?
// Comment déclarer un vector de lambdas ?
// Comment déclarer un membre de classe qui stocke une lambda ?
```

C++ offre plusieurs réponses à ce problème, chacune avec un compromis performance/flexibilité différent. Cette section explore l'ensemble du spectre, de la solution la plus flexible (`std::function`) à la plus performante (templates), en passant par les alternatives modernes.

---

## La famille des callable objects

Avant d'aborder `std::function`, il faut comprendre ce qu'est un **callable** en C++. Un callable est tout objet sur lequel on peut appliquer l'opérateur d'appel `()`. Le C++ en définit plusieurs catégories :

```cpp
// 1. Fonction libre
int add(int a, int b) { return a + b; }

// 2. Pointeur de fonction
int (*fn_ptr)(int, int) = &add;

// 3. Lambda
auto lambda = [](int a, int b) { return a + b; };

// 4. Foncteur (objet avec operator())
struct Adder {
    int operator()(int a, int b) const { return a + b; }
};

// 5. Pointeur vers fonction membre
struct Calculator {
    int add(int a, int b) const { return a + b; }
};

// 6. Objet lié (std::bind)
auto bound = std::bind(&Calculator::add, Calculator{}, std::placeholders::_1, std::placeholders::_2);
```

Tous ces callables ont la même **signature logique** — `(int, int) → int` — mais des **types** complètement différents. `std::function` est le type unificateur qui peut stocker n'importe lequel d'entre eux.

---

## `std::function` — le type erasure universel

### Déclaration et utilisation

`std::function<R(Args...)>` est un wrapper polymorphique défini dans `<functional>` qui peut stocker n'importe quel callable dont la signature est compatible avec `R(Args...)` :

```cpp
#include <functional>

// Déclarer une std::function qui accepte deux int et retourne un int
std::function<int(int, int)> operation;

// Stocker une fonction libre
operation = add;
std::print("{}\n", operation(3, 4));  // 7

// Stocker une lambda
operation = [](int a, int b) { return a * b; };
std::print("{}\n", operation(3, 4));  // 12

// Stocker un foncteur
operation = Adder{};
std::print("{}\n", operation(3, 4));  // 7

// Stocker une lambda avec capture
int offset = 10;
operation = [offset](int a, int b) { return a + b + offset; };
std::print("{}\n", operation(3, 4));  // 17
```

Un seul type — `std::function<int(int, int)>` — unifie des callables de types complètement différents. C'est le mécanisme de **type erasure** : le type concret du callable est "effacé" derrière une interface uniforme.

### Vérification de validité

Une `std::function` peut être vide (ne contenir aucun callable). L'appeler dans cet état est un comportement défini — elle lance une exception `std::bad_function_call` :

```cpp
std::function<void()> callback;

if (callback) {
    callback();  // N'est pas exécuté — callback est vide
}

// Ou via comparaison explicite
if (callback != nullptr) {
    callback();
}

// Appeler une std::function vide lève une exception
try {
    callback();
} catch (const std::bad_function_call& e) {
    std::print("Pas de callback enregistré\n");
}
```

Le test `if (callback)` est idiomatique et préférable au try/catch.

### Réinitialisation

```cpp
std::function<void()> fn = []() { std::print("Active\n"); };

fn();           // Active
fn = nullptr;   // Vide la function
// fn();        // Lèverait std::bad_function_call
```

---

## Comment `std::function` fonctionne en interne

Comprendre le mécanisme interne de `std::function` est essentiel pour en évaluer le coût. Le principe repose sur le **type erasure** — un pattern qui combine héritage virtuel et allocation dynamique pour masquer le type concret.

### Schéma conceptuel simplifié

```cpp
// Représentation conceptuelle (très simplifiée)
template<typename R, typename... Args>
class function<R(Args...)> {

    // Interface abstraite — type erasure via héritage virtuel
    struct callable_base {
        virtual R invoke(Args... args) = 0;
        virtual callable_base* clone() const = 0;
        virtual ~callable_base() = default;
    };

    // Implémentation concrète pour chaque type de callable
    template<typename F>
    struct callable_impl : callable_base {
        F functor;
        callable_impl(F f) : functor(std::move(f)) {}
        R invoke(Args... args) override { return functor(args...); }
        callable_base* clone() const override { return new callable_impl(functor); }
    };

    callable_base* impl_ = nullptr;  // Pointeur vers l'implémentation

public:
    template<typename F>
    function(F f) : impl_(new callable_impl<F>(std::move(f))) {}

    R operator()(Args... args) {
        return impl_->invoke(args...);
    }
};
```

Ce schéma révèle trois sources de surcoût.

**Allocation dynamique.** Le callable est stocké sur le heap via `new`. Cependant, la plupart des implémentations utilisent une **Small Buffer Optimization (SBO)** : si le callable est suffisamment petit (typiquement 16 à 32 octets selon l'implémentation), il est stocké directement dans le corps de la `std::function`, sans allocation heap.

**Appel indirect.** Chaque invocation passe par un appel virtuel (`invoke`), ce qui empêche l'inlining. Le compilateur ne peut pas savoir à la compilation quel callable sera exécuté — le dispatch est résolu à l'exécution.

**Copie potentiellement coûteuse.** Copier une `std::function` copie le callable sous-jacent. Si celui-ci capture des objets volumineux, la copie est coûteuse.

### Small Buffer Optimization en pratique

La SBO est une optimisation cruciale. Une lambda sans capture ou avec de petites captures (quelques scalaires) tient généralement dans le buffer interne :

```cpp
// Tient dans le SBO (pas d'allocation heap)
std::function<int(int)> small = [x = 42](int v) { return v + x; };

// Peut dépasser le SBO (allocation heap probable)
std::function<int(int)> large = [arr = std::array<int, 100>{}](int v) {
    return v + arr[0];
};
```

La taille exacte du SBO varie selon les implémentations : environ 16 octets pour libstdc++ (GCC), environ 24 octets pour libc++ (Clang). On peut vérifier la taille d'une `std::function` :

```cpp
std::print("sizeof(std::function<void()>) = {}\n", sizeof(std::function<void()>));
// Typiquement 32 (libstdc++) ou 48 (libc++)
```

---

## Le coût de `std::function` — mesure concrète

Le surcoût de `std::function` par rapport à un appel direct ou templaté est mesurable :

```cpp
// Appel direct — le compilateur inline la lambda
auto direct = [](int x) { return x * 2; };
int result1 = direct(42);  // Inliné → une seule instruction mul

// Via std::function — appel indirect, pas d'inlining
std::function<int(int)> indirect = [](int x) { return x * 2; };
int result2 = indirect(42);  // Appel virtuel → call indirect
```

Pour une opération triviale dans une boucle serrée, la différence peut être d'un facteur 2 à 10x. Pour une opération coûteuse (I/O, calcul complexe), le surcoût de l'indirection est négligeable relativement au travail effectif.

### Quand le surcoût est négligeable

Le coût de `std::function` est rarement un problème dans les contextes suivants : callbacks d'événements (UI, réseau), handlers appelés occasionnellement, registres d'observateurs, configuration de comportements au démarrage. Dans tous ces cas, le travail effectué par le callable domine largement le coût de l'indirection.

### Quand le surcoût est problématique

Le surcoût devient significatif dans les boucles critiques en performance : prédicats appelés des millions de fois dans des algorithmes de tri ou de recherche, fonctions de hashing, opérations pixel par pixel dans du traitement d'image, calculs dans des simulations numériques. Dans ces cas, les templates sont la solution (voir plus bas).

---

## Cas d'usage légitimes de `std::function`

### Membres de classe — callbacks et stratégies

Le cas d'usage principal : stocker un comportement configurable dans un objet. Les templates ne peuvent pas servir de type de membre sans rendre la classe entière templatée :

```cpp
class Button {
    std::string label_;
    std::function<void()> on_click_;
    std::function<void()> on_hover_;
public:
    Button(std::string label) : label_(std::move(label)) {}

    void set_on_click(std::function<void()> handler) {
        on_click_ = std::move(handler);
    }

    void set_on_hover(std::function<void()> handler) {
        on_hover_ = std::move(handler);
    }

    void click() {
        if (on_click_) on_click_();
    }
};

Button btn("Submit");
btn.set_on_click([]() { std::print("Form submitted!\n"); });
btn.click();
```

Sans `std::function`, il faudrait templatiser `Button` sur le type de chaque callback — ce qui est impraticable dès qu'il y en a plusieurs.

### Conteneurs de callables

```cpp
// Registre de commandes CLI
std::unordered_map<std::string, std::function<void(std::span<std::string_view>)>> commands;

commands["help"] = [](std::span<std::string_view>) {
    std::print("Available commands: help, quit, run\n");
};

commands["quit"] = [](std::span<std::string_view>) {
    std::print("Goodbye!\n");
    std::exit(0);
};

commands["run"] = [&pipeline](std::span<std::string_view> args) {
    pipeline.execute(args);
};

// Dispatch
auto it = commands.find(user_input);
if (it != commands.end()) {
    it->second(arguments);
}
```

Les types des lambdas sont tous différents, mais le `std::unordered_map` nécessite un type de valeur uniforme — `std::function` est la seule option.

### API publiques et limites de compilation

Dans une API publique (bibliothèque .so/.a), exposer des templates force l'implémentation dans les headers. `std::function` permet de déclarer l'interface dans le header et d'implémenter dans le .cpp :

```cpp
// api.h — interface publique
class EventBus {
public:
    using Handler = std::function<void(const Event&)>;
    void subscribe(std::string event_name, Handler handler);
    void publish(const Event& event);
};

// api.cpp — implémentation cachée
void EventBus::subscribe(std::string event_name, Handler handler) {
    handlers_[std::move(event_name)].push_back(std::move(handler));
}
```

Le code client n'a pas besoin de voir l'implémentation — la séparation .h/.cpp est préservée (voir section 46.2).

### `std::function` comme paramètre de signal/slot

Le pattern observateur utilise naturellement `std::function` pour les connexions signal/slot :

```cpp
class Signal {
    std::vector<std::function<void(int)>> slots_;
public:
    void connect(std::function<void(int)> slot) {
        slots_.push_back(std::move(slot));
    }

    void emit(int value) {
        for (auto& slot : slots_) {
            slot(value);
        }
    }
};

Signal on_value_changed;

on_value_changed.connect([](int v) { std::print("Logger: {}\n", v); });
on_value_changed.connect([](int v) { std::print("UI update: {}\n", v); });
on_value_changed.connect([&database](int v) { database.store(v); });

on_value_changed.emit(42);
```

---

## L'alternative performante : les templates

Lorsque le callable n'a pas besoin d'être stocké — quand il est utilisé immédiatement dans la même portée — un **paramètre template** est la solution optimale. Le compilateur connaît le type exact du callable et peut **inliner** l'appel :

```cpp
// Avec std::function — appel indirect, pas d'inlining
void process(const std::vector<int>& data, std::function<bool(int)> predicate) {
    for (int v : data) {
        if (predicate(v)) std::print("{} ", v);
    }
}

// Avec template — le compilateur connaît le type exact, inlining possible
template<typename Predicate>
void process(const std::vector<int>& data, Predicate predicate) {
    for (int v : data) {
        if (predicate(v)) std::print("{} ", v);
    }
}
```

La version template est potentiellement plus rapide car le compilateur voit le corps de la lambda à travers le paramètre template et peut l'inliner directement dans la boucle.

### Templates avec Concepts (C++20)

Le reproche historique aux templates est la qualité des messages d'erreur. Les Concepts (section 16.6) résolvent ce problème en contraignant les types acceptés :

```cpp
template<typename F>
    requires std::predicate<F, int>
void process(const std::vector<int>& data, F predicate) {
    for (int v : data) {
        if (predicate(v)) std::print("{} ", v);
    }
}

// Ou avec la syntaxe abrégée
void process(const std::vector<int>& data, std::predicate<int> auto predicate) {
    for (int v : data) {
        if (predicate(v)) std::print("{} ", v);
    }
}
```

Si on passe un callable qui n'est pas un prédicat valide pour `int`, le message d'erreur est clair et pointe directement vers la contrainte violée.

### `std::invocable` — le concept générique pour les callables

Le concept `std::invocable<F, Args...>` vérifie qu'un type `F` est appelable avec les arguments `Args...`. C'est le concept le plus général pour contraindre un callable :

```cpp
template<std::invocable<int, int> F>
auto apply(F&& func, int a, int b) {
    return std::invoke(std::forward<F>(func), a, b);
}

apply([](int a, int b) { return a + b; }, 3, 4);  // ✅
apply(add, 3, 4);                                   // ✅ Fonction libre
apply(Adder{}, 3, 4);                               // ✅ Foncteur
// apply(42, 3, 4);                                  // ❌ int n'est pas invocable
```

---

## `std::invoke` — appel uniforme de tout callable

`std::invoke` (C++17, dans `<functional>`) est une fonction utilitaire qui appelle n'importe quel callable de manière uniforme, y compris les **pointeurs vers fonctions membres** et les **pointeurs vers données membres**, qui ont une syntaxe d'appel non standard :

```cpp
struct Widget {
    int value = 42;
    int get_value() const { return value; }
    void set_value(int v) { value = v; }
};

Widget w;

// Fonction libre — std::invoke est transparent
std::print("{}\n", std::invoke(add, 3, 4));  // 7

// Lambda
std::print("{}\n", std::invoke([](int x) { return x * 2; }, 21));  // 42

// Pointeur vers fonction membre — syntaxe normalement lourde
// (w.*&Widget::get_value)() — illisible
std::print("{}\n", std::invoke(&Widget::get_value, w));  // 42

// Pointeur vers donnée membre
std::print("{}\n", std::invoke(&Widget::value, w));  // 42
```

Sans `std::invoke`, appeler un pointeur vers membre nécessite une syntaxe différente de celle d'une lambda ou d'une fonction libre. `std::invoke` unifie tout sous la même interface. C'est pourquoi les algorithmes de la STL l'utilisent en interne depuis C++20 — et c'est également pour cela qu'il est recommandé de l'utiliser dans les templates génériques qui acceptent des callables :

```cpp
template<typename F, typename... Args>
auto logged_call(F&& func, Args&&... args) {
    std::print("Calling function...\n");
    // std::invoke gère TOUS les types de callables uniformément
    auto result = std::invoke(std::forward<F>(func), std::forward<Args>(args)...);
    std::print("Done.\n");
    return result;
}

// Fonctionne avec tout
logged_call(add, 3, 4);                     // Fonction libre
logged_call([](int x) { return x; }, 42);   // Lambda
logged_call(&Widget::get_value, w);          // Fonction membre
```

---

## `std::bind` — l'approche pré-lambda (déconseillée)

`std::bind` (C++11) permet de créer un callable en fixant certains arguments d'une fonction existante. Il est mentionné ici pour deux raisons : vous le rencontrerez dans du code legacy, et il faut comprendre pourquoi les lambdas le remplacent avantageusement.

```cpp
int multiply(int a, int b) { return a * b; }

// std::bind fixe le premier argument à 10
auto times_ten = std::bind(multiply, 10, std::placeholders::_1);
std::print("{}\n", times_ten(5));  // 50

// Équivalent lambda — plus lisible
auto times_ten_lambda = [](int x) { return multiply(10, x); };
std::print("{}\n", times_ten_lambda(5));  // 50
```

### Pourquoi préférer les lambdas à `std::bind`

Les lambdas surpassent `std::bind` sur quasiment tous les critères.

La **lisibilité** est immédiatement meilleure. Les placeholders `_1`, `_2`, `_3` sont opaques — il faut compter pour savoir quel argument est passé où. La lambda nomme ses paramètres et le corps est visible :

```cpp
// std::bind — quel argument va où ?
auto f = std::bind(some_func, _2, 42, _1, _3);

// Lambda — explicite
auto f = [](auto a, auto b, auto c) { return some_func(b, 42, a, c); };
```

Le **debugging** est plus facile avec les lambdas. Le type généré par `std::bind` est un type interne à la bibliothèque standard, difficile à lire dans les messages d'erreur et les stack traces. Les lambdas génèrent des types avec le nom du fichier et le numéro de ligne.

La **performance** est souvent meilleure avec les lambdas. `std::bind` stocke des copies de tous les arguments liés et peut introduire des indirections supplémentaires. Les lambdas permettent un contrôle fin des captures.

La **composition** de `std::bind` avec lui-même est confuse et sujette aux erreurs. Les lambdas imbriquées sont naturelles.

> 💡 **Recommandation** : dans du code nouveau, utilisez toujours des lambdas au lieu de `std::bind`. Réservez `std::bind` à la maintenance de code legacy existant.

---

## `std::move_only_function` (C++23)

C++23 introduit `std::move_only_function`, une alternative à `std::function` optimisée pour les callables move-only — c'est-à-dire les lambdas qui capturent des `std::unique_ptr` ou d'autres types non-copiables :

```cpp
#include <functional>

auto ptr = std::make_unique<int>(42);

// std::function COPIE le callable — impossible avec un unique_ptr capturé
// std::function<int()> bad = [p = std::move(ptr)]() { return *p; };  // ❌

// std::move_only_function DÉPLACE le callable — fonctionne
std::move_only_function<int()> good = [p = std::move(ptr)]() { return *p; };
std::print("{}\n", good());  // 42
```

### Différences avec `std::function`

`std::move_only_function` ne supporte pas la copie — on ne peut que la déplacer. En contrepartie, elle offre deux avantages.

Elle **accepte les callables non-copiables**, ce que `std::function` refuse. C'est sa raison d'être principale — sans elle, stocker une lambda capturant un `unique_ptr` dans un membre de classe nécessite des contorsions (`std::shared_ptr` comme workaround, ou un wrapper personnalisé).

Elle encode la **qualification const** dans le type. Avec `std::function<int()>`, l'`operator()` est toujours `const`, même si le callable sous-jacent est mutable — ce qui peut mener à des surprises. `std::move_only_function` distingue explicitement les deux :

```cpp
// Le callable DOIT être const-invocable
std::move_only_function<int() const> const_fn = [](){ return 42; };

// Le callable peut être mutable
std::move_only_function<int()> mut_fn = [x = 0]() mutable { return ++x; };
```

### Quand utiliser `std::move_only_function`

Utilisez-la lorsque le callable capture des ressources move-only et que vous n'avez pas besoin de copier la `function`. C'est le cas typique des callbacks de complétion dans le code asynchrone :

```cpp
class AsyncTask {
    std::move_only_function<void(Result)> on_complete_;
public:
    void set_completion(std::move_only_function<void(Result)> handler) {
        on_complete_ = std::move(handler);
    }

    void finish(Result r) {
        if (on_complete_) {
            std::move(on_complete_)(std::move(r));  // Appel unique
        }
    }
};
```

---

## `std::function_ref` (C++26)

C++26 apporte `std::function_ref`, un wrapper **non-owning** de callable. Contrairement à `std::function` qui **possède** (et potentiellement alloue) le callable, `std::function_ref` ne stocke qu'une **référence** vers lui — comme un `std::string_view` pour les chaînes.

```cpp
// std::function_ref ne possède pas le callable — juste une vue
void process(const std::vector<int>& data, std::function_ref<bool(int)> pred) {
    for (int v : data) {
        if (pred(v)) std::print("{} ", v);
    }
}

// Utilisation — le callable vit dans la portée appelante
auto is_even = [](int x) { return x % 2 == 0; };
process(data, is_even);  // Pas d'allocation, pas de copie, pas d'indirection virtuelle
```

### Avantages sur `std::function`

`std::function_ref` n'alloue **jamais** sur le heap — il est garanti sans allocation dynamique. Son `sizeof` est minimal (typiquement deux pointeurs). Il n'y a pas de copie du callable. C'est le wrapper idéal pour les paramètres de fonction quand le callable n'a pas besoin d'être stocké au-delà de l'appel.

### Limitations

`std::function_ref` ne **possède** pas le callable. Il ne peut pas être stocké dans un membre de classe ou un conteneur — le callable doit vivre au moins aussi longtemps que la `function_ref`. C'est exactement la même relation qu'entre `std::string` et `std::string_view`.

---

## Arbre de décision : quel mécanisme choisir ?

Le choix entre les différents mécanismes dépend de deux axes : le callable doit-il être **stocké** (survit à l'appel), et le **type** est-il connu à la compilation ?

### Le callable est utilisé immédiatement (paramètre de fonction)

```
Le type peut être résolu à la compilation ?
├── Oui → Template (+ Concept C++20)          ← Performance maximale
│         void process(std::predicate<int> auto pred)
│
└── Non (compilation séparée, API binaire) ?
    ├── C++26 disponible → std::function_ref   ← Zéro allocation
    └── Sinon → const std::function&           ← Flexible mais coût indirect
```

### Le callable doit être stocké (membre, conteneur)

```
Le callable est-il copiable ?
├── Oui → std::function                       ← Choix par défaut
│
└── Non (capture unique_ptr, etc.)
    ├── C++23 disponible → std::move_only_function  ← Solution native
    └── Sinon → Wrapper avec shared_ptr             ← Workaround
```

### Résumé comparatif

| Mécanisme | Possession | Allocation | Inlining | Copie | Standard |
|---|---|---|---|---|---|
| Template | Non applicable | Aucune | Oui | Non applicable | C++98 |
| Template + Concept | Non applicable | Aucune | Oui | Non applicable | C++20 |
| `std::function` | Oui (owning) | SBO ou heap | Non | Oui | C++11 |
| `std::move_only_function` | Oui (owning) | SBO ou heap | Non | Non (move only) | C++23 |
| `std::function_ref` | Non (viewing) | Aucune | Possible | Non applicable | C++26 |
| Pointeur de fonction | Non | Aucune | Possible | Oui | C |
| `std::bind` | Oui | Possible | Rarement | Oui | C++11 (déconseillé) |

---

## Bonnes pratiques

**Préférez les templates** pour les fonctions qui acceptent un callable en paramètre et l'utilisent immédiatement. C'est la voie des algorithmes STL, et pour une bonne raison — zéro surcoût.

**Utilisez `std::function`** pour stocker des callables dans des membres de classe, des conteneurs, ou des API à compilation séparée. Acceptez le surcoût — il est le prix de la flexibilité.

**Passez `std::function` par `const&` ou par valeur** selon le contexte. Par `const&` si la fonction ne stocke pas le callable. Par valeur puis `std::move` si elle le stocke :

```cpp
// Ne stocke pas → const&
void execute_once(const std::function<void()>& fn) {
    fn();
}

// Stocke → valeur + move
class EventHandler {
    std::function<void(int)> handler_;
public:
    void set_handler(std::function<void(int)> h) {
        handler_ = std::move(h);
    }
};
```

**Ne faites pas de `std::function` un réflexe.** Le pattern le plus fréquent — passer une lambda à une fonction qui l'utilise immédiatement — ne nécessite pas `std::function`. Un template suffit et sera plus performant. Réservez `std::function` aux cas où le type doit être effacé.

**Migrez vers `std::move_only_function` (C++23)** quand vos callables capturent des ressources move-only. Migrez vers `std::function_ref` (C++26) pour les paramètres non-stockés quand votre base de code adopte C++26.

---

## Récapitulatif

| Concept | Description |
|---|---|
| Callable | Tout objet invocable avec `()` : fonctions, lambdas, foncteurs, pointeurs membres |
| `std::function` | Wrapper polymorphique owning — stocke n'importe quel callable, avec surcoût d'indirection |
| Type erasure | Mécanisme interne de `std::function` — efface le type concret derrière une interface uniforme |
| SBO | Small Buffer Optimization — évite l'allocation heap pour les petits callables |
| `std::invoke` | Appel uniforme de tout callable, y compris pointeurs vers membres |
| `std::bind` | Pré-application d'arguments — remplacé par les lambdas en C++ moderne |
| `std::move_only_function` | Version C++23 pour les callables non-copiables |
| `std::function_ref` | Version C++26 non-owning — vue légère sans allocation |
| Template + Concept | Alternative zéro-coût quand le type est résolu à la compilation |

---

*Ce chapitre sur les lambdas et la programmation fonctionnelle est maintenant complet. Le chapitre suivant explore les autres fonctionnalités majeures du C++ moderne.*

⏭️ [Nouveautés C++17/C++20/C++23/C++26](/12-nouveautes-cpp17-26/README.md)
