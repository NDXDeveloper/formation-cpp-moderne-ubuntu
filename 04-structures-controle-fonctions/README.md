🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 4 : Structures de Contrôle et Fonctions

## Module 2 — Les Fondamentaux du Langage · Niveau Débutant

---

## Objectif du chapitre

Le chapitre précédent vous a donné les briques élémentaires du langage : types, variables, opérateurs. Mais un programme ne se résume pas à déclarer des données — il doit **prendre des décisions**, **répéter des opérations** et **structurer sa logique** en unités réutilisables. C'est exactement le rôle des structures de contrôle et des fonctions.

Ce chapitre couvre deux axes fondamentaux :

1. **Le flux d'exécution** — comment le programme choisit quel chemin emprunter (`if`, `switch`) et comment il répète des blocs d'instructions (boucles `for`, `while`, `do-while`, et la forme moderne `range-based for`).
2. **Les fonctions** — comment découper un programme en unités logiques, passer des données entre ces unités de manière efficace, et tirer parti de mécanismes comme la surcharge et les valeurs par défaut.

En C++ moderne, ces mécanismes vont bien au-delà de leur équivalent en C. Le langage offre des constructions comme `if constexpr` (évaluation à la compilation), l'initialisation dans les `switch` (C++17), ou encore le `range-based for` qui simplifie drastiquement l'itération sur les conteneurs de la STL. Ce chapitre les couvre tous, en insistant systématiquement sur la forme idiomatique moderne.

---

## Ce que vous allez apprendre

- Écrire des conditions et des boucles en utilisant les formes modernes du langage (C++17 et au-delà).  
- Déclarer, définir et appeler des fonctions en C++.  
- Comprendre les différents modes de passage de paramètres (valeur, référence, référence constante, pointeur) et savoir **quand utiliser lequel**.  
- Exploiter la surcharge de fonctions pour fournir des interfaces expressives.  
- Utiliser les valeurs par défaut et le mot-clé `inline` à bon escient.

---

## Prérequis

Ce chapitre suppose que vous maîtrisez les notions vues au chapitre 3 :

- Les types fondamentaux et l'inférence de type (`auto`, `decltype`).  
- La notion de portée (scope) et de durée de vie (lifetime) d'une variable.  
- Les qualificateurs `const` et `constexpr`.

Si des notions comme la différence entre une variable sur la stack et sur le heap vous semblent floues, un retour au chapitre 5 (Gestion de la Mémoire) pourra être utile une fois ce chapitre terminé — les deux se complètent.

---

## Plan du chapitre

| Section | Sujet | Points clés |  
|---------|-------|-------------|  
| **4.1** | Conditionnelles et boucles | `if` / `else`, `if constexpr`, `switch` avec initialisation (C++17), `range-based for` |  
| **4.2** | Déclaration et définition de fonctions | Prototypes, séparation `.h`/`.cpp`, ODR (One Definition Rule) |  
| **4.3** | Passage de paramètres | Par valeur, par référence (`&`), par référence constante (`const &`), par pointeur (`*`) |  
| **4.4** | Surcharge de fonctions | Résolution de surcharge, ambiguïtés, bonnes pratiques |  
| **4.5** | Valeurs par défaut et fonctions `inline` | Paramètres optionnels, inlining et impact sur la compilation |

---

## Conventions de code

Tous les exemples de ce chapitre sont compilables avec :

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -o programme fichier.cpp
```

Certaines fonctionnalités comme `if constexpr` nécessitent au minimum **C++17**. Les cas où une version plus récente est requise sont signalés explicitement.

L'affichage se fait via `std::cout` dans ce chapitre, puisque `std::print` (C++23) est présenté en détail à la section 2.7 et couvert en profondeur à la section 12.7. Rien ne vous empêche de l'utiliser si votre compilateur le supporte déjà.

---

## Pourquoi ce chapitre est important

Les structures de contrôle et les fonctions constituent le **squelette** de tout programme C++. Même les abstractions les plus avancées — templates, lambdas, coroutines — reposent sur ces briques de base. Deux aspects méritent une attention particulière :

**Le passage de paramètres** est une source fréquente de bugs de performance et de logique chez les développeurs venant d'autres langages. En Python ou en Java, la question ne se pose quasiment pas. En C++, choisir entre passage par valeur, par référence ou par pointeur a un impact direct sur la copie des données, la durée de vie des objets et la lisibilité du code. La section 4.3 y est entièrement consacrée.

**La surcharge de fonctions** est un mécanisme puissant mais qui peut devenir piégeux si les règles de résolution ne sont pas bien comprises. La section 4.4 clarifie ces règles et pose les bases nécessaires avant d'aborder les templates au chapitre 16.

---

## Liens avec les autres chapitres

- **Chapitre 3** (Types, Variables et Opérateurs) — prérequis direct, notamment pour `const` et `auto`.  
- **Chapitre 5** (Gestion de la Mémoire) — comprendre stack vs heap éclaire le choix du mode de passage de paramètres.  
- **Chapitre 6** (Classes et Encapsulation) — les fonctions membres reprennent tous les concepts de ce chapitre.  
- **Chapitre 11** (Lambdas) — les lambdas sont des fonctions anonymes ; ce chapitre en pose les fondations.  
- **Chapitre 16** (Templates) — la surcharge de fonctions est le point de départ vers la programmation générique.

---

> 💡 **Conseil** — Ce chapitre se prête particulièrement bien à l'expérimentation. N'hésitez pas à compiler chaque exemple, à modifier les paramètres, et à observer le comportement du programme. Le compilateur est votre meilleur professeur : avec `-Wall -Wextra`, il vous signalera la plupart des erreurs classiques avant même l'exécution.

⏭️ [Conditionnelles et boucles](/04-structures-controle-fonctions/01-conditionnelles-boucles.md)
