🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 28.4 Meson : Build system montant dans l'écosystème Linux

> **Objectif** : Situer Meson dans l'écosystème des build systems C++, comprendre sa philosophie et ses forces, et savoir quand le considérer comme alternative à CMake — sans prétendre en faire une couverture exhaustive, le focus de cette formation restant CMake.

---

## Pourquoi parler de Meson ?

Cette formation est centrée sur CMake, et à raison — c'est le standard de l'industrie C++ en 2026, avec l'écosystème le plus large et le meilleur support IDE. Cependant, ignorer Meson serait une lacune pour un développeur C++ professionnel travaillant sous Linux, pour trois raisons.

Premièrement, **de nombreux projets Linux majeurs utilisent Meson**. Si vous contribuez à l'écosystème GNOME, si vous travaillez sur des pilotes graphiques, ou si vous intégrez des composants système, vous rencontrerez Meson. Savoir le lire et l'utiliser est une compétence pratique.

Deuxièmement, **Meson représente une vision différente** de ce que devrait être un build system. Là où CMake a évolué organiquement pendant 25 ans en accumulant des fonctionnalités et de la compatibilité, Meson est parti d'une feuille blanche avec des convictions fortes sur la simplicité et la correction. Comprendre cette vision enrichit votre compréhension globale du tooling C++.

Troisièmement, **Meson progresse régulièrement** en adoption et en fonctionnalités. Même si vous choisissez CMake pour vos projets, connaître Meson vous permet d'évaluer objectivement les alternatives et de faire des choix informés.

---

## Genèse et philosophie

Meson a été créé en 2013 par Jussi Pakkanen, un développeur finlandais frustré par la complexité d'autotools et par les incohérences de CMake. Son objectif affiché : créer un build system qui soit **simple à utiliser correctement** et **difficile à utiliser incorrectement**.

### Les convictions fondatrices

**Un langage non Turing-complet.** Le langage de Meson est intentionnellement limité : pas de boucles `while` arbitraires, pas de récursion, pas de manipulation de chaînes au niveau caractère. Vous pouvez itérer sur des listes avec `foreach`, mais vous ne pouvez pas écrire de logique de build arbitrairement complexe. Cette limitation est un choix de design, pas un manque : elle force les build files à rester déclaratifs et lisibles, et elle permet à Meson de parser et valider les fichiers très rapidement.

**Ninja comme seul backend.** Meson génère exclusivement des fichiers `build.ninja` (et des projets Visual Studio/Xcode sur les plateformes correspondantes). Il n'a pas de backend Make. Ce choix élimine une couche de complexité et garantit que tous les builds Meson bénéficient de la vitesse de Ninja.

**Correction par défaut.** Meson applique des choix sûrs par défaut : out-of-source builds obligatoires, pas de variables globales modifiables depuis les sous-projets, isolation stricte entre les cibles. Les erreurs de configuration qui sont des pièges classiques en CMake (oublier `CMAKE_CXX_STANDARD_REQUIRED`, utiliser `include_directories()` au lieu de `target_include_directories()`) n'ont pas d'équivalent en Meson — la seule façon de faire est la bonne.

**Vitesse de configuration.** Meson est écrit en Python mais son parsing est optimisé pour être rapide. La configuration d'un projet de taille moyenne prend typiquement moins d'une seconde. Combiné avec Ninja pour le build, l'ensemble du pipeline est très réactif.

---

## Meson vs CMake : positionnement

Les deux outils résolvent le même problème (décrire un build C++ de manière portable) mais font des compromis différents :

| Aspect | CMake | Meson |
|--------|-------|-------|
| Âge | 2000 (25+ ans) | 2013 (12+ ans) |
| Langage | Propre (Turing-complet) | Propre (non Turing-complet) |
| Backend | Make, Ninja, VS, Xcode, etc. | Ninja (+ VS/Xcode) |
| Écosystème de bibliothèques | Dominant (Conan, vcpkg, find_package) | Plus limité (wraps, pkg-config) |
| Support IDE | CLion, VS Code, VS (natif) | VS Code (extension), GNOME Builder |
| Adoption globale C++ | Standard de facto | Niche croissante |
| Adoption Linux system | Significative | Très forte (GNOME, systemd, Mesa) |
| Courbe d'apprentissage | Raide (legacy + modern) | Douce (une seule façon de faire) |
| Flexibilité | Très élevée | Modérée (intentionnellement) |
| Gestion des dépendances | find_package, FetchContent | Wraps, pkg-config, CMake subprojects |
| Cross-compilation | Fichiers toolchain | Cross files |
| Modules C++20 | Support stable (CMake 4.0+) | Support en cours |

L'écart le plus significatif est l'**écosystème** : la quasi-totalité des bibliothèques C++ publient des fichiers `*Config.cmake` pour la consommation via `find_package()`. Meson peut consommer ces fichiers via son module CMake, mais c'est un pont — pas une intégration native. Pour les projets qui dépendent de nombreuses bibliothèques tierces, cette différence d'écosystème pèse lourd en faveur de CMake.

---

## Un aperçu de la syntaxe

Pour donner une idée concrète de ce qu'est Meson, voici l'équivalent Meson du projet CMake que nous construisons depuis le chapitre 26 :

```meson
# meson.build (racine du projet)
project('my_project', 'cpp',
  version : '1.2.0',
  default_options : [
    'cpp_std=c++23',
    'warning_level=3',
    'werror=false',
    'buildtype=release',
  ]
)

# Dépendances externes
openssl_dep = dependency('openssl', required : true)  
threads_dep = dependency('threads')  
zlib_dep = dependency('zlib', required : false)  

# Sous-répertoires
subdir('libs/utils')  
subdir('src')  
subdir('apps')  

if get_option('tests')
  subdir('tests')
endif
```

```meson
# src/meson.build
core_lib = library('my_project_core',
  'core.cpp',
  'network.cpp',
  include_directories : include_directories('../include'),
  dependencies : [openssl_dep, threads_dep, zlib_dep, utils_dep],
  install : true,
)

core_dep = declare_dependency(
  link_with : core_lib,
  include_directories : include_directories('../include'),
)
```

```meson
# apps/meson.build
executable('my-app',
  'main.cpp',
  dependencies : core_dep,
  install : true,
)
```

Quelques observations immédiates par rapport à CMake :

**Lisibilité.** La syntaxe Meson ressemble à un langage de script moderne (proche de Python). Les fonctions ont des arguments nommés (`required : true`), pas de macros en majuscules, pas de `${}`.

**Dépendances via `dependency()`.** L'équivalent de `find_package()`. Meson utilise `pkg-config` comme mécanisme de recherche principal, complété par des détecteurs intégrés pour les bibliothèques courantes.

**Pas de PUBLIC/PRIVATE/INTERFACE.** Meson gère la visibilité automatiquement. Quand vous déclarez `declare_dependency(link_with : core_lib, include_directories : ...)`, Meson propage les bons chemins aux consommateurs. Le modèle est plus simple mais moins explicite que celui de CMake.

**Pas de générateur à choisir.** Il n'y a pas de `-G Ninja` — Meson génère toujours pour Ninja.

---

## Workflow Meson

Le workflow Meson est similaire à celui de CMake, en trois étapes :

```bash
# 1. Configuration (équivalent de cmake -B build)
meson setup build

# 2. Compilation (équivalent de cmake --build build)
meson compile -C build
# Ou directement : ninja -C build

# 3. Tests (équivalent de ctest)
meson test -C build

# 4. Installation (équivalent de cmake --install)
meson install -C build
```

Les options de build se configurent à l'initialisation ou via `meson configure` :

```bash
# Configuration avec options
meson setup build -Dbuildtype=debug -Dtests=true

# Modifier une option après configuration
meson configure build -Dbuildtype=release

# Lister les options disponibles
meson configure build
```

Meson impose les **out-of-source builds** — il n'est pas possible de configurer dans le répertoire source. C'est une bonne pratique que CMake recommande mais n'impose pas.

---

## Gestion des dépendances dans Meson

Meson propose plusieurs mécanismes pour intégrer des dépendances, avec une philosophie différente de CMake.

### `dependency()` — l'équivalent de `find_package()`

```meson
openssl_dep = dependency('openssl', version : '>=3.0')  
zlib_dep = dependency('zlib', required : false)  
```

Meson cherche les bibliothèques via **pkg-config** en priorité, puis via des détecteurs intégrés pour les bibliothèques courantes (Boost, Qt, Python, etc.). Pour les bibliothèques qui ne fournissent que des fichiers CMake Config (pas de `.pc`), Meson peut utiliser son module CMake :

```meson
cmake = import('cmake')  
spdlog_dep = cmake.subproject('spdlog').dependency('spdlog')  
```

### Wraps — l'équivalent de `FetchContent`

Le système de **wraps** permet de télécharger et d'intégrer des dépendances depuis un registre communautaire (WrapDB) ou un dépôt Git :

```ini
# subprojects/spdlog.wrap
[wrap-file]
directory = spdlog-1.15.3  
source_url = https://github.com/gabime/spdlog/archive/v1.15.3.tar.gz  
source_hash = abc123...  
patch_directory = spdlog  

[provide]
spdlog = spdlog_dep
```

```meson
# meson.build — utilisation transparente
spdlog_dep = dependency('spdlog', fallback : ['spdlog', 'spdlog_dep'])
```

Le pattern `fallback` est similaire au pattern `find_package` + `FetchContent` de CMake (section 26.3.2) : Meson cherche d'abord la bibliothèque sur le système, et si elle n'est pas trouvée, la télécharge et la compile comme sous-projet.

WrapDB contient environ 200 bibliothèques — significativement moins que ConanCenter (~1500) ou vcpkg (~2750). C'est une limite réelle de l'écosystème Meson pour les projets avec de nombreuses dépendances tierces.

---

## Forces et limites

### Ce que Meson fait mieux que CMake

**Simplicité et lisibilité.** Les `meson.build` sont plus faciles à lire et à écrire que les `CMakeLists.txt`. Il n'y a qu'une seule façon de déclarer une bibliothèque, pas d'ancien style vs style moderne, pas de commandes globales vs commandes par cible.

**Correction par défaut.** Les builds out-of-source sont obligatoires, les warnings sont configurés via un niveau (`warning_level`), la cross-compilation est structurée via des cross files, et l'isolation entre sous-projets est garantie.

**Vitesse de configuration.** Meson configure plus vite que CMake, et comme il génère pour Ninja exclusivement, le build est toujours rapide.

**Intégration pkg-config native.** Meson consomme et génère des fichiers `.pc` nativement, ce qui en fait un choix naturel pour les projets qui s'intègrent à l'écosystème Linux traditionnel (GNOME, freedesktop).

### Ce qui limite Meson face à CMake

**Écosystème de bibliothèques.** La majorité des bibliothèques C++ publient des fichiers CMake Config, pas des wraps Meson. WrapDB est plus petit que ConanCenter et vcpkg.

**Support IDE.** CLion a un support CMake natif de première classe. Le support Meson existe mais est moins mature. VS Code fonctionne bien avec les deux via des extensions.

**Adoption dans l'industrie C++ large.** En dehors de l'écosystème Linux system (GNOME, Mesa, systemd), Meson est rarement utilisé dans l'industrie C++ traditionnelle (finance, jeux vidéo, embarqué). CMake domine ces secteurs.

**Flexibilité.** Le langage non Turing-complet de Meson est une force pour la lisibilité, mais peut devenir une limitation pour les configurations de build très complexes ou non conventionnelles. CMake, malgré sa syntaxe moins élégante, peut exprimer n'importe quelle logique.

**Modules C++20.** Le support des modules dans Meson est en cours de développement mais n'a pas encore atteint la maturité de CMake 4.0+.

---

## Quand considérer Meson

| Scénario | Recommandation |
|----------|---------------|
| Nouveau projet Linux-only, peu de dépendances tierces | Meson est un excellent choix |
| Contribution à un projet GNOME/freedesktop | Meson est obligatoire |
| Projet multiplateforme avec de nombreuses dépendances | CMake (écosystème plus large) |
| Interopérabilité avec Conan/vcpkg | CMake (intégration native) |
| Équipe habituée à CMake, projet existant en CMake | Rester sur CMake |
| Nouveau projet, équipe sans préférence, cible Linux | Évaluer Meson — la simplicité a de la valeur |

Le choix entre CMake et Meson n'est pas binaire. Certaines organisations utilisent CMake pour leurs projets principaux et contribuent à des projets Meson dans l'écosystème Linux. La compétence est complémentaire, pas exclusive.

---

## Plan des sous-sections

| Sous-section | Thème | Ce que vous apprendrez |
|-------------|-------|----------------------|
| **28.4.1** | Syntaxe déclarative et philosophie | Fonctions, types, options, sous-répertoires, cross-compilation |
| **28.4.2** | Projets notables utilisant Meson | GNOME, systemd, Mesa, PipeWire — contexte et retours d'expérience |

---

> **À suivre** : La sous-section 28.4.1 détaille la syntaxe de Meson — son système de types, ses fonctions principales, la gestion des options et de la cross-compilation.

⏭️ [Syntaxe déclarative et philosophie](/28-make-ninja/04.1-syntaxe-meson.md)
