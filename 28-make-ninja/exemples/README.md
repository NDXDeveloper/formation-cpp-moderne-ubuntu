# Exemples du Chapitre 28 — Makefile, Ninja et Build Automation

Ce répertoire contient les exemples pratiques du chapitre 28, couvrant Make, Ninja et Meson.

## Prérequis

```bash
g++-15 --version    # GCC 15  
make --version       # GNU Make 4.3+  
cmake --version      # CMake 3.25+  
ninja --version      # Ninja 1.11+  
meson --version      # Meson 1.3+ (pour l'exemple 04 uniquement)  
```

---

## Liste des exemples

### 01\_makefile\_base

| | |
|---|---|
| **Section** | 28.1 |
| **Fichier .md** | `01-syntaxe-makefiles.md` |
| **Description** | Makefile illustrant les concepts fondamentaux : règles explicites, règle de pattern (`%.o: %.cpp`), variables (`CXX`, `CXXFLAGS`), variables automatiques (`$@`, `$<`, `$^`), et cible phony (`clean`). |

**Exécution :**
```bash
cd 01_makefile_base  
make  
./my_app
make clean
```

**Sortie attendue :**
```
Main: Hello  
Utils: ready  
Network: connected  
```

---

### 02\_makefile\_realiste

| | |
|---|---|
| **Section** | 28.1, 28.2 |
| **Fichiers .md** | `01-syntaxe-makefiles.md`, `02-variables-regles.md` |
| **Description** | Makefile réaliste avec build hors source (`build/`), dépendances de headers automatiques (`-MMD -MP` + `-include`), order-only prerequisites (`| $(BUILDDIR)`), fonctions Make (`wildcard`, `patsubst`), et cibles utilitaires (`help`). |

**Exécution :**
```bash
cd 02_makefile_realiste  
make  
./build/bin/my_tool
make help  
make clean  
```

**Sortie attendue :**
```
Hello, Make réaliste!
```

**Comportement attendu :**
- Les fichiers `.d` de dépendances sont générés dans `build/`
- La modification d'un header déclenche la recompilation des fichiers qui l'incluent
- `make` sans changement affiche "Rien à faire pour « all »."

---

### 03\_ninja\_diagnostic

| | |
|---|---|
| **Section** | 28.3, 28.3.1, 28.3.2 |
| **Fichiers .md** | `03-ninja.md`, `03.1-pourquoi-rapide.md`, `03.2-fichiers-ninja.md` |
| **Description** | Projet CMake + Ninja pour explorer les commandes de diagnostic : `ninja -t targets`, `ninja -t deps`, `ninja -n -v` (dry-run), `build.ninja`, et `compile_commands.json`. |

**Exécution :**
```bash
cd 03_ninja_diagnostic

# Build
cmake -B build -G Ninja -DCMAKE_CXX_COMPILER=g++-15  
cmake --build build  
./build/app

# Diagnostic Ninja
ninja -C build -t targets          # Lister les cibles  
ninja -C build -n -v               # Dry-run verbose  
ninja -C build -t deps CMakeFiles/core.dir/src/core.cpp.o  # Dépendances d'un .o  

# Lire le build.ninja
grep -A 5 "core.cpp.o:" build/build.ninja

# Nettoyage
rm -rf build
```

**Sortie attendue :**
```
Hello from Ninja diagnostic test!
```

**Comportement attendu :**
- `ninja -t targets` liste les cibles (core, app, all, clean, etc.)
- `ninja -t deps` montre les headers dont dépend `core.cpp.o`
- `build/compile_commands.json` est généré (grâce à `CMAKE_EXPORT_COMPILE_COMMANDS=ON`)

---

### 04\_meson\_projet

| | |
|---|---|
| **Section** | 28.4, 28.4.1 |
| **Fichiers .md** | `04-meson.md`, `04.1-syntaxe-meson.md` |
| **Description** | Projet Meson complet illustrant `project()`, `library()`, `declare_dependency()`, `executable()`, et `subdir()`. |

> **Prérequis** : `sudo apt install meson` (ou `pip3 install --user meson`)

**Exécution :**
```bash
cd 04_meson_projet

# Configuration (Meson génère toujours pour Ninja)
meson setup build

# Compilation
meson compile -C build

# Exécution
./build/demo

# Nettoyage
rm -rf build
```

**Sortie attendue :**
```
Hello from Meson, world!
```

**Comportement attendu :**
- `meson setup build` configure le projet et génère `build.ninja`
- Pas besoin de `-G Ninja` — Meson génère toujours pour Ninja
- `compile_commands.json` est généré automatiquement dans `build/`

---

## Nettoyage

```bash
# Depuis le répertoire exemples/
cd 01_makefile_base && make clean && cd ..  
cd 02_makefile_realiste && make clean && cd ..  
rm -rf 03_ninja_diagnostic/build  
rm -rf 04_meson_projet/build  
```
