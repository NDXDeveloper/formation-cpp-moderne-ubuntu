🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 3.4 — Portée des variables (Scope) et durée de vie (Lifetime)

> **Chapitre 3 — Types, Variables et Opérateurs** · Section 4 sur 5  
> Prérequis : [3.2 — Types primitifs, tailles et représentation mémoire](/03-types-variables-operateurs/02-types-primitifs.md)

---

## Introduction

Toute variable en C++ possède deux caractéristiques distinctes qui gouvernent son existence dans le programme : sa **portée** (*scope*) et sa **durée de vie** (*lifetime*). Ces deux notions sont souvent confondues par les débutants, car dans les cas simples elles coïncident. Mais elles désignent des concepts fondamentalement différents, et les distinguer est indispensable pour comprendre les mécanismes de construction, de destruction et de gestion des ressources en C++.

La **portée** est une propriété du **nom** : c'est la région du code source dans laquelle un identifiant (un nom de variable, de fonction, de type) est visible et utilisable. En dehors de sa portée, le nom n'existe pas — le compilateur ne le reconnaît pas.

La **durée de vie** est une propriété de l'**objet** : c'est la période pendant laquelle l'objet existe en mémoire, depuis sa construction jusqu'à sa destruction. Un objet peut exister en mémoire alors que son nom n'est plus accessible (pointeur vers un objet alloué dynamiquement), et inversement un nom peut être masqué par un autre sans que l'objet original soit détruit.

---

## Les différentes portées en C++

### Portée de bloc (block scope)

C'est la portée la plus courante. Une variable déclarée à l'intérieur d'un bloc — délimité par des accolades `{}` — est visible depuis sa déclaration jusqu'à la fin du bloc :

```cpp
void example() {
    // x n'existe pas encore ici

    int x = 42; // x est déclaré — sa portée commence

    if (x > 0) {
        int y = 10; // y est visible uniquement dans ce bloc if
        std::print("{} {}\n", x, y); // x et y sont accessibles
    }
    // y n'existe plus ici — hors de sa portée

    std::print("{}\n", x); // x est toujours accessible
    // std::print("{}\n", y); // ❌ Erreur : y n'est pas déclaré dans cette portée
}
// x n'existe plus ici
```

Chaque paire d'accolades crée un nouveau bloc, et chaque bloc définit une portée. Les blocs peuvent être imbriqués — une variable déclarée dans un bloc englobant est accessible dans tous les blocs imbriqués.

Les boucles `for`, `while` et les instructions `if` créent implicitement des blocs :

```cpp
for (int i = 0; i < 10; ++i) {
    // i est visible ici
    std::print("{}\n", i);
}
// i n'existe plus ici — sa portée est limitée à la boucle
```

Depuis C++17, `if` et `switch` peuvent inclure une déclaration dans leur condition, créant une variable dont la portée couvre l'ensemble de l'instruction :

```cpp
// C++17 : la variable 'it' a pour portée l'ensemble du if-else
if (auto it = map.find(key); it != map.end()) {
    std::print("Trouvé : {}\n", it->second);
} else {
    std::print("Non trouvé\n");
}
// it n'existe plus ici
```

Ce mécanisme est précieux pour limiter la portée d'une variable au strict nécessaire.

### Portée de fonction (function scope)

Seules les **étiquettes** (*labels*, utilisées avec `goto`) ont une portée de fonction — elles sont visibles dans toute la fonction où elles sont déclarées. Ce type de portée est marginal en C++ moderne car l'utilisation de `goto` est exceptionnelle.

### Portée de namespace (namespace scope)

Une variable ou une fonction déclarée dans un namespace (y compris le namespace global) est visible depuis sa déclaration jusqu'à la fin de l'unité de traduction (le fichier `.cpp` après préprocessing), et potentiellement dans d'autres unités si elle est déclarée dans un header :

```cpp
namespace config {
    int max_connections = 100; // Visible dans tout le namespace config
}

int global_counter = 0; // Portée globale (namespace global implicite)
```

Les variables à portée de namespace ont une durée de vie particulière — nous y reviendrons.

### Portée de classe (class scope)

Les membres d'une classe ont une portée liée à la classe. Ils sont accessibles via une instance de la classe (ou via `this` dans les méthodes), sous réserve des modificateurs d'accès (`public`, `private`, `protected`) :

```cpp
class Sensor {
    int id_;           // Portée de classe — accessible dans toutes les méthodes
    double value_;

public:
    void update(double new_value) {
        value_ = new_value; // id_ et value_ sont dans la portée de la classe
    }
};
```

La portée de classe sera examinée en détail au chapitre 6 (Classes et Encapsulation).

---

## Le masquage de noms (name shadowing)

Quand un bloc interne déclare une variable portant le même nom qu'une variable d'un bloc englobant, la variable interne **masque** (*shadows*) la variable externe. La variable externe existe toujours en mémoire, mais son nom n'est plus accessible dans le bloc interne :

```cpp
int x = 100; // Variable externe

void example() {
    std::print("{}\n", x); // 100 — la variable globale

    int x = 42; // Masque la variable globale
    std::print("{}\n", x); // 42 — la variable locale

    {
        int x = 7; // Masque la variable locale du bloc parent
        std::print("{}\n", x); // 7
    }

    std::print("{}\n", x); // 42 — retour à la variable locale de example()
}
```

Le masquage est légal en C++ mais il est une source fréquente de bugs subtils. Le compilateur émet un avertissement avec l'option `-Wshadow` (non incluse dans `-Wall` par défaut — il faut l'activer explicitement) :

```bash
g++ -Wall -Wextra -Wshadow main.cpp
```

**Recommandation** : activez `-Wshadow` dans vos projets. Le masquage intentionnel est extrêmement rare ; dans la grande majorité des cas, il résulte d'un choix de nom maladroit et le renommer est préférable.

---

## Les durées de vie (storage duration)

La norme C++ définit quatre catégories de durée de vie, chacune correspondant à un mécanisme de gestion mémoire distinct.

### Durée de vie automatique (automatic storage duration)

C'est la durée de vie par défaut des variables locales. L'objet est créé quand l'exécution atteint sa déclaration et détruit quand l'exécution quitte le bloc dans lequel il est déclaré :

```cpp
void process() {
    std::string message = "hello"; // Construction : allocation mémoire, copie des caractères

    if (true) {
        std::vector<int> data = {1, 2, 3}; // Construction
        // ... utilisation de data ...
    } // Destruction de data : libération de la mémoire du vecteur

    // ... utilisation de message ...
} // Destruction de message : libération de la mémoire de la chaîne
```

Les objets automatiques sont stockés sur la **pile** (*stack*). Leur allocation et libération sont extrêmement rapides (un simple ajustement du pointeur de pile). La destruction est **déterministe** — elle a lieu exactement à la sortie du bloc, dans l'**ordre inverse** de la construction :

```cpp
void deterministic_destruction() {
    std::print("Début\n");

    ResourceA a; // Construit en premier
    ResourceB b; // Construit en second
    ResourceC c; // Construit en troisième

} // Détruit dans l'ordre inverse : c, puis b, puis a
```

Ce comportement déterministe est la fondation du principe **RAII** (*Resource Acquisition Is Initialization*), l'un des idiomes les plus importants du C++, couvert en détail dans la section 6.3.

### Durée de vie statique (static storage duration)

Les objets à durée de vie statique existent depuis le démarrage du programme jusqu'à sa terminaison. Trois catégories de variables ont cette durée de vie :

**Variables globales** (déclarées en dehors de toute fonction) :

```cpp
int request_count = 0; // Créé avant main(), détruit après main()
```

**Variables locales statiques** (déclarées avec le mot-clé `static` à l'intérieur d'une fonction) :

```cpp
int generate_id() {
    static int next_id = 0; // Initialisé UNE SEULE FOIS, au premier appel
    return ++next_id;        // La valeur persiste entre les appels
}

// generate_id() → 1
// generate_id() → 2
// generate_id() → 3
```

La variable `next_id` est créée la première fois que l'exécution atteint sa déclaration, et elle persiste jusqu'à la fin du programme. Son nom a une portée de bloc (visible uniquement dans `generate_id`), mais sa durée de vie est statique. C'est un cas où **portée et durée de vie ne coïncident pas**.

Depuis C++11, l'initialisation des variables locales statiques est garantie **thread-safe** — le compilateur insère automatiquement les mécanismes de synchronisation nécessaires. C'est la base du pattern **Singleton Meyers** :

```cpp
Database& get_database() {
    static Database instance("connection_string"); // Thread-safe depuis C++11
    return instance;
}
```

**Variables statiques de classe** (déclarées avec `static` dans une classe) :

```cpp
class Connection {
    static int active_count; // Durée de vie statique, partagé entre toutes les instances
};
int Connection::active_count = 0; // Définition (nécessaire dans un .cpp)
```

### Durée de vie dynamique (dynamic storage duration)

Les objets à durée de vie dynamique sont créés explicitement par le programmeur (avec `new` ou des allocateurs) et détruits explicitement (avec `delete`). Ils sont stockés sur le **tas** (*heap*) :

```cpp
int* p = new int(42);  // Création sur le tas — durée de vie commence
// ... utilisation ...
delete p;               // Destruction — durée de vie se termine  
p = nullptr;            // Bonne pratique : éviter le dangling pointer  
```

La gestion manuelle de la durée de vie dynamique est la source la plus prolifique de bugs en C++ : fuites mémoire (oubli de `delete`), double libération (`delete` appelé deux fois), utilisation après libération (*use-after-free*). Le C++ moderne élimine ces problèmes grâce aux **smart pointers** (`std::unique_ptr`, `std::shared_ptr`), couverts au chapitre 9.

> 🔥 **En C++ moderne, on n'écrit quasiment jamais `new` et `delete` directement.** On utilise `std::make_unique` et `std::make_shared`, qui encapsulent l'allocation dynamique dans des objets RAII à durée de vie automatique. Cette approche est couverte en détail dans la section 9.4.

### Durée de vie thread-local (thread storage duration)

Les variables déclarées avec `thread_local` ont une instance distincte par thread. Chaque instance est créée quand le thread démarre et détruite quand le thread se termine :

```cpp
thread_local int request_id = 0; // Chaque thread a sa propre copie

void handle_request() {
    ++request_id; // Incrémente uniquement la copie du thread courant
    std::print("Thread request #{}\n", request_id);
}
```

`thread_local` est utile pour les caches par thread, les générateurs de nombres aléatoires, et les contextes de requête dans les serveurs concurrents. Ce sujet sera approfondi au chapitre 21 (Threads et Programmation Concurrente).

---

## Portée vs durée de vie : les cas de divergence

Dans les cas simples (variables locales automatiques), la portée et la durée de vie coïncident parfaitement : l'objet est créé quand le nom devient visible et détruit quand il cesse de l'être. Mais plusieurs situations font diverger les deux notions.

### Variables locales statiques

Comme vu précédemment, une variable `static` locale a une portée de bloc mais une durée de vie statique — elle survit bien au-delà de la portée de son nom :

```cpp
void counter() {
    static int n = 0; // Portée : le bloc de counter()
    ++n;               // Durée de vie : tout le programme
    std::print("{}\n", n);
}
// n n'est pas accessible ici (hors portée), mais il existe toujours en mémoire
```

### Objets dynamiques

Un objet alloué dynamiquement n'a **aucune portée** — il n'a pas de nom propre. C'est le pointeur qui le référence qui a une portée. L'objet lui-même vit sur le tas jusqu'à ce qu'il soit explicitement détruit :

```cpp
void create() {
    int* p = new int(42); // p a une portée de bloc
                           // L'objet *p a une durée de vie dynamique
    // Si on ne fait pas delete p avant la fin du bloc → fuite mémoire
}
// p est détruit (portée terminée), mais l'objet sur le tas existe toujours
// → Memory leak : l'objet est inaccessible mais non libéré
```

C'est la raison fondamentale pour laquelle les smart pointers existent : ils lient la durée de vie de l'objet dynamique à la portée du smart pointer qui le possède.

### Références et durée de vie prolongée

Les références constantes ont un comportement spécial : elles peuvent **prolonger la durée de vie** d'un objet temporaire :

```cpp
std::string create_greeting() {
    return "Bonjour le monde";
}

void example() {
    const std::string& ref = create_greeting();
    // L'objet temporaire retourné par create_greeting() est normalement détruit
    // immédiatement. Mais la liaison à une référence const prolonge sa durée de vie
    // jusqu'à la fin de la portée de ref.
    std::print("{}\n", ref); // ✅ Valide — l'objet temporaire est encore vivant
}
// L'objet temporaire est détruit ici, avec ref
```

Ce mécanisme de prolongation ne fonctionne qu'avec les références locales directes — il ne se propage pas à travers les appels de fonction ou les retours par référence.

---

## L'ordre de destruction : LIFO

Le C++ garantit que les objets automatiques d'un même bloc sont détruits dans l'**ordre inverse** de leur construction — le principe LIFO (*Last In, First Out*). Cette garantie est essentielle pour la gestion correcte des ressources :

```cpp
void ordered_destruction() {
    std::ofstream log("app.log");       // 1. Ouverture du fichier
    std::lock_guard lock(mutex);         // 2. Acquisition du verrou
    DatabaseTransaction tx(connection);  // 3. Début de la transaction

    // ... opérations ...

} // Destruction dans l'ordre inverse :
  // 3. tx détruit → transaction committée ou rollback
  // 2. lock détruit → verrou relâché
  // 1. log détruit → fichier fermé et flushé
```

L'ordre inverse est logique : les ressources acquises en dernier sont souvent celles qui dépendent des ressources acquises en premier. Le verrou doit être relâché avant de fermer le fichier ; la transaction doit être finalisée avant de relâcher le verrou.

---

## Initialisation des variables : le piège des variables non initialisées

En C++, les variables automatiques de types primitifs ne sont **pas** initialisées par défaut. Leur valeur est indéterminée — lire une variable non initialisée est un **comportement indéfini** :

```cpp
void dangerous() {
    int x;              // Non initialisé — contient une valeur indéterminée
    std::print("{}\n", x); // ❌ Comportement indéfini : lecture d'une variable non initialisée
}
```

Le compilateur n'émet pas toujours un avertissement pour ce cas (il le fait souvent avec `-Wall -Wextra -Wuninitialized`, mais pas dans tous les scénarios). Le résultat peut être n'importe quoi : la valeur qui se trouvait précédemment à cet emplacement de la pile, zéro, ou une valeur apparemment aléatoire.

Les objets de types composés (classes) sont dans une situation différente : si la classe possède un constructeur par défaut, celui-ci est appelé automatiquement :

```cpp
void safe() {
    std::string s;      // Constructeur par défaut appelé → chaîne vide ""
    std::vector<int> v; // Constructeur par défaut appelé → vecteur vide
    int x;              // ⚠️ Non initialisé — type primitif sans constructeur
}
```

**Recommandation** : initialisez toujours vos variables au moment de leur déclaration. L'utilisation de `auto` (qui exige un initialiseur) et de l'initialisation par accolades élimine cette catégorie de bugs :

```cpp
int x{};     // Initialisé à 0 (value-initialization avec accolades vides)  
int y = 0;   // Initialisé explicitement  
auto z = 0;  // auto exige un initialiseur — impossible d'oublier  
```

---

## Bonnes pratiques

**Déclarez les variables au plus près de leur première utilisation.** Contrairement au C89 qui imposait de déclarer toutes les variables en début de bloc, le C++ permet de déclarer une variable n'importe où. Déclarer au plus près réduit la portée, améliore la lisibilité et facilite la compréhension du flux de données :

```cpp
// ❌ Style C89 — toutes les variables en haut
void process(const std::vector<int>& data) {
    int sum;
    double average;
    std::size_t count;

    sum = 0;
    count = data.size();
    for (const auto& val : data) { sum += val; }
    average = static_cast<double>(sum) / count;
}

// ✅ Style C++ moderne — déclaration au plus près
void process(const std::vector<int>& data) {
    int sum = 0;
    for (const auto& val : data) { sum += val; }
    auto average = static_cast<double>(sum) / data.size();
}
```

**Réduisez la portée au strict minimum.** Utilisez les blocs `{}`, les `if` avec initialisation (C++17), et les boucles range-based pour limiter la visibilité des variables :

```cpp
// La variable 'lock' n'est nécessaire que pour la section critique
{
    std::lock_guard lock(mutex);
    shared_data.push_back(value);
} // lock détruit ici — le verrou est relâché au plus tôt
```

**Initialisez toujours vos variables.** Utilisez `{}` pour la value-initialization, `auto` pour forcer l'initialisation, et activez `-Wuninitialized` pour détecter les oublis.

**Préférez les variables automatiques aux variables dynamiques.** La durée de vie automatique est gérée par le compilateur — elle est gratuite, déterministe et sans risque de fuite. Réservez l'allocation dynamique aux cas où la durée de vie ne peut pas être liée à une portée lexicale (objets dont la taille n'est pas connue à la compilation, objets partagés entre composants, structures de données récursives).

**Évitez les variables globales mutables.** Elles introduisent un couplage implicite entre modules, rendent le code difficile à tester et créent des problèmes d'ordre d'initialisation entre unités de traduction (le fameux *static initialization order fiasco*). Si un état global est nécessaire, encapsulez-le dans une fonction retournant une référence vers une variable locale statique, ce qui garantit l'initialisation à la première utilisation :

```cpp
// ❌ Variable globale — ordre d'initialisation non garanti
Config global_config;

// ✅ Initialisation paresseuse — ordre garanti
Config& get_config() {
    static Config instance = load_config("/etc/app.yaml");
    return instance;
}
```

---

## En résumé

La portée et la durée de vie sont les deux dimensions de l'existence d'une variable en C++. La portée gouverne la **visibilité du nom** — où dans le code source on peut utiliser un identifiant. La durée de vie gouverne l'**existence de l'objet** — quand il est construit, quand il est détruit, et où il réside en mémoire (pile, tas, segment statique).

En C++ moderne, l'objectif est de minimiser les deux : déclarer les variables au plus près de leur utilisation (portée réduite) et préférer la durée de vie automatique (gestion par le compilateur). Quand la durée de vie doit dépasser la portée lexicale, les smart pointers (chapitre 9) offrent une gestion sûre et déterministe. Et quand la portée et la durée de vie se rejoignent — ce qui est le cas commun — le principe RAII (section 6.3) garantit que chaque ressource est correctement acquise et libérée, sans intervention manuelle du programmeur.

---


⏭️ [const, constexpr et consteval (C++20)](/03-types-variables-operateurs/05-const-constexpr-consteval.md)
