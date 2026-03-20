🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 38. CI/CD pour C++ ⭐

## Introduction

L'intégration continue (CI) et le déploiement continu (CD) sont devenus des pratiques incontournables dans le développement logiciel professionnel. Pourtant, dans l'écosystème C++, leur mise en place reste souvent perçue comme un défi nettement plus complexe que dans des langages comme Python, Go ou JavaScript. Cette perception n'est pas infondée : la compilation C++ est intrinsèquement plus lente, la gestion des dépendances moins standardisée, et la diversité des plateformes cibles (architectures, systèmes d'exploitation, compilateurs) ajoute une combinatoire que peu d'autres langages imposent.

Ce chapitre vous montre comment construire des pipelines CI/CD robustes, rapides et maintenables pour vos projets C++, en tirant parti des outils et des pratiques modernes disponibles en 2026.

## Pourquoi le CI/CD est critique pour un projet C++

Un projet C++ sans pipeline automatisé accumule rapidement de la dette technique invisible. Contrairement à un langage interprété où une erreur se manifeste à l'exécution, une régression en C++ peut prendre des formes insidieuses : un comportement indéfini silencieux, une fuite mémoire qui n'apparaît que sous charge, un bug de concurrence intermittent, ou simplement une rupture de compilation sur un compilateur que personne dans l'équipe n'a testé localement.

Un pipeline CI/CD bien conçu adresse ces problèmes en automatisant plusieurs étapes fondamentales :

- **La compilation multi-compilateur** — Un même code peut se comporter différemment entre GCC 15 et Clang 20. Compiler systématiquement avec les deux détecte les dépendances involontaires à un comportement spécifique d'un compilateur.  
- **L'exécution des tests** — Les tests unitaires (Google Test), les tests d'intégration et les benchmarks doivent tourner à chaque commit ou merge request, pas uniquement sur la machine du développeur.  
- **L'analyse statique et le formatage** — clang-tidy et clang-format, intégrés au pipeline, garantissent que le code respecte les standards définis par l'équipe sans reposer sur la discipline individuelle.  
- **La détection de problèmes mémoire** — Les sanitizers (AddressSanitizer, UndefinedBehaviorSanitizer, ThreadSanitizer) peuvent être activés dans des jobs CI dédiés pour capturer des bugs que les tests classiques ne révèlent pas.  
- **Le packaging et la distribution** — La génération automatique de paquets DEB, RPM, d'images Docker ou d'artifacts binaires élimine les erreurs de packaging manuel et garantit la reproductibilité.

## Les défis spécifiques au C++

Mettre en place du CI/CD pour un projet C++ diffère significativement de ce que l'on ferait pour un projet Python ou Node.js. Plusieurs contraintes méritent d'être comprises avant de concevoir un pipeline.

### Des temps de compilation élevés

Un projet C++ de taille moyenne (quelques centaines de fichiers source) peut facilement nécessiter plusieurs minutes de compilation complète. Un projet de grande envergure peut dépasser la demi-heure. Sans stratégie d'accélération, chaque push déclenche une attente qui ralentit toute l'équipe.

Les réponses à ce problème existent et seront détaillées dans ce chapitre : **ccache** pour le cache de compilation local, **sccache** pour le cache distribué dans le cloud, la génération avec **Ninja** plutôt que Make, et les **matrix builds** intelligemment découpés.

### La matrice compilateur × standard × plateforme

Un projet C++ sérieux doit souvent garantir la compatibilité avec plusieurs compilateurs (GCC, Clang, éventuellement MSVC), plusieurs versions de ces compilateurs, plusieurs standards du langage (C++17, C++20, C++23) et parfois plusieurs architectures (x86_64, ARM, RISC-V). Cette combinatoire explose rapidement le nombre de jobs CI si elle n'est pas gérée intelligemment via des **matrix builds**.

### La gestion des dépendances

Contrairement à `pip install` ou `npm install`, installer les dépendances d'un projet C++ en CI requiert une stratégie explicite. Selon le projet, cela peut impliquer Conan 2.0, vcpkg, FetchContent de CMake, ou des paquets système installés via `apt`. Chaque approche a des implications sur la reproductibilité et la vitesse du pipeline.

### Les binaires et le linkage

Le résultat d'un build C++ est un binaire natif, avec toutes les implications que cela comporte : dépendances aux librairies partagées (.so), compatibilité ABI, linkage statique vs dynamique. Le pipeline doit garantir que le binaire produit fonctionnera dans l'environnement cible, ce qui est trivial quand on distribue un script Python mais nettement moins évident avec un exécutable ELF.

## Architecture type d'un pipeline CI/CD pour C++

Un pipeline CI/CD mature pour un projet C++ s'organise généralement en plusieurs stages séquentiels, chacun agissant comme un filtre de qualité :

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│    Lint &   │    │    Build    │    │    Test &   │    │   Package   │    │   Deploy    │
│   Format    │───▶│  (compile)  │───▶│  Sanitize   │───▶│  (artifact) │───▶│  (release)  │
│             │    │             │    │             │    │             │    │             │
└─────────────┘    └─────────────┘    └─────────────┘    └─────────────┘    └─────────────┘
     ▲                   ▲                                      │
     │                   │                                      ▼
 clang-format        ccache /                             DEB, RPM,
 clang-tidy          sccache                              Docker image,
                                                          binaire statique
```

**Stage 1 — Lint & Format.** Vérifie le formatage (clang-format) et exécute l'analyse statique (clang-tidy). Ce stage est rapide et échoue tôt si le code ne respecte pas les conventions. Il ne nécessite pas de compilation complète.

**Stage 2 — Build.** Compile le projet avec CMake + Ninja, en exploitant ccache ou sccache pour accélérer les builds incrémentaux. C'est ici que les matrix builds interviennent pour tester plusieurs combinaisons compilateur/standard.

**Stage 3 — Test & Sanitize.** Exécute les tests unitaires et d'intégration via CTest ou directement les binaires Google Test. En parallèle, des jobs dédiés compilent et testent avec les sanitizers activés (ASan, UBSan, TSan).

**Stage 4 — Package.** Génère les artifacts de distribution : paquets DEB/RPM, images Docker (multi-stage builds), binaires statiques, ou archives. Ces artifacts sont versionnés et stockés.

**Stage 5 — Deploy.** Optionnel selon le contexte : déploiement sur un registre Docker, publication sur un dépôt de paquets, ou création d'une release GitHub/GitLab avec les binaires attachés.

## Plateformes CI/CD couvertes

Ce chapitre couvre en détail les deux plateformes CI/CD les plus utilisées dans l'écosystème C++ open source et professionnel :

**GitLab CI/CD** est particulièrement répandu dans les entreprises européennes et les organisations qui hébergent leur propre infrastructure. Son modèle de runners auto-hébergés offre un contrôle total sur l'environnement de build, ce qui est souvent apprécié pour les projets C++ nécessitant des toolchains spécifiques ou des ressources matérielles particulières (GPU, accès à du matériel embarqué).

**GitHub Actions** domine l'écosystème open source et offre un marketplace d'actions pré-construites qui simplifie considérablement la mise en place initiale. Les runners hébergés par GitHub fournissent un environnement Ubuntu préconfiguré avec GCC et Clang déjà installés, ce qui réduit le boilerplate.

Les principes fondamentaux — structure en stages, cache de compilation, matrix builds, intégration des sanitizers — sont identiques quelle que soit la plateforme. Seule la syntaxe de configuration diffère.

## Prérequis

Ce chapitre s'appuie sur des connaissances couvertes dans les chapitres précédents :

- **CMake** (chapitre 26) — le build system utilisé dans tous les exemples de pipeline.  
- **Google Test** (chapitre 33) — le framework de test référencé dans les stages de test.  
- **clang-tidy et clang-format** (chapitre 32) — les outils d'analyse statique intégrés aux pipelines.  
- **Docker et multi-stage builds** (chapitre 37) — nécessaire pour le stage de packaging en conteneur.  
- **ccache** (section 2.3) — le mécanisme de cache de compilation qui sera étendu au contexte CI.  
- **Ninja** (section 28.3) — le build system recommandé pour la vitesse en CI.

## Ce que vous apprendrez

À l'issue de ce chapitre, vous saurez :

1. Écrire un pipeline **GitLab CI** complet pour un projet C++ CMake, du lint au packaging.
2. Configurer un workflow **GitHub Actions** équivalent avec les actions spécifiques à C++.
3. Réduire drastiquement les temps de build en CI grâce à **ccache** et **sccache**.
4. Automatiser l'exécution des tests, de l'analyse statique et des sanitizers.
5. Gérer les artifacts, les releases et la publication de binaires.
6. Mettre en place de la **cross-compilation** pour ARM et RISC-V depuis un runner x86_64.
7. Exploiter les **matrix builds** pour valider votre code sur plusieurs compilateurs et standards simultanément.

---

> **Dans les sections suivantes**, nous commencerons par la mise en place d'un pipeline GitLab CI complet (section 38.1), avant de passer à GitHub Actions (section 38.2), puis aux techniques d'accélération (section 38.3) et à l'automatisation avancée (sections 38.4 à 38.7).

⏭️ [Pipeline GitLab CI : Build → Test → Package](/38-cicd/01-gitlab-ci.md)
