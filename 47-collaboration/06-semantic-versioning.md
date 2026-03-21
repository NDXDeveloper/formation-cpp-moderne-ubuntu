🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 47.6 — Semantic Versioning et changelogs

> **Chapitre 47 : Collaboration et Maintenance** · Module 17 : Architecture de Projet Professionnel  
> **Niveau** : Expert  
> **Prérequis** : Section 47.1 (Git et workflows — Conventional Commits), section 47.5 (Gestion de la dette technique), chapitre 27 (Gestion des dépendances)

---

## Introduction

Les sections précédentes de ce chapitre ont couvert comment organiser les branches (47.1), automatiser les vérifications (47.2-47.3), structurer les reviews (47.4) et gérer la dette technique (47.5). Il reste une question fondamentale : comment communiquer aux utilisateurs et aux consommateurs de votre code ce qui a changé, ce qui est compatible, et ce qui ne l'est plus ?

C'est le rôle du **versioning** et du **changelog**. Le versioning attribue un identifiant structuré à chaque release. Le changelog décrit en langage humain ce qui a changé entre deux versions. Ensemble, ils permettent aux utilisateurs de prendre des décisions éclairées : "puis-je mettre à jour sans rien casser ?", "quelles corrections de sécurité sont incluses ?", "quelles nouvelles fonctionnalités sont disponibles ?"

En C++, ces questions sont particulièrement aiguës. Une mise à jour de bibliothèque qui casse l'ABI impose une recompilation de tous les consommateurs. Un changement d'API qui supprime une surcharge de fonction casse le code source des utilisateurs. La différence entre un changement compatible et un changement destructif a des conséquences très concrètes — et le numéro de version doit communiquer cette distinction sans ambiguïté.

---

## Semantic Versioning (SemVer)

### Le principe

Le [Semantic Versioning](https://semver.org/) (SemVer) est un schéma de versioning qui encode la nature du changement dans le numéro de version lui-même. Le format est :

```
MAJOR.MINOR.PATCH
```

Chaque composant a une signification précise :

**MAJOR** — Incrémenté quand le changement est **incompatible** avec les versions précédentes. Les utilisateurs doivent modifier leur code pour s'adapter. En C++, cela inclut les changements d'API source *et* les ruptures d'ABI.

**MINOR** — Incrémenté quand de nouvelles fonctionnalités sont ajoutées de manière **rétro-compatible**. Les utilisateurs peuvent mettre à jour sans modifier leur code. L'API existante reste inchangée ; seules des additions sont faites.

**PATCH** — Incrémenté pour les corrections de bugs et les améliorations internes **sans changement d'API**. Le comportement visible ne change que pour corriger un défaut.

Exemples :

```
1.0.0 → 1.0.1   Correction d'un bug (PATCH)
1.0.1 → 1.1.0   Ajout d'une nouvelle classe/fonction (MINOR)
1.1.0 → 2.0.0   Suppression d'une fonction publique (MAJOR)
```

### Pré-release et metadata

SemVer autorise des suffixes pour les pré-releases et les métadonnées de build :

```
2.0.0-alpha.1        Pré-release alpha
2.0.0-beta.3         Pré-release beta
2.0.0-rc.1           Release candidate
2.0.0+build.4521     Métadonnée de build (ignorée dans le tri)
2.0.0-beta.3+linux   Combinaison pré-release + metadata
```

Les pré-releases sont triées lexicographiquement : `alpha.1 < alpha.2 < beta.1 < rc.1 < release`. Les métadonnées de build ne participent pas au tri — `2.0.0+build.1` et `2.0.0+build.99` sont considérées comme la même version.

### La version `0.x.y`

Les versions `0.x.y` signalent un projet en développement initial — l'API n'est pas stabilisée. En SemVer, toute modification est permise dans les `0.x.y` sans respecter les règles MAJOR/MINOR/PATCH. Par convention, beaucoup de projets traitent `0.MINOR` comme un équivalent de MAJOR (une rupture en `0.2.0` par rapport à `0.1.0` est attendue).

Passez à `1.0.0` quand l'API est suffisamment stable pour que des utilisateurs tiers puissent en dépendre avec confiance.

---

## Ce qui constitue un breaking change en C++

La notion de "changement incompatible" est plus complexe en C++ que dans la plupart des langages, car il faut distinguer trois niveaux de compatibilité.

### Compatibilité source (API)

Le code existant des utilisateurs **compile-t-il** avec la nouvelle version ? Les changements qui cassent la compatibilité source :

- Supprimer ou renommer une fonction, classe, méthode, enum ou typedef public.  
- Changer la signature d'une fonction publique (types de paramètres, type de retour).  
- Ajouter un paramètre sans valeur par défaut à une fonction existante.  
- Changer un `enum` en `enum class` (ou inversement).  
- Déplacer un symbole dans un autre namespace.  
- Rendre `explicit` un constructeur qui ne l'était pas (casse les conversions implicites).  
- Supprimer une surcharge qui était utilisée par des appels implicites.

### Compatibilité binaire (ABI)

Le code existant des utilisateurs **link-t-il et s'exécute-t-il** sans recompilation avec la nouvelle version de la bibliothèque partagée ? Les changements qui cassent l'ABI :

- Tous les changements qui cassent la compatibilité source (ci-dessus).  
- Ajouter, supprimer ou réordonner des membres de données dans une classe (change le layout mémoire).  
- Ajouter ou supprimer une fonction virtuelle (change le layout de la vtable).  
- Changer le type d'un membre de données.  
- Modifier une hiérarchie d'héritage (ajout ou suppression de classe de base).  
- Changer l'ordre des classes de base dans un héritage multiple.  
- Modifier la valeur d'une `constexpr` utilisée dans du code inline.

> **Point critique** : un changement peut préserver la compatibilité source tout en cassant l'ABI. Par exemple, ajouter un membre de données privé à une classe ne change pas l'API (les utilisateurs ne peuvent pas accéder aux membres privés) mais change le `sizeof` de la classe, ce qui casse l'ABI. C'est un piège spécifique au C++ que SemVer ne capture pas directement.

### Compatibilité comportementale

Le code existant des utilisateurs **se comporte-t-il de la même manière** ? Un changement de comportement observable (même sans changement d'API ni d'ABI) peut casser des programmes qui dépendent du comportement précédent. Exemples :

- Changer l'ordre de parcours d'un conteneur.  
- Modifier le format d'une sortie sérialisée.  
- Corriger un bug dont le comportement buggé était exploité par des utilisateurs.

Ces changements sont les plus difficiles à classifier. En SemVer strict, un changement de comportement qui corrige un bug documenté est un PATCH. Un changement de comportement intentionnel qui modifie la sémantique documentée est un MINOR (si rétro-compatible) ou un MAJOR (si breaking).

### Politique de versioning recommandée

Pour un projet C++, la politique la plus sûre est :

| Type de changement | Incrément |
|---|---|
| Correction de bug sans changement d'API ni d'ABI | PATCH |
| Ajout de fonctionnalité, API source et ABI préservées | MINOR |
| Rupture d'API source OU d'ABI | MAJOR |
| Ajout d'un membre privé dans une classe exportée (ABI break, pas API break) | MAJOR |
| Dépréciation d'une fonction (avec `[[deprecated]]`) | MINOR |
| Suppression d'une fonction précédemment dépréciée | MAJOR |

La règle est simple : **en cas de doute, incrémentez MAJOR**. Un MAJOR superflu est un inconvénient mineur pour les utilisateurs (ils vérifient les notes de release avant de mettre à jour). Un MINOR qui casse leur build est une catastrophe pour la confiance.

---

## Versioning dans CMake

### Déclaration de la version

La version du projet est déclarée dans le `CMakeLists.txt` principal via la commande `project()` :

```cmake
cmake_minimum_required(VERSION 3.25)  
project(MyProject  
    VERSION 2.4.1
    LANGUAGES CXX
    DESCRIPTION "High-performance networking library"
)
```

CMake décompose automatiquement `VERSION 2.4.1` en variables exploitables :

```cmake
message(STATUS "Version: ${PROJECT_VERSION}")          # 2.4.1  
message(STATUS "Major:   ${PROJECT_VERSION_MAJOR}")    # 2  
message(STATUS "Minor:   ${PROJECT_VERSION_MINOR}")    # 4  
message(STATUS "Patch:   ${PROJECT_VERSION_PATCH}")    # 1  
```

### Propagation dans le code source

Pour rendre la version accessible dans le code C++ au runtime (logs, `--version`, health checks), générez un header à partir d'un template :

```cmake
# cmake/version.h.in
#pragma once

namespace myproject {
    inline constexpr int VERSION_MAJOR = @PROJECT_VERSION_MAJOR@;
    inline constexpr int VERSION_MINOR = @PROJECT_VERSION_MINOR@;
    inline constexpr int VERSION_PATCH = @PROJECT_VERSION_PATCH@;
    inline constexpr const char* VERSION_STRING = "@PROJECT_VERSION@";
}
```

```cmake
# CMakeLists.txt
configure_file(
    cmake/version.h.in
    ${CMAKE_BINARY_DIR}/generated/myproject/version.h
    @ONLY
)

target_include_directories(myproject PUBLIC
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/generated>
)
```

Utilisation dans le code :

```cpp
#include <myproject/version.h>
#include <print>

int main(int argc, char* argv[]) {
    if (argc > 1 && std::string_view(argv[1]) == "--version") {
        std::println("myproject {}", myproject::VERSION_STRING);
        return 0;
    }
    // ...
}
```

### Versioning de bibliothèque partagée (SOVERSION)

Pour les bibliothèques partagées (`.so`), Linux utilise un schéma de versioning spécifique avec le `SOVERSION` (shared object version). Le SOVERSION encode la compatibilité ABI :

```cmake
add_library(mylib SHARED src/mylib.cpp)

set_target_properties(mylib PROPERTIES
    VERSION   ${PROJECT_VERSION}      # Version complète : libmylib.so.2.4.1
    SOVERSION ${PROJECT_VERSION_MAJOR} # ABI version : libmylib.so.2
)
```

Cela crée les fichiers et symlinks suivants :

```
libmylib.so       → libmylib.so.2       (symlink pour le linker)  
libmylib.so.2     → libmylib.so.2.4.1   (symlink ABI — suit le SOVERSION)  
libmylib.so.2.4.1                        (fichier réel)  
```

Quand vous incrémentez MAJOR (changement d'ABI), le SOVERSION passe de 2 à 3. Les programmes compilés contre `libmylib.so.2` continuent de fonctionner (le symlink `.so.2` pointe toujours vers l'ancienne version s'il elle est installée). Les programmes recompilés utilisent `libmylib.so.3`.

Quand vous incrémentez MINOR ou PATCH (ABI préservée), seule la version complète change : `libmylib.so.2.4.1` → `libmylib.so.2.5.0`. Le symlink `libmylib.so.2` est mis à jour pour pointer vers la nouvelle version — les programmes existants bénéficient des corrections sans recompilation.

---

## Tags Git et releases

### Convention de tagging

Chaque release correspond à un tag Git. La convention dominante préfixe le numéro de version avec `v` :

```bash
git tag -a v2.4.1 -m "Release 2.4.1: Fix connection timeout on ARM"  
git push origin v2.4.1  
```

Le tag annoté (`-a`) est préférable au tag léger car il contient des métadonnées (auteur, date, message) et est signable cryptographiquement.

### Automatisation avec Conventional Commits

Si l'équipe utilise les Conventional Commits (section 47.1) et le hook `commitizen` (section 47.2.2), le cycle de release peut être partiellement automatisé. `commitizen` déduit l'incrément de version à partir des types de commits depuis la dernière release :

```
feat: ...        → MINOR  
fix: ...         → PATCH  
feat!: ...       → MAJOR  
BREAKING CHANGE  → MAJOR  
```

La commande `cz bump` automatise le processus :

```bash
# commitizen analyse les commits depuis le dernier tag
# et déduit l'incrément approprié
cz bump
```

Ce que `cz bump` fait automatiquement :

1. Analyse les commits depuis le dernier tag.
2. Détermine l'incrément (MAJOR, MINOR ou PATCH).
3. Met à jour les fichiers de version (configurés dans `.cz.toml`).
4. Crée le commit de version.
5. Crée le tag Git.

Configuration dans `.cz.toml` :

```toml
[tool.commitizen]
name = "cz_conventional_commits"  
version = "2.4.1"  
tag_format = "v$version"  
version_files = [  
    "CMakeLists.txt:project\\(.*VERSION",
    "conanfile.py:version",
    "README.md:^Version:",
]
update_changelog_on_bump = true  
changelog_file = "CHANGELOG.md"  
```

Le champ `version_files` liste les fichiers et patterns où la version doit être mise à jour. `commitizen` utilise des expressions régulières pour localiser et remplacer le numéro de version dans chaque fichier.

### Workflow de release complet

```bash
# 1. S'assurer que main est propre et à jour
git checkout main  
git pull origin main  

# 2. Vérifier que tous les tests passent
cmake --build build && ctest --test-dir build

# 3. Bump automatique (analyse les commits, met à jour les fichiers)
cz bump

# 4. Pousser le commit de version et le tag
git push origin main --follow-tags

# 5. La CI détecte le tag et déclenche le pipeline de release
#    (build, tests complets, packaging, publication)
```

---

## Changelogs

### Pourquoi un changelog est indispensable

Un tag Git et un numéro de version indiquent qu'une nouvelle version existe. Ils ne disent pas *ce qui a changé*. Le changelog comble ce manque : c'est un document destiné aux **humains** (développeurs, opérateurs, utilisateurs finaux) qui décrit les changements entre chaque version dans un format lisible.

Un bon changelog répond à trois questions pour chaque version :

1. Quelles fonctionnalités ont été ajoutées ? (*Je peux faire quelque chose de nouveau.*)
2. Quels bugs ont été corrigés ? (*Un problème que j'avais est résolu.*)
3. Quels changements sont incompatibles ? (*Je dois modifier mon code pour mettre à jour.*)

### Le format Keep a Changelog

Le format [Keep a Changelog](https://keepachangelog.com/) est le standard de facto. Il est lisible par les humains, parsable par les machines, et compatible avec Markdown :

```markdown
# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),  
and this project adheres to [Semantic Versioning](https://semver.org/).  

## [Unreleased]

### Added
- Support for HTTP/3 in ConnectionPool (PROJ-1234)

### Changed
- Default timeout increased from 5s to 30s

## [2.4.1] - 2026-03-15

### Fixed
- Connection timeout not respected on ARM64 (PROJ-987)
- Memory leak in TLS handshake when server rejects client certificate

## [2.4.0] - 2026-02-28

### Added
- `ConnectionPool::stats()` method for monitoring active connections
- Support for Unix domain sockets in `Endpoint`
- std::expected-based error handling in new API (alongside existing exceptions)

### Changed
- Minimum CMake version bumped to 3.25 (was 3.20)
- Protobuf dependency updated from 4.25 to 5.28

### Deprecated
- `Connection::set_timeout(int ms)` — use `set_timeout(std::chrono::milliseconds)` instead
  Will be removed in 3.0.0.

## [2.3.0] - 2026-01-10

### Added
- gRPC bidirectional streaming support

### Fixed
- Race condition in connection pool under high contention (PROJ-654)

## [2.0.0] - 2025-10-01

### Changed
- **BREAKING**: Renamed `Endpoint` to `ServiceEndpoint`
- **BREAKING**: Removed deprecated `Connection::raw_fd()` method
- **BREAKING**: ABI break — added `priority_` member to `Request` class

### Migration guide
See [MIGRATION-2.0.md](docs/MIGRATION-2.0.md) for detailed upgrade instructions.

[Unreleased]: https://github.com/org/project/compare/v2.4.1...HEAD
[2.4.1]: https://github.com/org/project/compare/v2.4.0...v2.4.1
[2.4.0]: https://github.com/org/project/compare/v2.3.0...v2.4.0
[2.3.0]: https://github.com/org/project/compare/v2.0.0...v2.3.0
[2.0.0]: https://github.com/org/project/releases/tag/v2.0.0
```

### Catégories standard

| Catégorie | Contenu |
|---|---|
| **Added** | Nouvelles fonctionnalités |
| **Changed** | Modifications de fonctionnalités existantes |
| **Deprecated** | Fonctionnalités qui seront supprimées dans une version future |
| **Removed** | Fonctionnalités supprimées (breaking si publiques) |
| **Fixed** | Corrections de bugs |
| **Security** | Corrections de vulnérabilités |

### Bonnes pratiques pour le changelog C++

**Mentionner les breaking changes en gras.** Le mot `**BREAKING**` en début de ligne attire immédiatement l'attention. Pour les versions MAJOR, ajoutez une section "Migration guide" ou un lien vers un document de migration dédié.

**Distinguer les breaks d'API et d'ABI.** Un utilisateur qui link dynamiquement une bibliothèque a besoin de savoir si l'ABI a changé (il doit recompiler) ou seulement l'API source (il doit modifier son code). Mentionnez explicitement `ABI break` quand c'est le cas :

```markdown
### Changed
- **BREAKING (ABI)**: Added `priority_` member to `Request` — recompilation required
- **BREAKING (API)**: `parse()` now returns `std::expected<Config, ParseError>` instead of throwing
```

**Référencer les tickets.** Chaque entrée devrait référencer le ticket correspondant (PROJ-123, #456). Cela permet de remonter au contexte (discussion, design, tests) sans surcharger le changelog.

**Documenter les dépréciations avec un horizon.** Quand une fonction est dépréciée, indiquez dans quelle version elle sera supprimée. Cela donne aux utilisateurs un délai pour migrer :

```markdown
### Deprecated
- `Connection::set_timeout(int ms)` — use `set_timeout(std::chrono::milliseconds)`.
  Will be removed in 3.0.0.
```

Dans le code, la dépréciation est signalée par l'attribut standard :

```cpp
[[deprecated("Use set_timeout(std::chrono::milliseconds) instead. Removed in 3.0.0")]]
void set_timeout(int ms);
```

**La section `[Unreleased]`.** Elle accumule les changements depuis la dernière release. Au moment de la release, son contenu est déplacé sous le nouveau numéro de version et la section `[Unreleased]` est vidée. Cela permet à chaque développeur d'ajouter ses changements au changelog *au moment de la MR* plutôt que de reconstituer l'historique a posteriori.

### Génération automatique vs manuelle

**Génération automatique** — `commitizen` (ou `conventional-changelog`, `git-cliff`) peut générer le changelog à partir des Conventional Commits :

```bash
cz changelog
```

Le résultat est fonctionnel mais souvent *trop technique*. Les messages de commit sont écrits pour les développeurs du projet, pas pour les utilisateurs. Un message comme `fix(net): handle EAGAIN in epoll loop` est incompréhensible pour un utilisateur qui veut savoir si son bug de timeout est corrigé.

**Rédaction manuelle** — Un changelog rédigé par un humain est plus lisible et plus utile. L'auteur reformule en termes de valeur pour l'utilisateur : "Fixed: Connection timeout not respected on ARM64" plutôt que le message de commit technique.

**Approche hybride (recommandée)** — Utilisez la génération automatique comme base (elle garantit l'exhaustivité), puis éditez manuellement pour reformuler, regrouper et clarifier. La commande `cz bump --changelog` génère le brouillon ; le release manager le polit avant de publier.

### Processus de mise à jour du changelog

Le changelog ne doit pas être rédigé à la dernière minute avant la release. Chaque MR qui ajoute une fonctionnalité, corrige un bug ou introduit un breaking change devrait inclure une mise à jour de la section `[Unreleased]` du changelog. Ce point peut être inclus dans le template de MR (section 47.4) :

```markdown
## Checklist

- [ ] Les tests couvrent les nouveaux cas
- [ ] La documentation est à jour
- [ ] Le CHANGELOG.md est mis à jour (section [Unreleased])
```

Au moment de la release, la section `[Unreleased]` est transformée en section versionnée. Aucune rédaction de dernière minute, aucun historique à reconstituer.

---

## Politique de dépréciation

La dépréciation est le mécanisme qui permet de supprimer une API sans surprise pour les utilisateurs. En C++, une politique de dépréciation claire est d'autant plus importante que les ruptures sont coûteuses (recompilation, changement de code).

### Cycle de vie d'une dépréciation

```
Version N     : Nouvelle API introduite en parallèle de l'ancienne  
Version N     : Ancienne API marquée [[deprecated]] dans le header  
Version N     : Changelog mentionne la dépréciation avec horizon  
Version N+1   : Documentation mise à jour, exemples migrés  
...
Version N+K   : Ancienne API supprimée (MAJOR version bump)
```

### Convention de durée

La durée entre la dépréciation et la suppression dépend du type de projet :

| Contexte | Durée minimum de dépréciation |
|---|---|
| Bibliothèque interne (même équipe) | 1 version MINOR |
| Bibliothèque d'entreprise (plusieurs équipes) | 2-3 versions MINOR ou 6 mois |
| Bibliothèque open source | 1 version MAJOR complète ou 12 mois |

### Outillage pour la dépréciation

L'attribut `[[deprecated]]` (C++14) déclenche un warning à la compilation chez les utilisateurs, les informant qu'ils doivent migrer :

```cpp
namespace mylib {

// Nouvelle API (C++23 style)
std::expected<Config, ParseError> parse_config(std::string_view path);

// Ancienne API — dépréciée
[[deprecated("Use parse_config() returning std::expected. Removed in v3.0.0")]]
Config parse_config_legacy(const std::string& path);

} // namespace mylib
```

Pour les classes entières :

```cpp
struct [[deprecated("Use ServiceEndpoint instead. Removed in v3.0.0")]] Endpoint {
    // ...
};
```

En CI, vous pouvez compiler avec `-Werror=deprecated-declarations` pour vous assurer que votre propre code n'utilise plus les API dépréciées, tout en laissant les utilisateurs externes compiler avec un simple warning.

---

## Guide de migration pour les versions MAJOR

Quand une version MAJOR est publiée, un guide de migration dédié aide les utilisateurs à mettre à jour. Ce document est distinct du changelog (qui liste les changements) — il explique **comment** migrer concrètement.

Structure recommandée :

```markdown
# Migration Guide: v2.x → v3.0

## Overview
Version 3.0 introduces breaking changes to the networking API  
and requires recompilation of all dependent code (ABI break).  

## Step-by-step migration

### 1. Rename `Endpoint` → `ServiceEndpoint`

Search and replace in your codebase:
```cpp
// Before
mylib::Endpoint ep("localhost", 8080);

// After
mylib::ServiceEndpoint ep("localhost", 8080);
```

### 2. Adopt std::expected return values

Functions that previously threw now return std::expected:
```cpp
// Before (v2.x)
try {
    auto config = mylib::parse_config("app.toml");
} catch (const mylib::ParseError& e) {
    handle_error(e);
}

// After (v3.0)
auto result = mylib::parse_config("app.toml");  
if (!result) {  
    handle_error(result.error());
    return;
}
auto config = std::move(*result);
```

### 3. Recompile all dependent targets
ABI has changed — a simple relink is NOT sufficient.

```bash
cmake --build build --clean-first
```
```

Publiez le guide de migration en même temps que la release, et liez-le depuis le changelog.

---

## Récapitulatif du cycle de release

Le cycle complet, de la dernière release à la prochaine, intègre tous les mécanismes présentés dans ce chapitre :

```
  Développement continu
  ─────────────────────────────────────────────────────
  │  Conventional Commits (section 47.1)
  │  Pre-commit hooks (sections 47.2-47.3)
  │  Code reviews (section 47.4)
  │  Mise à jour CHANGELOG.md [Unreleased] par MR
  │
  ▼
  Décision de release
  ─────────────────────────────────────────────────────
  │  cz bump (déduit MAJOR/MINOR/PATCH)
  │  Met à jour : CMakeLists.txt, conanfile.py, CHANGELOG.md
  │  Crée le commit de version + tag Git
  │
  ▼
  CI pipeline de release (chapitre 38)
  ─────────────────────────────────────────────────────
  │  Build complet (Debug + Release)
  │  Tests + sanitizers
  │  Packaging (DEB, RPM, Docker, Conan)
  │  Publication des artefacts
  │
  ▼
  Communication
  ─────────────────────────────────────────────────────
     Release notes (basées sur CHANGELOG.md)
     Guide de migration (si MAJOR)
     Annonce (blog, Slack, mailing list)
```

---

## Résumé

Le Semantic Versioning en C++ impose une rigueur supplémentaire par rapport aux langages sans notion d'ABI : la distinction entre compatibilité source et compatibilité binaire doit être explicite dans le numéro de version et dans le changelog. Le triptyque Conventional Commits → `cz bump` → changelog automatique crée un flux où la version et les notes de release se déduisent naturellement de l'historique Git, avec un polissage humain pour la lisibilité. Le changelog maintenu incrémentalement (section `[Unreleased]` mise à jour dans chaque MR) élimine le travail de reconstitution de dernière minute. Et la politique de dépréciation avec `[[deprecated]]`, horizon documenté et guide de migration donne aux utilisateurs la visibilité nécessaire pour planifier leurs mises à jour en confiance.

---


⏭️ [Ressources et Veille Technologique](/48-ressources/README.md)
