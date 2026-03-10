🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Module 3 — Programmation Orientée Objet

> 🎯 Niveau : Débutant-Intermédiaire

Ce module introduit le paradigme objet en C++ en s'appuyant directement sur la gestion mémoire du module précédent. Classes, RAII, Rule of Five, héritage, polymorphisme dynamique via vtable, et surcharge d'opérateurs — y compris l'opérateur spaceship `<=>` (C++20). L'objectif n'est pas d'apprendre l'OOP en général, mais de comprendre comment C++ l'implémente, quel coût cela représente en mémoire et en performance, et quelles erreurs spécifiques au langage il faut éviter.

---

## Objectifs pédagogiques

1. **Implémenter** des classes avec constructeurs (défaut, paramétré, copie, déplacement), destructeurs, listes d'initialisation, et modificateurs d'accès (`public`, `private`, `protected`).
2. **Appliquer** le principe RAII pour lier la durée de vie des ressources à celle des objets.
3. **Maîtriser** la Rule of Five : destructeur, constructeur de copie, opérateur d'affectation par copie, constructeur de déplacement, opérateur d'affectation par déplacement.
4. **Comprendre** le mécanisme de vtable et le coût runtime du polymorphisme dynamique (`virtual`, `override`, `final`).
5. **Concevoir** des hiérarchies de classes avec héritage simple, multiple, et virtuel, en évitant le problème du diamant.
6. **Implémenter** la surcharge d'opérateurs (arithmétiques, affectation, conversion, appel de fonction) et utiliser l'opérateur spaceship `<=>` (C++20) pour générer automatiquement les comparaisons.

---

## Prérequis

- **Module 2, chapitre 5** : gestion mémoire (stack vs heap, `new`/`delete`, pointeurs, dangling pointers) — sans cette base, RAII et la Rule of Five n'ont pas de sens.
- **Module 2, chapitre 4** : passage de paramètres par valeur, référence et pointeur — utilisé dans chaque constructeur et opérateur de ce module.
- **Module 2, chapitre 3** : `const`, `constexpr`, portée et durée de vie des variables.

---

## Chapitres

### Chapitre 6 — Classes et Encapsulation

Conception de classes C++ depuis la définition des membres jusqu'à la Rule of Five. Ce chapitre est le plus long du module — il couvre l'ensemble du cycle de vie d'un objet et le principe RAII qui est la stratégie centrale de gestion des ressources en C++ moderne.

- Définition de classes : membres de données, méthodes, séparation interface/implémentation.
- Constructeurs : par défaut, paramétré, de copie, de déplacement, avec liste d'initialisation (member initializer list) pour éviter la construction par défaut suivie d'une affectation.
- Destructeurs et RAII : acquisition de ressource dans le constructeur, libération dans le destructeur — appliqué aux file descriptors, mémoire dynamique, locks.
- Modificateurs d'accès : `public`, `private`, `protected` — quand utiliser chacun et pourquoi `private` par défaut.
- Rule of Five : si vous définissez l'un des cinq (destructeur, copy constructor, copy assignment, move constructor, move assignment), vous devez définir ou explicitement `= default` / `= delete` les quatre autres.

### Chapitre 7 — Héritage et Polymorphisme

Héritage et polymorphisme dynamique avec un focus sur le mécanisme sous-jacent (vtable) et son coût. Ce chapitre ne se contente pas de montrer la syntaxe — il explique ce que le compilateur génère et ce que ça coûte à l'exécution.

- Héritage simple : dérivation, accès aux membres de la classe de base, constructeurs de la classe dérivée.
- Héritage multiple et problème du diamant : quand deux classes de base partagent un ancêtre commun — résolu par l'héritage virtuel.
- Fonctions virtuelles et vtable : comment le compilateur implémente le dispatch dynamique, taille du vptr, indirection à l'appel.
- `virtual`, `override`, `final` : `override` pour la vérification compile-time, `final` pour interdire la redéfinition ou la dérivation.
- Classes abstraites et interfaces pures : fonctions virtuelles pures (`= 0`), classes non instanciables.
- Coût du polymorphisme dynamique : surcoût mémoire (vptr par objet), surcoût CPU (indirection, impossibilité d'inlining), impact sur la prédiction de branchement.

### Chapitre 8 — Surcharge d'Opérateurs et Conversions

Surcharge des opérateurs pour donner un comportement naturel aux types personnalisés. Ce chapitre couvre les opérateurs classiques et introduit l'opérateur spaceship `<=>` (C++20) qui simplifie la génération des six opérateurs de comparaison.

- Opérateurs arithmétiques (`+`, `-`, `*`, `/`) et de comparaison (`==`, `!=`, `<`, `>`) : implémentation comme membre ou fonction libre, cohérence entre opérateurs.
- Opérateurs d'affectation (`=`, `+=`, `-=`, etc.) : lien avec la Rule of Five pour `operator=`.
- Opérateurs de conversion : `operator int()`, `explicit operator bool()` — contrôle des conversions implicites.
- `operator()` : foncteurs (callable objects), utilisés comme prédicats dans les algorithmes STL.
- Opérateur spaceship `<=>` (C++20) : déclaration d'un seul opérateur pour générer automatiquement `==`, `!=`, `<`, `<=`, `>`, `>=` — avec les catégories de comparaison (`strong_ordering`, `weak_ordering`, `partial_ordering`).

---

## Points de vigilance

- **Destructeur virtuel manquant sur une classe de base.** Si une classe est destinée à être dérivée et que des objets dérivés sont détruits via un pointeur vers la base, le destructeur de la base doit être `virtual`. Sans ça, seul le destructeur de la base est appelé — le destructeur de la classe dérivée n'est jamais exécuté, ce qui cause des fuites de ressources. C'est un bug silencieux que ni le compilateur ni les sanitizers ne détectent systématiquement. Règle simple : si votre classe a au moins une fonction virtuelle, son destructeur doit l'être aussi.

- **Rule of Five partielle.** Implémenter le move constructor sans le copy constructor (ou l'inverse) laisse la classe dans un état incohérent. Si vous définissez un move constructor mais pas de copy constructor, le compilateur supprime implicitement le copy constructor — votre classe devient non copiable sans que ce soit intentionnel. Définissez ou `= default` / `= delete` les cinq explicitement.

- **Slicing problem.** Passer un objet dérivé par valeur à une fonction qui prend la classe de base coupe (slice) les membres de la classe dérivée. L'objet reçu est une copie tronquée à la base. Le polymorphisme est perdu. Solution : passer par référence (`const Base&`) ou par pointeur (`Base*`), jamais par valeur quand le polymorphisme est attendu.

- **Surcharge d'opérateur qui viole les attentes sémantiques.** Surcharger `operator+` pour qu'il modifie l'objet gauche (au lieu de retourner un nouvel objet) ou surcharger `operator==` de manière non symétrique crée du code qui se comporte de façon inattendue. Les opérateurs doivent respecter les conventions mathématiques : `a + b` ne modifie ni `a` ni `b`, `a == b` implique `b == a`. L'opérateur spaceship `<=>` (C++20) élimine une partie de ce risque en générant les comparaisons de manière cohérente.

---

## Compétences acquises

À l'issue de ce module, vous savez :
- Concevoir des classes C++ avec gestion correcte du cycle de vie (constructeurs, destructeur, Rule of Five).
- Appliquer RAII pour garantir la libération des ressources sans gestion manuelle.
- Implémenter une hiérarchie de classes avec polymorphisme dynamique et évaluer son coût en performance.
- Choisir entre héritage simple, multiple, et virtuel selon le problème.
- Surcharger les opérateurs de manière cohérente et idiomatique, y compris avec `<=>` (C++20).
- Identifier et éviter les bugs classiques de l'OOP C++ : destructeur non virtuel, slicing, Rule of Five partielle.

---


⏭️ [Classes et Encapsulation](/06-classes-encapsulation/README.md)
