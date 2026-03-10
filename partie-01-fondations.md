🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Partie I — Fondations

Cette partie pose les bases sur lesquelles repose l'intégralité de la formation : l'outillage Linux, la maîtrise du langage C++ dans ses fondamentaux, et le paradigme objet.  
Aucun raccourci ici. Même avec une expérience préalable en C++, les chapitres sur la toolchain (GCC 15, Clang 20, ccache, CMake) et sur la gestion mémoire méritent une lecture attentive — les pratiques ont évolué, et les mauvaises habitudes prises sur des bases fragiles se paient cher dans les parties suivantes.  

---

## Ce que vous allez maîtriser

- Vous serez capable d'installer, configurer et maintenir une toolchain C++ complète sur Ubuntu (GCC, Clang, CMake, Ninja, ccache, GDB).
- Vous serez capable de compiler un programme en contrôlant chaque étape du pipeline (préprocesseur, compilation, assemblage, édition de liens) et d'inspecter le binaire ELF résultant.
- Vous serez capable de choisir et justifier les options de compilation adaptées à chaque contexte (`-Wall -Wextra -Werror`, `-O2`, `-fsanitize=address`, `-std=c++23`).
- Vous serez capable de manipuler le système de types C++ : types primitifs, inférence avec `auto`/`decltype`, conversions explicites (`static_cast`, `dynamic_cast`), et qualificateurs `const`/`constexpr`/`consteval`.
- Vous serez capable de gérer la mémoire manuellement (stack vs heap, `new`/`delete`) et de détecter les erreurs avec Valgrind et AddressSanitizer.
- Vous serez capable de concevoir des classes respectant le principe RAII et la Rule of Five.
- Vous serez capable d'implémenter un polymorphisme dynamique via héritage, fonctions virtuelles et vtable, en comprenant le coût runtime associé.
- Vous serez capable de surcharger les opérateurs, y compris l'opérateur spaceship `<=>` (C++20).

---

## Prérequis

Aucun prérequis interne à la formation — c'est le point d'entrée.

Prérequis externes recommandés :
- Accès à une machine Ubuntu (22.04 LTS ou plus récent), physique ou VM.
- Familiarité minimale avec le terminal Linux (navigation, édition de fichiers, droits).
- Notions de programmation dans un langage quelconque (variables, boucles, fonctions).

---

## Modules de cette partie

| # | Titre | Niveau | Chapitres | Lien |
|---|-------|--------|-----------|------|
| Module 1 | L'Environnement de Développement sur Linux | Débutant | 1, 2 | [module-01-environnement-developpement-linux.md](/module-01-environnement-developpement-linux.md) |
| Module 2 | Les Fondamentaux du Langage | Débutant | 3, 4, 5 | [module-02-fondamentaux-langage.md](/module-02-fondamentaux-langage.md) |
| Module 3 | Programmation Orientée Objet | Débutant-Intermédiaire | 6, 7, 8 | [module-03-programmation-orientee-objet.md](/module-03-programmation-orientee-objet.md) |

---

## Fil conducteur

Le Module 1 installe l'environnement de travail : compilateurs, outils de build, IDE, et premières compilations manuelles. Sans cette étape, rien de ce qui suit ne peut être vérifié ni débogué. Le Module 2 enchaîne sur le langage lui-même — système de types, structures de contrôle, fonctions, et surtout gestion de la mémoire (stack, heap, pointeurs, allocation dynamique). C'est volontairement le module le plus dense : comprendre ce qui se passe en mémoire est la compétence qui sépare un développeur C++ compétent d'un développeur qui produit du code fragile. Le Module 3 introduit le paradigme objet en s'appuyant directement sur ces acquis — RAII n'a de sens que si on comprend la durée de vie des objets, et la Rule of Five n'a de sens que si on comprend copie et déplacement en mémoire. À la sortie de cette partie, vous avez un environnement fonctionnel, une compréhension solide du modèle mémoire, et les outils conceptuels pour aborder le C++ moderne (Partie II).

---


⏭️ [Module 1 : L'Environnement de Développement sur Linux](/module-01-environnement-developpement-linux.md)
