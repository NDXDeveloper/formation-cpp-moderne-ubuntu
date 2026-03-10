🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 11.3 — Utilisation avec les algorithmes STL

## Le duo naturel

Les algorithmes de la STL et les lambdas sont faits l'un pour l'autre. Les algorithmes expriment le **quoi** (trier, filtrer, transformer, accumuler), et les lambdas expriment le **comment** (le critère de tri, la condition du filtre, la transformation à appliquer). Avant les lambdas, cette personnalisation passait par des foncteurs ou des pointeurs de fonction — fonctionnels mais verbeux. Avec les lambdas, le comportement est défini **au point d'appel**, rendant le code à la fois plus concis et plus lisible.

Cette section explore les combinaisons les plus courantes entre lambdas et algorithmes STL. Les algorithmes eux-mêmes sont couverts en profondeur au chapitre 15, et les Ranges (C++20) qui simplifient encore davantage ces patterns sont détaillés en section 15.6.

---

## Prédicats : le cas d'usage fondamental

Un **prédicat** est une fonction qui retourne un booléen. C'est le type de lambda le plus utilisé avec la STL — il exprime une condition que les algorithmes utilisent pour prendre des décisions.

### Recherche avec `std::find_if`

`std::find_if` retourne un itérateur vers le premier élément satisfaisant le prédicat :

```cpp
std::vector<int> values = {12, 7, 25, 3, 18, 42, 9};

// Trouver le premier nombre supérieur à 20
auto it = std::find_if(values.begin(), values.end(), [](int v) {
    return v > 20;
});

if (it != values.end()) {
    std::print("Premier > 20 : {}\n", *it);  // 25
}
```

La version négative `std::find_if_not` cherche le premier élément qui **ne satisfait pas** le prédicat :

```cpp
// Trouver le premier nombre qui n'est pas pair
auto it = std::find_if_not(values.begin(), values.end(), [](int v) {
    return v % 2 == 0;
});
// *it == 7
```

### Comptage avec `std::count_if`

```cpp
std::vector<std::string> words = {"apple", "avocado", "banana", "apricot", "cherry"};

auto count = std::count_if(words.begin(), words.end(), [](const std::string& w) {
    return w.starts_with('a');
});

std::print("Mots commençant par 'a' : {}\n", count);  // 3
```

### Test de conditions avec `std::all_of`, `std::any_of`, `std::none_of`

Ces trois algorithmes testent si un prédicat est satisfait par tous, au moins un, ou aucun des éléments :

```cpp
std::vector<int> scores = {75, 82, 91, 68, 88};

bool all_passing = std::all_of(scores.begin(), scores.end(),
    [](int s) { return s >= 60; });
// true — tous au-dessus de 60

bool any_perfect = std::any_of(scores.begin(), scores.end(),
    [](int s) { return s == 100; });
// false — aucun score parfait

bool none_negative = std::none_of(scores.begin(), scores.end(),
    [](int s) { return s < 0; });
// true — aucun score négatif
```

Ces algorithmes court-circuitent — `all_of` s'arrête au premier `false`, `any_of` au premier `true`. Avec de grandes collections, cela peut faire une différence significative.

### Prédicats avec capture : le contexte au service de la logique

La puissance des lambdas-prédicats vient de la **capture**. Sans elle, un prédicat ne pourrait dépendre que de l'élément courant. Avec la capture, il peut intégrer n'importe quel contexte :

```cpp
struct Product {
    std::string name;
    double price;
    std::string category;
};

void find_affordable(const std::vector<Product>& catalog,
                     double budget,
                     const std::string& target_category) {
    auto it = std::find_if(catalog.begin(), catalog.end(),
        [&budget, &target_category](const Product& p) {
            return p.category == target_category && p.price <= budget;
        });

    if (it != catalog.end()) {
        std::print("Trouvé : {} à {:.2f}€\n", it->name, it->price);
    }
}
```

Le prédicat combine deux critères externes (`budget`, `target_category`) avec les données de l'élément — un pattern impossible avec un simple pointeur de fonction.

---

## Tri et ordonnancement

### `std::sort` avec comparateur lambda

`std::sort` accepte un comparateur qui définit l'ordre strict faible (*strict weak ordering*). Par défaut, il trie par ordre croissant avec `operator<`. Une lambda permet de personnaliser l'ordre :

```cpp
struct Employee {
    std::string name;
    int age;
    double salary;
};

std::vector<Employee> team = {
    {"Alice", 30, 85000},
    {"Bob", 25, 72000},
    {"Carol", 35, 95000},
    {"Dave", 28, 85000}
};

// Tri par salaire décroissant
std::sort(team.begin(), team.end(), [](const Employee& a, const Employee& b) {
    return a.salary > b.salary;
});
```

### Tri multi-critères

Les lambdas permettent d'exprimer des critères de tri composites de façon naturelle :

```cpp
// Tri par salaire décroissant, puis par nom croissant en cas d'égalité
std::sort(team.begin(), team.end(), [](const Employee& a, const Employee& b) {
    if (a.salary != b.salary) return a.salary > b.salary;
    return a.name < b.name;
});
// Carol(95k), Alice(85k), Dave(85k), Bob(72k)
// Alice et Dave ont le même salaire → triés par nom
```

Avec `std::tie` (disponible depuis C++11), les comparaisons multi-critères peuvent être simplifiées en une seule expression — la comparaison lexicographique des tuples fait le travail :

```cpp
std::sort(team.begin(), team.end(), [](const Employee& a, const Employee& b) {
    // Salaire décroissant (b avant a), puis nom croissant (a avant b)
    return std::tie(b.salary, a.name) < std::tie(a.salary, b.name);
});
```

### `std::stable_sort` — préserver l'ordre relatif

`std::stable_sort` garantit que les éléments égaux conservent leur ordre relatif d'origine. C'est important lorsqu'on trie par un critère secondaire après un tri principal :

```cpp
struct Employee2 {
    std::string name;
    std::string department;
    double salary;
};

std::vector<Employee2> staff = {
    {"Alice", "Engineering", 85000},
    {"Bob", "Marketing", 72000},
    {"Carol", "Engineering", 95000},
    {"Dave", "Marketing", 85000}
};

// D'abord trier par département (stable)
std::stable_sort(staff.begin(), staff.end(), [](const Employee2& a, const Employee2& b) {
    return a.department < b.department;
});

// Puis trier par salaire au sein de chaque département (stable)
std::stable_sort(staff.begin(), staff.end(), [](const Employee2& a, const Employee2& b) {
    return a.salary > b.salary;
});
// L'ordre par département est préservé dans chaque groupe de salaire
```

### `std::partial_sort` et `std::nth_element` — tri partiel

Quand on n'a pas besoin de trier toute la collection :

```cpp
std::vector<int> scores = {72, 95, 88, 61, 43, 99, 77, 84};

// Obtenir les 3 meilleurs scores
std::partial_sort(scores.begin(), scores.begin() + 3, scores.end(),
    [](int a, int b) { return a > b; });
// Les 3 premiers éléments sont {99, 95, 88} — le reste est dans un ordre indéterminé
```

---

## Transformation et projection

### `std::transform` — appliquer une fonction à chaque élément

`std::transform` applique une lambda à chaque élément et stocke le résultat dans un conteneur de destination :

```cpp
std::vector<std::string> names = {"alice", "bob", "carol"};  
std::vector<std::string> upper_names(names.size());  

std::transform(names.begin(), names.end(), upper_names.begin(),
    [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), ::toupper);
        return s;
    });
// upper_names = {"ALICE", "BOB", "CAROL"}
```

Transformation in-place — le conteneur source et destination peuvent être le même :

```cpp
std::vector<double> prices = {9.99, 24.50, 3.75, 19.90};

// Appliquer une remise de 10%
std::transform(prices.begin(), prices.end(), prices.begin(),
    [](double price) { return price * 0.9; });
// prices = {8.991, 22.05, 3.375, 17.91}
```

### `std::transform` binaire — combiner deux séquences

La version à deux itérateurs d'entrée combine les éléments de deux conteneurs :

```cpp
std::vector<std::string> first_names = {"Alice", "Bob", "Carol"};  
std::vector<std::string> last_names = {"Smith", "Jones", "Lee"};  
std::vector<std::string> full_names(first_names.size());  

std::transform(first_names.begin(), first_names.end(),
               last_names.begin(), full_names.begin(),
    [](const std::string& first, const std::string& last) {
        return first + " " + last;
    });
// full_names = {"Alice Smith", "Bob Jones", "Carol Lee"}
```

### `std::for_each` — effets de bord contrôlés

`std::for_each` est similaire à une boucle range-based `for`, mais avec la possibilité de capturer un état via la lambda :

```cpp
struct Stats {
    int count = 0;
    double sum = 0.0;
    double min = std::numeric_limits<double>::max();
    double max = std::numeric_limits<double>::lowest();
};

std::vector<double> measurements = {23.5, 19.8, 27.3, 22.1, 25.6};

Stats stats;  
std::for_each(measurements.begin(), measurements.end(),  
    [&stats](double v) {
        ++stats.count;
        stats.sum += v;
        stats.min = std::min(stats.min, v);
        stats.max = std::max(stats.max, v);
    });

std::print("Count: {}, Avg: {:.1f}, Range: [{:.1f}, {:.1f}]\n",
           stats.count, stats.sum / stats.count, stats.min, stats.max);
// Count: 5, Avg: 23.7, Range: [19.8, 27.3]
```

> 💡 En C++ moderne, une boucle range-based `for` est souvent préférable à `std::for_each` pour la lisibilité. `std::for_each` devient intéressant avec les **politiques d'exécution parallèle** (section 15.7) où il permet de paralléliser le traitement sans changer la structure du code.

---

## Filtrage et suppression

### L'idiome erase-remove

Avant C++20, supprimer des éléments d'un conteneur séquentiel nécessitait la combinaison de `std::remove_if` (qui déplace les éléments à garder vers le début) et de `erase` (qui supprime effectivement les éléments déplacés à la fin) :

```cpp
std::vector<int> data = {1, -3, 5, -7, 2, -1, 8, -4};

// Supprimer les nombres négatifs (C++17 et avant)
data.erase(
    std::remove_if(data.begin(), data.end(), [](int v) { return v < 0; }),
    data.end()
);
// data = {1, 5, 2, 8}
```

### `std::erase_if` (C++20) — la forme simplifiée

C++20 introduit `std::erase_if` comme fonction libre qui combine les deux opérations :

```cpp
std::vector<int> data = {1, -3, 5, -7, 2, -1, 8, -4};

auto removed = std::erase_if(data, [](int v) { return v < 0; });

std::print("Supprimés : {}, Restants : {}\n", removed, data.size());
// Supprimés : 4, Restants : 4
```

C'est plus concis, moins sujet aux erreurs, et disponible pour tous les conteneurs standards (`vector`, `list`, `deque`, `map`, `set`, etc.).

### `std::copy_if` — filtrer dans un nouveau conteneur

Plutôt que de modifier le conteneur source, `std::copy_if` copie les éléments satisfaisant le prédicat dans un conteneur de destination :

```cpp
std::vector<int> source = {1, -3, 5, -7, 2, -1, 8, -4};  
std::vector<int> positives;  

std::copy_if(source.begin(), source.end(), std::back_inserter(positives),
    [](int v) { return v > 0; });
// positives = {1, 5, 2, 8}
// source est inchangé
```

### `std::partition` — séparer sans supprimer

`std::partition` réorganise les éléments en deux groupes — ceux qui satisfont le prédicat en premier, les autres ensuite — et retourne un itérateur vers la frontière :

```cpp
std::vector<int> data = {1, -3, 5, -7, 2, -1, 8, -4};

auto boundary = std::partition(data.begin(), data.end(),
    [](int v) { return v >= 0; });

std::print("Positifs : ");  
for (auto it = data.begin(); it != boundary; ++it) std::print("{} ", *it);  
std::print("\nNégatifs : ");  
for (auto it = boundary; it != data.end(); ++it) std::print("{} ", *it);  
// L'ordre relatif au sein de chaque groupe n'est pas garanti
```

---

## Réduction et accumulation

### `std::accumulate` — accumulation séquentielle

`std::accumulate` (dans `<numeric>`) réduit une séquence à une valeur unique en appliquant une opération binaire :

```cpp
std::vector<int> values = {1, 2, 3, 4, 5};

// Somme classique (la lambda est facultative ici — operator+ est le défaut)
int sum = std::accumulate(values.begin(), values.end(), 0);

// Produit
int product = std::accumulate(values.begin(), values.end(), 1,
    [](int acc, int val) { return acc * val; });
// product = 120

// Concaténation de chaînes
std::vector<std::string> words = {"C++", "is", "powerful"};  
auto sentence = std::accumulate(words.begin(), words.end(), std::string{},  
    [](const std::string& acc, const std::string& word) {
        return acc.empty() ? word : acc + " " + word;
    });
// sentence = "C++ is powerful"
```

### Réduction avec changement de type

`std::accumulate` permet un accumulateur de type différent des éléments — la lambda effectue la conversion :

```cpp
struct Order {
    std::string product;
    int quantity;
    double unit_price;
};

std::vector<Order> orders = {
    {"Widget", 5, 12.50},
    {"Gadget", 2, 45.00},
    {"Doohickey", 10, 3.75}
};

double total = std::accumulate(orders.begin(), orders.end(), 0.0,
    [](double acc, const Order& order) {
        return acc + order.quantity * order.unit_price;
    });
// total = 5*12.50 + 2*45.00 + 10*3.75 = 190.0
```

### `std::reduce` (C++17) — accumulation potentiellement parallèle

`std::reduce` est le pendant parallélisable de `std::accumulate`. La différence clé : l'opération doit être **commutative et associative** car l'ordre d'application n'est pas garanti :

```cpp
#include <numeric>
#include <execution>

std::vector<double> data(1'000'000, 1.0);

// Réduction parallèle
double sum = std::reduce(std::execution::par, data.begin(), data.end(), 0.0,
    [](double a, double b) { return a + b; });
```

La parallélisation est couverte en détail en section 15.7. L'essentiel à retenir ici : si votre lambda est associative et commutative, préférez `std::reduce` à `std::accumulate` pour pouvoir bénéficier de la parallélisation ultérieurement.

---

## Génération et remplissage

### `std::generate` — remplir avec une lambda stateful

`std::generate` appelle une lambda **sans argument** pour chaque élément et stocke le résultat :

```cpp
std::vector<int> fibonacci(10);

// Lambda mutable avec état interne
std::generate(fibonacci.begin(), fibonacci.end(),
    [a = 0, b = 1]() mutable {
        int current = a;
        a = b;
        b = current + b;
        return current;
    });
// fibonacci = {0, 1, 1, 2, 3, 5, 8, 13, 21, 34}
```

C'est un cas d'utilisation naturel des init captures (section 11.1.4) combinées avec `mutable` — la lambda maintient un état interne qui évolue à chaque appel.

```cpp
// Générer des identifiants uniques
std::vector<std::string> ids(5);

std::generate(ids.begin(), ids.end(),
    [n = 0]() mutable {
        return std::format("ID-{:04d}", ++n);
    });
// ids = {"ID-0001", "ID-0002", "ID-0003", "ID-0004", "ID-0005"}
```

### `std::generate_n` — générer un nombre fixe d'éléments

```cpp
std::vector<double> samples;  
samples.reserve(100);  

std::mt19937 rng(42);  // Générateur de nombres aléatoires  
std::normal_distribution<double> dist(0.0, 1.0);  

std::generate_n(std::back_inserter(samples), 100,
    [&rng, &dist]() { return dist(rng); });
// 100 échantillons d'une distribution normale
```

---

## Composition de lambdas avec les algorithmes

### Enchaîner des algorithmes

Un pattern courant consiste à enchaîner plusieurs algorithmes, chacun avec sa propre lambda :

```cpp
struct LogEntry {
    std::string level;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
};

void analyze_logs(std::vector<LogEntry>& logs) {
    // 1. Trier par timestamp
    std::sort(logs.begin(), logs.end(),
        [](const LogEntry& a, const LogEntry& b) {
            return a.timestamp < b.timestamp;
        });

    // 2. Compter les erreurs
    auto error_count = std::count_if(logs.begin(), logs.end(),
        [](const LogEntry& e) { return e.level == "ERROR"; });

    // 3. Extraire les messages d'erreur
    std::vector<std::string> error_messages;
    std::for_each(logs.begin(), logs.end(),
        [&error_messages](const LogEntry& e) {
            if (e.level == "ERROR") {
                error_messages.push_back(e.message);
            }
        });

    // 4. Supprimer les entrées de debug
    std::erase_if(logs, [](const LogEntry& e) { return e.level == "DEBUG"; });

    std::print("Erreurs : {}, Logs restants : {}\n", error_count, logs.size());
}
```

Cet enchaînement fonctionne mais est verbeux — chaque étape itère sur la collection. Les **Ranges** (section 15.6) permettent de composer ces opérations en un seul pipeline évalué paresseusement :

```cpp
// Aperçu Ranges (C++20) — couvert en section 15.6
namespace rv = std::views;

auto error_messages = logs
    | rv::filter([](const LogEntry& e) { return e.level == "ERROR"; })
    | rv::transform([](const LogEntry& e) { return e.message; });
```

### Réutiliser des lambdas entre algorithmes

Une lambda stockée dans une variable `auto` peut être réutilisée avec plusieurs algorithmes :

```cpp
auto is_expired = [now = std::chrono::system_clock::now()](const Session& s) {
    return s.last_activity + s.timeout < now;
};

// Utilisation avec différents algorithmes
auto expired_count = std::count_if(sessions.begin(), sessions.end(), is_expired);  
auto first_expired = std::find_if(sessions.begin(), sessions.end(), is_expired);  
std::erase_if(sessions, is_expired);  
```

La lambda `is_expired` est définie une seule fois avec son contexte (le timestamp `now`), puis réutilisée. C'est plus maintenable que de dupliquer la condition dans chaque appel.

---

## Lambdas comme comparateurs pour les conteneurs associatifs

Les conteneurs ordonnés (`std::set`, `std::map`) et les tas (`std::priority_queue`) acceptent un comparateur personnalisé. En C++11, il fallait passer le type du comparateur comme paramètre de template — lourd avec les lambdas dont le type est anonyme. La solution classique utilise `decltype` :

```cpp
auto cmp = [](const std::string& a, const std::string& b) {
    return a.size() < b.size();  // Tri par longueur
};

// Le type de la lambda doit être passé comme paramètre de template
std::set<std::string, decltype(cmp)> words_by_length(cmp);

words_by_length.insert("apple");  
words_by_length.insert("hi");  
words_by_length.insert("banana");  
// Itération : "hi", "apple", "banana" (ordre par longueur)
```

Depuis C++20, les lambdas sans capture sont **default-constructible**, ce qui simplifie la syntaxe — on n'a plus besoin de passer l'instance au constructeur :

```cpp
// C++20 : lambda sans capture, default-constructible
auto cmp = [](const std::string& a, const std::string& b) {
    return a.size() < b.size();
};

std::set<std::string, decltype(cmp)> words_by_length;  // Pas besoin de passer cmp  
words_by_length.insert("apple");  
```

Pour `std::priority_queue` :

```cpp
auto priority = [](const Task& a, const Task& b) {
    return a.priority < b.priority;  // Plus haute priorité en premier
};

std::priority_queue<Task, std::vector<Task>, decltype(priority)> queue(priority);
```

---

## Lambdas et algorithmes numériques

L'en-tête `<numeric>` contient des algorithmes qui bénéficient particulièrement des lambdas.

### `std::inner_product` — produit scalaire généralisé

```cpp
std::vector<double> prices = {10.0, 25.0, 5.0};  
std::vector<int> quantities = {3, 1, 8};  

// Produit scalaire classique : somme des (prix × quantité)
double total = std::inner_product(
    prices.begin(), prices.end(),
    quantities.begin(), 0.0);
// total = 10*3 + 25*1 + 5*8 = 95.0

// Version personnalisée : trouver l'écart maximum
double max_diff = std::inner_product(
    prices.begin(), prices.end(),
    quantities.begin(), 0.0,
    [](double acc, double diff) { return std::max(acc, diff); },  // reduce
    [](double price, int qty) { return std::abs(price - qty); }   // transform
);
```

### `std::adjacent_difference` avec lambda

```cpp
std::vector<int> readings = {100, 103, 98, 105, 102};  
std::vector<int> changes(readings.size());  

std::adjacent_difference(readings.begin(), readings.end(), changes.begin(),
    [](int current, int previous) { return current - previous; });
// changes = {100, 3, -5, 7, -3}  (le premier élément est inchangé)
```

### `std::transform_reduce` (C++17) — map-reduce en une seule passe

`std::transform_reduce` combine transformation et réduction, et supporte la parallélisation :

```cpp
struct Pixel {
    uint8_t r, g, b;
};

std::vector<Pixel> image = { /* ... */ };

// Calculer la luminosité moyenne (map: pixel→luminosité, reduce: somme)
double total_lum = std::transform_reduce(
    image.begin(), image.end(),
    0.0,
    std::plus<>{},                                                // reduce: somme
    [](const Pixel& p) { return 0.299*p.r + 0.587*p.g + 0.114*p.b; }  // transform
);
double avg_lum = total_lum / image.size();
```

---

## Lambdas avec `std::sort` et stabilité : attention aux pièges

### Le comparateur doit définir un ordre strict faible

Un comparateur pour `std::sort` doit respecter les propriétés de l'ordre strict faible : irréflexivité (`comp(a, a)` est `false`), asymétrie (si `comp(a, b)` alors `!comp(b, a)`), et transitivité. Violer ces propriétés est un **comportement indéfini** — et le bug peut être intermittent :

```cpp
// ⚠️ UNDEFINED BEHAVIOR : utiliser <= au lieu de < viole l'irréflexivité
std::sort(data.begin(), data.end(), [](int a, int b) {
    return a <= b;  // ❌ comp(x, x) est true → UB
});

// ✅ Correct : < est un ordre strict faible
std::sort(data.begin(), data.end(), [](int a, int b) {
    return a < b;
});
```

### Captures mutables et algorithmes : danger

Les algorithmes de la STL peuvent copier le prédicat ou le comparateur. Si la lambda est `mutable` et maintient un état interne, les copies créent des états **indépendants**, ce qui peut produire des résultats incohérents :

```cpp
// ⚠️ Résultat potentiellement incorrect
auto counter_pred = [count = 0](int) mutable {
    return ++count <= 3;  // Accepter les 3 premiers
};

// std::remove_if PEUT copier le prédicat — le compteur est réinitialisé dans la copie
auto new_end = std::remove_if(data.begin(), data.end(), counter_pred);
// Le nombre d'éléments supprimés peut ne pas être celui attendu
```

La règle est de ne pas dépendre d'un état mutable interne à la lambda pour les prédicats passés aux algorithmes, sauf si l'algorithme garantit qu'il n'effectue pas de copie (ce qui est rarement spécifié dans le standard).

---

## Lambdas génériques et algorithmes : le combo universel

Les lambdas génériques (section 11.2) permettent d'écrire des prédicats et des transformations réutilisables avec n'importe quel type d'élément :

```cpp
// Comparateur générique réutilisable
auto descending = [](const auto& a, const auto& b) { return a > b; };

std::vector<int> ints = {3, 1, 4, 1, 5};  
std::vector<double> doubles = {2.7, 1.4, 3.1};  
std::vector<std::string> words = {"banana", "apple", "cherry"};  

std::sort(ints.begin(), ints.end(), descending);  
std::sort(doubles.begin(), doubles.end(), descending);  
std::sort(words.begin(), words.end(), descending);  
```

Un pattern avancé consiste à créer des **fabriques de lambdas** — des fonctions qui retournent des lambdas configurées :

```cpp
auto field_comparator = [](auto member_ptr) {
    return [member_ptr](const auto& a, const auto& b) {
        return std::invoke(member_ptr, a) < std::invoke(member_ptr, b);
    };
};

std::sort(team.begin(), team.end(), field_comparator(&Employee::salary));  
std::sort(team.begin(), team.end(), field_comparator(&Employee::name));  
std::sort(team.begin(), team.end(), field_comparator(&Employee::age));  
```

Une seule fabrique produit des comparateurs pour n'importe quel champ de n'importe quel type de structure.

---

## Avant/Après : la lisibilité en perspective

Pour mesurer l'apport des lambdas, comparons le même traitement avec et sans lambdas.

**Objectif** : à partir d'une liste de mesures, extraire celles qui dépassent un seuil, les trier par valeur décroissante, et calculer leur moyenne.

### Sans lambdas (C++03 style)

```cpp
struct AboveThreshold {
    double threshold;
    AboveThreshold(double t) : threshold(t) {}
    bool operator()(double v) const { return v > threshold; }
};

struct DescOrder {
    bool operator()(double a, double b) const { return a > b; }
};

struct Summer {
    double& sum;
    int& count;
    Summer(double& s, int& c) : sum(s), count(c) {}
    void operator()(double v) { sum += v; ++count; }
};

void analyze(const std::vector<double>& data, double threshold) {
    std::vector<double> above;
    std::copy_if(data.begin(), data.end(), std::back_inserter(above),
                 AboveThreshold(threshold));

    std::sort(above.begin(), above.end(), DescOrder());

    double sum = 0;
    int count = 0;
    std::for_each(above.begin(), above.end(), Summer(sum, count));

    if (count > 0) {
        std::print("Average of {} values above {}: {:.2f}\n",
                   count, threshold, sum / count);
    }
}
```

### Avec lambdas (C++14+)

```cpp
void analyze(const std::vector<double>& data, double threshold) {
    std::vector<double> above;
    std::copy_if(data.begin(), data.end(), std::back_inserter(above),
        [threshold](double v) { return v > threshold; });

    std::sort(above.begin(), above.end(),
        [](double a, double b) { return a > b; });

    double sum = 0;
    std::for_each(above.begin(), above.end(), [&sum](double v) { sum += v; });

    if (!above.empty()) {
        std::print("Average of {} values above {}: {:.2f}\n",
                   above.size(), threshold, sum / above.size());
    }
}
```

Trois classes supprimées, la logique est au point d'utilisation, et le code tient en un seul bloc lisible de haut en bas.

---

## Récapitulatif : algorithmes courants et patterns de lambdas

| Algorithme | Type de lambda | Signature typique | Usage |
|---|---|---|---|
| `find_if` / `count_if` | Prédicat unaire | `(const T&) → bool` | Recherche / comptage conditionnel |
| `all_of` / `any_of` / `none_of` | Prédicat unaire | `(const T&) → bool` | Test de condition globale |
| `sort` / `stable_sort` | Comparateur binaire | `(const T&, const T&) → bool` | Ordonnancement personnalisé |
| `transform` | Transformation unaire | `(const T&) → U` | Projection / conversion |
| `for_each` | Action avec effets de bord | `(const T&) → void` | Accumulation, side effects |
| `remove_if` / `erase_if` | Prédicat unaire | `(const T&) → bool` | Suppression conditionnelle |
| `copy_if` | Prédicat unaire | `(const T&) → bool` | Filtrage dans un nouveau conteneur |
| `partition` | Prédicat unaire | `(const T&) → bool` | Séparation en deux groupes |
| `accumulate` / `reduce` | Opération binaire | `(Acc, const T&) → Acc` | Réduction à une valeur unique |
| `generate` | Générateur sans argument | `() → T` | Remplissage avec état |
| `transform_reduce` | Transform + reduce | Transform: `(const T&) → U`, Reduce: `(U, U) → U` | Map-reduce en une passe |

---


⏭️ [std::function et callable objects](/11-lambdas/04-std-function.md)
