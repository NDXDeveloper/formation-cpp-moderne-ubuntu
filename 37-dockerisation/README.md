🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 37 : Dockerisation d'Applications C++

> **Niveau** : Avancé  
> **Module** : 13 — C++ dans une Approche DevOps  
> **Partie** : V — DevOps et Cloud Native  
> **Prérequis** : Chapitres 26 (CMake), 27 (Gestion des dépendances), 28 (Build systems)  
> **Durée estimée** : 4–5 heures

---

## Pourquoi ce chapitre est essentiel

Compiler un binaire C++ sur une machine de développement et le déployer en production sur un serveur différent est un exercice qui, historiquement, a toujours posé problème. Différences de versions de la glibc, librairies partagées absentes, incompatibilités ABI entre distributions : le fameux *"ça marche sur ma machine"* est presque un rite de passage pour tout développeur C++.

Docker résout ce problème en encapsulant l'intégralité de l'environnement d'exécution — le binaire, ses dépendances, et le système de fichiers minimal nécessaire — dans un conteneur reproductible et portable. Mais contrairement à Python ou Node.js, où la conteneurisation est relativement directe, le C++ impose des contraintes spécifiques qui méritent un traitement dédié :

- **Le binaire compilé est natif** : il dépend de l'architecture CPU et de l'ABI du système cible. On ne copie pas simplement des fichiers source dans un conteneur — il faut compiler *à l'intérieur* ou *pour* l'environnement cible.  
- **Les dépendances sont complexes** : librairies partagées (.so), versions de la glibc ou de musl, dépendances transitives installées via Conan, vcpkg ou le système de paquets de la distribution.  
- **La taille des images compte** : une toolchain C++ complète (GCC/Clang, CMake, headers, librairies de développement) pèse facilement plusieurs gigaoctets. Expédier tout cela en production est un gaspillage de ressources et une surface d'attaque inutile.  
- **La sécurité est critique** : un binaire C++ tourne souvent avec des responsabilités système élevées (serveurs réseau, processing de données sensibles). Réduire la surface d'attaque du conteneur est une priorité.

Ce chapitre vous apprend à construire des images Docker optimisées, sécurisées et adaptées aux spécificités du C++ compilé.

---

## Ce que vous allez apprendre

Ce chapitre couvre l'intégralité du workflow de conteneurisation d'une application C++, de la construction de l'image à son déploiement.

**Choix de l'image de base** — Ubuntu offre un écosystème familier et une compatibilité maximale avec les librairies, tandis qu'Alpine, basée sur musl, produit des images beaucoup plus légères mais impose des contraintes de compilation. Vous apprendrez à faire un choix éclairé en fonction de votre contexte.

**Multi-stage builds** — La technique fondamentale pour séparer l'environnement de compilation (lourd, riche en outils) de l'environnement d'exécution (minimal, ne contenant que le binaire et ses dépendances runtime). C'est la clé pour passer d'images de plusieurs gigaoctets à des images de quelques dizaines de mégaoctets.

**Gestion des librairies partagées** — Identifier, copier et organiser les `.so` nécessaires à l'exécution du binaire dans le conteneur final, en évitant les erreurs classiques de linkage dynamique.

**Bonnes pratiques de sécurité et reproductibilité** — Utilisateurs non-root, images signées, pinning des versions, scans de vulnérabilités, et construction déterministe.

**Images distroless** — L'approche la plus radicale : des conteneurs qui ne contiennent littéralement rien d'autre que le binaire et ses dépendances minimales — pas de shell, pas de gestionnaire de paquets, pas de surface d'attaque superflue.

---

## Le défi spécifique du C++ conteneurisé

Pour bien comprendre pourquoi ce chapitre existe en tant que tel, comparons brièvement avec d'autres langages.

En Python, un `Dockerfile` typique installe l'interpréteur, copie les sources, lance `pip install` et exécute le script. L'image finale contient tout : l'interpréteur, les sources, les dépendances. Le processus est simple et linéaire.

En Go, le compilateur produit un binaire statique sans dépendances externes. Le `Dockerfile` compile, puis copie le binaire seul dans une image `scratch` ou `distroless`. Le résultat est trivial et ultra-léger.

En C++, la situation est intermédiaire mais plus complexe que les deux :

- Comme Go, on produit un binaire natif — mais ce binaire a généralement des **dépendances dynamiques** (glibc, libstdc++, OpenSSL, etc.).  
- Comme Python, on a besoin d'un **environnement de build riche** — mais cet environnement ne doit absolument pas se retrouver en production.  
- Contrairement aux deux, les **incompatibilités ABI** entre distributions et versions de librairies rendent la portabilité du binaire non triviale.

Le multi-stage build est donc la pierre angulaire de toute conteneurisation C++ sérieuse, et c'est pourquoi une section entière lui est consacrée.

---

## Architecture typique d'un Dockerfile C++

Voici le schéma général que nous allons détailler dans les sections suivantes :

```
┌─────────────────────────────────────────────────┐
│  Stage 1 : BUILD                                │
│  Image de base : ubuntu:24.04 ou debian:bookworm│
│                                                 │
│  ► Installation toolchain (g++, cmake, ninja)   │
│  ► Installation dépendances (conan, vcpkg, apt) │
│  ► Copie des sources                            │
│  ► Compilation (cmake --build)                  │
│  ► Tests (ctest)                                │
│                                                 │
│  Résultat : binaire + .so identifiées           │
├─────────────────────────────────────────────────┤
│  Stage 2 : RUNTIME                              │
│  Image de base : ubuntu:24.04 (minimal)         │
│             ou : distroless/cc-debian12         │
│             ou : alpine:3.20                    │
│                                                 │
│  ► Copie du binaire depuis stage 1              │
│  ► Copie des .so nécessaires                    │
│  ► Configuration utilisateur non-root           │
│  ► ENTRYPOINT / CMD                             │
│                                                 │
│  Résultat : image de production (50-150 MB)     │
└─────────────────────────────────────────────────┘
```

Ce schéma peut être étendu avec des stages intermédiaires supplémentaires — par exemple un stage dédié à Conan pour le cache des dépendances, ou un stage de tests séparé pour ne pas alourdir l'image finale.

---

## Plan du chapitre

| Section | Contenu | Points clés |
|---------|---------|-------------|
| **37.1** | Création d'images Docker pour C++ (Ubuntu vs Alpine) | Choix de la distribution de base, implications sur la glibc/musl, taille des images |
| **37.2** | Multi-stage builds : Optimisation de la taille | Séparation build/runtime, stage de compilation, stage d'exécution minimal |
| **37.3** | Gestion des librairies partagées dans les conteneurs | `ldd`, copie des `.so`, `LD_LIBRARY_PATH`, linkage statique vs dynamique |
| **37.4** | Best practices : Sécurité et reproductibilité | Utilisateur non-root, `.dockerignore`, layer caching, pinning, scans CVE |
| **37.5** | Distroless images : Conteneurs minimaux | Images Google distroless, `scratch`, surface d'attaque minimale |

---

## Prérequis techniques

Avant d'aborder ce chapitre, assurez-vous de maîtriser les bases suivantes.

**C++ et build systems** — Vous devez être à l'aise avec CMake (chapitre 26), la gestion des dépendances avec Conan ou vcpkg (chapitre 27), et comprendre la différence entre linkage statique et dynamique (section 27.4). Les concepts de compilation vus dans les chapitres 1 et 2 (cycle de compilation, inspection des binaires avec `ldd`, `nm`, `objdump`) seront directement réutilisés.

**Docker** — Ce chapitre suppose que vous avez une connaissance fonctionnelle de Docker : construction d'images (`docker build`), exécution de conteneurs (`docker run`), syntaxe de base d'un `Dockerfile` (instructions `FROM`, `RUN`, `COPY`, `CMD`). Si ce n'est pas le cas, la documentation officielle Docker constitue un excellent point de départ.

**Linux** — Les notions de permissions Unix, de gestion des paquets apt, et de librairies partagées sont mobilisées tout au long du chapitre.

---

## Conventions utilisées dans ce chapitre

Les exemples de `Dockerfile` sont conçus pour fonctionner sur une machine hôte x86_64 sous Ubuntu. Les commandes `docker build` et `docker run` sont exécutées depuis la racine d'un projet C++ ayant la structure CMake standard vue au chapitre 46.

Tous les fichiers `Dockerfile` présentés sont complets et testables. Les tailles d'images mentionnées sont indicatives et correspondent à des mesures effectuées en mars 2026 avec Docker Engine 27.x.

Lorsqu'un choix de conception est discutable (par exemple, Ubuntu vs Alpine, linkage statique vs dynamique), les deux options sont présentées avec leurs compromis respectifs afin que vous puissiez prendre une décision adaptée à votre contexte.

---

*Commençons par le choix fondamental : quelle image de base utiliser pour compiler et exécuter du C++ dans un conteneur ?*


⏭️ [Création d'images Docker pour C++ (Ubuntu vs Alpine)](/37-dockerisation/01-images-docker.md)
