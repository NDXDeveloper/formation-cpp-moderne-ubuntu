🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 10.4 Perfect Forwarding avec std::forward

## Le problème : la perte de catégorie de valeur

Quand vous écrivez une fonction **wrapper** — une fonction qui reçoit des arguments et les transmet à une autre fonction — vous vous heurtez à un problème fondamental : la transmission des arguments peut **modifier leur catégorie de valeur**, ce qui déclenche des copies là où des déplacements étaient attendus.

Prenons un exemple concret. Vous voulez écrire une fonction `creer` qui transmet ses arguments au constructeur d'un objet :

```cpp
class Widget {
public:
    Widget(const std::string& nom) {
        std::print("[copie] Widget créé avec '{}'\n", nom);
    }
    Widget(std::string&& nom) {
        std::print("[move] Widget créé avec '{}'\n", nom);
    }
};

// Tentative naïve n°1 : paramètre par référence constante
template <typename T>
Widget creer_v1(const T& arg) {
    return Widget(arg);  // arg est TOUJOURS une lvalue → TOUJOURS copie
}

// Tentative naïve n°2 : paramètre par référence rvalue
template <typename T>
Widget creer_v2(T&& arg) {
    return Widget(arg);  // arg a un nom → c'est une lvalue → TOUJOURS copie !
}
```

La tentative n°1 échoue parce que `const T&` efface la distinction : tout devient une lvalue constante. La tentative n°2 est plus subtile — `arg` a le type `T&&`, mais comme nous l'avons vu en [section 10.1](/10-move-semantics/01-lvalues-rvalues.md), une **référence rvalue nommée est une lvalue**. Le résultat est le même : l'appel à `Widget(arg)` invoque systématiquement le constructeur de copie.

```cpp
std::string nom = "Alice";

creer_v2(nom);                   // T = string&, arg est une lvalue → copie ✅ (correct)
creer_v2(std::string("Bob"));    // T = string, arg est une lvalue → copie ❌ (devrait move)
creer_v2(std::move(nom));        // T = string, arg est une lvalue → copie ❌ (devrait move)
```

Le *perfect forwarding* résout ce problème : il transmet chaque argument **avec sa catégorie de valeur d'origine** intacte.

---

## Les deux piliers : forwarding references et std::forward

### Forwarding references (références universelles)

En [section 10.1](/10-move-semantics/01-lvalues-rvalues.md), nous avons mentionné que `T&&` dans un template déduit n'est **pas** une référence rvalue ordinaire — c'est une **forwarding reference**. Elle peut se lier aussi bien aux lvalues qu'aux rvalues.

Une forwarding reference existe quand **deux conditions** sont réunies :

1. Le type est de la forme `T&&` où `T` est un **paramètre template déduit**.
2. La déduction de type est effectuée directement à partir de l'argument.

```cpp
template <typename T>
void f(T&& arg);           // ✅ Forwarding reference — T est déduit

template <typename T>
void g(std::vector<T>&& v); // ❌ PAS une forwarding reference
                             //    c'est une référence rvalue vers vector<T>

template <typename T>
class Wrapper {
    void h(T&& arg);        // ❌ PAS une forwarding reference
                             //    T est fixé à l'instanciation de la classe,
                             //    pas déduit à l'appel de h
};

auto&& x = expr;            // ✅ Forwarding reference — auto est déduit
```

### Reference collapsing : les règles de résolution

Quand une forwarding reference est instanciée, le compilateur applique les **règles de reference collapsing** pour déterminer le type final. Ces règles résolvent les cas où des références s'empilent :

| Expression dans le code | T déduit comme | T&& devient | Résultat |
|---|---|---|---|
| `f(lvalue)` | `X&` | `X& &&` | `X&` (lvalue ref) |
| `f(rvalue)` | `X` | `X&&` | `X&&` (rvalue ref) |

La règle simplifiée : **si une `&` apparaît quelque part dans l'empilement, le résultat est `&`**. La seule façon d'obtenir `&&` est que les deux niveaux soient `&&`.

| Combinaison | Résultat |
|---|---|
| `T& &` | `T&` |
| `T& &&` | `T&` |
| `T&& &` | `T&` |
| `T&& &&` | `T&&` |

Concrètement, quand vous appelez `f(arg)` :

```cpp
template <typename T>
void f(T&& arg);

std::string s = "Hello";

f(s);                      // s est une lvalue
                            // T déduit comme string&
                            // T&& = string& && = string& → arg est une lvalue ref

f(std::string("Hello"));  // temporaire est une rvalue
                            // T déduit comme string
                            // T&& = string&& → arg est une rvalue ref

f(std::move(s));           // std::move(s) est une rvalue
                            // T déduit comme string
                            // T&& = string&& → arg est une rvalue ref
```

La forwarding reference **capture** la catégorie de valeur dans le type `T`. L'information n'est pas perdue — elle est encodée : `T` contient `&` si l'argument est une lvalue, et ne le contient pas si l'argument est une rvalue.

---

## std::forward : restaurer la catégorie d'origine

Le type `T` contient l'information sur la catégorie de valeur, mais `arg` reste une lvalue (il a un nom). Il faut un mécanisme pour **restaurer** la catégorie d'origine au moment de la transmission. C'est le rôle de `std::forward<T>` :

- Si `T` est `X&` (l'argument original était une lvalue) → `std::forward<T>(arg)` retourne une **lvalue reference** → copie.
- Si `T` est `X` (l'argument original était une rvalue) → `std::forward<T>(arg)` retourne une **rvalue reference** → déplacement.

```cpp
template <typename T>
Widget creer(T&& arg) {
    return Widget(std::forward<T>(arg));  // ✅ Catégorie préservée
}

std::string nom = "Alice";

creer(nom);                   // T = string& → forward retourne string& → copie ✅
creer(std::string("Bob"));    // T = string  → forward retourne string&& → move ✅
creer(std::move(nom));        // T = string  → forward retourne string&& → move ✅
```

C'est le *perfect forwarding* : chaque argument est transmis exactement comme il a été reçu, sans perte d'information.

### L'implémentation de std::forward

Comme `std::move`, `std::forward` est un cast conditionnel. Voici une version simplifiée :

```cpp
// Cas lvalue : T est X& → retourne X&
template <typename T>
constexpr T&& forward(std::remove_reference_t<T>& arg) noexcept {
    return static_cast<T&&>(arg);
    // Si T = X&  → static_cast<X& &&>(arg) → static_cast<X&>(arg)  → lvalue
    // Si T = X   → static_cast<X&&>(arg)                            → rvalue
}
```

Le paramètre template `T` est **toujours spécifié explicitement** — c'est une différence majeure avec `std::move` qui déduit son paramètre template :

```cpp
std::forward<T>(arg);    // ✅ T spécifié explicitement — obligatoire
std::forward(arg);       // ❌ Ne compile pas — T ne peut pas être déduit
```

C'est logique : `std::forward` a besoin de connaître `T` pour savoir si l'argument original était une lvalue ou une rvalue. Cette information est dans `T`, pas dans `arg`.

---

## std::move vs std::forward : quand utiliser lequel

Les deux fonctions sont des casts vers des références rvalue, mais leur intention est différente :

| Aspect | `std::move` | `std::forward<T>` |
|---|---|---|
| **Intention** | « Je n'ai plus besoin de cet objet » | « Transmets cet argument tel qu'il a été reçu » |
| **Résultat** | Toujours une rvalue | Lvalue ou rvalue, selon T |
| **Contexte** | Partout — variables locales, membres, retours | Exclusivement dans les templates avec forwarding references |
| **Paramètre template** | Déduit automatiquement | Spécifié explicitement (obligatoire) |
| **Inconditionnel / conditionnel** | Inconditionnel — toujours rvalue | Conditionnel — dépend de T |

La règle est simple :

- Vous avez une **forwarding reference** `T&& arg` → utilisez `std::forward<T>(arg)`.
- Vous avez une **référence rvalue concrète** `Widget&& arg` ou une variable locale → utilisez `std::move(arg)`.
- Vous avez une **lvalue** que vous voulez abandonner → utilisez `std::move(arg)`.

```cpp
// ✅ forward dans un template avec forwarding reference
template <typename T>
void wrapper(T&& arg) {
    destination(std::forward<T>(arg));
}

// ✅ move dans une fonction concrète avec rvalue reference
void consommer(Widget&& w) {
    stockage.push_back(std::move(w));
}

// ❌ forward sans forwarding reference — absurde
void mauvais(Widget&& w) {
    // std::forward<Widget>(w);  // Techniquement valide mais confus
    std::move(w);                // ✅ Utilisez move ici
}
```

---

## Cas d'usage : les fonctions factory

Le perfect forwarding est la technique qui rend possible `std::make_unique`, `std::make_shared`, `emplace_back`, et toute fonction factory qui construit un objet en transmettant des arguments à son constructeur.

### Reconstruire make_unique

Voici une implémentation simplifiée de `std::make_unique` qui illustre le perfect forwarding en action :

```cpp
template <typename T, typename... Args>
std::unique_ptr<T> mon_make_unique(Args&&... args) {
    return std::unique_ptr<T>(
        new T(std::forward<Args>(args)...)
    );
}
```

Décomposons :

- `Args&&... args` — un **parameter pack** de forwarding references. Chaque argument est capturé avec sa catégorie de valeur encodée dans son type `Args`.
- `std::forward<Args>(args)...` — chaque argument est retransmis au constructeur de `T` avec sa catégorie d'origine restaurée.

```cpp
struct Config {
    Config(std::string nom, int version) { /* ... */ }
};

std::string nom = "prod";

// Chaque argument est forwarded individuellement :
mon_make_unique<Config>(nom, 42);
// Args = {string&, int}
// forward<string&>(nom) → lvalue → copie de nom
// forward<int>(42) → rvalue → move (trivial pour int)

mon_make_unique<Config>(std::move(nom), 42);
// Args = {string, int}
// forward<string>(nom) → rvalue → move de nom
// forward<int>(42) → rvalue → move (trivial pour int)
```

### emplace_back : construction in-place

`std::vector::emplace_back` utilise le même mécanisme pour construire un élément directement dans le buffer du vector, sans aucun objet intermédiaire :

```cpp
std::vector<std::pair<std::string, int>> vec;

std::string cle = "alpha";

// push_back : construit une paire, puis la copie/déplace dans le vector
vec.push_back(std::make_pair(cle, 42));

// emplace_back : forward les arguments directement au constructeur de pair
vec.emplace_back(std::move(cle), 42);
// Le constructeur de pair est appelé in-place dans le buffer du vector
// Aucun objet pair temporaire n'est créé
```

Sous le capot, `emplace_back` ressemble à ceci :

```cpp
template <typename T, typename Alloc>
template <typename... Args>
void vector<T, Alloc>::emplace_back(Args&&... args) {
    // ... gestion de la capacité ...
    ::new (end_ptr) T(std::forward<Args>(args)...);  // Placement new + forward
    ++size_;
}
```

---

## Cas d'usage : les wrappers transparents

Le perfect forwarding est indispensable pour écrire des **wrappers** qui ajoutent un comportement (logging, timing, validation) sans altérer la sémantique des appels.

### Wrapper avec logging

```cpp
template <typename Func, typename... Args>
decltype(auto) avec_log(const std::string& label, Func&& func, Args&&... args) {
    std::print("[LOG] Début de '{}'\n", label);

    decltype(auto) resultat = std::invoke(
        std::forward<Func>(func),
        std::forward<Args>(args)...
    );

    std::print("[LOG] Fin de '{}'\n", label);
    return resultat;
}

// Utilisation — la sémantique des arguments est parfaitement préservée
std::string nom = "Alice";
auto widget = avec_log("création", creer_widget, std::move(nom));
// nom est déplacé dans creer_widget, pas copié
```

Quelques points à noter :

- `decltype(auto)` préserve le type de retour exact (référence ou valeur) de la fonction appelée.
- `std::invoke` est une abstraction qui appelle n'importe quel callable (fonction, lambda, méthode membre, foncteur).
- Sans `std::forward`, tous les arguments seraient transmis par lvalue — les rvalues deviendraient des copies inutiles.

### Wrapper avec mesure de temps

```cpp
template <typename Func, typename... Args>
decltype(auto) chrono_mesure(Func&& func, Args&&... args) {
    auto debut = std::chrono::high_resolution_clock::now();

    decltype(auto) resultat = std::invoke(
        std::forward<Func>(func),
        std::forward<Args>(args)...
    );

    auto fin = std::chrono::high_resolution_clock::now();
    auto duree = std::chrono::duration_cast<std::chrono::microseconds>(fin - debut);
    std::print("[PERF] Durée : {} µs\n", duree.count());

    return resultat;
}
```

---

## Forwarding dans les lambdas (C++20)

Avant C++20, le perfect forwarding dans les lambdas était verbeux et peu intuitif. C++20 introduit les **template lambdas** qui rendent le pattern naturel :

```cpp
// C++14 : forwarding approximatif avec auto&&
auto wrapper_14 = [](auto&&... args) {
    return destination(std::forward<decltype(args)>(args)...);
};

// C++20 : forwarding explicite avec template lambda
auto wrapper_20 = []<typename... Args>(Args&&... args) {
    return destination(std::forward<Args>(args)...);
};
```

La version C++20 est plus lisible et suit exactement le même pattern que les fonctions templates classiques. Le `decltype(args)` de la version C++14 fonctionne, mais il est moins explicite sur l'intention.

### Capture par forwarding dans les lambdas (C++20)

C++20 permet aussi de **capturer** un argument par forwarding dans une lambda pour une exécution différée :

```cpp
template <typename... Args>
auto creer_differe(Args&&... args) {
    // Capture par perfect forwarding avec init capture
    return [...args = std::forward<Args>(args)]() mutable {
        return Widget(std::move(args)...);
    };
}

std::string nom = "Alice";
auto factory = creer_differe(std::move(nom));
// nom a été déplacé dans la capture de la lambda

auto widget = factory();  // Widget construit à l'exécution de la lambda
```

L'init capture `...args = std::forward<Args>(args)` déplace les rvalues dans la capture et copie les lvalues — exactement ce qu'on attend.

---

## Pièges courants

### Piège n°1 : forward sans paramètre template explicite

```cpp
template <typename T>
void f(T&& arg) {
    g(std::forward(arg));       // ❌ Ne compile pas — T ne peut pas être déduit
    g(std::forward<T>(arg));    // ✅ T spécifié explicitement
}
```

### Piège n°2 : forward sur un argument qui n'est pas une forwarding reference

```cpp
void f(std::string&& s) {
    g(std::forward<std::string>(s));  // ⚠️ Fonctionne mais trompeur
    g(std::move(s));                   // ✅ Plus clair — c'est une rvalue ref concrète
}
```

`std::forward` sur une référence rvalue concrète fonctionne techniquement, mais elle masque l'intention. Réservez `std::forward` aux forwarding references.

### Piège n°3 : forwarder le même argument plusieurs fois

```cpp
template <typename T>
void f(T&& arg) {
    g(std::forward<T>(arg));   // Peut déplacer arg
    h(std::forward<T>(arg));   // ⚠️ arg est potentiellement vidé !
}
```

Si `arg` est une rvalue, le premier `std::forward` autorise un déplacement qui vide `arg`. Le second `std::forward` opère alors sur un objet dans un état indéterminé. C'est le même problème qu'utiliser un objet après `std::move`.

La solution est de ne forwarder un argument qu'**une seule fois**, lors de sa dernière utilisation :

```cpp
template <typename T>
void f(T&& arg) {
    g(arg);                        // Première utilisation — lvalue, pas de move
    h(std::forward<T>(arg));       // Dernière utilisation — forward ici
}
```

### Piège n°4 : confondre forwarding reference et rvalue reference

```cpp
template <typename T>
class Container {
public:
    void push(T&& value);  // ❌ PAS une forwarding reference !
                            //    T est fixé à l'instanciation du template de classe
};

Container<std::string> c;
std::string s = "Hello";
c.push(s);              // ❌ Ne compile pas — T&& = string&&, n'accepte que les rvalues

// Pour une forwarding reference, il faut un paramètre template de la MÉTHODE :
template <typename T>
class Container {
public:
    template <typename U>
    void push(U&& value);  // ✅ Forwarding reference — U est déduit à l'appel
};
```

---

## Résumé

| Concept | Détail |
|---|---|
| **Le problème** | Une référence rvalue nommée est une lvalue → la catégorie de valeur est perdue à la transmission |
| **Forwarding reference** | `T&&` avec T déduit — se lie aux lvalues et rvalues, encode la catégorie dans T |
| **Reference collapsing** | `& + && = &`, `&& + && = &&` — une seule `&` suffit à produire une lvalue ref |
| **std::forward&lt;T&gt;** | Cast conditionnel : lvalue si T est `X&`, rvalue si T est `X` |
| **std::move vs std::forward** | `move` = inconditionnel vers rvalue ; `forward` = conditionnel, préserve la catégorie |
| **Cas d'usage** | Factory functions (`make_unique`), emplace, wrappers transparents, lambdas C++20 |
| **Règle d'or** | Ne forwarder un argument qu'une seule fois — à sa dernière utilisation |

> **Règle pratique** — Si vous écrivez une fonction template qui reçoit `T&& arg` et transmet `arg` à une autre fonction, utilisez `std::forward<T>(arg)`. Si vous n'êtes pas dans un template avec déduction de type, utilisez `std::move`. Ne mélangez jamais les deux dans le même contexte.

⏭️ [Return Value Optimization (RVO) et Copy Elision](/10-move-semantics/05-rvo-copy-elision.md)
