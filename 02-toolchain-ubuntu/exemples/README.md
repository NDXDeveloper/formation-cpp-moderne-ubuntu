# Exemples — Chapitre 2 : Toolchain Ubuntu

Ce dossier contient les exemples compilables extraits des fichiers `.md` du chapitre 2.

> **Compilateur requis** : GCC 14+ pour les exemples C++23 (`std::print`, `std::format`).
> Sur Ubuntu 24.04, le compilateur par défaut est GCC 13 — utilisez `g++-14` ou `g++-15`.

---

## Liste des exemples

### Section 2.0 — README (Introduction)

| Fichier | Description | Compilation | Sortie attendue |
|---------|-------------|-------------|-----------------|
| `readme_hello.cpp` | Premier programme avec `std::print` | `g++-14 -std=c++23 -Wall -Wextra readme_hello.cpp -o readme_hello` | `Hello, Ubuntu!` |

### Section 2.1.2 — Comparaison GCC vs Clang

| Fichier | Description | Compilation | Sortie attendue |
|---------|-------------|-------------|-----------------|
| `add_arrays.cpp` | Boucle vectorisable pour comparer l'assembleur GCC vs Clang | `g++ -O2 -march=native -Wall -Wextra add_arrays.cpp -o add_arrays` | `Résultat : 0 11 22 33 44 55 66 77` |

### Section 2.5 — Premier programme

| Fichier | Description | Compilation | Sortie attendue |
|---------|-------------|-------------|-----------------|
| `main_mono.cpp` | Programme mono-fichier avec `aire_cercle` | `g++-14 -std=c++23 -Wall -Wextra main_mono.cpp -o main_mono` | Affiche bienvenue, version, aire, sqrt |
| `mathutils.hpp` | Header avec déclarations `aire_cercle`, `perimetre_cercle` | — (header) | — |
| `mathutils.cpp` | Implémentation de `mathutils.hpp` | — (compilé avec main_multi) | — |
| `main_multi.cpp` | Programme multi-fichiers utilisant `mathutils` | `g++-14 -std=c++23 -Wall -Wextra mathutils.cpp main_multi.cpp -o main_multi` | Affiche bienvenue, aire, périmètre, sqrt |

### Section 2.5.1 — Compilation étape par étape

| Fichier | Description | Compilation | Comportement attendu |
|---------|-------------|-------------|---------------------|
| `test_double_include.cpp` | Vérifie que les include guards empêchent la double inclusion | `g++-14 -std=c++23 -Wall -Wextra test_double_include.cpp -o test_double_include` | Compile sans erreur (pas de sortie) |

### Section 2.5.3 — Dépendances dynamiques

| Fichier | Description | Compilation | Sortie attendue |
|---------|-------------|-------------|-----------------|
| `exemple_dlopen.cpp` | Chargement dynamique avec `dlopen`/`dlsym` sur `libm` | `g++-14 -std=c++23 -Wall -Wextra exemple_dlopen.cpp -o exemple_dlopen -ldl` | `sqrt(144) = 12` |

### Section 2.6 / 2.6.1 — Options de compilation / Warnings

| Fichier | Description | Compilation | Comportement attendu |
|---------|-------------|-------------|---------------------|
| `suspect.cpp` | Code volontairement bogué pour illustrer les warnings | `g++ -Wall suspect.cpp -o suspect` | 3 warnings (-Wall), +1 avec -O1 (`-Wmaybe-uninitialized`) |
| `type_error.cpp` | Erreur de type intentionnelle (int → string) | `g++ -Wall -Wextra type_error.cpp -o type_error` | **Ne compile PAS** — erreur de conversion attendue |

### Section 2.6.3 — Debug (-g, -ggdb3)

| Fichier | Description | Compilation | Sortie attendue |
|---------|-------------|-------------|-----------------|
| `compute_debug.cpp` | Fonction `compute()` pour illustrer l'impact de `-O2` sur le débogage | `g++ -O0 -g compute_debug.cpp -o compute_debug` | `compute(3, 4, 5) = 32` |

### Section 2.6.4 — Standard (-std=c++20/23)

| Fichier | Description | Compilation | Sortie attendue |
|---------|-------------|-------------|-----------------|
| `ranges_demo.cpp` | Démo des ranges C++20 — filtre sur un vector | `g++-14 -std=c++20 -Wall -Wextra ranges_demo.cpp -o ranges_demo` | `Nombres pairs : 4 2` |
| `feature_test.cpp` | Vérification des feature test macros | `g++-14 -std=c++23 -Wall -Wextra feature_test.cpp -o feature_test` | Affiche les valeurs de `__cplusplus`, `__cpp_concepts`, `__cpp_lib_expected`, `__cpp_lib_format`, `__cpp_lib_print` |

### Section 2.7 — Introduction à std::print (C++23)

| Fichier | Description | Compilation | Sortie attendue |
|---------|-------------|-------------|-----------------|
| `hello_print.cpp` | Premier programme avec `std::println` | `g++-14 -std=c++23 -Wall -Wextra hello_print.cpp -o hello_print` | Bienvenue sur Ubuntu 24, Pi, hex |

### Section 2.7 / 2.7.1 — Syntaxe et comparaison

| Fichier | Description | Compilation | Sortie attendue |
|---------|-------------|-------------|-----------------|
| `print_comparaison.cpp` | Comparaison printf vs cout vs println | `g++-14 -std=c++23 -Wall -Wextra print_comparaison.cpp -o print_comparaison` | Trois blocs identiques (printf, cout, println) |
| `placeholders_demo.cpp` | Démo complète des placeholders : alignement, bases, remplissage, positionnels, booléens, accolades | `g++-14 -std=c++23 -Wall -Wextra placeholders_demo.cpp -o placeholders_demo` | 9 sections de formatage |
| `colonnes_demo.cpp` | Affichage en colonnes avec printf, cout, println | `g++-14 -std=c++23 -Wall -Wextra colonnes_demo.cpp -o colonnes_demo` | Tableau produits/qté/prix en 3 variantes |

### Section 2.7.2 — Formatage type-safe et performant

| Fichier | Description | Compilation | Sortie attendue |
|---------|-------------|-------------|-----------------|
| `format_demo.cpp` | Utilisation de `std::format` pour construire des chaînes | `g++-14 -std=c++23 -Wall -Wextra format_demo.cpp -o format_demo` | Message formaté + 5 lignes numérotées (000–004) |

---

## Nettoyage

```bash
# Supprimer tous les binaires compilés
rm -f readme_hello main_mono main_multi test_double_include exemple_dlopen suspect compute_debug ranges_demo feature_test hello_print print_comparaison placeholders_demo colonnes_demo format_demo add_arrays
```
