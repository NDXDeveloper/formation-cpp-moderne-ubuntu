🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 18 — Assertions et Débogage Défensif

## Module 6 : Gestion des Erreurs et Robustesse · Niveau Intermédiaire

---

## Introduction

Le chapitre 17 a couvert les mécanismes de gestion d'erreurs destinés à traiter les situations **prévisibles** en production : exceptions, `std::expected`, codes de retour. Ce chapitre aborde un angle complémentaire et tout aussi essentiel : la détection des erreurs **qui ne devraient jamais se produire** — les bugs du programmeur.

La programmation défensive repose sur une idée simple : plutôt que de faire confiance aveuglément au bon fonctionnement du code, on y insère des vérifications explicites qui signalent immédiatement toute violation des hypothèses du développeur. Quand une assertion échoue, le programme s'arrête net avec un message clair, au lieu de propager silencieusement des données corrompues qui ne provoqueront un crash que bien plus tard, dans un endroit sans rapport avec la cause réelle.

La distinction fondamentale est la suivante :

- **Une exception** gère un cas d'erreur anticipé (fichier introuvable, réseau indisponible, entrée utilisateur invalide). Le programme peut et doit s'en remettre.  
- **Une assertion** vérifie un invariant interne du programme (précondition d'une fonction, état censé être impossible). Si elle échoue, c'est un bug — il n'y a rien à « gérer », il faut corriger le code.

Confondre les deux conduit soit à des programmes fragiles qui crashent en production sur des erreurs récupérables, soit à des programmes silencieux qui avalent les bugs et les transforment en comportements erratiques impossibles à diagnostiquer.


## Ce que vous apprendrez

Ce chapitre couvre l'ensemble des outils de débogage défensif disponibles en C++ moderne, du plus classique au plus récent.

**`assert` et `static_assert`** constituent le premier rempart. `assert` vérifie une condition à l'exécution et termine le programme si elle est fausse — mais uniquement en mode debug. `static_assert`, introduit en C++11, déplace cette vérification à la compilation, permettant de détecter certaines catégories d'erreurs avant même que le programme ne soit exécutable. Ensemble, ils forment la base de toute stratégie de débogage défensif.

**La compilation conditionnelle** avec les directives du préprocesseur (`#ifdef DEBUG`, `#ifndef NDEBUG`) permet d'insérer du code de diagnostic qui disparaît complètement des builds de production. Cette technique, bien que relevant du préprocesseur C hérité, reste incontournable pour contrôler finement le niveau de vérification en fonction du contexte de compilation.

**Le logging et les traces d'exécution** offrent une approche moins intrusive : plutôt que d'arrêter le programme, on enregistre son comportement pour analyse ultérieure. Entre un `assert` qui stoppe tout et un silence complet, le logging structuré fournit un compromis précieux, en particulier pour les problèmes intermittents ou les environnements où un crash n'est pas acceptable.

**`std::stacktrace` (C++23)** apporte enfin au langage ce qui nécessitait auparavant des bibliothèques tierces ou du code spécifique à chaque plateforme : la capacité d'obtenir une trace d'appel lisible directement depuis le code C++. Combiné aux assertions ou au logging, cet outil transforme radicalement la qualité des diagnostics d'erreur.


## Assertions vs Exceptions vs Contrats : positionnement

Il est utile de situer les assertions par rapport aux autres mécanismes couverts dans cette formation :

| Mécanisme | Quand l'utiliser | Désactivable en production | Standard |  
|---|---|---|---|  
| `assert` | Invariants internes, préconditions de debug | Oui (`NDEBUG`) | C / C++ |  
| `static_assert` | Contraintes vérifiables à la compilation | N/A (compile-time) | C++11 |  
| Exceptions | Erreurs récupérables à l'exécution | Non | C++98 |  
| `std::expected` | Erreurs récupérables sans exceptions | Non | C++23 |  
| Contrats (C++26) | Préconditions / postconditions / invariants | Configurable | C++26 |

Les contrats C++26, couverts en section 12.14.1 et introduits en section 17.6, représentent l'évolution naturelle du mécanisme d'assertion. Ils offriront à terme une alternative standardisée et plus expressive à `assert`, avec un contrôle granulaire sur le comportement en cas de violation. Cependant, la maîtrise de `assert` et de la compilation conditionnelle reste indispensable : ce sont les outils disponibles aujourd'hui dans tout code C++ existant, et la compréhension de leurs forces et limites permet de mieux appréhender les choix de conception des contrats.


## Philosophie du débogage défensif

Le débogage défensif s'appuie sur un principe directeur : **échouer bruyamment et le plus tôt possible**. Un programme qui détecte un état incohérent et s'arrête immédiatement est infiniment plus facile à corriger qu'un programme qui continue silencieusement avec des données corrompues.

Ce principe se décline en plusieurs pratiques concrètes que nous développerons dans les sections suivantes :

- **Vérifier les préconditions** à l'entrée d'une fonction : les paramètres sont-ils dans les plages attendues ? Les pointeurs sont-ils non nuls ?  
- **Vérifier les postconditions** à la sortie : le résultat est-il cohérent ? L'état interne de l'objet est-il toujours valide ?  
- **Vérifier les invariants** aux points critiques : les structures de données sont-elles dans un état cohérent après une opération complexe ?  
- **Tracer l'exécution** pour pouvoir reconstituer le chemin qui a mené à un problème, même si celui-ci ne se manifeste que sporadiquement.

L'objectif n'est pas de transformer le code en une suite de vérifications qui ralentissent l'exécution. Les mécanismes présentés dans ce chapitre sont conçus pour être activés graduellement : vérifications maximales en développement et en test, vérifications ciblées ou désactivées en production, avec un logging structuré qui prend le relais.


## Prérequis

Ce chapitre s'appuie sur les connaissances suivantes :

- **Chapitre 1** — Cycle de compilation et rôle du préprocesseur (section 1.3.1), nécessaire pour comprendre la compilation conditionnelle et le fonctionnement de `NDEBUG`.  
- **Chapitre 17** — Exceptions et gestion d'erreurs, pour bien distinguer le rôle des assertions de celui des exceptions.  
- **Chapitre 5** — Gestion de la mémoire, car de nombreux exemples d'assertions portent sur la validité des pointeurs et l'intégrité des allocations.


## Plan du chapitre

- **18.1** — `assert` et `static_assert` : vérifications à l'exécution et à la compilation  
- **18.2** — Compilation conditionnelle (`#ifdef DEBUG`) : contrôler le code de diagnostic  
- **18.3** — Logging et traces d'exécution : observer sans interrompre  
- **18.4** — `std::stacktrace` (C++23) : traces d'appel standardisées

---

> 📎 *`std::stacktrace` est également couvert en section 12.12 sous l'angle des nouveautés C++23, et en section 29.5 dans le contexte du débogage avancé. La présente section se concentre sur son intégration dans une stratégie de débogage défensif.*

> 💡 *Les contrats C++26 (section 12.14.1, introduits en section 17.6) fourniront à terme un mécanisme standardisé de préconditions et postconditions. Ce chapitre pose les fondations conceptuelles qui permettront de les adopter efficacement.*

⏭️ [assert et static_assert](/18-assertions-debug/01-assert-static-assert.md)
