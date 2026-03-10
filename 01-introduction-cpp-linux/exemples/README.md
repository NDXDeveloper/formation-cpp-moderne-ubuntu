# Exemples — Chapitre 1 : Introduction au C++ et à l'écosystème Linux

Ce dossier contient les exemples compilables extraits des sections du chapitre 1.

## Prérequis

- **GCC 14+** pour les exemples C++23 (`std::ranges::to`, `constexpr auto`)
- **GCC 15+** pour `std::print` (`<print>`)
- **binutils** (`nm`, `readelf`, `objdump`, `c++filt`, `size`, `strings`) pour l'inspection ELF

## Installation des outils

Sur Ubuntu 24.04, GCC 13 est installé par défaut. Pour les exemples C++23/C++26, il faut installer GCC 14 et 15 via le PPA :

```bash
# 1. Ajouter le PPA pour GCC 14/15
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test  
sudo apt update  

# 2. Installer GCC 14 et 15
sudo apt install -y g++-14 g++-15

# 3. Installer les outils d'inspection binaire (normalement déjà présents)
sudo apt install -y binutils nasm

# 4. Outils utiles pour les exemples et les chapitres suivants
sudo apt install -y cmake ninja-build gdb valgrind strace ltrace
```

Vérification :

```bash
g++-14 --version   # GCC 14 — support C++23 complet (std::ranges::to, etc.)  
g++-15 --version   # GCC 15 — support C++23/C++26 (std::print, contracts, etc.)  
```

## Compilation rapide de tous les exemples

```bash
# Exemples mono-fichier
g++-15 -std=c++23 -o 01-00-bienvenue 01-00-bienvenue.cpp  
g++-14 -std=c++23 -o 01-02-zero-cost-ranges 01-02-zero-cost-ranges.cpp  
g++    -o 01-03-hello-pipeline 01-03-hello-pipeline.cpp  
g++-14 -std=c++23 -o 01-03-01-macros-constantes 01-03-01-macros-constantes.cpp  
g++-14 -std=c++23 -o 01-03-01-macros-parametrees 01-03-01-macros-parametrees.cpp  
g++-14 -std=c++23 -o 01-03-01-stringification 01-03-01-stringification.cpp  
g++-14 -std=c++23 -o 01-03-01-log-error-macro 01-03-01-log-error-macro.cpp  
g++-14 -std=c++23 -o 01-03-01-compilation-conditionnelle 01-03-01-compilation-conditionnelle.cpp  
g++-14 -std=c++23 -o 01-03-01-include-guard 01-03-01-include-guard.cpp  
g++-14 -std=c++17 -o 01-03-03-inline-header 01-03-03-inline-header.cpp  
g++-14 -std=c++23 -g -O0 -o 01-04-01-reference-elf 01-04-01-reference-elf.cpp  

# Exemple multi-fichier (compilation séparée + linkage)
g++ -c 01-03-03-main-linkage.cpp -o main.o  
g++ -c 01-03-03-math-utils.cpp -o math_utils.o  
g++ main.o math_utils.o -o 01-03-03-programme  
```

## Nettoyage

```bash
rm -f 01-00-bienvenue 01-02-zero-cost-ranges 01-03-hello-pipeline \
      01-03-01-macros-constantes 01-03-01-macros-parametrees \
      01-03-01-stringification 01-03-01-log-error-macro \
      01-03-01-compilation-conditionnelle 01-03-01-include-guard \
      01-03-03-programme 01-03-03-inline-header 01-04-01-reference-elf \
      main.o math_utils.o *.ii *.s reference_debug reference_release reference_stripped
```

---

## Liste des exemples

### `01-00-bienvenue.cpp`

| | |
|---|---|
| **Section** | 1.0 — Introduction au chapitre |
| **Fichier source** | `README.md` |
| **Description** | Premier exemple de la formation — affichage avec `std::print` (C++23) |
| **Compilateur** | GCC 15+ (`<print>` requis) |
| **Sortie attendue** | `Bienvenue dans la formation C++ moderne !` |

---

### `01-02-zero-cost-ranges.cpp`

| | |
|---|---|
| **Section** | 1.2 — Pourquoi C++ pour le DevOps et le System Programming |
| **Fichier source** | `02-pourquoi-cpp-devops.md` |
| **Description** | Zero-cost abstraction avec `std::views::filter`, `std::views::transform` et `std::ranges::to` (C++23). Filtre les éléments actifs d'un vecteur et extrait leurs valeurs. |
| **Compilateur** | GCC 14+ (C++23, `std::ranges::to`) |
| **Sortie attendue** | `10 30 50` |

---

### `01-03-hello-pipeline.cpp`

| | |
|---|---|
| **Section** | 1.3 — Le cycle de compilation |
| **Fichier source** | `03-cycle-compilation.md` |
| **Description** | Programme minimal traversant les 4 étapes du pipeline de compilation. Utilise `printf` et une macro `#define`. Peut être compilé étape par étape avec `-E`, `-S`, `-c`. |
| **Sortie attendue** | `Bonjour depuis le pipeline de compilation !` |
| **Comportement** | Étapes intermédiaires observables : `g++ -E` produit un `.ii` de ~1000 lignes, `g++ -S` produit l'assembleur, `g++ -c` produit le fichier objet, `nm hello.o` montre les symboles. |

---

### `01-03-01-macros-constantes.cpp`

| | |
|---|---|
| **Section** | 1.3.1 — Le préprocesseur : #include, #define, macros |
| **Fichier source** | `03.1-preprocesseur.md` |
| **Description** | Comparaison entre les macros constantes (`#define`) et les alternatives C++ modernes (`constexpr`, `std::string_view`). |
| **Sortie attendue** | |

```
=== Style hérité (#define) ===
MAX_CONNECTIONS = 1024  
VERSION = 3.2.1  
PI = 3.14159  
=== Style C++ moderne (constexpr) ===
max_connections = 1024  
version = 3.2.1  
pi = 3.14159  
```

---

### `01-03-01-macros-parametrees.cpp`

| | |
|---|---|
| **Section** | 1.3.1 — Le préprocesseur : #include, #define, macros |
| **Fichier source** | `03.1-preprocesseur.md` |
| **Description** | Pièges des macros paramétrées : parenthésage incorrect (`CARRE_MAUVAIS(3+1)` donne 7 au lieu de 16) et alternative sûre avec `constexpr`. |
| **Sortie attendue** | |

```
=== Macro bien parenthésée ===
CARRE(5) = 25  
MIN(3, 7) = 3  
=== Macro MAL parenthésée (piège !) ===
CARRE_MAUVAIS(3 + 1) = 7  (attendu 16, obtenu 7 !)
=== constexpr (sûr) ===
carre(5) = 25  
carre(3 + 1) = 16  (correct)  
```

---

### `01-03-01-stringification.cpp`

| | |
|---|---|
| **Section** | 1.3.1 — Le préprocesseur : #include, #define, macros |
| **Fichier source** | `03.1-preprocesseur.md` |
| **Description** | Opérateurs préprocesseur `#` (stringification) et `##` (concaténation de tokens). |
| **Sortie attendue** | |

```
=== Stringification (#) ===
TO_STRING(42 + 3) = 42 + 3  
TO_STRING(hello) = hello  
=== Concaténation (##) ===
value1 = 10  
value2 = 20  
```

---

### `01-03-01-log-error-macro.cpp`

| | |
|---|---|
| **Section** | 1.3.1 — Le préprocesseur : #include, #define, macros |
| **Fichier source** | `03.1-preprocesseur.md` |
| **Description** | Macro multi-ligne avec l'idiome `do { ... } while(0)`. Affiche des messages d'erreur avec `__FILE__` et `__LINE__` automatiques. |
| **Sortie attendue** (stderr) | |

```
[ERROR] 01-03-01-log-error-macro.cpp:21 - Connexion refusée
[ERROR] 01-03-01-log-error-macro.cpp:22 - Fichier introuvable
```

> Les numéros de ligne peuvent varier si le fichier est modifié.

---

### `01-03-01-compilation-conditionnelle.cpp`

| | |
|---|---|
| **Section** | 1.3.1 — Le préprocesseur : #include, #define, macros |
| **Fichier source** | `03.1-preprocesseur.md` |
| **Description** | Compilation conditionnelle avec `__cplusplus`, `__GNUC__`, `__clang__`. Détecte le standard C++ et le compilateur utilisé. |
| **Sortie attendue** (avec `g++-14 -std=c++23`) | |

```
=== Macros prédéfinies ===
__cplusplus = 202302
__FILE__ = 01-03-01-compilation-conditionnelle.cpp
Compilateur : GCC 14.3
=== Compilation conditionnelle ===
C++23 ou plus récent
```

> Les valeurs varient selon le compilateur et la version utilisés.

---

### `01-03-01-include-guard.cpp` + `01-03-01-include-guard.h`

| | |
|---|---|
| **Section** | 1.3.1 — Le préprocesseur : #include, #define, macros |
| **Fichier source** | `03.1-preprocesseur.md` |
| **Description** | Garde d'inclusion (`#ifndef`/`#define`/`#endif`). Le `.h` est inclus deux fois volontairement — le garde empêche la redéfinition de `struct Point`. |
| **Sortie attendue** | `Point: (3.5, 7.2)` |
| **Comportement** | Sans le garde, la double inclusion provoquerait une erreur `redefinition of 'struct Point'`. |

---

### `01-03-03-main-linkage.cpp` + `01-03-03-math-utils.cpp`

| | |
|---|---|
| **Section** | 1.3.3 — L'édition de liens : Résolution des symboles |
| **Fichier source** | `03.3-edition-liens.md` |
| **Description** | Compilation séparée et édition de liens. `main.cpp` déclare `ajouter()` et `multiplier()`, `math_utils.cpp` les définit. Démontre le name mangling (`_Z7ajouterii`, `_Z10multiplierii`) et l'inspection avec `nm`. |
| **Compilation** | Multi-fichier : `g++ -c` séparé puis linkage |
| **Sortie attendue** | `Somme: 7, Produit: 12` |
| **Inspection** | `nm main.o` montre `U _Z7ajouterii` (undefined), `nm math_utils.o` montre `T _Z7ajouterii` (défini). `echo "_Z7ajouterii" \| c++filt` → `ajouter(int, int)`. |

---

### `01-03-03-inline-header.cpp`

| | |
|---|---|
| **Section** | 1.3.3 — L'édition de liens : Résolution des symboles |
| **Fichier source** | `03.3-edition-liens.md` |
| **Description** | Variable globale `inline` (C++17) pour éviter `multiple definition`, et `extern "C"` pour désactiver le name mangling. |
| **Compilateur** | C++17 minimum |
| **Sortie attendue** | |

```
compteur = 0  
fonction_c appelée avec x = 42  
```

---

### `01-04-01-reference-elf.cpp`

| | |
|---|---|
| **Section** | 1.4.1 — Structure du format ELF / 1.4.2 — Inspection avec readelf et objdump |
| **Fichier source** | `04.1-structure-elf.md`, `04.2-inspection-elf.md` |
| **Description** | Programme de référence pour l'analyse ELF. Contient des éléments répartis dans différentes sections : chaîne littérale (`.rodata`), pointeur global (`.data`), entier = 42 (`.data`), tableau de 1000 int à zéro (`.bss`), fonctions (`.text`). |
| **Compilation** | Compiler avec `-g -O0` pour l'analyse complète |
| **Sortie attendue** | `Formation C++ moderne (compteur: 42)` |
| **Inspection** | |

```bash
readelf -h ./reference     # en-tête ELF (type DYN/PIE, architecture x86-64)  
readelf -S ./reference     # sections (.text ~325 octets, .bss ~4032 octets NOBITS)  
readelf -l ./reference     # segments (LOAD R+E code, LOAD RW données, GNU_STACK RW)  
readelf -s ./reference     # symboles (message D, compteur_global D, tableau_zero B)  
readelf -d ./reference     # section .dynamic (NEEDED libc.so.6, BIND_NOW)  
size ./reference           # text ~1500, data ~600, bss ~4032  
nm -C ./reference          # symboles démanglés (afficher(), main)  
objdump -d -M intel ./ref  # désassemblage de main et afficher  
objdump -s -j .rodata ./ref  # "Formation C++ moderne" visible en ASCII  
strings ./reference | grep formation  # chaîne en clair dans le binaire  
xxd -l 4 ./reference       # nombre magique 7f 45 4c 46 (.ELF)  
```
