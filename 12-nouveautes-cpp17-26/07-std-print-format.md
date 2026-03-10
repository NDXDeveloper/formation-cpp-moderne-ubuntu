🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 12.7 std::print et std::format (C++23) : Formatage moderne ⭐

> 💡 *Cette section est la couverture approfondie du système de formatage C++. Pour la prise en main rapide et les premiers exemples, voir **section 2.7** (introduction à std::print).*

## La fin d'un compromis historique

Pendant trente ans, les développeurs C++ ont dû choisir entre deux systèmes d'affichage, chacun avec des défauts majeurs :

- **`printf`** (hérité du C) — Concis et rapide, mais dangereux. Les format strings ne sont pas vérifiées à la compilation, les types ne sont pas contrôlés, et une erreur de correspondance entre `%d` et le type réel de l'argument est un comportement indéfini silencieux. Passer un `std::string` à `%s` compile, crash à l'exécution.

- **`std::cout`** (C++98) — Type-safe grâce à la surcharge d'opérateurs, mais verbeux et difficile à formater. Aligner des colonnes, formater des nombres, mélanger texte et valeurs — tout cela requiert des manipulateurs (`std::setw`, `std::setprecision`, `std::fixed`) qui détruisent la lisibilité et dont l'état est persistant entre les appels.

C++20 introduit `std::format` et C++23 ajoute `std::print`, apportant enfin un système qui combine le meilleur des deux mondes : la concision de `printf`, la type-safety de `std::cout`, et une puissance de formatage qui dépasse les deux. Ce système est directement inspiré de la bibliothèque `{fmt}` de Victor Zverovich, adoptée dans le standard après des années de succès en tant que bibliothèque tierce.

## std::format : construire des chaînes formatées

`std::format` (C++20, header `<format>`) retourne un `std::string` formaté à partir d'une format string et d'arguments :

```cpp
#include <format>
#include <string>

std::string msg = std::format("Bonjour, {} ! Vous avez {} messages.", "Alice", 42);
// msg == "Bonjour, Alice ! Vous avez 42 messages."
```

Les accolades `{}` sont des *placeholders* (emplacements de substitution). Chaque `{}` est remplacé par l'argument correspondant, dans l'ordre. Le type de chaque argument est détecté automatiquement — pas de `%d`, `%s`, `%f` à mémoriser.

### Vérification à la compilation

Contrairement à `printf`, la format string est vérifiée **à la compilation**. Une erreur de format est détectée avant l'exécution :

```cpp
std::format("{} {} {}", 1, 2);       // Erreur de compilation : 3 placeholders, 2 arguments
std::format("{:d}", "hello");          // Erreur de compilation : 'd' n'est pas valide pour un string
```

Cette vérification est rendue possible par le fait que la format string doit être une constante connue à la compilation (un littéral de chaîne ou un objet `consteval`). C'est un progrès fondamental par rapport à `printf`, où ces erreurs ne se manifestent qu'à l'exécution — souvent en production.

## std::print : afficher directement

`std::print` (C++23, header `<print>`) combine `std::format` et l'écriture sur la sortie standard en une seule opération. C'est le remplacement naturel de `std::cout <<` et de `printf` pour l'affichage :

```cpp
#include <print>

std::print("Température : {:.1f}°C\n", 22.5);
// Sortie : Température : 22.5°C

std::println("Ligne avec retour à la ligne automatique : {}", 42);
// Sortie : Ligne avec retour à la ligne automatique : 42\n
```

`std::println` est la variante qui ajoute automatiquement un retour à la ligne — l'équivalent de `fmt::println` ou du `println!` de Rust. En pratique, `std::println` est la forme la plus utilisée pour l'affichage de diagnostic et de logging.

### Écrire vers un flux spécifique

Par défaut, `std::print` écrit sur `stdout`. On peut spécifier un flux en premier argument :

```cpp
#include <print>
#include <cstdio>

std::print(stderr, "Erreur : fichier '{}' introuvable\n", filename);

// Avec un FILE* quelconque
FILE* log_file = std::fopen("app.log", "w");
std::print(log_file, "[{}] {}\n", timestamp, message);
std::fclose(log_file);
```

### std::print vs std::format : quand utiliser lequel

`std::print` est optimisé pour l'affichage direct — il écrit dans le flux sans créer de `std::string` intermédiaire, ce qui évite une allocation. `std::format` est nécessaire quand on veut stocker, manipuler ou transmettre la chaîne formatée :

```cpp
// Affichage direct → std::print (pas d'allocation intermédiaire)
std::print("Score : {}\n", score);

// Construction d'une chaîne → std::format
std::string log_entry = std::format("[{}] {} — score: {}", timestamp, name, score);
logger.write(log_entry);
```

## La syntaxe de formatage en détail

La puissance du système réside dans sa mini-syntaxe de formatage, placée après les deux-points `:` à l'intérieur des accolades. La forme générale est :

```
{[index]:[fill][align][sign][#][0][width][.precision][type]}
```

Chaque composant est optionnel. Explorons-les un par un.

### Indexation des arguments

Par défaut, les arguments sont consommés dans l'ordre. On peut aussi les référencer par index (0-based) :

```cpp
std::print("{0} a {1} ans. {0} habite à Paris.", "Alice", 30);
// Alice a 30 ans. Alice habite à Paris.
```

L'indexation permet de réutiliser un même argument plusieurs fois sans le passer en double. On ne peut pas mélanger indexation automatique et manuelle dans une même format string.

### Largeur et alignement

La largeur minimum du champ est spécifiée par un entier. L'alignement contrôle le positionnement de la valeur dans cet espace :

```cpp
// Alignement à gauche (<), à droite (>), centré (^)
std::print("[{:<10}]", "hello");     // [hello     ]
std::print("[{:>10}]", "hello");     // [     hello]
std::print("[{:^10}]", "hello");     // [  hello   ]

// Caractère de remplissage personnalisé
std::print("[{:*<10}]", "hello");    // [hello*****]
std::print("[{:*>10}]", "hello");    // [*****hello]
std::print("[{:-^20}]", "TITRE");    // [-------TITRE--------]
```

La largeur peut aussi être dynamique, en la passant comme argument :

```cpp
int width = 15;
std::print("{:>{}}", "hello", width);   // [          hello]
```

### Formatage des entiers

```cpp
// Bases numériques
std::print("{:d}", 255);     // 255        (décimal — par défaut)
std::print("{:b}", 255);     // 11111111   (binaire)
std::print("{:o}", 255);     // 377        (octal)
std::print("{:x}", 255);     // ff         (hexadécimal minuscule)
std::print("{:X}", 255);     // FF         (hexadécimal majuscule)

// Préfixe de base avec #
std::print("{:#b}", 255);    // 0b11111111
std::print("{:#o}", 255);    // 0377
std::print("{:#x}", 255);    // 0xff

// Séparateur de milliers (locale-dépendant)
std::print("{:L}", 1'000'000);   // 1,000,000 (ou 1.000.000 selon la locale)

// Padding avec des zéros
std::print("{:08d}", 42);    // 00000042
std::print("{:08x}", 255);   // 000000ff
```

### Formatage des nombres flottants

```cpp
// Notation par défaut (la plus lisible automatiquement)
std::print("{}", 3.14159);         // 3.14159

// Notation fixe
std::print("{:f}", 3.14159);       // 3.141590
std::print("{:.2f}", 3.14159);     // 3.14

// Notation scientifique
std::print("{:e}", 3.14159);       // 3.141590e+00
std::print("{:.3e}", 0.000042);    // 4.200e-05

// Notation générale (choisit la plus compacte)
std::print("{:g}", 3.14159);       // 3.14159
std::print("{:g}", 0.000042);      // 4.2e-05

// Largeur + précision
std::print("{:10.2f}", 3.14159);   // [      3.14]
std::print("{:010.2f}", 3.14159);  // [0000003.14]

// Signe explicite
std::print("{:+.2f}", 3.14);       // +3.14
std::print("{:+.2f}", -3.14);      // -3.14
std::print("{: .2f}", 3.14);       // espace devant les positifs : " 3.14"
```

### Formatage des chaînes

```cpp
// Troncature avec la précision
std::print("{:.5}", "Hello, World!");     // Hello

// Largeur + troncature
std::print("{:10.5}", "Hello, World!");   // Hello     (5 chars, cadré dans 10)

// Échappement (C++23) — utile pour le debugging
std::print("{:?}", "Hello\tWorld\n");     // "Hello\tWorld\n"
```

Le spécificateur `?` (C++23) produit une représentation « debug » de la chaîne, avec les caractères spéciaux échappés et la chaîne entourée de guillemets. C'est l'équivalent du `{:?}` de Rust, particulièrement utile pour les logs et le debugging.

### Formatage des booléens et pointeurs

```cpp
// Booléens : affichés en texte par défaut
std::print("{}", true);        // true
std::print("{}", false);       // false

// Pointeurs
int x = 42;
std::print("{}", static_cast<void*>(&x));    // 0x7ffd5e8c1abc (adresse)
```

### Formatage des chrono types (C++20)

Le système de formatage s'intègre avec `<chrono>` pour formater directement les dates et durées :

```cpp
#include <chrono>
#include <print>

auto now = std::chrono::system_clock::now();
std::print("Date : {:%Y-%m-%d %H:%M:%S}\n", now);
// Date : 2026-03-10 14:30:45

auto duration = std::chrono::hours(2) + std::chrono::minutes(30);
std::print("Durée : {:%H:%M}\n", duration);
// Durée : 02:30
```

Les spécificateurs `%Y`, `%m`, `%d`, `%H`, `%M`, `%S` suivent la convention `strftime`, familière aux développeurs C et Python.

## Rendre ses propres types formattables

L'un des aspects les plus puissants du système est l'extensibilité : on peut rendre n'importe quel type utilisateur compatible avec `std::format` et `std::print` en spécialisant `std::formatter`.

### Spécialisation basique

```cpp
#include <format>
#include <string>

struct Point {
    double x, y;
};

// Spécialisation de std::formatter pour Point
template <>
struct std::formatter<Point> {
    // parse() : interpréter les spécificateurs de format (après le ':')
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();   // Pas de spécificateur personnalisé
    }

    // format() : écrire la représentation formatée
    auto format(const Point& p, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "({:.2f}, {:.2f})", p.x, p.y);
    }
};
```

Le type est maintenant utilisable dans toute format string :

```cpp
Point origin{0.0, 0.0};
Point target{3.14, 2.72};

std::print("De {} vers {}\n", origin, target);
// De (0.00, 0.00) vers (3.14, 2.72)

std::string s = std::format("Position : {}", target);
// s == "Position : (3.14, 2.72)"
```

### Spécialisation avec format specs personnalisés

On peut interpréter ses propres spécificateurs de format. Voici un `Point` qui supporte un mode compact (`c`) et un mode verbose (`v`) :

```cpp
template <>
struct std::formatter<Point> {
    char mode = 'c';   // 'c' = compact, 'v' = verbose

    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && (*it == 'c' || *it == 'v')) {
            mode = *it;
            ++it;
        }
        return it;
    }

    auto format(const Point& p, std::format_context& ctx) const {
        if (mode == 'v') {
            return std::format_to(ctx.out(), "Point(x={:.2f}, y={:.2f})", p.x, p.y);
        }
        return std::format_to(ctx.out(), "({:.2f}, {:.2f})", p.x, p.y);
    }
};

// Utilisation :
Point p{3.14, 2.72};
std::print("{:c}\n", p);    // (3.14, 2.72)
std::print("{:v}\n", p);    // Point(x=3.14, y=2.72)
std::print("{}\n", p);      // (3.14, 2.72) — mode compact par défaut
```

### Hériter d'un formatter existant

Pour les types qui se représentent naturellement comme un type standard (chaîne, entier, etc.), on peut hériter du `formatter` de ce type pour réutiliser toute sa logique de formatage :

```cpp
struct UserId {
    uint64_t value;
};

template <>
struct std::formatter<UserId> : std::formatter<uint64_t> {
    auto format(const UserId& id, std::format_context& ctx) const {
        // Réutilise tout le formatage de uint64_t (largeur, padding, base, etc.)
        return std::formatter<uint64_t>::format(id.value, ctx);
    }
};

// Supporte automatiquement : {:08x}, {:>20}, {:b}, etc.
UserId user{12345};
std::print("User #{:08x}\n", user);    // User #00003039
```

## std::format_to : écrire sans allocation

`std::format` retourne un `std::string`, ce qui implique une allocation. Pour les cas où on souhaite écrire dans un buffer existant, `std::format_to` écrit directement vers un itérateur de sortie :

```cpp
#include <format>
#include <vector>
#include <string>
#include <iterator>

// Écrire dans un vector<char> existant
std::vector<char> buffer;
std::format_to(std::back_inserter(buffer), "x={}, y={}", 10, 20);
// buffer contient les caractères de "x=10, y=20"

// Écrire dans un string existant (append)
std::string log;
std::format_to(std::back_inserter(log), "[INFO] {}\n", "Démarrage");
std::format_to(std::back_inserter(log), "[INFO] {}\n", "Prêt");
// Une seule allocation pour log, pas une par appel

// Écrire dans un buffer de taille fixe
char fixed_buf[64];
auto result = std::format_to_n(fixed_buf, sizeof(fixed_buf) - 1, "Score: {}", 42);
*result.out = '\0';   // Terminer manuellement la chaîne
```

`std::format_to_n` est la variante qui limite le nombre de caractères écrits — utile pour les buffers de taille fixe dans du code embarqué ou système.

### std::formatted_size

Pour connaître la taille nécessaire avant d'allouer :

```cpp
auto size = std::formatted_size("Le résultat est {:.4f}", 3.14159);
// size == 23 — nombre de code units (octets UTF-8) qui seraient produits
```

## Performance : format vs printf vs cout

Le système `std::format`/`std::print` a été conçu avec la performance comme objectif de premier plan. Voici le positionnement relatif des trois approches.

### Vitesse

Dans les benchmarks publiés par l'auteur de `{fmt}`, `std::format` est comparable ou supérieur à `printf` pour la plupart des patterns de formatage, et significativement plus rapide que `std::cout` avec des manipulateurs. L'avantage vient de l'absence de parsing à l'exécution de la format string (la vérification est faite à la compilation) et de l'écriture directe sans synchronisation de flux C++ (pour `std::print`).

`std::print` est particulièrement efficace car il peut écrire directement dans le descripteur de fichier sans passer par le mécanisme de buffering de `std::ostream`. Sur les plateformes qui le supportent, `std::print` détecte si la sortie est un terminal Unicode et effectue la conversion d'encodage appropriée.

### Sécurité de type

C'est l'avantage décisif sur `printf`. Une erreur de type dans une format string `printf` est un comportement indéfini — crash, corruption de données, vulnérabilité de sécurité (format string attacks). Avec `std::format`, c'est une erreur de compilation.

```cpp
// printf : compile, crash ou corruption à l'exécution
printf("%d", "hello");            // Comportement indéfini silencieux

// std::format : erreur de compilation
std::format("{:d}", "hello");     // Erreur : 'd' invalide pour const char*
```

### Tableau comparatif

| Critère | `printf` | `std::cout` | `std::format` / `std::print` |
|---------|----------|-------------|-------------------------------|
| Type-safety | Non | Oui | Oui (+ compile-time) |
| Vérification compile-time | Non | Partielle | Oui |
| Extensibilité | Non | `operator<<` | `std::formatter` |
| Lisibilité | Bonne | Médiocre (manipulateurs) | Excellente |
| Performance | Rapide | Lente (sync, virtual) | Rapide (comparable à printf) |
| Format dynamique | Oui | Non | Possible (`std::vformat`) |
| i18n (réordonnancement) | Non standard | Non | Oui (indexation) |
| Encodage Unicode | Non | Partiel | Oui (C++23) |

## Format strings dynamiques : std::vformat

Par défaut, la format string doit être un littéral connu à la compilation. Dans les cas rares où elle est construite dynamiquement (internationalisation, configuration), `std::vformat` accepte une chaîne runtime :

```cpp
#include <format>
#include <string>

std::string pattern = load_translation("greeting");  // ex: "Bonjour, {} !"

// std::format(pattern, "Alice");  // Erreur : pattern n'est pas un compile-time string

// std::vformat accepte une string runtime
std::string result = std::vformat(pattern, std::make_format_args("Alice"));
```

La vérification de type ne peut pas se faire à la compilation dans ce cas — si la format string est invalide, une exception `std::format_error` est lancée à l'exécution. C'est le prix à payer pour la flexibilité ; il est donc recommandé de privilégier les format strings littérales et de n'utiliser `std::vformat` que pour les besoins réels d'internationalisation ou de configuration dynamique.

## Migration depuis printf et cout

### Depuis printf

La correspondance est quasi directe — les spécificateurs changent légèrement de syntaxe :

```cpp
// printf                           → std::print
printf("%d", 42);                   // std::print("{}", 42);
printf("%08x", 255);                // std::print("{:08x}", 255);
printf("%.2f", 3.14);              // std::print("{:.2f}", 3.14);
printf("%-20s", "hello");           // std::print("{:<20}", "hello");
printf("%+.1f", 3.14);             // std::print("{:+.1f}", 3.14);
```

Le bénéfice principal : les `std::string` passent directement comme arguments, sans `.c_str()`. Et les erreurs de type deviennent des erreurs de compilation.

### Depuis cout

Le gain en lisibilité est spectaculaire pour le formatage :

```cpp
// std::cout avec manipulateurs
std::cout << std::setw(10) << std::setfill('0') << std::hex << 255 << std::endl;

// std::print équivalent
std::print("{:010x}\n", 255);
```

De plus, les manipulateurs de `std::cout` ont un état persistant — `std::hex` affecte tous les entiers suivants jusqu'au prochain `std::dec`. Les format specs de `std::print` sont locales à chaque placeholder, ce qui élimine une classe entière de bugs.

## Relation avec la bibliothèque {fmt}

`std::format` et `std::print` sont la standardisation de la bibliothèque open-source `{fmt}` créée par Victor Zverovich. La bibliothèque `{fmt}` continue d'exister et reste pertinente pour plusieurs raisons :

- **Support des anciens standards** — `{fmt}` fonctionne avec C++11 et ultérieur, utile pour les projets qui ne peuvent pas encore migrer vers C++20/C++23.
- **Fonctionnalités avancées** — `{fmt}` inclut parfois des fonctionnalités qui n'ont pas encore été adoptées dans le standard (couleurs terminal, format de ranges, etc.).
- **Performance** — Les dernières versions de `{fmt}` peuvent être en avance sur les implémentations de la bibliothèque standard en termes d'optimisation.

La section 36.3 couvre `{fmt}` dans le contexte des outils CLI, notamment pour le formatage avec couleurs et styles.

Pour un projet nouveau ciblant C++23, `std::print` et `std::format` sont le choix naturel — pas besoin de dépendance externe. Pour un projet sur un standard plus ancien, `{fmt}` offre la même API avec le préfixe `fmt::` au lieu de `std::`.

## Bonnes pratiques

**Adopter `std::print` / `std::println` comme standard d'affichage.** Pour tout nouveau code C++23, c'est le remplacement naturel de `printf`, `std::cout`, et `fmt::print`. Il n'y a pas de raison de continuer à utiliser les anciennes API sauf contrainte de compatibilité.

**Préférer `std::format` à la concaténation de chaînes.** L'expression `"Résultat : " + std::to_string(x) + " (status: " + status + ")"` est moins lisible, moins performante (allocations multiples) et moins maintenable que `std::format("Résultat : {} (status: {})", x, status)`.

**Rendre ses types formattables.** Spécialiser `std::formatter` pour les types métier importants du projet. Cela rend le logging, le debugging et l'affichage naturels et cohérents dans tout le codebase.

**Utiliser `std::format_to` dans le code sensible aux allocations.** Pour le logging haute fréquence ou les boucles critiques, écrire dans un buffer pré-alloué avec `std::format_to` évite les allocations répétées de `std::format`.

**Utiliser le spécificateur `?` (C++23) pour le debugging.** `std::print("{:?}", value)` produit une représentation sans ambiguïté qui montre les caractères spéciaux, les guillemets, et les échappements — idéal pour les logs de diagnostic.

**Garder les format strings comme littéraux.** La vérification à la compilation est le principal avantage de sécurité du système. Ne recourir à `std::vformat` que pour les cas de format strings véritablement dynamiques (i18n, configuration).

---

>  
> 📎 [2.7 Introduction à std::print — Prise en main rapide](/02-toolchain-ubuntu/07-std-print.md)  
>  
> 📎 [36.3 fmt : Formatage avancé (pré-C++23)](/36-interfaces-cli/03-fmt.md)

⏭️ [std::expected (C++23) : Gestion d'erreurs sans exceptions](/12-nouveautes-cpp17-26/08-std-expected.md)
