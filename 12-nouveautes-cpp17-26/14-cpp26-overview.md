🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 12.14 C++26 : Standard ratifié — Les grandes nouveautés 🔥

## Le plus grand standard depuis C++20

C++26 est le standard le plus ambitieux depuis C++20 — et certains argumentent que son impact à long terme sera encore plus profond. Là où C++20 avait introduit quatre piliers (Concepts, Ranges, Coroutines, Modules), C++26 apporte trois fonctionnalités transformatrices qui changent la manière de concevoir, de vérifier et d'exécuter du code C++ : les **Contrats**, la **Réflexion statique** et **`std::execution`** (Senders/Receivers). Le **Pattern Matching**, bien qu'ayant reçu un fort soutien, n'a pas franchi le gel des fonctionnalités de C++26 et est désormais ciblé pour C++29.

Le processus de standardisation de C++26 a suivi le calendrier triennal établi par le comité ISO. Le gel des fonctionnalités (*feature freeze*) a eu lieu lors de la réunion de Sofia en juin 2025. Le brouillon a ensuite été envoyé en ballot international, et les commentaires ont été résolus lors des réunions de Kona (novembre 2025) et de Londres/Croydon (mars 2026). La publication officielle du standard est attendue dans le courant de l'année 2026.

## Les trois piliers de C++26

### Contrats : la sécurité par spécification

Les Contrats introduisent un mécanisme standard pour exprimer les préconditions, les postconditions et les assertions directement dans le code. C'est une fonctionnalité que la communauté C++ attend depuis plus de quinze ans — des propositions existaient dès C++11, et une première tentative avait été intégrée puis retirée de C++20 en raison de désaccords de design.

La version adoptée dans C++26 (basée sur P2900) offre un système pragmatique avec des sémantiques d'évaluation configurables :

```cpp
int sqrt_int(int x)
    pre (x >= 0)                    // Précondition : x doit être >= 0
    post (r: r >= 0)                // Postcondition : le résultat est >= 0
{
    contract_assert(x < 1'000'000); // Assertion dans le corps
    // ...
}
```

Les contrats ne sont pas de simples assertions améliorées. Ils peuvent être activés ou désactivés selon le mode de build (debug, release, audit), suivant quatre sémantiques d'évaluation : *ignore* (pas de vérification), *observe* (log en cas de violation, continue l'exécution), *enforce* (termine le programme en cas de violation), et *quick-enforce* (terminaison immédiate sans diagnostic). Cette flexibilité permet de les utiliser en production avec un coût contrôlé.

L'impact attendu est considérable — les contrats rendent les bugs détectables plus tôt, documentent les interfaces de manière vérifiable, et offrent une alternative légère aux exceptions pour la validation des préconditions.

> 📎 *La section 12.14.1 couvre les contrats en détail : syntaxe complète, sémantiques d'évaluation, interaction avec les exceptions, et exemples d'utilisation.*

### Réflexion statique : l'introspection à la compilation

La réflexion statique est peut-être la fonctionnalité la plus puissante de C++26 — et celle qui modifiera le plus profondément la manière d'écrire du code générique. Elle permet à un programme d'**examiner sa propre structure à la compilation** : inspecter les membres d'une classe, itérer sur les enumerateurs d'un enum, examiner les paramètres d'une fonction, et générer du code en conséquence.

La syntaxe repose sur l'opérateur de réflexion `^^` et un ensemble de fonctions `consteval` opérant sur des *meta-objects* :

```cpp
#include <meta>

enum class Color { Red, Green, Blue };

// Convertir automatiquement un enum en string — sans macro, sans boilerplate
consteval std::string enum_to_string(Color c) {
    // ^^Color produit un meta-object représentant le type Color
    template for (constexpr auto e : std::meta::enumerators_of(^^Color)) {
        if (std::meta::constant_of(e) == c) {
            return std::string(std::meta::identifier_of(e));
        }
    }
    return "<unknown>";
}
```

Ce qui nécessitait auparavant des macros complexes, des X-macros, ou des bibliothèques de réflexion tierce (Boost.Describe, magic_enum) devient une fonctionnalité du langage, type-safe et efficace à la compilation. Les cas d'usage sont vastes : sérialisation/désérialisation automatique, ORM, génération d'interfaces, binding vers d'autres langages, validation de schéma, et toute forme de métaprogrammation structurelle.

La réflexion statique utilise aussi les *expansion statements* (`template for`), une boucle à la compilation qui itère sur des séquences de meta-objects — un outil de métaprogrammation radicalement plus lisible que les fold expressions ou la récursion template.

> 📎 *La section 12.14.2 couvre la réflexion statique en détail : l'opérateur `^^`, les fonctions meta, les expansion statements, et des exemples concrets (enum-to-string, sérialisation automatique).*

### std::execution (Senders/Receivers) : l'asynchronisme standardisé

`std::execution` (basé sur P2300, promu dans C++26) fournit un cadre complet pour la programmation asynchrone et le parallélisme. C'est le remplacement à long terme de `std::async` et `std::future` (C++11), qui étaient reconnus comme insuffisants pour les besoins réels de la programmation concurrente.

Le modèle repose sur trois concepts fondamentaux :

- **Sender** — Un objet qui décrit un travail à effectuer, sans l'exécuter immédiatement. C'est une description paresseuse d'une opération asynchrone.
- **Receiver** — Un objet qui consomme le résultat d'un sender (succès, erreur, ou annulation).
- **Scheduler** — Un objet qui détermine *où* et *quand* le travail est exécuté (thread pool, event loop, GPU, etc.).

```cpp
#include <execution>

// Créer un scheduler (par exemple un thread pool)
auto scheduler = std::execution::system_context().get_scheduler();

// Décrire un pipeline de travail asynchrone
auto work = std::execution::schedule(scheduler)
    | std::execution::then([] { return fetch_data(); })
    | std::execution::then([](Data d) { return process(d); })
    | std::execution::then([](Result r) { save(r); });

// Exécuter et attendre
std::execution::sync_wait(work);
```

Le modèle Sender/Receiver est composable (les opérations se chaînent comme les views Ranges), annulable (support natif de l'annulation), et intégrable avec n'importe quel contexte d'exécution — thread pools, event loops I/O, GPU, ou schedulers custom. C'est l'aboutissement de années de travail qui a commencé avec les « executors » et qui a mûri à travers des implémentations comme libunifex (Meta) et stdexec (NVIDIA).

> 📎 *La section 12.14.4 couvre `std::execution` en détail : les concepts Sender/Receiver, le remplacement de `std::async`/`std::future`, l'intégration avec les schedulers et les thread pools.*

## Pattern Matching : une fonctionnalité attendue pour C++29

Le pattern matching est l'une des fonctionnalités les plus attendues par la communauté C++. La proposition P2688 (*Pattern Matching: `match` Expression*) a reçu un fort soutien du comité lors de la réunion de Wrocław (novembre 2024), avec 17 « strongly favor » et 16 « favor ». Cependant, **elle n'a pas franchi le gel des fonctionnalités de C++26** lors de la réunion de Sofia (juin 2025), et est désormais ciblée pour C++29.

La syntaxe proposée utilise le mot-clé `match` (et non `inspect`, qui figurait dans des propositions antérieures) :

```cpp
#include <variant>
#include <string>
#include <print>

using JsonValue = std::variant<int, double, bool, std::string>;

// Syntaxe proposée (P2688 — attendue pour C++29, non disponible en C++26)
void display(const JsonValue& val) {
    val match {
        i: int          => std::print("Entier : {}\n", i);
        d: double       => std::print("Flottant : {:.2f}\n", d);
        true            => std::print("Vrai\n");
        false           => std::print("Faux\n");
        s: std::string  => std::print("Chaîne : '{}'\n", s);
    };
}
```

Le pattern matching sera plus qu'un `switch` amélioré. Il supportera la déstructuration (extraire des éléments d'un tuple, d'un variant, d'une structure), les gardes (conditions additionnelles), et l'exhaustivité vérifiée à la compilation — le compilateur signalera si un cas n'est pas couvert. Combiné avec `std::variant`, il offrira une alternative idiomatique et lisible au pattern `std::visit` + `overloaded` (section 12.2).

En attendant C++29, le pattern `std::visit` avec l'astuce `overloaded` (section 12.2) reste l'idiome standard pour le dispatch sur des variants.

> 📎 *La section 12.14.3 présente le pattern matching tel qu'il est proposé pour C++29 : syntaxe `match`, patterns structurels, gardes, exhaustivité, et comparaison avec `std::visit`.*

## Autres nouveautés notables

Au-delà des quatre fonctionnalités majeures, C++26 apporte un ensemble de fonctionnalités complémentaires significatives.

### std::simd — Types parallèles de données

La bibliothèque `<simd>` standardise les opérations SIMD (Single Instruction, Multiple Data) en fournissant des types vectoriels portables. Au lieu d'écrire des intrinsics spécifiques à chaque architecture (SSE, AVX, NEON), on manipule des `std::simd<float>` dont la largeur s'adapte automatiquement au matériel :

```cpp
#include <simd>

std::simd<float> a = {1.0f, 2.0f, 3.0f, 4.0f};  
std::simd<float> b = {5.0f, 6.0f, 7.0f, 8.0f};  
auto c = a + b;  // Opération SIMD — un seul instruction pour 4 additions  
```

C'est un ajout majeur pour le calcul scientifique, le traitement de signal, et les moteurs de jeux.

### std::hive — Conteneur à stabilité d'itérateurs

`std::hive` (anciennement `plf::colony`) est un conteneur optimisé pour les insertions et suppressions fréquentes avec stabilité garantie des itérateurs et pointeurs. Il est adapté aux simulations de particules, aux systèmes d'entités (ECS), et aux cas où les éléments sont créés et détruits fréquemment.

### std::linalg — Algèbre linéaire standard

La bibliothèque `<linalg>` fournit des opérations d'algèbre linéaire basées sur BLAS, opérant sur des `std::mdspan` (section 12.10). C'est la première bibliothèque d'algèbre linéaire standard du C++.

### Placeholder _ 

Le caractère `_` devient un identifiant spécial qui peut être déclaré plusieurs fois dans un même scope sans erreur — résolvant enfin le problème des variables « ignorées » dans les structured bindings (section 12.1) :

```cpp
auto [_, _, z] = get_point_3d();    // C++26 : valide, les deux _ sont ignorés  
auto [_, value] = map.insert({k, v}); // Ignore l'itérateur, garde le bool  
```

### constexpr amélioré et consteval étendu

C++26 poursuit l'expansion du calcul à la compilation avec le support du placement `new` en contexte `constexpr`, des pointeurs et références inconnus dans les expressions constantes, et d'autres assouplissements qui rendent la métaprogrammation compile-time plus naturelle.

### Améliorations de sécurité

Plusieurs ajouts ciblent la sécurité mémoire et temporelle : interdiction de lier une référence retournée à un temporaire, diagnostics améliorés pour les *dangling references*, et les contrats eux-mêmes qui formalisent la vérification des invariants.

> 📎 *La section 45.6 couvre en détail les réponses du comité C++ aux pressions réglementaires sur la sécurité mémoire (NSA, CISA, Cyber Resilience Act) et l'état des Safety Profiles en 2026.*

## Support compilateur

En mars 2026, le support de C++26 dans les compilateurs est — naturellement — encore partiel. Les fonctionnalités de C++26 sont accessibles via le flag `-std=c++2c` (GCC, Clang) ou `/std:c++latest` (MSVC). Voici l'état général :

**Contrats** — Le support est en cours d'implémentation dans les trois compilateurs. GCC 15 et Clang 20 ont un support partiel ou expérimental. C'est une fonctionnalité qui nécessite des changements significatifs dans le frontend du compilateur et dans les outils de diagnostic.

**Réflexion statique** — C'est la fonctionnalité la plus complexe à implémenter. Des prototypes fonctionnels existent (notamment le fork EDG/Bloomberg utilisé pour valider la proposition), mais le support dans les compilateurs de production est encore au stade initial. L'implémentation complète prendra vraisemblablement jusqu'en 2027-2028 pour les trois compilateurs.

**std::execution** — Le modèle Sender/Receiver dispose d'implémentations de référence matures (stdexec de NVIDIA, libunifex de Meta). L'intégration dans les bibliothèques standard (libstdc++, libc++, MSVC STL) est en cours.

**Pattern Matching** — Non inclus dans C++26. La proposition P2688 est ciblée pour C++29. Aucun support compilateur n'est attendu dans le cycle C++26.

**Fonctionnalités plus petites** — Le placeholder `_`, les améliorations `constexpr`, `std::simd`, `std::hive` et les corrections de sécurité sont généralement implémentés plus rapidement que les fonctionnalités architecturales.

> 📎 *La section 12.14.5 détaille l'état du support compilateur de C++26 en mars 2026, fonctionnalité par fonctionnalité.*

## Organisation des sous-sections

Les sous-sections suivantes approfondissent chaque fonctionnalité majeure :

- **12.14.1** — Contrats : préconditions, postconditions, assertions, sémantiques d'évaluation.
- **12.14.2** — Réflexion statique : opérateur `^^`, fonctions meta, expansion statements, cas d'usage.
- **12.14.3** — Pattern Matching : syntaxe `match` proposée pour C++29, patterns structurels, gardes, exhaustivité.
- **12.14.4** — `std::execution` : modèle Sender/Receiver, schedulers, remplacement de `std::async`.
- **12.14.5** — État du support compilateur C++26 en mars 2026.

## Pourquoi C++26 compte

Chacune des fonctionnalités majeures de C++26 adresse un manque historique du langage :

Les **contrats** répondent à l'absence de vérification d'interface standard — un domaine où Eiffel et Ada excellaient depuis les années 1980, et que C++ laissait aux assertions manuelles et à la documentation.

La **réflexion statique** élimine le recours aux macros et au boilerplate de métaprogrammation — le principal reproche fait à C++ par les développeurs venant de langages avec introspection native (Python, Java, C#).

**`std::execution`** comble le vide laissé par l'échec reconnu de `std::async`/`std::future` — une API dont les limitations étaient documentées dès sa standardisation en C++11.

Le **pattern matching** (prévu pour C++29) fermera l'écart avec les langages fonctionnels et Rust — `std::visit` + `overloaded` fonctionne mais ne peut rivaliser en lisibilité.

Prises ensemble, les fonctionnalités de C++26 positionnent ce standard comme celui qui rend le C++ non seulement performant et proche du matériel, mais aussi expressif, sûr et agréable à écrire. L'ajout attendu du pattern matching en C++29 complétera le tableau. C'est la continuation directe de la trajectoire décrite dans l'introduction de ce chapitre (section 12) : chaque standard rapproche un peu plus le langage de cet objectif.

---


⏭️ [Contrats (Contracts) : Préconditions, postconditions, assertions](/12-nouveautes-cpp17-26/14.1-contracts.md)
