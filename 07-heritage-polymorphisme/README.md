🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 7 — Héritage et Polymorphisme

## Module 3 : Programmation Orientée Objet · Niveau Débutant-Intermédiaire

---

## Introduction

L'héritage et le polymorphisme constituent deux des piliers fondamentaux de la programmation orientée objet. Là où le chapitre 6 a posé les bases avec les classes, l'encapsulation et le cycle de vie des objets (RAII, Rule of Five), ce chapitre franchit une étape décisive : il s'agit désormais de **modéliser des hiérarchies de types** et de permettre à un même code de se comporter différemment selon l'objet concret qu'il manipule.

En C++, ces mécanismes ne sont pas de simples abstractions théoriques. Ils ont des implications directes sur la **disposition mémoire des objets**, sur les **performances à l'exécution** et sur la **maintenabilité** du code à long terme. Comprendre comment le compilateur implémente le polymorphisme — notamment à travers le mécanisme de vtable — est ce qui distingue un développeur C++ d'un développeur qui écrit du code orienté objet dans un langage à runtime managé.

Ce chapitre aborde la question sous trois angles complémentaires : la mécanique du langage (comment ça s'écrit), l'implémentation interne (comment ça fonctionne en mémoire) et les considérations de performance (combien ça coûte).

---

## Objectifs du chapitre

À l'issue de ce chapitre, vous serez en mesure de :

- Concevoir des hiérarchies de classes en utilisant l'héritage simple et comprendre les subtilités de l'héritage multiple, y compris le problème du diamant et sa résolution par l'héritage virtuel.  
- Expliquer le fonctionnement interne des fonctions virtuelles et du mécanisme de vtable tel qu'il est implémenté par GCC et Clang sur Linux.  
- Utiliser correctement les mots-clés `virtual`, `override` et `final` pour mettre en œuvre un polymorphisme dynamique robuste et résistant aux erreurs de refactoring.  
- Définir des classes abstraites et des interfaces pures pour établir des contrats clairs entre composants.  
- Évaluer le coût réel du polymorphisme dynamique en termes de performance (indirection, cache misses, impossibilité d'inlining) et identifier les situations où des alternatives comme le CRTP sont préférables.

---

## Prérequis

Ce chapitre s'appuie directement sur les notions introduites dans les chapitres précédents. Avant de commencer, assurez-vous de maîtriser :

- **Chapitre 5 — Gestion de la mémoire** : La distinction stack/heap, l'allocation dynamique et les pointeurs sont indispensables pour comprendre comment les objets polymorphiques sont manipulés (le polymorphisme dynamique nécessite des pointeurs ou des références).  
- **Chapitre 6 — Classes et encapsulation** : Constructeurs, destructeurs, modificateurs d'accès et surtout le principe RAII. La Rule of Five prend une importance particulière dans le contexte de l'héritage, car un destructeur non virtuel dans une classe de base est une source classique de fuites mémoire.

---

## Vue d'ensemble des sections

### 7.1 — Héritage simple et multiple

L'héritage permet à une classe dérivée de réutiliser et d'étendre le comportement d'une classe de base. Cette section couvre la syntaxe de l'héritage simple, puis aborde les cas plus complexes de l'héritage multiple — un mécanisme puissant mais qui introduit le célèbre **problème du diamant**. Vous verrez comment l'héritage virtuel résout cette ambiguïté, et pourquoi l'héritage multiple reste un outil à utiliser avec discernement.

### 7.2 — Fonctions virtuelles et mécanisme de vtable

Quand une méthode est déclarée `virtual`, le compilateur met en place un mécanisme d'indirection pour résoudre l'appel à l'exécution plutôt qu'à la compilation. Cette section descend au niveau de l'implémentation : la **vtable** (table de pointeurs de fonctions) et le **vptr** (pointeur interne à chaque objet). Vous inspecterez ces structures avec des outils comme `g++ -fdump-class-hierarchy` et `objdump` pour démystifier ce qui se passe réellement sous le capot.

### 7.3 — Polymorphisme dynamique : `virtual`, `override`, `final`

Cette section est le cœur pratique du chapitre. Elle couvre l'utilisation de `virtual` pour déclarer des méthodes redéfinissables, de `override` (C++11) pour sécuriser les redéfinitions contre les erreurs silencieuses, et de `final` (C++11) pour verrouiller une méthode ou une classe entière contre toute extension ultérieure. Vous verrez pourquoi `override` devrait être systématique dans tout code C++ moderne.

### 7.4 — Classes abstraites et interfaces pures

Une classe contenant au moins une fonction virtuelle pure (`= 0`) ne peut pas être instanciée : elle devient abstraite. Cette section montre comment utiliser les classes abstraites pour définir des **contrats d'interface**, un pattern architectural fondamental en C++. Vous verrez également comment simuler le concept d'interface (tel qu'il existe en Java ou Go) dans un langage qui ne le formalise pas en tant que mot-clé.

### 7.5 — Coût du polymorphisme dynamique en performance

Le polymorphisme dynamique a un prix. Chaque appel à une fonction virtuelle passe par une indirection via la vtable, ce qui empêche le compilateur d'inliner l'appel et peut provoquer des **cache misses** dans du code à haute fréquence d'appel. Cette section quantifie ce coût, présente les cas où il est négligeable et ceux où il devient un goulot d'étranglement. Elle ouvre également la voie vers les alternatives de **polymorphisme statique** (CRTP, `std::variant` + `std::visit`) qui seront approfondies dans les modules suivants.

---

## Conventions utilisées dans ce chapitre

Tout au long de ce chapitre, les exemples sont compilés avec GCC 15 ou Clang 20 sur Ubuntu, en utilisant le standard **C++23** (les exemples utilisent `std::println` du header `<print>`) :

```bash
g++ -std=c++23 -Wall -Wextra -Wpedantic -g -o programme source.cpp
```

L'option `-g` est systématiquement incluse pour permettre l'inspection avec GDB et les outils d'analyse (voir chapitre 29 pour le débogage avancé). L'option `-Wsuggest-override` (GCC) ou `-Wmissing-override` (Clang) est fortement recommandée pour détecter les redéfinitions de fonctions virtuelles qui oublient le mot-clé `override`.

---

## Positionnement dans la formation

```
Module 3 : Programmation Orientée Objet
│
├── Chapitre 6 — Classes et Encapsulation ✅ (prérequis)
│   └── Constructeurs, destructeurs, RAII, Rule of Five
│
├── 📍 Chapitre 7 — Héritage et Polymorphisme (vous êtes ici)
│   └── Hiérarchies de types, vtable, polymorphisme dynamique
│
└── Chapitre 8 — Surcharge d'Opérateurs et Conversions
    └── operator(), operator<=>, conversions
```

> 💡 Le polymorphisme dynamique présenté ici sera mis en perspective avec le **polymorphisme statique** (CRTP, section 44.3) et les **Concepts C++20** (section 16.6), qui offrent des alternatives à coût zéro à l'exécution. La section 7.5 pose les bases de cette réflexion.

---

## Sommaire du chapitre

- [7.1 — Héritage simple et multiple](/07-heritage-polymorphisme/01-heritage-simple-multiple.md)
    - [7.1.1 — Héritage simple](/07-heritage-polymorphisme/01.1-heritage-simple.md)
    - [7.1.2 — Héritage multiple et problème du diamant](/07-heritage-polymorphisme/01.2-heritage-multiple.md)
    - [7.1.3 — Héritage virtuel](/07-heritage-polymorphisme/01.3-heritage-virtuel.md)
- [7.2 — Fonctions virtuelles et mécanisme de vtable](/07-heritage-polymorphisme/02-fonctions-virtuelles-vtable.md)  
- [7.3 — Polymorphisme dynamique : virtual, override, final](/07-heritage-polymorphisme/03-polymorphisme-dynamique.md)  
- [7.4 — Classes abstraites et interfaces pures](/07-heritage-polymorphisme/04-classes-abstraites.md)  
- [7.5 — Coût du polymorphisme dynamique en performance](/07-heritage-polymorphisme/05-cout-polymorphisme.md)

⏭️ [Héritage simple et multiple](/07-heritage-polymorphisme/01-heritage-simple-multiple.md)
