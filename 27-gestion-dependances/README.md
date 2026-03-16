🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 27. Gestion des Dépendances ⭐

> **Niveau** : Avancé  
> **Partie** : IV — Tooling et Build Systems  
> **Module** : 9 — Build Systems et Gestion de Projet  
> **Durée estimée** : 5–7 heures  
> **Prérequis** : Chapitre 26 (CMake), en particulier les sections 26.3 (`find_package`, `FetchContent`, `add_subdirectory`) et 26.4 (génération de fichiers de configuration d'export)

---

## Objectifs du chapitre

À l'issue de ce chapitre, vous serez capable de :

- identifier les défis spécifiques de la gestion des dépendances en C++ et les raisons historiques de cette complexité ;
- installer, configurer et utiliser **Conan 2.0** pour gérer les dépendances d'un projet CMake professionnel ;
- comprendre le fonctionnement de **vcpkg** et savoir quand le privilégier par rapport à Conan ;
- maîtriser les implications du **linkage statique** versus **dynamique** et faire le bon choix selon le contexte ;
- installer et distribuer vos propres bibliothèques sur un système Linux ;
- standardiser les configurations de build avec les **CMake Presets**.

---

## De CMake aux gestionnaires de paquets

Le chapitre 26 a couvert les mécanismes natifs de CMake pour intégrer des dépendances : `find_package()` pour les bibliothèques pré-installées, `FetchContent` pour le téléchargement automatique, et `add_subdirectory()` pour les sources embarquées. Ces mécanismes sont fondamentaux et suffisent pour de nombreux projets. Mais ils laissent une question ouverte : **qui installe les bibliothèques que `find_package()` cherche ?**

Sur un poste de développement, la réponse est souvent `apt install libssl-dev` ou une compilation manuelle. En CI/CD, c'est un script de setup dans le pipeline. Sur la machine d'un collègue, c'est « ça devrait marcher si tu as les bonnes versions ». Cette approche ad hoc est la source d'un problème que tout développeur C++ a rencontré : le build qui fonctionne sur une machine et casse sur une autre, à cause d'une version de bibliothèque différente, d'un header manquant, ou d'une incompatibilité ABI silencieuse.

Les **gestionnaires de paquets C++** résolvent ce problème en automatisant le téléchargement, la compilation, l'installation et la gestion de versions des bibliothèques tierces, de manière **reproductible** et **déclarative**. Ils comblent le vide que CMake ne prétend pas remplir : CMake sait *consommer* des dépendances, les gestionnaires de paquets savent les *fournir*.

---

## L'écosystème en 2026

Deux gestionnaires de paquets dominent l'écosystème C++ en 2026, avec des philosophies et des forces différentes.

### Conan 2.0

Conan est un gestionnaire de paquets décentralisé, développé par JFrog. Sa version 2.0, sortie en 2023, a été une refonte majeure de l'API et du modèle de configuration. Conan est orienté vers la flexibilité : il supporte de multiples build systems, de multiples plateformes, et offre un contrôle fin sur les settings de compilation (compilateur, standard C++, architecture, type de build). Son modèle repose sur des **recettes** Python (`conanfile.py`) qui décrivent comment construire et consommer chaque paquet.

Conan est particulièrement adapté aux projets qui ciblent plusieurs plateformes, qui nécessitent de la cross-compilation, ou qui ont des contraintes ABI spécifiques. Son écosystème de recettes communautaires, **ConanCenter**, contient des milliers de bibliothèques prêtes à l'emploi.

### vcpkg

vcpkg est un gestionnaire de paquets développé par Microsoft. Sa philosophie est la simplicité : un fichier `vcpkg.json` déclare les dépendances, et vcpkg les installe. L'intégration avec CMake est transparente grâce à un fichier toolchain. vcpkg fonctionne en mode « manifest » (déclaratif, par projet) ou en mode « classic » (installation globale, similaire à `apt`).

vcpkg est particulièrement adapté aux projets qui ciblent principalement Windows et Linux, qui veulent une intégration CMake minimale, ou qui préfèrent une approche « ça marche out of the box ». Son catalogue de bibliothèques est vaste et régulièrement mis à jour.

### Positionnement comparé

| Aspect | Conan 2.0 | vcpkg |
|--------|-----------|-------|
| Philosophie | Flexible, décentralisé | Simple, centralisé |
| Configuration | `conanfile.py` (Python) | `vcpkg.json` (JSON) |
| Intégration CMake | Via toolchain généré | Via toolchain vcpkg |
| Gestion ABI | Contrôle fin (settings, options) | Triplets (plus simple) |
| Cross-compilation | Support natif (profils host/build) | Via triplets custom |
| Serveur privé | Conan Server, Artifactory | Registres vcpkg custom |
| Binaires pré-compilés | Cache binaire configurable | Cache binaire GitHub |
| Écosystème | ConanCenter (~1500+ recettes) | vcpkg registry (~2500+ ports) |
| Courbe d'apprentissage | Modérée (Python, concepts ABI) | Douce (JSON, triplets) |

Les deux outils sont matures, activement maintenus, et utilisés en production dans l'industrie. Le choix entre eux dépend souvent du contexte de l'équipe et de l'infrastructure existante plutôt que de différences techniques fondamentales. Nous couvrirons Conan 2.0 en profondeur (section 27.2) car sa flexibilité illustre mieux les problématiques sous-jacentes, puis vcpkg de manière plus concise (section 27.3).

---

## Au-delà des gestionnaires de paquets

La gestion des dépendances ne se limite pas à l'installation de bibliothèques. Elle englobe des questions architecturales que ce chapitre aborde :

**Linkage statique vs dynamique** (section 27.4) : faut-il embarquer les dépendances dans le binaire ou les charger au runtime ? Ce choix a des implications sur la taille des binaires, les mises à jour de sécurité, la distribution, et la compatibilité. La réponse dépend du contexte — outil CLI, service cloud, bibliothèque système, application embarquée — et nous examinerons chaque cas.

**Distribution sur Linux** (section 27.5) : une fois votre bibliothèque construite, comment la rendre disponible aux autres ? Installation dans `/usr/local`, paquets `.deb`, intégration dans Conan/vcpkg — chaque approche a ses contraintes.

**CMake Presets** (section 27.6) : les presets standardisent les configurations de build et s'intègrent naturellement avec les gestionnaires de paquets. Un preset peut spécifier le toolchain Conan ou vcpkg, le type de build, le générateur, et le compilateur — tout ce qu'un contributeur a besoin de savoir pour reproduire exactement votre build.

---

## Fil conducteur du chapitre

Nous repartons du projet exemple structuré au chapitre 26. Ses dépendances actuelles sont gérées via `find_package()` et `FetchContent` :

```cmake
# État actuel (chapitre 26)
find_package(OpenSSL REQUIRED)  
find_package(Threads REQUIRED)  

FetchContent_Declare(spdlog ...)  
FetchContent_Declare(googletest ...)  
FetchContent_MakeAvailable(spdlog googletest)  
```

Au fil du chapitre, nous migrerons progressivement vers une gestion par Conan, puis montrerons l'équivalent vcpkg, pour aboutir à un projet dont les dépendances sont entièrement déclaratives, reproductibles, et indépendantes de l'état du système hôte.

---

## Plan du chapitre

| Section | Thème | Ce que vous apprendrez |
|---------|-------|----------------------|
| **27.1** | Le problème des librairies en C++ | Pourquoi la gestion des dépendances est plus complexe en C++ qu'ailleurs |
| **27.2** | Conan 2.0 🔥 | Installation, `conanfile.py`, profils, intégration CMake complète |
| **27.3** | vcpkg | Installation, `vcpkg.json`, mode manifest, intégration CMake |
| **27.4** | Linkage statique vs dynamique | `.a` vs `.so`, implications pratiques, choix selon le contexte |
| **27.5** | Distribution sur Linux | Installation système, `pkg-config`, bonnes pratiques |
| **27.6** | CMake Presets ⭐ | Standardisation des configurations, intégration CI/CD et gestionnaires de paquets |

---

## Prérequis techniques

Les outils suivants seront utilisés dans ce chapitre. Leur installation sera détaillée dans chaque section, mais voici un aperçu :

```bash
# Python 3.10+ (nécessaire pour Conan)
python3 --version

# pip (pour installer Conan)
pip3 --version

# CMake 3.25+ et Ninja (couverts au chapitre 26)
cmake --version  
ninja --version  

# Git (nécessaire pour vcpkg)
git --version
```

Conan s'installe via pip, vcpkg se clone depuis GitHub. Les deux s'intègrent avec CMake via des fichiers toolchain — le mécanisme couvert en section 26.6.

---

> **À suivre** : La section 27.1 pose le diagnostic — pourquoi la gestion des dépendances en C++ est fondamentalement plus complexe que dans les langages modernes, et quelles sont les causes techniques de cette difficulté.

⏭️ [Le problème des librairies en C++](/27-gestion-dependances/01-probleme-librairies.md)
