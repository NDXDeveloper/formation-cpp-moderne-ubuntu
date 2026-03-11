🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 17 — Exceptions et Gestion d'Erreurs

## Module 6 : Gestion des Erreurs et Robustesse · Niveau Intermédiaire

---

## Objectifs du chapitre

À l'issue de ce chapitre, vous serez capable :

- de comprendre le mécanisme d'exceptions de C++ et son fonctionnement interne (stack unwinding),
- de maîtriser la syntaxe `try` / `catch` / `throw` et les bonnes pratiques associées,
- de naviguer dans la hiérarchie des exceptions standard (`std::exception` et ses dérivées),
- de concevoir vos propres classes d'exceptions adaptées à vos domaines métier,
- de tirer parti de `noexcept` pour exprimer les garanties d'exception et améliorer les performances,
- d'évaluer et d'adopter les alternatives modernes aux exceptions : `std::expected` (C++23), codes d'erreur typés,
- de comprendre l'apport des **contrats** (C++26) dans une stratégie de gestion d'erreurs globale.

---

## Pourquoi la gestion d'erreurs est un sujet central en C++

Tout programme réel interagit avec un environnement imprévisible : fichiers absents, connexions réseau rompues, allocations mémoire échouées, données d'entrée invalides. La manière dont un programme détecte, signale et récupère ces situations anormales détermine directement sa **robustesse**, sa **maintenabilité** et sa **fiabilité en production**.

En C, la gestion d'erreurs repose historiquement sur des codes de retour entiers et sur la variable globale `errno`. Ce modèle présente des faiblesses bien connues : rien n'oblige l'appelant à vérifier le code de retour, la valeur d'erreur se mélange avec la valeur utile, et la propagation manuelle à travers plusieurs couches d'appels produit un code verbeux et fragile.

C++ a introduit les **exceptions** dès ses premières versions pour répondre à ces limitations. Le mécanisme d'exceptions sépare le chemin nominal du chemin d'erreur, garantit que les erreurs ne peuvent pas être silencieusement ignorées, et s'appuie sur le **stack unwinding** pour libérer automatiquement les ressources via les destructeurs — un pilier du principe RAII que vous avez découvert au chapitre 6.

Cependant, les exceptions ne sont pas une solution universelle. Dans certains contextes — systèmes embarqués, code temps réel, chemins critiques en performance — leur coût ou leur imprévisibilité les rend inadaptées. C'est pourquoi le langage a progressivement enrichi sa palette d'outils : `noexcept` pour exprimer statiquement l'absence d'exceptions, `std::optional` (C++17) pour les valeurs potentiellement absentes, `std::expected` (C++23) pour combiner valeur de retour et information d'erreur de manière type-safe, et enfin les **contrats** (C++26) pour vérifier les préconditions et postconditions directement dans l'interface des fonctions.

Ce chapitre vous guide à travers l'ensemble de ces mécanismes, des fondamentaux aux approches les plus récentes.

---

## Le spectre des stratégies de gestion d'erreurs en C++ moderne

Avant de plonger dans chaque mécanisme, il est utile de comprendre comment les différentes approches se positionnent les unes par rapport aux autres. C++ moderne ne propose pas une solution unique, mais un **spectre de stratégies** complémentaires, chacune adaptée à un contexte particulier.

### Erreurs de programmation vs erreurs d'exécution

La première distinction fondamentale sépare deux natures d'erreurs radicalement différentes.

Les **erreurs de programmation** (bugs) correspondent à des violations de contrat interne : déréférencement d'un pointeur nul, indice hors limites d'un tableau, précondition non respectée. Ces erreurs ne devraient jamais se produire dans un programme correct. Elles relèvent de `assert`, `static_assert`, et désormais des **contrats C++26**. L'objectif n'est pas de les récupérer, mais de les détecter le plus tôt possible — idéalement à la compilation, sinon en phase de test.

Les **erreurs d'exécution** (erreurs attendues) correspondent à des situations anormales mais prévisibles : fichier introuvable, timeout réseau, entrée utilisateur invalide. Ces erreurs font partie du fonctionnement normal du programme et doivent être détectées, signalées et récupérées proprement. C'est ici qu'interviennent les exceptions, `std::expected`, et les codes d'erreur.

### Quel mécanisme pour quel contexte ?

Le tableau suivant résume les cas d'usage typiques de chaque approche :

| Mécanisme | Nature de l'erreur | Récupération | Coût au chemin nominal | Contexte typique |
|---|---|---|---|---|
| `assert` / `static_assert` | Bug (programmation) | Non — arrêt du programme | Nul (retiré en release) | Préconditions internes, invariants |
| Contrats (C++26) | Bug (programmation) | Configurable | Nul à faible | Interface publique, API |
| Exceptions (`throw`) | Exécution (inattendue) | Oui — via `catch` | Quasi-nul (zero-cost en nominal) | Erreurs rares et graves |
| `std::expected` (C++23) | Exécution (fréquente) | Oui — via inspection | Faible et prévisible | Parsing, I/O, validation |
| Codes d'erreur | Exécution (fréquente) | Oui — via test de retour | Minimal | Code C, interfaces système |

Ces approches ne s'excluent pas mutuellement. Un projet moderne combine généralement plusieurs d'entre elles selon les couches de l'architecture.

---

## Les exceptions C++ : principes fondamentaux

### Le mécanisme de stack unwinding

Lorsqu'une instruction `throw` est exécutée, le runtime C++ interrompt le flux d'exécution normal et commence à **dérouler la pile d'appels** (stack unwinding). À chaque frame de la pile, les destructeurs des objets locaux sont appelés dans l'ordre inverse de leur construction. Ce processus se poursuit jusqu'à ce qu'un bloc `catch` compatible soit trouvé, ou, en l'absence d'un tel bloc, jusqu'à l'appel de `std::terminate()` qui met fin au programme.

Ce mécanisme est la raison pour laquelle le RAII (chapitre 6) est si important en C++ : si vos ressources sont encapsulées dans des objets dont les destructeurs libèrent correctement les ressources, le stack unwinding garantit qu'aucune fuite ne se produira, même en cas d'erreur.

```cpp
#include <fstream>
#include <vector>
#include <stdexcept>

void traiter_fichier(const std::string& chemin) {
    std::ifstream fichier(chemin);          // RAII : fermé automatiquement
    std::vector<std::string> lignes;        // RAII : mémoire libérée automatiquement

    if (!fichier.is_open()) {
        throw std::runtime_error("Impossible d'ouvrir : " + chemin);
        // → stack unwinding : fichier et lignes sont détruits proprement
    }

    // ... traitement ...
}
```

### Le modèle « zero-cost exceptions »

Les compilateurs modernes (GCC, Clang) implémentent les exceptions selon le modèle dit **zero-cost** (ou *table-driven*). Ce modèle a une propriété essentielle : lorsque aucune exception n'est levée, le chemin nominal ne paie **aucun coût d'exécution** supplémentaire — pas de vérification de code de retour, pas de branchement conditionnel. Le prix est payé uniquement lorsqu'une exception est effectivement levée, sous forme d'un parcours de tables générées à la compilation pour identifier le handler approprié.

Ce compromis est avantageux lorsque les erreurs sont **rares**. En revanche, si les erreurs sont fréquentes (par exemple dans une boucle de parsing où chaque ligne peut échouer), le coût du lancement d'exception devient significatif et une approche par `std::expected` ou code de retour sera plus appropriée.

---

## Au-delà des exceptions : les alternatives modernes

### `std::expected` (C++23) — la gestion d'erreurs sans exceptions

`std::expected<T, E>` est un type discriminé qui contient soit une valeur de type `T` (le cas de succès), soit une erreur de type `E` (le cas d'échec). Il rend la gestion d'erreurs **explicite dans le type de retour**, sans recourir aux exceptions ni sacrifier l'information sur la nature de l'erreur.

```cpp
#include <expected>
#include <string>
#include <fstream>

enum class ErreurFichier { introuvable, permission_refusee, format_invalide };

std::expected<std::string, ErreurFichier> lire_config(const std::string& chemin) {
    std::ifstream f(chemin);
    if (!f.is_open())
        return std::unexpected(ErreurFichier::introuvable);

    std::string contenu(/* ... */);
    return contenu;  // succès
}
```

L'appelant est contraint, par le système de types, de prendre en compte la possibilité d'une erreur. Ce mécanisme est détaillé en section 17.5 et au chapitre 12 (section 12.8).

### Contrats (C++26) — vérifier les préconditions dans l'interface

Les contrats, ratifiés avec C++26, permettent d'exprimer directement dans la signature d'une fonction les conditions qui doivent être vérifiées à l'entrée (préconditions) et à la sortie (postconditions). Ils comblent un vide entre `assert` (interne à l'implémentation) et les exceptions (destinées aux erreurs d'exécution).

```cpp
double racine_carree(double x)
    pre(x >= 0.0)       // précondition : contrat C++26
    post(r : r >= 0.0)  // postcondition
{
    return std::sqrt(x);
}
```

Les contrats sont introduits en section 17.6 dans le contexte de la gestion d'erreurs. Leur couverture complète se trouve en section 12.14.1.

---

## Plan du chapitre

Ce chapitre est organisé en six sections qui vous guident progressivement de la syntaxe fondamentale vers les approches les plus modernes.

**Section 17.1 — Syntaxe : `try`, `catch`, `throw`**
Le mécanisme de base des exceptions : lancer, capturer, et propager des erreurs. Stack unwinding en détail. Captures par type, par référence, et le catch-all. Bonnes pratiques de structuration des blocs try/catch.

**Section 17.2 — Hiérarchie des exceptions standard (`std::exception`)**
L'arbre d'héritage des exceptions de la bibliothèque standard. Quand utiliser `std::runtime_error`, `std::logic_error`, `std::invalid_argument` et leurs dérivées. Comment choisir la bonne classe de base.

**Section 17.3 — Exceptions personnalisées**
Concevoir des classes d'exceptions adaptées à votre domaine métier. Héritage depuis `std::exception` ou ses dérivées. Transporter de l'information contextuelle (codes d'erreur, messages, traces).

**Section 17.4 — `noexcept` et garanties d'exception**
Exprimer statiquement qu'une fonction ne lèvera jamais d'exception. Impact sur les performances (move semantics, optimisations du compilateur). Les trois niveaux de garanties d'exception : basique, forte, no-throw.

**Section 17.5 — Alternatives modernes : `std::expected` (C++23), codes d'erreur**
Quand préférer une approche sans exceptions. `std::expected<T, E>` en détail : construction, inspection, chaînage. Comparaison avec les codes d'erreur classiques et `std::error_code`.

**Section 17.6 — Contrats (C++26) : Préconditions et postconditions**
Introduction aux contrats dans le contexte de la gestion d'erreurs. Syntaxe `pre`, `post`, `contract_assert`. Complémentarité avec les exceptions et `std::expected`.

---

## Prérequis

Avant d'aborder ce chapitre, assurez-vous de maîtriser les notions suivantes :

- **RAII et destructeurs** (chapitre 6, section 6.3) — le stack unwinding repose entièrement sur la destruction automatique des objets locaux.
- **Smart pointers** (chapitre 9) — `std::unique_ptr` et `std::shared_ptr` sont les outils RAII par excellence pour la gestion mémoire en présence d'exceptions.
- **Sémantique de mouvement** (chapitre 10) — `noexcept` joue un rôle clé dans l'optimisation des move constructors.
- **`std::optional` et `std::variant`** (section 12.2) — prérequis conceptuel pour comprendre `std::expected`.

---

## Conventions utilisées dans ce chapitre

Les exemples de code sont compilés avec **GCC 15** ou **Clang 20** en mode C++23 minimum (`-std=c++23`). Les sections portant sur les contrats C++26 utilisent `-std=c++26` et précisent l'état du support compilateur.

Les flags de compilation recommandés pour suivre ce chapitre :

```bash
g++ -std=c++23 -Wall -Wextra -Wpedantic -Werror -g -o programme source.cpp
```

Pour les sections sur les contrats (17.6), le support compilateur des contrats C++26 (P2900) n'est pas encore disponible dans GCC 15 ni Clang 20 mainline (mars 2026). Des forks expérimentaux sont disponibles sur [Compiler Explorer](https://godbolt.org/) pour tester la syntaxe. L'intégration dans les compilateurs mainline est attendue pour GCC 16 / Clang 21.

```bash
# Lorsque le support sera disponible :
g++ -std=c++26 -Wall -Wextra -Wpedantic -g -o programme source.cpp
```

> 📎 *La section 17.6 est une introduction aux contrats dans le contexte de la gestion d'erreurs. Pour la couverture complète de la syntaxe, de la sémantique et des stratégies de déploiement, reportez-vous à la **section 12.14.1** (Contrats C++26).*

⏭️ [Syntaxe : try, catch, throw](/17-exceptions/01-syntaxe-try-catch.md)
