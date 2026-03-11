🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 17.6 — Contrats (C++26) : Préconditions et postconditions

## Standard ratifié — Introduction dans le contexte de la gestion d'erreurs

> 📎 *Cette section est une introduction aux contrats dans le contexte de la gestion d'erreurs. Elle couvre les concepts essentiels, la syntaxe de base et l'articulation avec les mécanismes étudiés précédemment dans ce chapitre (exceptions, `noexcept`, `std::expected`). Pour la couverture complète — syntaxe avancée, sémantiques d'évaluation en détail, stratégies de déploiement, handler de violation personnalisé — reportez-vous à la **section 12.14.1** (Contrats C++26).*

---

## Introduction

Tout au long de ce chapitre, nous avons exploré un spectre de mécanismes pour détecter et signaler les erreurs : les exceptions pour les échecs d'exécution imprévus, `noexcept` pour garantir l'absence d'échec, `std::expected` pour les erreurs fréquentes et attendues. Mais il reste une catégorie d'erreurs que ces mécanismes ne couvrent pas de manière satisfaisante : les **erreurs de programmation** — les bugs.

Quand un appelant passe un indice négatif à une fonction qui attend un entier positif, quand un pointeur nul arrive là où un objet valide est requis, quand une fonction retourne une valeur qui viole ses propres invariants — ce ne sont pas des situations « attendues » qu'il faut gérer et récupérer. Ce sont des violations de contrat entre le code appelant et le code appelé. Le programme est dans un état que le développeur n'a jamais prévu, et la meilleure réaction est de le détecter le plus tôt possible.

Jusqu'à C++23, les outils pour exprimer ces contrats étaient limités et insatisfaisants :

- La macro `assert` (de `<cassert>`) permet de vérifier des conditions, mais c'est un outil du préprocesseur, sujet à toutes les limitations des macros : conflits de noms, impossibilité de les utiliser dans des expressions constantes, désactivation globale via `NDEBUG` sans granularité possible, et aucune intégration avec le système de types ni les outils d'analyse statique.
- Les exceptions peuvent techniquement vérifier des préconditions, mais elles impliquent que l'appelant « s'attend » à l'erreur et prévoit de la récupérer — ce qui est sémantiquement incorrect pour un bug.
- Les `if` + `throw` en début de fonction produisent un code verbeux et mélangent la vérification de préconditions avec la logique de gestion d'erreurs d'exécution.

C++26 comble ce vide en intégrant les **contrats** (*contracts*) au langage. Issus d'un long travail de conception (proposal P2900), les contrats permettent d'exprimer directement dans la signature d'une fonction les conditions qui doivent être vérifiées à l'entrée et à la sortie, ainsi que des assertions à n'importe quel point du code. C'est l'une des fonctionnalités les plus importantes de C++26 pour la sûreté du code.

---

## Les trois formes de contrats

C++26 introduit trois mots-clés contextuels pour exprimer des assertions contractuelles :

### `pre` — Préconditions

Une précondition est un prédicat qui doit être vrai **avant** l'exécution du corps de la fonction. Elle exprime ce que l'appelant s'engage à fournir. La précondition est évaluée après l'initialisation des paramètres mais avant le début du corps de la fonction.

```cpp
double racine_carree(double x)
    pre(x >= 0.0)
{
    return std::sqrt(x);
}
```

### `post` — Postconditions

Une postcondition est un prédicat qui doit être vrai **après** l'exécution de la fonction. Elle exprime ce que la fonction s'engage à garantir. La postcondition peut référencer la valeur de retour via un identifiant placé avant le prédicat, suivi de deux-points.

```cpp
double racine_carree(double x)
    pre(x >= 0.0)
    post(r : r >= 0.0)
{
    return std::sqrt(x);
}
```

La postcondition est évaluée après la construction de la valeur de retour et la destruction des variables locales, mais avant que les paramètres ne sortent de leur portée. Cela garantit que la valeur de retour et les paramètres `const` restent accessibles pour la vérification.

### `contract_assert` — Assertions dans le corps

`contract_assert` remplace la macro `assert` par un vrai mot-clé du langage. Il s'utilise à l'intérieur du corps d'une fonction pour vérifier un invariant à un point précis de l'exécution.

```cpp
void trier(std::span<int> donnees)
    pre(!donnees.empty())
    post(: std::is_sorted(donnees.begin(), donnees.end()))
{
    // ... algorithme de tri ...

    // Vérification intermédiaire après la première passe
    contract_assert(donnees[0] <= donnees[donnees.size() - 1]);

    // ... suite de l'algorithme ...
}
```

Contrairement à `assert`, `contract_assert` est un vrai mot-clé (et non une macro). Il ne souffre d'aucun des problèmes du préprocesseur et s'intègre naturellement dans le système de types et les outils d'analyse.

> **Pourquoi `contract_assert` et non simplement `assert` ?** — Le mot-clé idéal aurait été `assert`, mais ce nom est déjà utilisé par la macro C hérité de `<cassert>`. Pour éviter l'ambiguïté syntaxique, le comité a opté pour le nom plus explicite `contract_assert`.

---

## Préconditions et postconditions multiples

Une fonction peut déclarer un nombre arbitraire de préconditions et de postconditions. Elles peuvent être entremêlées librement dans la déclaration :

```cpp
int recherche_binaire(std::span<const int> donnees, int cible)
    pre(!donnees.empty())
    pre(std::is_sorted(donnees.begin(), donnees.end()))
    post(r : r == -1 || (r >= 0 && r < static_cast<int>(donnees.size())))
    post(r : r == -1 || donnees[r] == cible)
{
    // ... implémentation ...
}
```

Chaque précondition est évaluée dans l'ordre de déclaration. Si l'une d'elles échoue, les suivantes ne sont pas nécessairement évaluées — le comportement dépend de la sémantique d'évaluation choisie.

---

## Les quatre sémantiques d'évaluation

Un aspect fondamental de la conception des contrats C++26 est que le **code source ne spécifie pas** ce qui se passe en cas de violation. Le comportement est contrôlé par la configuration de compilation (typiquement des flags du compilateur), ce qui permet de modifier la politique de vérification sans toucher au code.

Quatre sémantiques d'évaluation sont définies :

**`ignore`** — Le prédicat n'est pas évalué. Les contrats restent dans le code source (et sont vérifiés syntaxiquement et sémantiquement par le compilateur), mais n'ont aucun coût à l'exécution. C'est le mode approprié pour les builds de production où la performance est prioritaire et où les contrats ont été validés en phase de test.

**`observe`** — Le prédicat est évalué. En cas de violation, le *handler de violation* est invoqué, mais l'exécution continue ensuite normalement. Ce mode est idéal pour le déploiement progressif dans un système existant : vous activez les vérifications, vous loguez les violations, et vous corrigez les bugs découverts sans risquer un arrêt brutal en production.

**`enforce`** — Le prédicat est évalué. En cas de violation, le handler de violation est invoqué, puis le programme est terminé (comportement par défaut). C'est la sémantique la plus comparable à un `assert` classique, mais avec le bénéfice du handler personnalisable.

**`quick_enforce`** — Le prédicat est évalué. En cas de violation, le programme est terminé **immédiatement**, sans invoquer le handler de violation et sans produire de diagnostic. Ce mode est destiné aux systèmes critiques en sûreté ou temps réel où le coût même d'un appel au handler est inacceptable.

La sélection de la sémantique se fait au niveau de la chaîne de compilation. Les forks expérimentaux de GCC et Clang proposent un flag `-fcontract-semantic` pour configurer globalement le comportement (le nom exact du flag pourra évoluer lors de l'intégration dans les compilateurs mainline) :

```bash
# Développement : vérifier et terminer en cas de violation
g++ -std=c++26 -fcontract-semantic=enforce source.cpp

# Production graduelle : vérifier, loguer, mais continuer
g++ -std=c++26 -fcontract-semantic=observe source.cpp

# Production finale : ignorer les vérifications
g++ -std=c++26 -fcontract-semantic=ignore source.cpp

# Système critique : terminer immédiatement, sans overhead
g++ -std=c++26 -fcontract-semantic=quick_enforce source.cpp
```

> **Note** : une implémentation conforme n'est pas obligée de supporter les quatre sémantiques. Elle peut aussi proposer ses propres sémantiques additionnelles. La granularité de sélection (par fichier, par fonction, par catégorie de contrat) est également définie par l'implémentation.

---

## Le handler de violation

Lorsqu'un contrat est violé avec la sémantique `enforce` ou `observe`, un **handler de violation** est invoqué. L'implémentation fournit un handler par défaut (qui produit typiquement un message de diagnostic vers `stderr`), mais vous pouvez le remplacer par votre propre implémentation en définissant une fonction globale avec la signature suivante :

```cpp
#include <contracts>

void handle_contract_violation(std::contracts::contract_violation& v) {
    // Logging personnalisé
    logger::critical("Violation de contrat : {}", v.comment());
    logger::critical("  Fichier : {}:{}", v.location().file_name(),
                                          v.location().line());

    // Éventuellement : envoi de télémétrie, capture de stacktrace, etc.
    telemetrie::envoyer_crash_report(v);
}
```

La fonction `handle_contract_violation` est définie dans l'**espace de noms global** (pas dans `std::contracts`). L'implémentation fournit un handler par défaut, et le standard permet à l'utilisateur de le remplacer par sa propre définition. La fonction `std::contracts::invoke_default_contract_violation_handler` permet d'appeler explicitement le handler par défaut depuis un handler personnalisé.

L'objet `std::contracts::contract_violation` fournit des informations détaillées sur la violation : la localisation dans le code source (`location()`), un commentaire textuel (`comment()`), le type de contrat violé (`kind()` — précondition, postcondition ou assertion), la sémantique d'évaluation active (`semantic()`), si la violation est terminante (`is_terminating()`), le mode de détection (`detection_mode()` — prédicat faux ou exception levée pendant l'évaluation), et un accès à l'éventuelle exception lancée pendant l'évaluation du prédicat (`evaluation_exception()`).

Ce mécanisme est fondamentalement plus puissant que la macro `assert`, qui ne laisse aucun contrôle sur la réaction en cas d'échec (elle appelle `abort()` et c'est tout). Avec le handler de violation, vous pouvez implémenter la stratégie de diagnostic la plus adaptée à votre contexte : logging structuré en JSON pour l'agrégation, envoi de rapports de crash, capture d'un `std::stacktrace` (section 18.4), ou même — dans certains cas avancés — lancer une exception pour tenter un déroulement de pile contrôlé.

---

## Contrats vs exceptions : deux rôles distincts

L'introduction des contrats ne rend pas les exceptions obsolètes. Ces deux mécanismes couvrent des catégories d'erreurs fondamentalement différentes et sont **complémentaires**, pas concurrents.

### Quand utiliser des contrats

Les contrats vérifient des **erreurs de programmation** — des situations qui ne devraient jamais se produire dans un programme correct. L'appelant a violé le protocole d'utilisation de la fonction. L'objectif est la **détection précoce**, pas la récupération.

```cpp
// Contrat : l'appelant DOIT passer un diviseur non nul.
// Un diviseur nul est un bug de l'appelant, pas une erreur d'exécution.
int diviser(int a, int b)
    pre(b != 0)
{
    return a / b;
}
```

### Quand utiliser des exceptions (ou `std::expected`)

Les exceptions et `std::expected` gèrent des **erreurs d'exécution** — des situations anormales mais légitimes, qui font partie du fonctionnement attendu du programme dans un environnement imprévisible. L'appelant doit pouvoir les récupérer.

```cpp
// Exception (ou std::expected) : le fichier peut ne pas exister.
// Ce n'est pas un bug de l'appelant — c'est une condition d'exécution légitime.
std::expected<Config, ErreurConfig> charger_config(const std::string& chemin) {
    std::ifstream f(chemin);
    if (!f.is_open())
        return std::unexpected(ErreurConfig::fichier_introuvable);
    // ...
}
```

### Le tableau de décision

La question à se poser est : **l'appelant peut-il raisonnablement prévenir cette erreur ?**

| Question | Oui → **Contrat** | Non → **Exception / expected** |
|---|---|---|
| L'appelant contrôle les conditions ? | `pre(index < size())` | Fichier absent, réseau coupé |
| Un programme correct évite l'erreur ? | Indice hors limites = bug | Timeout = condition externe |
| La récupération a-t-elle du sens ? | Non — c'est un bug à corriger | Oui — retry, fallback, message |
| Fréquence attendue ? | Jamais dans un programme correct | Régulière en fonctionnement normal |

### Combiner contrats et exceptions dans une même API

En pratique, une fonction réaliste combine souvent les deux mécanismes. Les contrats vérifient les préconditions (responsabilité de l'appelant), et les exceptions ou `std::expected` signalent les échecs d'exécution (responsabilité de l'environnement).

```cpp
std::expected<Utilisateur, ErreurDB> trouver_utilisateur(
    DatabaseConnection& db,
    int64_t id
)
    pre(id > 0)                    // Contrat : l'id doit être valide (bug sinon)
    pre(db.est_connectee())        // Contrat : la connexion doit être établie
    post(r : !r.has_value() || r->id() == id)  // Contrat : si trouvé, l'id correspond
{
    // Erreurs d'exécution : gérées via std::expected
    auto resultat = db.executer("SELECT * FROM users WHERE id = ?", id);
    if (!resultat)
        return std::unexpected(resultat.error());

    if (resultat->vide())
        return std::unexpected(ErreurDB::utilisateur_non_trouve);

    return Utilisateur::depuis_row(resultat->premiere_ligne());
}
```

Dans cet exemple, `id > 0` et `db.est_connectee()` sont des préconditions — les violer est un bug de l'appelant. En revanche, l'échec de la requête SQL ou l'absence de l'utilisateur en base sont des conditions d'exécution légitimes que l'appelant doit pouvoir gérer.

---

## Contrats vs `assert` : pourquoi migrer

Les contrats C++26 remplacent avantageusement la macro `assert` à tous les niveaux. Voici les gains concrets de la migration.

### Visibilité dans l'interface publique

La macro `assert` ne peut s'utiliser que dans le corps d'une fonction — elle est invisible dans l'interface. Les préconditions et postconditions, en revanche, apparaissent dans la **déclaration** de la fonction, ce qui les rend visibles dans les en-têtes, dans la documentation générée, et pour les outils d'analyse statique.

```cpp
// Avec assert : la précondition est cachée dans le corps
double ancien_style(double x) {
    assert(x >= 0.0);   // l'appelant ne voit pas cette condition dans le .h
    return std::sqrt(x);
}

// Avec contrat : la précondition est visible dans la déclaration
double nouveau_style(double x)
    pre(x >= 0.0)       // visible dans le header, documentée, analysable
{
    return std::sqrt(x);
}
```

### Granularité de contrôle

La macro `assert` ne connaît que deux états : activée ou désactivée, contrôlée par `NDEBUG`, globalement pour toute l'unité de compilation. Les contrats offrent quatre sémantiques d'évaluation sélectionnables indépendamment, avec la possibilité (selon l'implémentation) de différencier la politique par catégorie (préconditions, postconditions, assertions) ou par niveau de criticité.

### Diagnostic enrichi

Quand `assert` échoue, vous obtenez un message brut suivi d'un `abort()`. Avec les contrats, le handler de violation reçoit un objet structuré contenant la localisation, la sémantique d'évaluation, et le mode de détection. Vous pouvez produire un diagnostic riche, envoyer de la télémétrie, capturer une `std::stacktrace`, et choisir la politique de terminaison.

### Compatibilité avec `constexpr`

La macro `assert` pose des problèmes dans les contextes `constexpr` et `consteval`. Les contrats sont conçus pour fonctionner dans les évaluations à la compilation : une violation détectée au *compile-time* produit une erreur de compilation, ce qui est le meilleur moment possible pour découvrir un bug.

```cpp
constexpr int factorielle(int n)
    pre(n >= 0)
    pre(n <= 20)  // éviter l'overflow dans un int64_t
{
    if (n <= 1) return 1;
    return n * factorielle(n - 1);
}

constexpr auto ok = factorielle(5);    // ✅ compile : 120
// constexpr auto ko = factorielle(-1); // ❌ erreur de compilation : violation de pre
```

---

## État du support compilateur (mars 2026)

Les contrats C++26 (P2900) sont ratifiés dans le standard, mais leur implémentation dans les compilateurs est encore **en cours de développement** :

| Compilateur | État | Remarque |
|---|---|---|
| GCC 15 | Non disponible | Implémentation en développement dans des branches expérimentales |
| Clang 20 | Non disponible | Fork expérimental disponible sur Compiler Explorer |
| MSVC | Non disponible | En cours de développement |

Des **forks expérimentaux** de GCC et Clang sont disponibles sur [Compiler Explorer (godbolt.org)](https://godbolt.org/) pour tester la syntaxe des contrats. Ces forks acceptent les flags `-fcontract-semantic=enforce|observe|ignore|quick_enforce`. Les exemples de cette section sont vérifiables avec ces forks.

Le support dans les compilateurs mainline est attendu pour GCC 16 et Clang 21. En attendant, les contrats peuvent être étudiés sur Compiler Explorer et simulés dans le code de production via des macros ou des bibliothèques comme `contract_lite`.

---

## Limites du MVP C++26

Le design des contrats C++26 suit la philosophie du **produit minimum viable** (*MVP — Minimum Viable Product*). Certaines fonctionnalités, identifiées comme trop complexes ou insuffisamment mûres, ont été intentionnellement exclues pour être proposées dans des versions futures (C++29 ou au-delà).

**Ce que C++26 ne supporte pas (encore)** :

- Les préconditions et postconditions sur les **fonctions virtuelles**. L'héritage de contrats dans une hiérarchie polymorphique soulève des questions sémantiques complexes (substitution de Liskov, renforcement vs affaiblissement des conditions) qui restent à résoudre.
- Les contrats sur les **pointeurs de fonctions** et les **pointeurs de fonctions membres**. L'association d'un contrat à un type callable indirect n'est pas triviale.
- La référence aux **valeurs initiales des paramètres** dans les postconditions (« old values »). Par exemple, exprimer que `size()` a augmenté de 1 après un `push_back` nécessiterait de capturer `size()` avant l'appel. Ce mécanisme est en cours de conception.
- L'attribution de **niveaux d'assertion** (*assertion levels*) pour différencier les vérifications bon marché (à conserver en production) des vérifications coûteuses (uniquement en debug). La granularité actuelle repose sur les catégories syntaxiques (`pre`, `post`, `contract_assert`), pas sur des niveaux explicites.

Ces limitations n'empêchent pas une adoption immédiate dans la grande majorité des cas d'usage. Les préconditions et postconditions sur les fonctions non-virtuelles, combinées à `contract_assert` dans les corps de fonctions, couvrent l'essentiel des besoins pratiques.

---

## Stratégie de déploiement progressif

L'un des avantages majeurs de la sémantique `observe` est qu'elle permet un déploiement **incrémental et sans risque** des contrats dans un projet existant.

### Phase 1 : Ajout des contrats, sémantique `observe`

Vous annotez vos fonctions critiques avec des préconditions et postconditions, et vous compilez en mode `observe`. Les violations sont détectées et loguées, mais le programme continue. Cela vous permet de découvrir les bugs existants sans casser la production.

```bash
g++ -std=c++26 -fcontract-semantic=observe -o monapp source.cpp
```

### Phase 2 : Correction des violations, passage à `enforce`

Une fois les violations identifiées et corrigées, vous passez en mode `enforce`. Toute violation restante provoque un arrêt immédiat, ce qui empêche le programme de continuer dans un état incohérent.

```bash
g++ -std=c++26 -fcontract-semantic=enforce -o monapp source.cpp
```

### Phase 3 : Production optimisée, sémantique `ignore` ou `quick_enforce`

Pour les builds de production finaux, vous pouvez choisir entre `ignore` (aucun coût, confiance maximale dans le code testé) et `quick_enforce` (coût minimal, protection maximale). Le choix dépend du compromis performance/sûreté propre à votre domaine.

Cette approche est analogue à la montée progressive des niveaux de *warnings* (`-Wall` → `-Wextra` → `-Werror`) : on commence par observer, puis on durcit au fur et à mesure que le code mûrit.

---

## Articulation avec les mécanismes du chapitre

Pour clore cette introduction aux contrats, récapitulons comment ils s'articulent avec l'ensemble des mécanismes de gestion d'erreurs étudiés dans ce chapitre :

| Mécanisme | Rôle | Quand l'utiliser |
|---|---|---|
| **Contrats** (`pre`, `post`, `contract_assert`) | Détecter les bugs le plus tôt possible | Préconditions, postconditions, invariants internes |
| **Exceptions** (`throw` / `catch`) | Signaler et récupérer des erreurs rares | Échecs d'exécution imprévus (réseau, mémoire, I/O) |
| **`noexcept`** | Garantir l'absence d'exception | Destructeurs, move, swap, accesseurs |
| **`std::expected`** (C++23) | Retourner succès ou erreur dans le type | Erreurs d'exécution fréquentes et attendues |
| **`assert`** / **`static_assert`** | Vérifications héritées (C) et compile-time | `static_assert` reste pertinent ; `assert` remplacé par `contract_assert` |

Les contrats ne remplacent aucun des autres mécanismes — ils comblent le vide qui existait entre `assert` (trop limité) et les exceptions (sémantiquement inadaptées aux bugs). Un code C++26 robuste utilise les cinq outils, chacun pour ce qu'il fait le mieux.

> 📎 *Pour la couverture complète des contrats C++26 — syntaxe avancée, sémantiques d'évaluation en détail, implémentation du handler personnalisé, support compilateur GCC 15 / Clang 20, et patterns d'utilisation avancés — reportez-vous à la **section 12.14.1** (Contrats C++26).*

⏭️ [Assertions et Débogage Défensif](/18-assertions-debug/README.md)
