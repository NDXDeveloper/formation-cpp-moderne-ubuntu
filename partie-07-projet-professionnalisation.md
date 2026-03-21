🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Partie VII — Projet et Professionnalisation

Cette partie traite de ce que les livres techniques omettent : comment organiser, maintenir et faire évoluer un projet C++ en équipe sur la durée. Structure de répertoires, séparation headers/sources, standards de codage (Google, LLVM, C++ Core Guidelines), pre-commit hooks avec clang-format et clang-tidy, workflows Git, code reviews, gestion de la dette technique, semantic versioning. C'est aussi le moment de construire votre veille technologique — C++26 est ratifié, C++29 est en préparation, et les conférences et communautés actives sont votre meilleur outil pour rester à jour.

---

## Ce que vous allez maîtriser

- Vous serez capable de structurer un projet C++ professionnel selon les conventions établies (`src/`, `include/`, `tests/`, `docs/`), avec une séparation `.h`/`.cpp` qui optimise la compilation incrémentale.
- Vous serez capable d'utiliser les namespaces pour éviter la pollution de l'espace de noms global et de structurer un codebase de grande taille.
- Vous serez capable de documenter du code avec Doxygen et de générer une documentation technique exploitable.
- Vous serez capable de choisir et d'appliquer un standard de codage (Google C++ Style Guide, LLVM Style, C++ Core Guidelines) et de l'imposer automatiquement via les outils.
- Vous serez capable de configurer un pipeline pre-commit complet pour C++ : clang-format pour le formatage, clang-tidy pour l'analyse statique, et tests rapides — exécutés avant chaque commit.
- Vous serez capable de choisir un workflow Git adapté à votre équipe (GitFlow vs trunk-based development) et de mener des code reviews efficaces.
- Vous serez capable d'identifier, de quantifier et de gérer la dette technique dans un projet C++ à long terme.
- Vous serez capable d'appliquer le Semantic Versioning et de maintenir des changelogs exploitables.
- Vous serez capable de suivre l'évolution du standard C++ (processus ISO, proposals sur open-std.org et GitHub), de positionner votre codebase par rapport à C++26 et d'anticiper C++29.
- Vous serez capable d'identifier les trajectoires professionnelles accessibles (system programming, backend haute performance, embedded/IoT, finance quantitative, DevOps/SRE, game development) et les compétences différenciantes pour chacune.

---

## Prérequis

- **Partie IV — Tooling et Build Systems** : CMake (chapitre 26), clang-tidy et clang-format (chapitre 32), Google Test (chapitre 33) — les pre-commit hooks et la structure de projet s'appuient directement sur ces outils.
- **Partie V — DevOps et Cloud Native** : CI/CD (chapitre 38) — les pre-commit hooks complètent le pipeline CI et doivent être compris en cohérence avec lui.
- **Parties I à VI complétées** pour la Conclusion et la checklist Cloud Native, qui synthétisent l'ensemble de la formation.

---

## Modules de cette partie

| # | Titre | Niveau | Chapitres | Lien |
|---|-------|--------|-----------|------|
| Module 17 | Architecture de Projet Professionnel | Expert | 46, 47, 48 + Conclusion | [module-17-architecture-projet.md](/module-17-architecture-projet.md) |

---

## Fil conducteur

Le chapitre 46 pose la structure technique d'un projet : organisation des répertoires, séparation headers/sources pour la compilation incrémentale, namespaces, documentation Doxygen, et choix d'un standard de codage. Le chapitre 47 passe à la dimension collaborative : workflows Git, pre-commit hooks (clang-format, clang-tidy, tests rapides), code reviews, dette technique et semantic versioning. C'est le chapitre le plus opérationnel — il transforme un ensemble de bonnes pratiques individuelles en un processus d'équipe reproductible. Le chapitre 48 ouvre la perspective au-delà de la formation : livres de référence (Effective C++, C++ Concurrency in Action, A Tour of C++, Embracing Modern C++ Safely), conférences (CppCon, Meeting C++, C++ Now, ACCU), suivi du processus de standardisation ISO avec un aperçu des premières proposals C++29, et communautés actives (r/cpp, Discord C++, Compiler Explorer). La Conclusion récapitule les compétences acquises, trace les trajectoires professionnelles possibles, et fournit une checklist opérationnelle pour le développeur C++ Cloud Native.

---


⏭️ [Module 17 : Architecture de Projet Professionnel](/module-17-architecture-projet.md)
