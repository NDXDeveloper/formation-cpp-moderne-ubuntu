# Exemples du Chapitre 38 — CI/CD pour C++

Le chapitre 38 est principalement composé de configurations YAML (GitLab CI, GitHub Actions) non testables localement. Ce répertoire contient le seul exemple C++ compilable du chapitre.

## Prérequis

```bash
g++-15 --version    # GCC 15  
cmake --version     # CMake 3.20+  
ninja --version     # Ninja  
```

---

## 01\_version\_configure\_file (section 38.5)

| | |
|---|---|
| **Section** | 38.5 |
| **Fichier .md** | `05-artifacts-releases.md` |
| **Description** | Projet CMake avec `configure_file` pour injecter la version du projet dans un header C++ (`version.hpp.in` → `version.hpp`). Le binaire supporte `--version`. |

### Compilation et exécution

```bash
cmake -B build -G Ninja -DCMAKE_CXX_COMPILER=g++-15  
cmake --build build  
./build/myapp
./build/myapp --version
```

### Sortie attendue

```
mon-application v2.1.3 — prêt
```

```
mon-application v2.1.3
```

---

## Nettoyage

```bash
rm -rf build
```
