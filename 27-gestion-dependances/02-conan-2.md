🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 27.2 Conan 2.0 : Nouvelle API et `conanfile.py` 🔥

> **Objectif** : Comprendre l'architecture et les concepts fondamentaux de Conan 2.0 — le modèle de paquets, le rôle du `conanfile`, les profils, le cache binaire, et l'intégration avec CMake — avant de plonger dans l'installation et l'utilisation détaillées dans les sous-sections.

---

## Pourquoi Conan ?

La section 27.1 a posé le diagnostic : la gestion des dépendances en C++ est complexe à cause de la matrice de configurations (OS, architecture, compilateur, standard, type de build, mode de linkage). Conan est conçu pour dompter cette matrice. C'est un gestionnaire de paquets qui comprend que deux compilations de la même bibliothèque avec des paramètres différents produisent des **artefacts binaires différents** — et il gère chaque combinaison comme un paquet distinct.

Conan est aussi le gestionnaire de paquets C++ le plus flexible en 2026. Là où vcpkg (section 27.3) fait des choix par défaut pour simplifier l'expérience, Conan expose chaque paramètre de la matrice et vous laisse le contrôle total. Cette flexibilité a un coût en termes de courbe d'apprentissage, mais elle rend Conan capable de s'adapter à des scénarios que d'autres outils ne couvrent pas : cross-compilation avec des toolchains exotiques, builds reproductibles avec des contraintes ABI précises, ou distribution de binaires à travers une organisation via un serveur de paquets privé.

---

## Conan 1.x vs Conan 2.0 : une refonte, pas une mise à jour

Conan 2.0, sorti en mars 2023, n'est pas une simple évolution de Conan 1.x. C'est une **réécriture conceptuelle** de l'API et du modèle de configuration. Si vous avez déjà utilisé Conan 1.x, oubliez la plupart de ce que vous savez — les noms de commandes, la structure des fichiers, et le modèle de génération ont changé en profondeur.

Les différences les plus significatives :

| Aspect | Conan 1.x (obsolète) | Conan 2.0 |
|--------|---------------------|-----------|
| Fichier de dépendances | `conanfile.txt` (format plat) | `conanfile.py` (Python, recommandé) ou `conanfile.txt` (simplifié) |
| Intégration CMake | Générateurs multiples (`cmake`, `cmake_find_package`, `cmake_multi`) | Un seul générateur : `CMakeToolchain` + `CMakeDeps` |
| Profils | Profil unique | Double profil : `--profile:host` + `--profile:build` |
| Commande d'installation | `conan install ..` | `conan install .` |
| Layouts | Implicites, fragiles | `cmake_layout()` — explicite et standardisé |
| Cache local | Structure opaque | Structure inspectable, cache binaire intégré |
| Recettes | Héritage de `ConanFile` avec méthodes legacy | Nouvelle API `ConanFile` avec méthodes clarifiées |

Ce chapitre couvre **exclusivement Conan 2.0**. Si vous rencontrez des tutoriels ou des recettes utilisant `cmake_find_package`, `conan install ..` (avec deux points), ou le profil unique `default` sans distinction host/build, il s'agit de Conan 1.x — ne les suivez pas.

---

## Architecture et concepts fondamentaux

### Le modèle de paquets

Un **paquet Conan** est un artefact identifié par trois éléments :

```
nom/version@user/channel
```

Par exemple : `openssl/3.2.1`, `spdlog/1.15.3`, `gtest/1.17.0`. Les champs `user` et `channel` sont optionnels et rarement utilisés avec les paquets de ConanCenter.

Chaque paquet peut exister en **plusieurs variantes binaires** — une par combinaison de settings et d'options. Conan calcule un **package ID** (un hash) à partir de ces paramètres. Quand vous demandez `openssl/3.2.1` avec GCC 15, C++23, Linux x86_64, Release, statique, Conan cherche un binaire dont le package ID correspond. S'il existe dans le cache (local ou distant), il est réutilisé. Sinon, Conan peut le compiler depuis les sources.

```
openssl/3.2.1
├── package_id: abc123...  (GCC 15, C++23, Release, static, x86_64)
│   ├── include/openssl/*.h
│   ├── lib/libssl.a
│   └── lib/libcrypto.a
├── package_id: def456...  (GCC 15, C++23, Debug, shared, x86_64)
│   ├── include/openssl/*.h
│   ├── lib/libssl.so
│   └── lib/libcrypto.so
└── package_id: ghi789...  (Clang 20, C++20, Release, static, aarch64)
    └── ...
```

### Les recettes

Une **recette** (`conanfile.py`) décrit comment un paquet est construit et consommé. Pour les paquets de ConanCenter, les recettes sont maintenues par la communauté. Pour vos propres bibliothèques, vous écrivez votre propre recette.

Quand vous **consommez** des dépendances (le cas le plus courant), votre `conanfile.py` est simple — il déclare les dépendances et la configuration souhaitée :

```python
from conan import ConanFile  
from conan.tools.cmake import cmake_layout, CMakeToolchain, CMakeDeps  

class MyProjectRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    def requirements(self):
        self.requires("openssl/3.2.1")
        self.requires("spdlog/1.15.3")
        self.requires("zlib/1.3.1")

    def build_requirements(self):
        self.test_requires("gtest/1.17.0")

    def layout(self):
        cmake_layout(self)
```

### Les profils

Un **profil** décrit l'environnement de compilation : le système d'exploitation, l'architecture, le compilateur et sa version, le standard C++, et le type de build. Conan 2.0 utilise systématiquement **deux profils** :

- Le **profil host** (`--profile:host` ou `-pr:h`) : décrit la plateforme **cible** — celle où le binaire s'exécutera.
- Le **profil build** (`--profile:build` ou `-pr:b`) : décrit la plateforme **de compilation** — celle où la compilation s'effectue.

Pour un build natif (compiler sur x86_64 pour exécuter sur x86_64), les deux profils sont identiques — typiquement le profil `default` auto-détecté par Conan. Pour la cross-compilation, ils diffèrent :

```bash
# Build natif — les deux profils sont identiques (implicitement default)
conan install . --build=missing

# Cross-compilation — host ≠ build
conan install . --profile:host=arm64 --profile:build=default --build=missing
```

Le profil par défaut est généré automatiquement par `conan profile detect` et ressemble à :

```ini
[settings]
os=Linux  
arch=x86_64  
compiler=gcc  
compiler.version=15  
compiler.libcxx=libstdc++11  
build_type=Release  
```

### Les générateurs CMake

Conan 2.0 utilise deux générateurs pour s'intégrer avec CMake :

**`CMakeToolchain`** génère un fichier `conan_toolchain.cmake` qui configure les variables CMake (compilateur, standard, flags, type de build) pour correspondre au profil Conan. Ce fichier est passé à CMake via `-DCMAKE_TOOLCHAIN_FILE`.

**`CMakeDeps`** génère les fichiers de configuration CMake (`*Config.cmake`, `*Targets.cmake`) pour chaque dépendance. Vos `find_package()` trouvent alors les bibliothèques installées par Conan de manière transparente.

Le résultat est que vos `CMakeLists.txt` restent **identiques** — les mêmes `find_package()` et `target_link_libraries()` fonctionnent aussi bien avec des dépendances installées par Conan, par `apt`, ou compilées manuellement. C'est le découplage fondamental que nous avons souligné en section 26.3.1.

### Le cache binaire

Conan maintient un **cache local** sur votre machine, dans `~/.conan2/`. Chaque paquet téléchargé ou compilé y est stocké par package ID. Les installations suivantes réutilisent le cache sans retélécharger ni recompiler.

Au-delà du cache local, Conan supporte des **caches binaires distants** — des serveurs qui stockent les artefacts pré-compilés. ConanCenter est le cache public par défaut. Les organisations peuvent déployer leurs propres serveurs (JFrog Artifactory, Conan Server) pour partager des binaires entre développeurs et pipelines CI/CD, évitant ainsi que chaque machine recompile les mêmes dépendances.

---

## Le workflow Conan + CMake

Le workflow quotidien avec Conan se résume à deux commandes, suivies de la configuration et du build CMake habituels :

```bash
# 1. Installer les dépendances (télécharge/compile si nécessaire)
conan install . --output-folder=build --build=missing

# 2. Configurer CMake avec le toolchain Conan
cmake -B build -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release

# 3. Compiler
cmake --build build

# 4. Tester
ctest --test-dir build --output-on-failure
```

L'option `--build=missing` dit à Conan : « si un binaire pré-compilé n'existe pas pour ma configuration, compile-le depuis les sources ». Sans cette option, Conan échoue si le binaire exact n'est pas disponible dans le cache.

L'option `--output-folder=build` place les fichiers générés par Conan (toolchain, configs CMake) dans le même répertoire que le build CMake. La fonction `cmake_layout()` dans le `conanfile.py` coordonne cette organisation pour que tout s'emboîte naturellement.

### Visualisation du flux

```
conanfile.py (déclaration des dépendances)
       │
       │  conan install . --build=missing
       ▼
~/.conan2/p/ (cache local)
       │  Téléchargement depuis ConanCenter si nécessaire
       │  Compilation depuis les sources si --build=missing
       ▼
build/conan_toolchain.cmake    ← CMakeToolchain  
build/openssl-config.cmake     ← CMakeDeps  
build/spdlog-config.cmake      ← CMakeDeps  
build/zlib-config.cmake        ← CMakeDeps  
       │
       │  cmake -B build -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake
       ▼
CMakeLists.txt
  find_package(OpenSSL)   → trouve openssl-config.cmake (généré par Conan)
  find_package(spdlog)    → trouve spdlog-config.cmake (généré par Conan)
  find_package(ZLIB)      → trouve zlib-config.cmake (généré par Conan)
       │
       │  cmake --build build
       ▼
Binaires finaux (liés contre les bibliothèques du cache Conan)
```

La beauté de cette architecture est sa transparence vis-à-vis de CMake. Vos `CMakeLists.txt` ne mentionnent jamais Conan. Si un utilisateur préfère installer les dépendances via `apt` ou vcpkg, les mêmes `find_package()` fonctionnent — seule l'étape d'installation des dépendances change.

---

## ConanCenter : le dépôt communautaire

ConanCenter est le dépôt central de recettes Conan, maintenu par la communauté et hébergé par JFrog. Il contient plus de 1500 recettes pour les bibliothèques C/C++ les plus utilisées : OpenSSL, Boost, Protobuf, gRPC, spdlog, fmt, nlohmann/json, Catch2, Google Test, Qt, et bien d'autres.

Les recettes de ConanCenter sont revues et testées sur de multiples configurations. Le serveur fournit des binaires pré-compilés pour les configurations les plus courantes (GCC et Clang récents, x86_64, Linux/Windows/macOS, Debug/Release). Si votre configuration est couverte, `conan install` ne compile rien — il télécharge simplement les binaires, ce qui prend quelques secondes.

Pour chercher un paquet :

```bash
conan search "spdlog*" -r=conancenter
```

Ou consultez le site web de ConanCenter pour parcourir le catalogue et voir les options disponibles pour chaque paquet.

---

## Quand choisir Conan (vs FetchContent, vs vcpkg)

| Scénario | Outil recommandé | Raison |
|----------|-----------------|--------|
| Projet simple, 2-3 dépendances légères | `FetchContent` | Pas de dépendance externe, tout est dans CMake |
| Dépendances lourdes à compiler (Boost, Qt, gRPC) | **Conan** ou vcpkg | Cache binaire — évite la recompilation à chaque build propre |
| Cross-compilation avec contraintes ABI | **Conan** | Profils host/build, contrôle fin des settings |
| Équipe sur Windows avec Visual Studio | vcpkg | Intégration Microsoft native |
| Partage de binaires entre CI et développeurs | **Conan** | Serveur de cache binaire configurable (Artifactory) |
| Gestionnaire de paquets déjà en place dans l'équipe | Celui qui est déjà en place | La migration a un coût — ne changez que si nécessaire |

En pratique, Conan et FetchContent coexistent souvent dans le même projet : Conan gère les dépendances lourdes et système (OpenSSL, Boost, Protobuf), FetchContent gère les dépendances légères et header-only (nlohmann/json, CLI11). Le `conanfile.py` déclare les unes, le `CMakeLists.txt` déclare les autres — les deux mécanismes sont complémentaires.

---

## Plan des sous-sections

| Sous-section | Thème | Ce que vous apprendrez |
|-------------|-------|----------------------|
| **27.2.1** | Installation et configuration | Installer Conan, générer le profil par défaut, structure du cache |
| **27.2.2** | `conanfile.py` vs `conanfile.txt` | Écrire un fichier de dépendances, méthodes essentielles, options |
| **27.2.3** | Profils et settings | Créer des profils personnalisés, gérer les configurations multiples |
| **27.2.4** | Intégration CMake | `CMakeToolchain`, `CMakeDeps`, `cmake_layout()`, workflow complet |

---

> **À suivre** : La sous-section 27.2.1 couvre l'installation de Conan 2.0, la génération du profil par défaut, et la découverte de la structure du cache local.

⏭️ [Installation et configuration](/27-gestion-dependances/02.1-installation-conan.md)
