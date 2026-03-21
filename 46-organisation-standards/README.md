🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 46 : Organisation et Standards

> **Module 17 : Architecture de Projet Professionnel** · Partie VII : Projet et Professionnalisation  
> **Niveau** : Expert · **Prérequis** : Modules 1–16

---

## Introduction

Un programme C++ qui compile et produit le bon résultat n'est pas nécessairement un bon programme. Ce qui sépare un prototype fonctionnel d'un projet industriel maintenable, c'est l'ensemble des conventions, des structures et des pratiques qui entourent le code lui-même. L'organisation d'un projet C++ n'est pas un sujet cosmétique : elle a des conséquences directes sur la vitesse de compilation, la facilité d'intégration de nouveaux développeurs, la capacité à faire évoluer l'architecture sans tout casser, et la fiabilité du pipeline CI/CD.

En C++, cette question se pose avec une acuité particulière. Le modèle de compilation hérité du C — avec sa séparation entre fichiers d'en-tête et fichiers d'implémentation, son préprocesseur textuel et ses règles de linkage complexes — impose des contraintes qu'aucun autre langage mainstream ne partage. Un mauvais agencement de headers peut transformer une modification mineure en recompilation intégrale de vingt minutes. Un namespace mal choisi peut provoquer des collisions silencieuses entre des modules qui n'ont rien en commun. Une absence de convention de nommage rend les code reviews pénibles et les refactorings hasardeux.

Ce chapitre aborde les cinq piliers d'un projet C++ professionnel bien structuré.

---

## Ce que vous allez apprendre

### Organisation physique du projet

La disposition des répertoires n'est pas arbitraire. La convention `src/`, `include/`, `tests/`, `docs/` s'est imposée dans l'écosystème C++ pour des raisons pratiques : elle permet à CMake de découvrir les cibles automatiquement, elle clarifie ce qui est API publique (les headers dans `include/`) et ce qui est implémentation interne (les sources dans `src/`), et elle facilite l'installation des bibliothèques sur le système. Nous verrons comment structurer un projet depuis un petit utilitaire CLI jusqu'à un monorepo multi-bibliothèques, en passant par les projets intermédiaires typiques du monde professionnel.

### Séparation header/source et compilation incrémentale

La séparation `.h`/`.cpp` est l'un des mécanismes fondamentaux du C++, mais elle est souvent mal comprise ou mal exploitée. Nous examinerons comment cette séparation interagit avec le système de compilation incrémentale : pourquoi une modification dans un `.cpp` ne déclenche que la recompilation d'une seule unité de traduction, alors qu'un changement dans un `.h` peut provoquer une cascade de recompilations. Nous aborderons des techniques concrètes — forward declarations, idiome Pimpl, header guards vs `#pragma once` — pour minimiser les dépendances de compilation et maintenir des temps de build raisonnables même sur des projets de grande envergure.

### Namespaces et encapsulation des symboles

Les namespaces sont le mécanisme de C++ pour éviter la pollution de l'espace global de noms. Au-delà de la simple encapsulation, nous verrons comment concevoir une hiérarchie de namespaces cohérente, comment utiliser les namespaces anonymes et `inline namespaces` pour gérer le versioning d'API, et pourquoi `using namespace std;` dans un header est une erreur que l'on paie tôt ou tard.

### Documentation avec Doxygen

Le code qui n'est pas documenté est du code qui sera mal compris, mal utilisé, puis réécrit. Doxygen reste l'outil de référence pour générer une documentation technique à partir de commentaires structurés dans le code source C++. Nous couvrirons la syntaxe des commentaires Doxygen, les bonnes pratiques pour documenter une API publique sans surcharger le code d'implémentation, et la génération automatisée de documentation HTML intégrée au pipeline CI/CD.

### Standards de codage

Un standard de codage n'est pas une question d'esthétique personnelle : c'est un contrat entre les membres d'une équipe. Nous passerons en revue les trois guides de style majeurs de l'industrie — le Google C++ Style Guide, le LLVM Style et les C++ Core Guidelines de Stroustrup et Sutter — en analysant leurs philosophies respectives, leurs points de convergence et leurs divergences. L'objectif n'est pas de décréter qu'un guide est supérieur aux autres, mais de comprendre les raisonnements sous-jacents pour faire un choix éclairé et, surtout, s'y tenir.

---

## Pourquoi ce chapitre arrive en fin de formation

Il aurait été tentant de placer ce contenu au début du parcours, puisqu'on « devrait » organiser son projet correctement dès le départ. En pratique, les conventions d'organisation ne prennent leur plein sens que lorsqu'on a expérimenté les problèmes qu'elles résolvent. Après avoir vu CMake en détail (chapitre 26), configuré des pipelines CI/CD (chapitre 38), écrit des tests unitaires (chapitre 33) et souffert de temps de compilation excessifs, vous avez maintenant le contexte nécessaire pour comprendre *pourquoi* chaque convention existe — et pas seulement *comment* l'appliquer mécaniquement.

C'est aussi le moment d'unifier tout ce que vous avez appris. L'organisation d'un projet professionnel est un point de convergence : elle touche au build system, au testing, au déploiement, à la documentation, à la collaboration en équipe. Ce chapitre est le liant entre les compétences techniques acquises dans les modules précédents et leur mise en œuvre dans un contexte réel.

---

## Liens avec les autres chapitres

Ce chapitre s'appuie directement sur plusieurs sections de la formation et les complète :

- **Chapitre 26 — CMake** : l'organisation des répertoires détermine la structure du `CMakeLists.txt`. Les conventions `PUBLIC`/`PRIVATE`/`INTERFACE` (section 26.2.4) prennent tout leur sens lorsqu'on distingue clairement headers publics et sources internes.  
- **Chapitre 27 — Gestion des dépendances** : Conan et vcpkg s'intègrent d'autant mieux que le projet suit une structure canonique. Les CMake Presets (section 27.6) standardisent la configuration par-dessus l'organisation physique.  
- **Chapitre 32 — Analyse statique** : `clang-format` (section 32.3) et `clang-tidy` (section 32.1) appliquent automatiquement une partie des standards de codage abordés ici.  
- **Chapitre 33 — Google Test** : la séparation `tests/` et la convention de nommage des fichiers de test s'inscrivent dans l'organisation globale du projet.  
- **Chapitre 38 — CI/CD** : un projet bien structuré est un projet dont le pipeline CI/CD est simple à écrire et à maintenir.  
- **Chapitre 47 — Collaboration et maintenance** : les pre-commit hooks (section 47.2) et les workflows Git (section 47.1) viennent appliquer et enforcer les standards définis dans ce chapitre.

---

## Sommaire du chapitre

- **46.1** — Organisation des répertoires (`src/`, `include/`, `tests/`, `docs/`)  
- **46.2** — Séparation `.h`/`.cpp` et compilation incrémentale  
- **46.3** — Namespaces et éviter la pollution globale  
- **46.4** — Documentation : Doxygen et commentaires  
    - 46.4.1 — Syntaxe Doxygen  
    - 46.4.2 — Génération de documentation  
- **46.5** — Standards de codage  
    - 46.5.1 — Google C++ Style Guide  
    - 46.5.2 — LLVM Style  
    - 46.5.3 — C++ Core Guidelines

---


⏭️ [Organisation des répertoires (src/, include/, tests/, docs/)](/46-organisation-standards/01-organisation-repertoires.md)
