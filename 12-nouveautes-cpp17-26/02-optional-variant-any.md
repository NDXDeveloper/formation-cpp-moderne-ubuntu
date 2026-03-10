🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 12.2 std::optional, std::variant, std::any (C++17)

## Les types vocabulaires : exprimer l'intention dans le type

C++17 introduit trois types utilitaires qui comblent des lacunes historiques du langage. `std::optional` représente une valeur qui peut être absente, `std::variant` une valeur qui peut être de plusieurs types, et `std::any` une valeur de type totalement arbitraire. Ces trois types partagent une philosophie commune : encoder dans le système de types des situations qui étaient auparavant gérées par des conventions fragiles — pointeurs nuls, unions non typées, `void*` ou valeurs sentinelles.

On les appelle souvent « types vocabulaires » (*vocabulary types*) parce qu'ils standardisent des concepts que chaque projet réinventait à sa manière. Leur introduction dans la bibliothèque standard a unifié les pratiques et rendu le code C++ plus expressif et plus sûr.

---

## std::optional : la valeur qui peut ne pas exister

### Le problème

Comment représenter l'absence d'une valeur en C++ ? Avant C++17, plusieurs approches coexistaient, toutes avec des défauts :

```cpp
// Approche 1 : Pointeur — mais qui possède la mémoire ? Peut-on retourner nullptr ?
int* find_score(const std::string& name);

// Approche 2 : Valeur sentinelle — mais -1 est-il invalide ou un score légitime ?
int find_score(const std::string& name);  // retourne -1 si non trouvé

// Approche 3 : Paramètre de sortie + booléen — lourd et peu naturel
bool find_score(const std::string& name, int& out_score);

// Approche 4 : Paire — sémantique floue
std::pair<int, bool> find_score(const std::string& name);
```

Chaque approche impose au développeur de connaître et respecter une convention non exprimée dans le type. Un pointeur nul oublié provoque un crash. Une valeur sentinelle mal documentée crée un bug silencieux. Un paramètre de sortie mélange entrées et sorties dans la signature.

### La solution : std::optional\<T\>

`std::optional<T>` encapsule une valeur de type `T` qui peut être présente ou absente. L'absence est un état explicite, vérifié à la compilation et à l'exécution :

```cpp
#include <optional>
#include <string>
#include <map>
#include <print>

std::map<std::string, int> scores = {{"Alice", 95}, {"Bob", 87}};

std::optional<int> find_score(const std::string& name) {
    if (auto it = scores.find(name); it != scores.end()) {
        return it->second;       // Retourne un optional contenant la valeur
    }
    return std::nullopt;          // Retourne un optional vide
}
```

Côté appelant, l'intention est limpide :

```cpp
auto result = find_score("Alice");

if (result) {                           // Test de présence (convertible en bool)
    std::print("Score : {}\n", *result); // Accès via opérateur *
}

// Ou avec value_or pour fournir une valeur par défaut :
int score = find_score("Charlie").value_or(0);
std::print("Score de Charlie : {}\n", score);  // 0 — Charlie n'existe pas
```

Le type `std::optional<int>` dans la signature de `find_score` communique immédiatement que la fonction peut légitimement ne rien retourner. Pas besoin de lire la documentation pour deviner quelle valeur magique signifie « absent ».

### Construction et affectation

Un `std::optional` peut être construit de plusieurs façons :

```cpp
#include <optional>
#include <string>

// Vide
std::optional<std::string> empty;                    // Vide par défaut
std::optional<std::string> also_empty = std::nullopt; // Explicitement vide

// Avec valeur
std::optional<std::string> name = "Alice";            // Conversion implicite
std::optional<int> score{95};                         // Construction directe

// Construction in-place (évite une copie/déplacement)
std::optional<std::string> built(std::in_place, 5, 'x');  // Contient "xxxxx"
```

`std::make_optional` offre une syntaxe concise avec déduction de type :

```cpp
auto name = std::make_optional<std::string>("Alice");
auto score = std::make_optional(95);  // std::optional<int>
```

L'affectation fonctionne naturellement :

```cpp
std::optional<int> value;
value = 42;              // Contient maintenant 42
value = std::nullopt;    // Vidé
value = 100;             // Contient maintenant 100
value.reset();           // Vidé (équivalent à = std::nullopt)
```

### Accès à la valeur

Quatre mécanismes permettent d'accéder à la valeur contenue :

```cpp
std::optional<std::string> name = "Alice";

// 1. Opérateur * (pas de vérification — UB si vide)
std::string s1 = *name;

// 2. Opérateur -> (accès aux membres, pas de vérification)
std::size_t len = name->size();

// 3. value() — lance std::bad_optional_access si vide
std::string s2 = name.value();

// 4. value_or(default) — retourne la valeur ou le défaut si vide
std::string s3 = name.value_or("inconnu");
```

Le choix entre ces mécanismes dépend du contexte. Après un test `if (opt)`, l'opérateur `*` est sûr et performant. `value()` est adapté quand l'absence est une erreur de programmation qu'on souhaite détecter via une exception. `value_or()` est idéal pour fournir un défaut propre sans branchement conditionnel.

### Combinaison avec les structured bindings

`std::optional` se combine naturellement avec les structured bindings vus en section 12.1 et les instructions `if` avec initialiseur :

```cpp
#include <map>
#include <string>
#include <optional>
#include <print>

std::optional<std::pair<std::string, int>> find_entry(const std::string& key) {
    static std::map<std::string, int> db = {{"timeout", 30}, {"retries", 3}};
    if (auto it = db.find(key); it != db.end()) {
        return *it;
    }
    return std::nullopt;
}

// Combinaison if-init + structured binding
if (auto entry = find_entry("timeout"); entry) {
    auto [key, value] = *entry;
    std::print("{} = {}\n", key, value);
}
```

### Monadic operations (C++23)

C++23 enrichit `std::optional` avec trois opérations monadiques qui permettent de chaîner des transformations sans branchements `if` imbriqués. Ce sont des ajouts inspirés des langages fonctionnels :

```cpp
#include <optional>
#include <string>
#include <charconv>
#include <print>

std::optional<std::string> get_env(const std::string& name) {
    // Simulation : retourne une valeur ou nullopt
    if (name == "PORT") return "8080";
    return std::nullopt;
}

std::optional<int> parse_int(const std::string& s) {
    int value{};
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
    if (ec == std::errc{}) return value;
    return std::nullopt;
}

// and_then : chaîne une opération qui retourne elle-même un optional
std::optional<int> port = get_env("PORT")
    .and_then(parse_int);               // std::optional<int>

// transform : applique une fonction à la valeur contenue (si présente)
std::optional<std::string> port_str = get_env("PORT")
    .transform([](const std::string& s) { return "Port: " + s; });

// or_else : fournit un optional alternatif si vide
std::optional<std::string> value = get_env("CUSTOM_PORT")
    .or_else([]() { return get_env("PORT"); });
```

Ces trois opérations éliminent un pattern courant de `if` imbriqués :

```cpp
// Avant C++23 : cascade de vérifications
auto env = get_env("PORT");
if (env) {
    auto parsed = parse_int(*env);
    if (parsed) {
        use_port(*parsed);
    }
}

// C++23 : pipeline lisible
get_env("PORT")
    .and_then(parse_int)
    .transform([](int p) { use_port(p); return p; });
```

---

## std::variant : l'union discriminée type-safe

### Le problème

Les `union` du C sont dangereuses : rien n'empêche de lire un membre qui n'est pas celui qui a été écrit. Le type actif n'est pas suivi, et accéder au mauvais membre est un comportement indéfini :

```cpp
// Union C : pas de sécurité de type
union Value {
    int i;
    double d;
    char str[32];
};

Value v;
v.i = 42;
double d = v.d;  // Comportement indéfini : on lit un int comme un double
```

Pour pallier ce problème, les projets C++ utilisaient typiquement une union accompagnée d'un discriminant manuel — un champ `enum` ou `int` indiquant le type actif. Ce pattern est fastidieux à maintenir, sujet aux erreurs, et n'offre aucune vérification à la compilation.

### La solution : std::variant\<Types...\>

`std::variant` est une union discriminée type-safe. Elle peut contenir exactement une valeur parmi les types listés, et le type actif est suivi automatiquement :

```cpp
#include <variant>
#include <string>
#include <print>

std::variant<int, double, std::string> value;

value = 42;                  // Contient un int
value = 3.14;                // Contient maintenant un double
value = "hello"s;            // Contient maintenant un string
```

Contrairement aux unions C, accéder au mauvais type est détecté et produit une exception :

```cpp
value = 42;
std::get<double>(value);     // Lance std::bad_variant_access !
```

### Accès à la valeur

Plusieurs mécanismes permettent d'accéder au contenu :

```cpp
std::variant<int, double, std::string> v = 42;

// 1. std::get<T> — par type (lance une exception si mauvais type)
int i = std::get<int>(v);

// 2. std::get<I> — par index (0 = int, 1 = double, 2 = string)
int j = std::get<0>(v);

// 3. std::get_if<T> — retourne un pointeur (nullptr si mauvais type)
if (auto* ptr = std::get_if<int>(&v)) {
    std::print("C'est un int : {}\n", *ptr);
}

// 4. index() — retourne l'index du type actif
std::size_t idx = v.index();  // 0, car le type actif est int
```

`std::get_if` est souvent préférable à `std::get` car il permet un test sans exception, suivant le même pattern qu'un `dynamic_cast` vers un pointeur.

### Le pattern visitor avec std::visit

`std::visit` est la manière idiomatique de traiter un `std::variant`. Il applique un callable (fonction, lambda, objet fonctionnel) à la valeur contenue, quel que soit son type actif :

```cpp
#include <variant>
#include <string>
#include <print>

using JsonValue = std::variant<int, double, bool, std::string>;

JsonValue val = "hello"s;

std::visit([](const auto& v) {
    std::print("Valeur : {}\n", v);
}, val);
```

La lambda générique `[](const auto& v)` est instanciée pour chaque type possible du variant. Le compilateur vérifie à la compilation que tous les types sont gérés.

Pour un traitement différencié par type, l'approche classique utilise un « overload set » — un objet qui combine plusieurs lambdas :

```cpp
// Helper pour créer un overload set (idiome standard)
template <class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

// Utilisation :
JsonValue val = 3.14;

std::visit(overloaded{
    [](int i)                { std::print("Entier : {}\n", i); },
    [](double d)             { std::print("Flottant : {:.2f}\n", d); },
    [](bool b)               { std::print("Booléen : {}\n", b); },
    [](const std::string& s) { std::print("Chaîne : '{}'\n", s); }
}, val);
```

Le compilateur garantit à la compilation que toutes les alternatives sont couvertes. Si un type du variant n'a pas de surcharge correspondante, la compilation échoue. C'est l'équivalent C++ du pattern matching exhaustif de Rust ou Haskell.

> 💡 **Depuis C++17**, la déduction de paramètres de template pour les classes (CTAD) dispense d'écrire les types du `overloaded` explicitement. En C++20, la ligne de déduction `template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;` n'est même plus nécessaire grâce à la déduction automatique des guides.

### Cas d'usage typiques

**Arbre syntaxique (AST) ou structures de données hétérogènes :**

```cpp
#include <variant>
#include <string>
#include <vector>
#include <memory>

// Nœud JSON simplifié
struct JsonNode;
using JsonArray = std::vector<JsonNode>;
using JsonObject = std::vector<std::pair<std::string, JsonNode>>;

struct JsonNode {
    std::variant<
        std::nullptr_t,     // null
        bool,               // true / false
        double,             // nombre
        std::string,        // chaîne
        JsonArray,          // tableau
        JsonObject          // objet
    > value;
};
```

**Machine à états :**

```cpp
#include <variant>

struct Idle {};
struct Connecting { std::string host; int port; };
struct Connected { int socket_fd; };
struct Error { std::string message; };

using ConnectionState = std::variant<Idle, Connecting, Connected, Error>;

ConnectionState state = Idle{};

// Transition
state = Connecting{"api.example.com", 443};

// Traitement de l'état
std::visit(overloaded{
    [](const Idle&)         { /* ... */ },
    [](const Connecting& c) { /* connect to c.host:c.port */ },
    [](const Connected& c)  { /* use c.socket_fd */ },
    [](const Error& e)      { /* log e.message */ }
}, state);
```

Ce pattern de machine à états par variant est supérieur à un `enum` + `switch` car le compilateur vérifie l'exhaustivité des cas et chaque état peut porter ses propres données.

### std::monostate : le variant par défaut

Un `std::variant` est toujours dans un état valide — il contient toujours une valeur de l'un des types listés. Par défaut, il est construit avec le premier type de la liste. Si ce premier type n'est pas constructible par défaut, la compilation échoue :

```cpp
struct NoDefault {
    NoDefault(int) {}  // Pas de constructeur par défaut
};

// std::variant<NoDefault, int> v;  // Erreur : NoDefault n'est pas default-constructible
```

`std::monostate` résout ce problème en servant de type « vide » constructible par défaut :

```cpp
std::variant<std::monostate, NoDefault, int> v;  // OK : contient monostate
```

`std::monostate` est aussi utile sémantiquement pour représenter un état « non initialisé » ou « pas de valeur », sans recourir à un `std::optional<std::variant<...>>`.

### variant et sécurité des exceptions

Que se passe-t-il si l'affectation d'une nouvelle valeur au variant lance une exception ? Le variant se retrouverait-il dans un état invalide ? Pour éviter cela, `std::variant` dispose d'un état spécial dit *valueless by exception*. Cet état survient uniquement quand une opération de modification (affectation, `emplace`) lance une exception après avoir détruit la valeur précédente mais avant d'avoir construit la nouvelle. On peut le détecter avec `valueless_by_exception()` :

```cpp
if (v.valueless_by_exception()) {
    // Situation exceptionnelle — ne devrait pas arriver en usage normal
}
```

En pratique, cet état est extrêmement rare. Il ne se produit qu'avec des types dont le constructeur de déplacement peut lancer des exceptions, ce qui est déconseillé par les bonnes pratiques modernes (voir section 10.3 sur les move constructors `noexcept`).

---

## std::any : le conteneur de type effacé

### Le problème

Parfois, on a besoin de stocker une valeur dont le type n'est pas connu à la compilation. C'est le cas dans les systèmes de plugins, les configurations dynamiques, les conteneurs hétérogènes génériques, ou les couches de « property bags ». Avant C++17, la solution était souvent `void*` — sûr tant qu'on ne se trompait pas de cast, c'est-à-dire jamais en pratique :

```cpp
// void* : aucune sécurité de type
void* data = new int(42);
double d = *static_cast<double*>(data);  // Comportement indéfini silencieux
```

### La solution : std::any

`std::any` peut contenir une valeur de n'importe quel type copiable. Le type est effacé (*type erasure*) mais l'information de type est conservée en interne pour permettre une extraction sûre :

```cpp
#include <any>
#include <string>
#include <print>

std::any value;

value = 42;                          // Contient un int
value = std::string("hello");        // Contient maintenant un string
value = 3.14;                        // Contient maintenant un double

// Extraction sûre avec std::any_cast
try {
    double d = std::any_cast<double>(value);
    std::print("Valeur : {}\n", d);
} catch (const std::bad_any_cast& e) {
    std::print("Mauvais type !\n");
}
```

Comme pour `std::variant`, il existe une version par pointeur qui évite l'exception :

```cpp
if (auto* ptr = std::any_cast<double>(&value)) {
    std::print("C'est un double : {}\n", *ptr);
}
```

### API essentielle

```cpp
#include <any>
#include <string>

std::any a = 42;

a.has_value();          // true — contient une valeur
a.type();               // typeid(int) — le type_info du contenu
a.type() == typeid(int) // true
a.reset();              // Vide le conteneur
a.has_value();          // false

// Construction in-place
a.emplace<std::string>(5, 'x');  // Contient "xxxxx"

// std::make_any pour la déduction
auto b = std::make_any<std::string>("hello");
```

### Quand utiliser std::any — et quand ne pas l'utiliser

`std::any` est l'outil le moins typé des trois types vocabulaires. Il doit être utilisé avec parcimonie, et seulement quand les alternatives sont insuffisantes :

**Cas légitimes :**

- Systèmes de propriétés dynamiques (*property maps*) où les types sont déterminés à l'exécution, par exemple un conteneur `std::map<std::string, std::any>` pour des configurations chargées depuis un fichier.
- Interfaces de plugins où le type exact de la valeur échangée n'est pas connu à la compilation.
- Couches d'abstraction qui doivent transporter des données opaques entre composants sans les interpréter.

**Préférer std::variant quand :**

- L'ensemble des types possibles est connu à la compilation. `std::variant` offre des vérifications de type à la compilation et un pattern matching exhaustif, ce que `std::any` ne peut pas fournir.

**Préférer un template ou un concept quand :**

- Le type est connu à la compilation mais varie entre les usages. La généricité statique est toujours préférable au type erasure dynamique en C++.

---

## Comparaison : optional vs variant vs any

Voici une synthèse des trois types pour clarifier leur positionnement :

```
┌─────────────────────────────────────────────────────────────────────┐
│                   std::optional<T>                                  │
│  "Il y a une valeur de type T, ou il n'y en a pas."                 │
│                                                                     │
│  - 1 seul type possible                                             │
│  - 2 états : valeur présente / absente                              │
│  - Alternative à : pointeurs nuls, valeurs sentinelles              │
│  - Surcoût mémoire : sizeof(T) + 1 byte (+ alignement)              │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│                   std::variant<T1, T2, ..., Tn>                     │
│  "Il y a une valeur, et c'est un des types listés."                 │
│                                                                     │
│  - N types connus à la compilation                                  │
│  - Toujours exactement 1 valeur active                              │
│  - Alternative à : union + enum, hiérarchie de classes              │
│  - Surcoût mémoire : max(sizeof(Ti)) + discriminant                 │
│  - Pattern matching exhaustif avec std::visit                       │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│                   std::any                                          │
│  "Il y a une valeur, de n'importe quel type."                       │
│                                                                     │
│  - Type inconnu à la compilation                                    │
│  - Pas de vérification statique possible                            │
│  - Alternative à : void*, boost::any                                │
│  - Surcoût : allocation heap possible (small buffer optimization)   │
│  - Vérification de type dynamique uniquement (any_cast)             │
└─────────────────────────────────────────────────────────────────────┘
```

La règle de choix est simple : utiliser le type le plus contraint qui répond au besoin. `std::optional` quand un seul type est en jeu, `std::variant` quand l'ensemble des types est fini et connu, `std::any` seulement en dernier recours quand aucune information de type n'est disponible à la compilation.

## Performance et allocation

**`std::optional<T>`** n'alloue jamais sur le heap. La valeur est stockée en place, à côté d'un flag interne. Le surcoût mémoire est typiquement de `sizeof(T)` plus un octet pour le flag, arrondi à l'alignement de `T`. Pour les types petits (entiers, pointeurs), le coût est négligeable.

**`std::variant<Ts...>`** n'alloue jamais sur le heap non plus. Sa taille est le maximum des `sizeof(Ti)` plus un discriminant (typiquement 1 à 4 octets selon le nombre de types). Toutes les valeurs sont stockées dans un même espace mémoire interne. C'est un type stack-friendly et cache-friendly.

**`std::any`** peut allouer sur le heap si la valeur dépasse un seuil interne (Small Buffer Optimization, ou SBO). Les implémentations typiques évitent l'allocation pour les types petits (≤ 16 ou 32 octets selon la bibliothèque standard), mais les types plus gros déclenchent un `new`. Ce surcoût d'allocation est une raison supplémentaire de préférer `std::variant` quand les types sont connus.

## Bonnes pratiques

**Utiliser `std::optional` pour tout retour qui peut légitimement échouer.** C'est le remplacement naturel des pointeurs nuls de retour, des valeurs sentinelles (`-1`, `""`, `nullptr`) et des paramètres de sortie par référence. Une signature `std::optional<T> find(...)` est auto-documentée.

**Utiliser `std::variant` pour les types somme avec ensemble fini.** Machine à états, nœuds d'AST, résultats d'opérations qui peuvent être de plusieurs types — `std::variant` apporte l'exhaustivité à la compilation via `std::visit`. Combiner avec le helper `overloaded` pour un traitement lisible.

**Réserver `std::any` aux situations de type erasure dynamique.** Si les types sont connus à la compilation, `std::variant` est toujours préférable. `std::any` est l'outil des frameworks, pas du code applicatif courant.

**Combiner `std::optional` et `std::variant` pour la gestion d'erreurs (pré-C++23).** Avant `std::expected` (section 12.8), le pattern `std::variant<ResultType, ErrorType>` ou `std::optional<ResultType>` était courant pour éviter les exceptions dans les chemins de code critiques en performance. C++23 formalise cette approche avec `std::expected`.

**Toujours vérifier avant d'accéder.** L'opérateur `*` de `std::optional` et `std::get<T>` de `std::variant` ne vérifient pas le type/la présence. Utiliser `has_value()` / `if (opt)` pour optional, et `std::get_if` ou `std::visit` pour variant. Les versions à exception (`value()`, `std::get<T>`) sont appropriées quand l'absence est une erreur de programmation.

---


⏭️ [std::span (C++20) : Vue sur données contiguës](/12-nouveautes-cpp17-26/03-std-span.md)
