# Exemples du Chapitre 34 — Couverture de Code

## Prérequis

```bash
g++-15 --version    # GCC 15  
gcov-15 --version   # gcov (inclus avec GCC)  
lcov --version      # lcov (sudo apt install lcov)  
```

---

## 01\_coverage\_demo.cpp

| | |
|---|---|
| **Section** | 34.1 |
| **Fichiers .md** | `README.md`, `01-gcov-lcov.md` |
| **Description** | Programme avec deux fonctions (`classify`, `validate_port`) pour démontrer le workflow gcov/lcov/genhtml. |

### Workflow complet

```bash
# 1. Compiler avec instrumentation
g++-15 --coverage -O0 -g -std=c++20 -o coverage_demo 01_coverage_demo.cpp

# 2. Exécuter
./coverage_demo

# 3. Analyser avec gcov
gcov-15 -b coverage_demo-01_coverage_demo.gcno

# 4. Lire le rapport gcov
cat 01_coverage_demo.cpp.gcov

# 5. Agréger avec lcov
lcov --capture --directory . --output-file coverage.info --gcov-tool gcov-15  
lcov --remove coverage.info '/usr/*' --output-file coverage_filtered.info  

# 6. Générer le rapport HTML
genhtml coverage_filtered.info --output-directory report/ \
    --branch-coverage --title "Demo" --legend

# 7. Ouvrir
xdg-open report/index.html
```

### Sortie attendue

```
classify(42)  = positive  
classify(0)   = zero  
classify(-5)  = negative  
port(8080)    = valid  
port(0)       = invalid  
```

### Couverture attendue

- **Lignes** : 100% (toutes les branches de `classify` sont testées)
- **Fonctions** : 100% (3 fonctions : `classify`, `validate_port`, `main`)

---

## Nettoyage

```bash
rm -rf coverage_demo *.gcno *.gcda *.gcov *.info report/
```
