🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 9. Smart Pointers : Gestion Automatique de la Mémoire ⭐

## Objectifs du chapitre

Ce chapitre constitue un tournant dans votre apprentissage du C++ moderne. Après avoir exploré en [chapitre 5](/05-gestion-memoire/README.md) les mécanismes bruts de gestion mémoire — `new`, `delete`, pointeurs nus, et les dangers qui les accompagnent — vous allez découvrir les outils que le C++ moderne met à votre disposition pour **éliminer ces problèmes à la source**.

Les smart pointers ne sont pas un gadget ou un sucre syntaxique : ils représentent un **changement fondamental de philosophie**. En C++ moderne (C++11 et au-delà), la gestion manuelle de la mémoire avec `new` et `delete` est considérée comme une pratique obsolète dans l'immense majorité des cas.

À la fin de ce chapitre, vous serez capable de :

- Comprendre le concept de **possession** (*ownership*) d'une ressource et pourquoi il est central en C++.
- Utiliser `std::unique_ptr` pour modéliser une possession exclusive.
- Utiliser `std::shared_ptr` et `std::weak_ptr` pour modéliser une possession partagée.
- Créer des smart pointers de manière sûre avec `std::make_unique` et `std::make_shared`.
- Identifier et éviter les pièges classiques (cycles de références, mauvaise sémantique de propriété).
- Écrire du code C++ moderne **sans jamais appeler `new` ou `delete` directement**.

---

## Prérequis

Avant d'aborder ce chapitre, assurez-vous de maîtriser :

- **La mémoire Stack vs Heap** ([section 5.1](/05-gestion-memoire/01-stack-vs-heap.md)) — Vous devez comprendre pourquoi certaines données vivent sur le tas et ce que cela implique en termes de durée de vie.
- **L'allocation dynamique** ([section 5.2](/05-gestion-memoire/02-allocation-dynamique.md)) — Vous devez savoir ce que font `new` et `delete`, même si l'objectif est justement de ne plus les utiliser directement.
- **Les dangers mémoire** ([section 5.4](/05-gestion-memoire/04-dangers-memoire.md)) — Memory leaks, dangling pointers et double free sont les problèmes que les smart pointers résolvent.
- **Le principe RAII** ([section 6.3](/06-classes-encapsulation/03-destructeurs-raii.md)) — Les smart pointers sont l'application la plus directe et la plus puissante du RAII.
- **La sémantique de mouvement** ([chapitre 10](/10-move-semantics/README.md)) — Bien que ce chapitre vienne après dans le sommaire, les concepts de `std::move` et de transfert de propriété sont intimement liés à `std::unique_ptr`. Une lecture croisée est recommandée.

---

## Le problème : pourquoi les pointeurs nus sont dangereux

Considérons un scénario courant avec des pointeurs bruts :

```cpp
#include <stdexcept>

void traiter_donnees() {
    int* donnees = new int[1000];

    // ... du traitement ...

    if (condition_erreur()) {
        throw std::runtime_error("Erreur de traitement");
        // ⚠️ delete[] donnees n'est jamais appelé → MEMORY LEAK
    }

    // ... encore du traitement ...

    delete[] donnees;  // Atteint uniquement si aucune exception
}
```

Ce code souffre de plusieurs faiblesses fondamentales :

- **Fuite mémoire en cas d'exception.** Si une exception est levée entre le `new` et le `delete`, la mémoire n'est jamais libérée. Le programme accumule alors de la mémoire inutilisée, ce qui peut mener à un épuisement progressif des ressources.
- **Responsabilité ambiguë.** Qui est responsable de la libération ? Le code qui a fait l'allocation ? L'appelant ? Le code qui reçoit le pointeur en paramètre ? Avec un pointeur brut, rien dans le type ne l'indique.
- **Fragilité face aux modifications.** Ajouter un `return` anticipé, une nouvelle branche conditionnelle, ou un appel à une fonction susceptible de lever une exception — chacune de ces modifications peut introduire un chemin d'exécution qui contourne le `delete`.

---

## La solution : le RAII appliqué aux pointeurs

Le principe RAII (*Resource Acquisition Is Initialization*), que vous avez découvert en [section 6.3](/06-classes-encapsulation/03-destructeurs-raii.md), apporte une réponse élégante : **lier la durée de vie d'une ressource à la durée de vie d'un objet sur la stack**.

Quand l'objet est détruit — que ce soit par sortie normale du scope ou par déroulement de la pile lors d'une exception — son destructeur libère automatiquement la ressource. C'est exactement ce que font les smart pointers :

```cpp
#include <memory>
#include <stdexcept>

void traiter_donnees() {
    auto donnees = std::make_unique<int[]>(1000);

    // ... du traitement ...

    if (condition_erreur()) {
        throw std::runtime_error("Erreur de traitement");
        // ✅ donnees est automatiquement libéré par le destructeur
        //    de unique_ptr lors du déroulement de la pile
    }

    // ... encore du traitement ...

}   // ✅ donnees est automatiquement libéré ici aussi
```

Plus de `delete` à écrire. Plus de chemins d'exécution à surveiller. La mémoire est **toujours** libérée, quoi qu'il arrive.

---

## Le concept central : la sémantique de possession (*ownership*)

Les smart pointers ne sont pas de simples wrappers autour de pointeurs bruts. Ils encodent dans le **système de types** une information cruciale : **qui possède la ressource et qui est responsable de sa libération**.

C'est cette notion de possession qui guide le choix du smart pointer approprié :

| Situation | Smart pointer | Sémantique |
|---|---|---|
| **Un seul propriétaire** à la fois | `std::unique_ptr` | Possession exclusive — la ressource a un unique responsable, et la propriété peut être transférée mais jamais partagée. |
| **Plusieurs propriétaires** simultanés | `std::shared_ptr` | Possession partagée — la ressource reste en vie tant qu'au moins un propriétaire existe. Un compteur de références interne assure le suivi. |
| **Observateur sans possession** | `std::weak_ptr` | Référence non-possédante — permet d'observer une ressource gérée par `shared_ptr` sans empêcher sa destruction. Résout le problème des cycles de références. |
| **Pas d'allocation dynamique nécessaire** | Aucun | Si la donnée peut vivre sur la stack, c'est la meilleure option. Les smart pointers ne remplacent pas la stack, ils remplacent `new`/`delete`. |

La règle d'or est de **choisir le smart pointer le plus restrictif possible**. Dans la grande majorité des cas, `std::unique_ptr` suffit. Le recours à `std::shared_ptr` devrait être une décision consciente et justifiée, car il introduit un coût supplémentaire (compteur de références atomique, allocation du bloc de contrôle).

---

## Vue d'ensemble des sections

Ce chapitre est organisé en quatre sections qui couvrent progressivement les smart pointers du plus simple au plus nuancé :

**[9.1 — std::unique_ptr : Possession exclusive](/09-smart-pointers/01-unique-ptr.md)**
Le smart pointer que vous utiliserez le plus souvent. Il modélise un propriétaire unique et transfère la propriété via `std::move`. C'est le remplacement direct de `new`/`delete` dans le code moderne.

**[9.2 — std::shared_ptr et std::weak_ptr](/09-smart-pointers/02-shared-weak-ptr.md)**
Pour les cas où la possession doit être partagée entre plusieurs parties du code. Vous apprendrez le fonctionnement du compteur de références, ainsi que le rôle essentiel de `std::weak_ptr` pour briser les cycles.

**[9.3 — std::make_unique et std::make_shared](/09-smart-pointers/03-make-unique-shared.md)**
Les fonctions *factory* recommandées pour créer des smart pointers. Elles offrent des garanties de sécurité (exception safety) et des optimisations de performance que la construction directe ne permet pas.

**[9.4 — Ne jamais utiliser new/delete dans du code moderne](/09-smart-pointers/04-jamais-new-delete.md)** 🔥
La synthèse qui formalise la règle : dans du C++ moderne, les appels directs à `new` et `delete` ne devraient plus apparaître dans le code applicatif. Cette section explore les rares exceptions légitimes et les stratégies pour moderniser du code existant.

---

## Petit historique

Les smart pointers n'ont pas toujours existé sous leur forme actuelle en C++. Le standard C++98 proposait `std::auto_ptr`, un premier essai de smart pointer à possession exclusive. Malheureusement, `auto_ptr` souffrait d'un défaut majeur : il transférait la propriété lors de la copie, ce qui produisait un comportement contre-intuitif et source de bugs insidieux.

```cpp
// ⚠️ Code C++98 — NE PAS REPRODUIRE
std::auto_ptr<int> a(new int(42));
std::auto_ptr<int> b = a;  // "copie" qui vide silencieusement a
// a est maintenant nul — surprise !
```

C'est l'introduction de la **sémantique de mouvement** en C++11 qui a rendu possible la création de `std::unique_ptr` : un smart pointer qui interdit la copie mais autorise le déplacement explicite via `std::move`. Le transfert de propriété devient alors intentionnel et visible dans le code.

`std::auto_ptr` a été déprécié en C++11 puis officiellement supprimé en C++17. Si vous rencontrez du code qui l'utilise, c'est un signal clair que ce code nécessite une modernisation.

---

## Conventions utilisées dans ce chapitre

Tout au long de ce chapitre, les exemples de code respectent les conventions suivantes :

- **Standard minimum : C++17**, sauf mention contraire. La majorité des exemples compilent avec `-std=c++17`.
- **Header requis** : tous les smart pointers sont définis dans `<memory>`.
- **Compilation** : les exemples sont compilables avec GCC 15 ou Clang 20 sur Ubuntu, conformément à l'environnement de la formation.
- Les commentaires `// ✅` et `// ⚠️` signalent respectivement les bonnes pratiques et les pièges à éviter.

---

> **En résumé** — Les smart pointers transforment la gestion mémoire en C++ : au lieu de reposer sur la discipline du développeur (penser à appeler `delete` au bon moment, sur chaque chemin d'exécution), elle repose sur le **système de types et le compilateur**. C'est plus sûr, plus lisible, et c'est la norme du C++ moderne depuis C++11.

⏭️ [std::unique_ptr : Possession exclusive](/09-smart-pointers/01-unique-ptr.md)
