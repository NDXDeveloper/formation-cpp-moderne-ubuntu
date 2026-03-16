# Exemples du Chapitre 32 — Analyse Statique et Linting

Ce répertoire contient les exemples pratiques du chapitre 32 — programmes avec défauts détectables par clang-tidy, cppcheck, et code mal formaté pour démontrer clang-format.

## Prérequis

```bash
clang-tidy --version     # LLVM 18+  
cppcheck --version       # Cppcheck 2.x  
clang-format --version   # LLVM 18+  
```

---

## Liste des exemples

### 01\_clang\_tidy\_exemple.cpp

| | |
|---|---|
| **Section** | 32.1 |
| **Fichier .md** | `01-clang-tidy.md` |
| **Description** | Programme avec défauts détectables par clang-tidy : paramètre copié inutilement, méthode non-const, pointeur vers variable locale, comparaison signée/non-signée. |

**Exécution :**
```bash
clang-tidy 01_clang_tidy_exemple.cpp -- -std=c++23
# Ou avec le .clang-tidy fourni :
clang-tidy 01_clang_tidy_exemple.cpp -- -std=c++23
```

**Comportement attendu :** clang-tidy détecte :
- `performance-unnecessary-value-param` (paramètre `h` copié)
- `readability-make-member-function-const` (`getPortPtr`)
- `clang-analyzer-core.StackAddressEscape` (pointeur local retourné)
- `modernize-loop-convert` (boucle for indexée → range-based)

---

### 02\_cppcheck\_defauts.cpp

| | |
|---|---|
| **Section** | 32.2 |
| **Fichier .md** | `02-cppcheck.md` |
| **Description** | Programme avec défauts détectables par cppcheck : pointeur vers variable locale, fuite mémoire, variable non initialisée. |

**Exécution :**
```bash
cppcheck --enable=all 02_cppcheck_defauts.cpp
```

**Comportement attendu :** cppcheck détecte :
- `returnDanglingLifetime` (pointeur vers tableau local)
- `memleak` (fuite sur le chemin d'erreur)
- `uninitvar` (variable `x` non initialisée)
- `uninitdata` (lecture de mémoire non initialisée `p[0]`)

---

### 03\_clang\_format\_demo.cpp

| | |
|---|---|
| **Section** | 32.3 |
| **Fichier .md** | `03-clang-format.md` |
| **Description** | Code volontairement mal formaté pour démontrer clang-format. |

**Exécution :**
```bash
# Prévisualiser (style Google)
clang-format --style=Google 03_clang_format_demo.cpp

# Prévisualiser (style LLVM)
clang-format --style=LLVM 03_clang_format_demo.cpp

# Prévisualiser (style du fichier .clang-format local)
clang-format --style=file 03_clang_format_demo.cpp

# Appliquer en place
clang-format -i --style=file 03_clang_format_demo.cpp
```

**Comportement attendu :** Le code est reformaté avec indentation correcte, espaces autour des opérateurs, accolades sur la même ligne, includes triés.

---

### Fichiers de configuration

| Fichier | Section | Description |
|---------|---------|-------------|
| `.clang-tidy` | 32.1.1 | Configuration clang-tidy avec checks bugprone, performance, modernize, readability |
| `.clang-format` | 32.3 | Configuration clang-format basée sur le style Google avec personnalisations |

---

## Nettoyage

Aucun binaire n'est produit — ces exemples sont analysés sans compilation.
